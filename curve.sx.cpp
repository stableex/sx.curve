#include <eosio.token/eosio.token.hpp>
#include <sx.utils/utils.hpp>
#include <sx.rex/rex.hpp>

#include "curve.sx.hpp"
#include "src/maintenance.cpp"
#include "src/actions.cpp"

/**
 * Notify contract when any token transfer notifiers relay contract
 */
[[eosio::on_notify("*::transfer")]]
void sx::curve::on_transfer( const name from, const name to, const asset quantity, const string memo )
{
    // authenticate incoming `from` account
    require_auth( from );

    // ignore transfers
    if ( to != get_self() || memo == get_self().to_string() || from == "eosio.ram"_n) return;

    // tables
    sx::curve::config_table _config( get_self(), get_self().value );
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    // config
    check( _config.exists(), "Curve.sx: contract must first be initialized");
    const name status = _config.get_or_default().status;
    check( (status == "ok"_n || status == "testing"_n), "Curve.sx: contract is under maintenance");

    // TEMP - DURING TESTING PERIOD
    if ( status == "testing"_n ) check( from.suffix() == "sx"_n, "account must be *.sx during testing period");

    // user input params
    const auto parsed_memo = parse_memo( memo );
    const extended_asset ext_in = { quantity, get_first_receiver() };

    // withdraw liquidity
    if ( _pairs.find( quantity.symbol.code().raw() ) != _pairs.end() ) {
        withdraw_liquidity( from, ext_in );

    // add liquidity
    } else if ( parsed_memo.action == "deposit"_n ) {
        add_liquidity( from, parsed_memo.symcodes[0], ext_in );

    // swap convert (ex: USDT => USN)
    } else if ( parsed_memo.action == "swap"_n) {
        // convert( from, ext_in, parsed_memo.symcodes, parsed_memo.min_return );
        print( from, ext_in, parsed_memo.symcodes[0], "-", parsed_memo.symcodes[1], ":", parsed_memo.min_return);

    } else {
        check( false, ERROR_INVALID_MEMO );
    }
}

[[eosio::action]]
void sx::curve::deposit( const name owner, const symbol_code pair_id )
{
    require_auth( owner );

    sx::curve::config_table _config( get_self(), get_self().value );
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    sx::curve::orders_table _orders( get_self(), pair_id.raw() );

    // configs
    check( _config.exists(), "Curve.sx: contract is under maintenance");
    auto config = _config.get();

    // get current order & pairs
    auto & pair = _pairs.get( pair_id.raw(), "Curve.sx: pair does not exist");
    auto & orders = _orders.get( owner.value, "Curve.sx: no deposits for this user");
    check((pair.reserve0.quantity.amount && pair.reserve1.quantity.amount) || (pair.reserve0.quantity.amount==0 && pair.reserve0.quantity.amount==0), "invalid pair reserves");
    check(orders.quantity0.quantity.amount && orders.quantity1.quantity.amount, "Curve.sx: one of the currencies not provided");

    // symbol helpers
    const symbol sym0 = pair.reserve0.quantity.symbol;
    const symbol sym1 = pair.reserve1.quantity.symbol;

    // calculate total deposits based on reserves
    // if reserves empty, fallback to 1
    const int64_t supply = mul_amount(pair.liquidity.quantity.amount, MAX_PRECISION, pair.liquidity.quantity.symbol.precision());
    const int64_t reserve0 = pair.reserve0.quantity.amount ? mul_amount(pair.reserve0.quantity.amount, MAX_PRECISION, sym0.precision()) : 1;
    const int64_t reserve1 = pair.reserve1.quantity.amount ? mul_amount(pair.reserve1.quantity.amount, MAX_PRECISION, sym1.precision()) : 1;
    const int64_t reserves = reserve0 + reserve1;
    const double reserve_ratio0 = double(reserve0) / reserves;
    const double reserve_ratio1 = double(reserve1) / reserves;

    // get owner order and calculate payment
    const int64_t amount0 = mul_amount(orders.quantity0.quantity.amount, MAX_PRECISION, sym0.precision());
    const int64_t amount1 = mul_amount(orders.quantity1.quantity.amount, MAX_PRECISION, sym1.precision());
    const int64_t payment = amount0 + amount1;
    const double amount_ratio0 = double(amount0) / payment;
    const double amount_ratio1 = double(amount1) / payment;

    // actual amounts to deposit
    int64_t deposit0 = amount0;
    int64_t deposit1 = amount1;

    // ??? BETTER DESCRIPTION ???
    // calculate excess deposit
    if (amount_ratio0 <= reserve_ratio0) deposit1 = ((uint128_t) amount0) * reserve1 / reserve0;
    else deposit0 = ((uint128_t) amount1) * reserve0 / reserve1;

    // send back excess deposit to owner
    if (deposit0 < amount0) {
        const int64_t excess_amount = div_amount(amount0, MAX_PRECISION, sym0.precision()) - div_amount(deposit0, MAX_PRECISION, sym0.precision());
        const extended_asset excess = { excess_amount, pair.reserve0.get_extended_symbol() };
        transfer( get_self(), owner, excess, "Curve.sx: excess");
    }
    if (deposit1 < amount1) {
        const int64_t excess_amount = div_amount(amount1, MAX_PRECISION, sym1.precision()) - div_amount(deposit1, MAX_PRECISION, sym1.precision());
        const extended_asset excess = { excess_amount, pair.reserve1.get_extended_symbol() };
        transfer( get_self(), owner, excess, "Curve.sx: excess");
    }

    // normalize final deposits
    const extended_asset ext_deposit0 = { div_amount(deposit0, MAX_PRECISION, sym0.precision()), pair.reserve0.get_extended_symbol()};
    const extended_asset ext_deposit1 = { div_amount(deposit1, MAX_PRECISION, sym1.precision()), pair.reserve1.get_extended_symbol()};

    // issue liquidity
    const int64_t issued_amount = rex::issue(deposit0 + deposit1, reserves, supply, 1);
    const extended_asset issued = { div_amount(issued_amount, MAX_PRECISION, pair.liquidity.quantity.symbol.precision()), pair.liquidity.get_extended_symbol()};

    // add liquidity deposits & newly issued liquidity
    _pairs.modify(pair, get_self(), [&]( auto & row ) {
        row.reserve0 += ext_deposit0;
        row.reserve1 += ext_deposit1;
        row.liquidity += issued;

        // log liquidity change
        sx::curve::liquiditylog_action liquiditylog( get_self(), { get_self(), "active"_n });
        liquiditylog.send( pair_id, owner, issued, orders.quantity0, orders.quantity1, row.liquidity, row.reserve0, row.reserve1 );
    });

    // issue & transfer to owner
    issue( issued, "Curve.sx: deposit" );
    transfer( get_self(), owner, issued, "Curve.sx: deposit");

    // delete any remaining liquidity deposit order
    _orders.erase( orders );
}

// returns any remaining orders to owner account
[[eosio::action]]
void sx::curve::cancel( const name owner, const symbol_code pair_id )
{
    if ( !has_auth( get_self() )) require_auth( owner );

    sx::curve::orders_table _orders( get_self(), pair_id.raw() );
    auto & orders = _orders.get( owner.value, "Curve.sx: no deposits for this user in this pool");
    if ( orders.quantity0.quantity.amount ) transfer( get_self(), owner, orders.quantity0, "Curve.sx: cancel");
    if ( orders.quantity1.quantity.amount ) transfer( get_self(), owner, orders.quantity1, "Curve.sx: cancel");

    _orders.erase( orders );
}

[[eosio::action]]
void sx::curve::removepair( const symbol_code pair_id )
{
    has_auth( get_self() );

    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    auto & pair = _pairs.get( pair_id.raw(), "Curve.sx: pairs does not exist");
    check( !pair.liquidity.quantity.amount, "Curve.sx: liquidity must be empty before removing");
    _pairs.erase( pair );
}

void sx::curve::withdraw_liquidity( const name owner, const extended_asset value )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    // get current pairs
    const symbol_code pair_id = value.quantity.symbol.code();
    auto & pair = _pairs.get( pair_id.raw(), "Curve.sx: pairs does not exist");

    // extended symbols
    const extended_symbol ext_sym0 = pair.reserve0.get_extended_symbol();
    const extended_symbol ext_sym1 = pair.reserve1.get_extended_symbol();
    const symbol sym0 = pair.reserve0.quantity.symbol;
    const symbol sym1 = pair.reserve1.quantity.symbol;

    // calculate total deposits based on reserves
    const int64_t supply = mul_amount(pair.liquidity.quantity.amount, MAX_PRECISION, pair.liquidity.quantity.symbol.precision());
    const int64_t reserve0 = pair.reserve0.quantity.amount ? mul_amount(pair.reserve0.quantity.amount, MAX_PRECISION, sym0.precision()) : 1;
    const int64_t reserve1 = pair.reserve1.quantity.amount ? mul_amount(pair.reserve1.quantity.amount, MAX_PRECISION, sym1.precision()) : 1;
    const int64_t reserves = reserve0 + reserve1;
    const double reserve_ratio0 = double(reserve0) / reserves;
    const double reserve_ratio1 = double(reserve1) / reserves;

    // calculate withdraw amounts
    const int64_t payment = mul_amount(value.quantity.amount, MAX_PRECISION, value.quantity.symbol.precision());
    const int64_t retire_amount = rex::retire( payment, reserves, supply );

    // get owner order and calculate payment
    int64_t amount0 = retire_amount * reserve_ratio0;
    int64_t amount1 = retire_amount * reserve_ratio1;
    if (amount0 == reserve0 || amount1 == reserve1) {         //deal with rounding error on final withdrawal
        amount0 = reserve0;
        amount1 = reserve1;
    }
    const extended_asset out0 = { div_amount(amount0, MAX_PRECISION, sym0.precision()), ext_sym0 };
    const extended_asset out1 = { div_amount(amount1, MAX_PRECISION, sym1.precision()), ext_sym1 };
    check( out0.quantity.amount || out1.quantity.amount, "Curve.sx: withdraw amount too small");

    // add liquidity deposits & newly issued liquidity
    _pairs.modify(pair, get_self(), [&]( auto & row ) {
        row.reserve0 -= out0;
        row.reserve1 -= out1;
        row.liquidity -= value;

        // log liquidity change
        sx::curve::liquiditylog_action liquiditylog( get_self(), { get_self(), "active"_n });
        liquiditylog.send( pair_id, owner, value, out0, out1, row.liquidity, row.reserve0, row.reserve1 );
    });

    // issue & transfer to owner
    retire( value, "withdraw" );
    if ( out0.quantity.amount ) transfer( get_self(), owner, out0, "Curve.sx: withdraw");
    if ( out1.quantity.amount ) transfer( get_self(), owner, out1, "Curve.sx: withdraw");
}

void sx::curve::add_liquidity( const name owner, const symbol_code pair_id, const extended_asset value )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    sx::curve::orders_table _orders( get_self(), pair_id.raw() );

    // get current order & pairs
    auto pair = _pairs.get( pair_id.raw(), "Curve.sx: pairs does not exist");
    auto itr = _orders.find( owner.value );

    // extended symbols
    const extended_symbol ext_sym_in = value.get_extended_symbol();
    const extended_symbol ext_sym0 = pair.reserve0.get_extended_symbol();
    const extended_symbol ext_sym1 = pair.reserve1.get_extended_symbol();

    // initialize quantities
    auto insert = [&]( auto & row ) {
        row.owner = owner;
        row.quantity0 = { itr == _orders.end() ? 0 : itr->quantity0.quantity.amount, ext_sym0 };
        row.quantity1 = { itr == _orders.end() ? 0 : itr->quantity1.quantity.amount, ext_sym1 };

        // add & validate deposit
        if ( ext_sym_in == ext_sym0 ) row.quantity0 += value;
        else if ( ext_sym_in == ext_sym1 ) row.quantity1 += value;
        else check( false, "Curve.sx: invalid extended symbol when adding liquidity");
    };

    // create/modify order
    if ( itr == _orders.end() ) _orders.emplace( get_self(), insert );
    else _orders.modify( itr, get_self(), insert );
}

// increase/decrease amplifier of given pair id
[[eosio::action]]
void sx::curve::ramp( const symbol_code pair_id, const uint64_t target_amplifier, const int64_t minutes )
{
    require_auth( get_self() );

    sx::curve::ramp_table _ramp_table( get_self(), get_self().value );
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    auto pair = _pairs.get(pair_id.raw(), "`pair_id` does not exist in `pairs`");

    // validation
    check( target_amplifier > 0 && target_amplifier <= MAX_AMPLIFIER, "Curve.sx: target amplifier should be within within valid range");
    check( minutes > 0, "Curve.sx: minutes should be above 0");
    check( minutes * 60 >= MIN_RAMP_TIME, "Curve.sx: minimum ramp timeframe must exceed " + to_string(MIN_RAMP_TIME) + " seconds");

    auto insert = [&]( auto & row ) {
        row.pair_id = pair_id;
        row.start_amplifier = pair.amplifier;
        row.target_amplifier = target_amplifier;
        row.start_time = current_time_point();
        row.end_time = current_time_point() + eosio::minutes(minutes);
    };

    auto itr = _ramp_table.find(pair_id.raw());
    if ( itr == _ramp_table.end() ) _ramp_table.emplace( get_self(), insert );
    else _ramp_table.modify( itr, get_self(), insert );
}

[[eosio::action]]
void sx::curve::stopramp( const symbol_code pair_id )
{
    require_auth( get_self() );

    sx::curve::ramp_table _ramp( get_self(), get_self().value );
    auto & ramp = _ramp.get(pair_id.raw(), "Curve.sx: `pair_id` does not exist in `ramp` table");
    _ramp.erase( ramp );
}

[[eosio::action]]
void sx::curve::setfee( const uint8_t trade_fee, const optional<uint8_t> protocol_fee, const optional<name> fee_account )
{
    require_auth( get_self() );

    // config
    sx::curve::config_table _config( get_self(), get_self().value );
    auto config = _config.get_or_default();

    // required params
    check( trade_fee + *protocol_fee <= 300, "`trade_fee` + `protocol_fee` cannot exceed 3%");

    // optional params
    if ( fee_account->value ) check( is_account( *fee_account ), "`fee_account` does not exist");
    if ( *protocol_fee ) check( fee_account->value, "must provide `fee_account` if `protocol_fee` is defined");

    // set config
    config.trade_fee = trade_fee;
    config.protocol_fee = *protocol_fee;
    config.fee_account = *fee_account;
    _config.set( config, get_self() );
}

[[eosio::action]]
void sx::curve::setstatus( const name status )
{
    require_auth( get_self() );

    sx::curve::config_table _config( get_self(), get_self().value );
    auto config = _config.get_or_default();
    config.status = status;
    _config.set( config, get_self() );
}

[[eosio::action]]
void sx::curve::createpair( const name creator, const symbol_code pair_id, const extended_symbol reserve0, const extended_symbol reserve1, const uint64_t amplifier )
{
    // `creator` must be contract itself during beta period
    if ( !has_auth( get_self() ) ) check( false, "`creator` is disabled from creating pair during beta period");
    require_auth( creator );

    // =======
    //  TO-DO
    // =======
    // 1. create liquidity from incremental `pair_id` ( 1 => SXA, 2 => SXB, 27 => SXAA )
    // 2. auto increment `config::pair_id`

    // tables
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    sx::curve::config_table _config( get_self(), get_self().value );

    // reserve params
    const name contract0 = reserve0.get_contract();
    const name contract1 = reserve1.get_contract();
    const symbol sym0 = reserve0.get_symbol();
    const symbol sym1 = reserve1.get_symbol();

    // check reserves
    check( is_account( contract0 ), "reserve0 contract does not exists");
    check( is_account( contract1 ), "reserve1 contract does not exists");
    check( token::get_supply( contract0, sym0.code() ).symbol == sym0, "reserve0 symbol mismatch" );
    check( token::get_supply( contract1, sym1.code() ).symbol == sym1, "reserve1 symbol mismatch" );
    // check( !find_pair_id( sym0.code(), sym1.code() ).is_valid(), "pair with these reserves already exists" );
    check( _pairs.find( pair_id.raw() ) == _pairs.end(), "pair id already exists" );
    check( amplifier > 0 && amplifier <= MAX_AMPLIFIER, "Curve.sx: invalid amplifier" );

    // create liquidity token
    const extended_symbol liquidity = {{ pair_id, max(sym0.precision(), sym1.precision())}, TOKEN_CONTRACT };

    // in case supply already exists
    token::stats _stats( TOKEN_CONTRACT, pair_id.raw() );
    auto stats_itr = _stats.find( pair_id.raw() );

    // create token if supply does not exist
    if ( stats_itr == _stats.end() ) create( liquidity );
    // supply must be empty
    else check( !stats_itr->supply.amount, "`createpair` requires zero existing supply" );

    // create pair
    _pairs.emplace( creator, [&]( auto & row ) {
        row.id = pair_id;
        row.reserve0 = { 0, reserve0 };
        row.reserve1 = { 0, reserve1 };
        row.liquidity = { 0, liquidity };
        row.amplifier = amplifier;
        row.volume0 = { 0, sym0 };
        row.volume1 = { 0, sym1 };
        row.last_updated = current_time_point();
    });
}

// calculate reserve amounts relative to supply
double sx::curve::calculate_virtual_price( const asset value0, const asset value1, const asset supply )
{
    const int64_t amount0 = mul_amount( value0.amount, MAX_PRECISION, value0.symbol.precision() );
    const int64_t amount1 = mul_amount( value1.amount, MAX_PRECISION, value1.symbol.precision() );
    const int64_t amount2 = mul_amount( supply.amount, MAX_PRECISION, supply.symbol.precision() );
    return static_cast<double>( amount0 + amount1 ) / amount2;
}

// calculate last price per trade
double sx::curve::calculate_price( const asset value0, const asset value1 )
{
    const int64_t amount0 = mul_amount( value0.amount, MAX_PRECISION, value0.symbol.precision() );
    const int64_t amount1 = mul_amount( value1.amount, MAX_PRECISION, value1.symbol.precision() );
    return static_cast<double>(amount0) / amount1;
}

// Memo schemas
// ============
// Swap: `swap,<min_return>,<pair_ids>`
// Deposit: `deposit,<pair_id>`
sx::curve::memo_schema sx::curve::parse_memo( const string memo )
{
    // split memo into parts
    const vector<string> parts = sx::utils::split(memo, ",");
    check(parts.size() <= 3, ERROR_INVALID_MEMO );

    // memo result
    memo_schema result;
    result.action = sx::utils::parse_name(parts[0]);
    result.min_return = 0;

    // swap action
    if ( result.action == "swap"_n ) {
        result.symcodes = parse_memo_symcodes( parts[2] );
        result.min_return = std::stoi( parts[1] );
        check( result.min_return >= 0, ERROR_INVALID_MEMO );
        check( result.symcodes.size() >= 1, ERROR_INVALID_MEMO );

    // deposit action
    } else if ( result.action == "deposit"_n ) {
        result.symcodes = parse_memo_symcodes( parts[1] );
        check( result.symcodes.size() == 1, ERROR_INVALID_MEMO );
    }
    return result;
}

vector<symbol_code> sx::curve::parse_memo_symcodes( const string memo )
{
    vector<symbol_code> symcodes;
    for ( const string str : sx::utils::split(memo, "-") ) {
        const symbol_code symcode = sx::utils::parse_symbol_code( str );
        check( symcode.raw(), ERROR_INVALID_MEMO );
        symcodes.push_back( symcode );
    }
    return symcodes;
}
