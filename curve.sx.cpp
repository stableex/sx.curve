#include <eosio.token/eosio.token.hpp>
#include <sx.utils/utils.hpp>
#include <sx.safemath/safemath.hpp>
#include <sx.rex/rex.hpp>

#include "curve.sx.hpp"
#include "src/actions.cpp"

namespace sx {

/**
 * Notify contract when any token transfer notifiers relay contract
 */
[[eosio::on_notify("*::transfer")]]
void curve::on_transfer( const name from, const name to, const asset quantity, const string memo )
{
    // authenticate incoming `from` account
    require_auth( from );

    // tables
    curve::config_table _config( get_self(), get_self().value );
    curve::pairs_table _pairs( get_self(), get_self().value );

    // config
    check( _config.exists(), ERROR_CONFIG_NOT_EXISTS );
    const name status = _config.get().status;
    check( status == "ok"_n, "curve::on_transfer: contract is under maintenance");

    // ignore transfers
    if ( to != get_self() || from == "eosio.ram"_n ) return;

    // user input params
    const auto parsed_memo = parse_memo( memo );
    const extended_asset ext_in = { quantity, get_first_receiver() };
    const bool is_liquidity = _pairs.find( quantity.symbol.code().raw() ) != _pairs.end();

    // only allow liquidity withdraws to be available
    if ( status == "withdraw"_n ) check( is_liquidity, "curve::on_transfer: only accepts liquidity tokens during `withdraw` status");

    // add liquidity (memo required => "deposit,<pair_id>")
    if ( parsed_memo.action == "deposit"_n ) {
        add_liquidity( from, parsed_memo.pair_ids[0], ext_in );

    // swap convert (memo required => "swap,<min_return>,<pair_ids>")
    } else if ( parsed_memo.action == "swap"_n) {
        convert( from, ext_in, parsed_memo.pair_ids, parsed_memo.min_return );

    // withdraw liquidity (no memo required)
    } else if ( is_liquidity ) {
        withdraw_liquidity( from, ext_in );

    } else {
        check( false, ERROR_INVALID_MEMO );
    }

    // accounts to be notified via inline action
    notify();
}

[[eosio::action]]
void curve::init( const name token_contract )
{
    require_auth( get_self() );

    curve::config_table _config( get_self(), get_self().value );
    auto config = _config.exists() ? _config.get() : curve::config_row{};

    // token contract can not be modified once initialized
    check( is_account( config.token_contract ), "curve::init: `token_contract` does not exist");
    if ( config.token_contract.value ) check( config.token_contract == token_contract, "curve::init: `token_contract` cannot be modified once initialized");

    // set config
    config.token_contract = token_contract;
    _config.set( config, get_self() );
}

[[eosio::action]]
void curve::reset()
{
    require_auth( get_self() );

    curve::config_table _config( get_self(), get_self().value );
    _config.remove();
}

void curve::convert( const name owner, const extended_asset ext_in, const vector<symbol_code> pair_ids, const int64_t min_return )
{
    // execute the trade by updating all involved pools
    const extended_asset out = apply_trade( owner, ext_in, pair_ids );

    // enforce minimum return (slippage protection)
    check(out.quantity.amount != 0 && out.quantity.amount >= min_return, "curve::convert: invalid minimum return");

    // transfer amount to owner
    transfer( get_self(), owner, out, "curve.sx: swap token" );
}

extended_asset curve::apply_trade( const name owner, const extended_asset ext_quantity, const vector<symbol_code> pair_ids )
{
    curve::pairs_table _pairs( get_self(), get_self().value );
    curve::config_table _config( get_self(), get_self().value );
    check( _config.exists(), ERROR_CONFIG_NOT_EXISTS );
    auto config = _config.get();

    // initial quantities
    extended_asset ext_out;
    extended_asset ext_in = ext_quantity;

    // iterate over each liquidity pool per each `pair_id` provided in swap memo
    for ( const symbol_code pair_id : pair_ids ) {
        const auto& pairs = _pairs.get( pair_id.raw(), "curve::apply_trade: `pair_id` does not exist");
        const bool is_in = pairs.reserve0.quantity.symbol == ext_in.quantity.symbol;
        const extended_asset reserve_in = is_in ? pairs.reserve0 : pairs.reserve1;
        const extended_asset reserve_out = is_in ? pairs.reserve1 : pairs.reserve0;

        // validate input quantity & reserves
        check(reserve_in.get_extended_symbol() == ext_in.get_extended_symbol(), "curve::apply_trade: invalid extended symbol");
        check(reserve_in.quantity.amount != 0 && reserve_out.quantity.amount != 0, "curve::apply_trade: empty pool reserves");

        // calculate out
        ext_out = { get_amount_out( ext_in.quantity, pair_id ), reserve_out.contract };

        // send protocol fees to fee account
        const extended_asset protocol_fee = { ext_in.quantity.amount * config.protocol_fee / 10000, ext_in.get_extended_symbol() };
        const extended_asset trade_fee = { ext_in.quantity.amount * config.trade_fee / 10000, ext_in.get_extended_symbol() };
        const extended_asset fee = protocol_fee + trade_fee;

        // modify reserves
        _pairs.modify( pairs, get_self(), [&]( auto & row ) {
            if ( is_in ) {
                row.reserve0.quantity += ext_in.quantity - protocol_fee.quantity;
                row.reserve1.quantity -= ext_out.quantity;
                row.volume0 += ext_in.quantity;
            } else {
                row.reserve1.quantity += ext_in.quantity - protocol_fee.quantity;
                row.reserve0.quantity -= ext_out.quantity;
                row.volume1 += ext_in.quantity;
            }
            // calculate last price
            const double price = calculate_price( ext_in.quantity, ext_out.quantity );
            row.amplifier = get_amplifier( pair_id );
            row.virtual_price = calculate_virtual_price( row.reserve0.quantity, row.reserve1.quantity, row.liquidity.quantity );
            row.price0_last = is_in ? 1 / price : price;
            row.price1_last = is_in ? price : 1 / price;
            row.trades += 1;
            row.last_updated = current_time_point();

            // swap log
            curve::swaplog_action swaplog( get_self(), { get_self(), "active"_n });
            swaplog.send( pair_id, owner, "swap"_n, ext_in.quantity, ext_out.quantity, fee.quantity, price, row.reserve0.quantity, row.reserve1.quantity );
        });
        // send protocol fees
        if ( protocol_fee.quantity.amount ) transfer( get_self(), config.fee_account, protocol_fee, "curve.sx: protocol fee");

        // swap input as output to prepare for next conversion
        ext_in = ext_out;
    }

    return ext_out;
}

[[eosio::action]]
void curve::deposit( const name owner, const symbol_code pair_id, const optional<int64_t> min_amount )
{
    require_auth( owner );

    curve::config_table _config( get_self(), get_self().value );
    curve::pairs_table _pairs( get_self(), get_self().value );
    curve::orders_table _orders( get_self(), pair_id.raw() );

    // configs
    check( _config.exists(), ERROR_CONFIG_NOT_EXISTS );
    auto config = _config.get();

    // get current order & pairs
    auto & pair = _pairs.get( pair_id.raw(), "curve::deposit: `pair_id` does not exist");
    auto & orders = _orders.get( owner.value, "curve::deposit: no deposits available for this user");
    check( orders.quantity0.quantity.amount && orders.quantity1.quantity.amount, "curve::deposit: one of the deposit is empty");

    // symbol helpers
    const symbol sym0 = pair.reserve0.quantity.symbol;
    const symbol sym1 = pair.reserve1.quantity.symbol;
    const uint8_t precision_norm = max( sym0.precision(), sym1.precision() );

    // calculate total deposits based on reserves: reserves ratio should remain the same
    // if reserves empty, fallback to 1
    const int128_t reserve0 = pair.reserve0.quantity.amount ? mul_amount(pair.reserve0.quantity.amount, precision_norm, sym0.precision()) : 1;
    const int128_t reserve1 = pair.reserve1.quantity.amount ? mul_amount(pair.reserve1.quantity.amount, precision_norm, sym1.precision()) : 1;
    const int128_t reserves = reserve0 + reserve1;

    // get owner order and calculate payment
    const int128_t amount0 = mul_amount(orders.quantity0.quantity.amount, precision_norm, sym0.precision());
    const int128_t amount1 = mul_amount(orders.quantity1.quantity.amount, precision_norm, sym1.precision());
    const int128_t payment = amount0 + amount1;

    // calculate actual amounts to deposit
    const int128_t deposit0 = (amount0 * reserves <= reserve0 * payment) ? amount0 : (amount1 * reserve0 / reserve1);
    const int128_t deposit1 = (amount0 * reserves <= reserve0 * payment) ? (amount0 * reserve1 / reserve0) : amount1;

    // send back excess deposit to owner
    if (deposit0 < amount0) {
        const int64_t excess_amount = div_amount(static_cast<int64_t>(amount0 - deposit0), precision_norm, sym0.precision());
        const extended_asset excess = { excess_amount, pair.reserve0.get_extended_symbol() };
        if(excess.quantity.amount) transfer( get_self(), owner, excess, "curve.sx: excess");
    }
    if (deposit1 < amount1) {
        const int64_t excess_amount = div_amount(static_cast<int64_t>(amount1 - deposit1), precision_norm, sym1.precision());
        const extended_asset excess = { excess_amount, pair.reserve1.get_extended_symbol() };
        if(excess.quantity.amount) transfer( get_self(), owner, excess, "curve.sx: excess");
    }

    // normalize final deposits
    const extended_asset ext_deposit0 = { div_amount(deposit0, precision_norm, sym0.precision()), pair.reserve0.get_extended_symbol()};
    const extended_asset ext_deposit1 = { div_amount(deposit1, precision_norm, sym1.precision()), pair.reserve1.get_extended_symbol()};

    // issue liquidity
    const int64_t supply = mul_amount(pair.liquidity.quantity.amount, precision_norm, pair.liquidity.quantity.symbol.precision());
    const int64_t issued_amount = rex::issue(deposit0 + deposit1, reserves, supply, 1);
    const extended_asset issued = { div_amount(issued_amount, precision_norm, pair.liquidity.quantity.symbol.precision()), pair.liquidity.get_extended_symbol()};

    // add liquidity deposits & newly issued liquidity
    _pairs.modify(pair, get_self(), [&]( auto & row ) {
        row.reserve0 += ext_deposit0;
        row.reserve1 += ext_deposit1;
        row.liquidity += issued;

        // log liquidity change
        curve::liquiditylog_action liquiditylog( get_self(), { get_self(), "active"_n });
        liquiditylog.send( pair_id, owner, "deposit"_n, issued.quantity, ext_deposit0.quantity, ext_deposit1.quantity, row.liquidity.quantity, row.reserve0.quantity, row.reserve1.quantity );
    });

    // issue & transfer to owner
    issue( issued, "curve.sx: deposit" );
    transfer( get_self(), owner, issued, "curve.sx: deposit");

    // deposit slippage protection
    if ( min_amount ) check( issued.quantity.amount >= *min_amount, "curve::deposit: deposit amount must exceed `min_amount`");

    // delete any remaining liquidity deposit order
    _orders.erase( orders );
}

// returns any remaining orders to owner account
[[eosio::action]]
void curve::cancel( const name owner, const symbol_code pair_id )
{
    if ( !has_auth( get_self() )) require_auth( owner );

    curve::orders_table _orders( get_self(), pair_id.raw() );
    auto & orders = _orders.get( owner.value, "curve::cancel: no deposits for this user in this pool");
    if ( orders.quantity0.quantity.amount ) transfer( get_self(), owner, orders.quantity0, "curve.sx: cancel");
    if ( orders.quantity1.quantity.amount ) transfer( get_self(), owner, orders.quantity1, "curve.sx: cancel");

    _orders.erase( orders );
}

[[eosio::action]]
void curve::removepair( const symbol_code pair_id )
{
    require_auth( get_self() );

    curve::pairs_table _pairs( get_self(), get_self().value );
    auto & pair = _pairs.get( pair_id.raw(), "curve::removepair: [pair_id] does not exist");
    check( pair.liquidity.quantity.amount == 0, "curve::removepair: liquidity amount must be empty");
    _pairs.erase( pair );
}

void curve::withdraw_liquidity( const name owner, const extended_asset value )
{
    curve::pairs_table _pairs( get_self(), get_self().value );

    // get current pairs
    const symbol_code pair_id = value.quantity.symbol.code();
    auto & pair = _pairs.get( pair_id.raw(), "curve::withdraw_liquidity: `pair_id` does not exist");

    // prevent invalid liquidity token contracts
    check(pair.liquidity.get_extended_symbol() == value.get_extended_symbol(), "curve::withdraw_liquidity: invalid extended symbol");

    // extended symbols
    const extended_symbol ext_sym0 = pair.reserve0.get_extended_symbol();
    const extended_symbol ext_sym1 = pair.reserve1.get_extended_symbol();
    const symbol sym0 = pair.reserve0.quantity.symbol;
    const symbol sym1 = pair.reserve1.quantity.symbol;
    const uint8_t precision_norm = max( sym0.precision(), sym1.precision() );

    // calculate total deposits based on reserves
    const int64_t supply = mul_amount(pair.liquidity.quantity.amount, precision_norm, pair.liquidity.quantity.symbol.precision());
    const int128_t reserve0 = pair.reserve0.quantity.amount ? mul_amount(pair.reserve0.quantity.amount, precision_norm, sym0.precision()) : 1;
    const int128_t reserve1 = pair.reserve1.quantity.amount ? mul_amount(pair.reserve1.quantity.amount, precision_norm, sym1.precision()) : 1;
    const int128_t reserves = reserve0 + reserve1;

    // calculate withdraw amounts
    const int64_t payment = mul_amount(value.quantity.amount, precision_norm, value.quantity.symbol.precision());
    const int64_t retire_amount = rex::retire( payment, reserves, supply );

    // get owner order and calculate payment
    int64_t amount0 = static_cast<int64_t>( retire_amount * reserve0 / reserves );
    int64_t amount1 = static_cast<int64_t>( retire_amount * reserve1 / reserves );
    if (amount0 == reserve0 || amount1 == reserve1) {         //deal with rounding error on final withdrawal
        amount0 = static_cast<int64_t>( reserve0 );
        amount1 = static_cast<int64_t>( reserve1 );
    }
    const extended_asset out0 = { div_amount(amount0, precision_norm, sym0.precision()), ext_sym0 };
    const extended_asset out1 = { div_amount(amount1, precision_norm, sym1.precision()), ext_sym1 };
    check( out0.quantity.amount || out1.quantity.amount, "curve::withdraw_liquidity: withdraw amount too small");

    // add liquidity deposits & newly issued liquidity
    _pairs.modify(pair, get_self(), [&]( auto & row ) {
        row.reserve0 -= out0;
        row.reserve1 -= out1;
        row.liquidity -= value;

        // log liquidity change
        curve::liquiditylog_action liquiditylog( get_self(), { get_self(), "active"_n });
        liquiditylog.send( pair_id, owner, "withdraw"_n, value.quantity, -out0.quantity, -out1.quantity, row.liquidity.quantity, row.reserve0.quantity, row.reserve1.quantity );
    });

    // issue & transfer to owner
    retire( value, "curve.sx: withdraw" );
    if ( out0.quantity.amount ) transfer( get_self(), owner, out0, "curve.sx: withdraw");
    if ( out1.quantity.amount ) transfer( get_self(), owner, out1, "curve.sx: withdraw");
}

void curve::add_liquidity( const name owner, const symbol_code pair_id, const extended_asset value )
{
    curve::pairs_table _pairs( get_self(), get_self().value );
    curve::orders_table _orders( get_self(), pair_id.raw() );

    // get current order & pairs
    auto pair = _pairs.get( pair_id.raw(), "curve::add_liquidity: `pair_id` does not exist");
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
        else check( false, "curve::add_liquidity: invalid extended symbol");
    };

    // create/modify order
    if ( itr == _orders.end() ) _orders.emplace( get_self(), insert );
    else _orders.modify( itr, get_self(), insert );
}

// increase/decrease amplifier of given pair id
[[eosio::action]]
void curve::ramp( const symbol_code pair_id, const uint64_t target_amplifier, const int64_t minutes )
{
    require_auth( get_self() );

    curve::ramp_table _ramp_table( get_self(), get_self().value );
    curve::pairs_table _pairs( get_self(), get_self().value );
    auto pair = _pairs.get(pair_id.raw(), "curve::ramp: `pair_id` does not exist in `pairs`");

    // validation
    check( target_amplifier > 0 && target_amplifier <= MAX_AMPLIFIER, "curve::ramp: target amplifier should be within within valid range");
    check( minutes > 0, "curve::ramp: minutes should be above 0");
    check( minutes * 60 >= MIN_RAMP_TIME, "curve::ramp: minimum ramp timeframe must exceed " + to_string(MIN_RAMP_TIME) + " seconds");

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
void curve::stopramp( const symbol_code pair_id )
{
    require_auth( get_self() );

    curve::ramp_table _ramp( get_self(), get_self().value );
    auto & ramp = _ramp.get(pair_id.raw(), "curve::stopramp: `pair_id` does not exist in `ramp` table");
    _ramp.erase( ramp );
}

[[eosio::action]]
void curve::setfee( const uint8_t trade_fee, const optional<uint8_t> protocol_fee, const optional<name> fee_account )
{
    require_auth( get_self() );

    // config
    curve::config_table _config( get_self(), get_self().value );
    check( _config.exists(), ERROR_CONFIG_NOT_EXISTS );
    auto config = _config.get();

    // required params
    check( trade_fee <= MAX_TRADE_FEE, "curve::setfee: `trade_fee` has exceeded maximum limit");
    check( *protocol_fee <= MAX_PROTOCOL_FEE, "curve::setfee: `protocol_fee` has exceeded maximum limit");

    // optional params
    if ( fee_account->value ) check( is_account( *fee_account ), "curve::setfee: `fee_account` does not exist");
    if ( *protocol_fee ) check( fee_account->value, "curve::setfee: must provide `fee_account` if `protocol_fee` is defined");

    // set config
    config.trade_fee = trade_fee;
    config.protocol_fee = *protocol_fee;
    config.fee_account = *fee_account;
    _config.set( config, get_self() );
}


[[eosio::action]]
void curve::setnotifiers( const vector<name> notifiers )
{
    require_auth( get_self() );

    for ( const name notifier : notifiers ) {
        check( is_account( notifier ), "curve::setnotifiers: `notifier` does not exist");
    }
    curve::config_table _config( get_self(), get_self().value );
    check( _config.exists(), ERROR_CONFIG_NOT_EXISTS );
    auto config = _config.get();
    config.notifiers = notifiers;
    _config.set( config, get_self() );
}

[[eosio::action]]
void curve::setstatus( const name status )
{
    require_auth( get_self() );

    curve::config_table _config( get_self(), get_self().value );
    check( _config.exists(), ERROR_CONFIG_NOT_EXISTS );
    auto config = _config.get();
    config.status = status;
    _config.set( config, get_self() );
}

[[eosio::action]]
void curve::createpair( const name creator, const symbol_code pair_id, const extended_symbol reserve0, const extended_symbol reserve1, const uint64_t amplifier )
{
    // `creator` must be contract itself during beta period
    if ( !has_auth( get_self() ) ) check( false, "curve::createpair: `creator` is disabled from creating pair during beta period");
    require_auth( creator );

    // tables
    curve::pairs_table _pairs( get_self(), get_self().value );
    curve::config_table _config( get_self(), get_self().value );
    check( _config.exists(), ERROR_CONFIG_NOT_EXISTS );
    const name token_contract = _config.get().token_contract;

    // reserve params
    const name contract0 = reserve0.get_contract();
    const name contract1 = reserve1.get_contract();
    const symbol sym0 = reserve0.get_symbol();
    const symbol sym1 = reserve1.get_symbol();

    // check reserves
    check( is_account( contract0 ), "curve::createpair: reserve0 contract does not exists");
    check( is_account( contract1 ), "curve::createpair: reserve1 contract does not exists");
    check( token::get_supply( contract0, sym0.code() ).symbol == sym0, "curve::createpair: reserve0 extended symbol mismatch supply" );
    check( token::get_supply( contract1, sym1.code() ).symbol == sym1, "curve::createpair: reserve1 extended symbol mismatch supply" );
    check( _pairs.find( pair_id.raw() ) == _pairs.end(), "curve::createpair: `pair_id` already exists" );
    check( amplifier > 0 && amplifier <= MAX_AMPLIFIER, "curve::createpair: invalid amplifier" );

    // create liquidity token
    const extended_symbol liquidity = {{ pair_id, max(sym0.precision(), sym1.precision())}, token_contract };

    // in case supply already exists
    token::stats _stats( token_contract, pair_id.raw() );
    auto stats_itr = _stats.find( pair_id.raw() );

    // create token if supply does not exist
    if ( stats_itr == _stats.end() ) create( liquidity );
    // supply must be empty
    else check( !stats_itr->supply.amount, "curve::createpair: creating new pair requires existing supply to be zero" );

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
double curve::calculate_virtual_price( const asset value0, const asset value1, const asset supply )
{
    const uint8_t precision_norm = max( value0.symbol.precision(), value1.symbol.precision() );
    const int64_t amount0 = mul_amount( value0.amount, precision_norm, value0.symbol.precision() );
    const int64_t amount1 = mul_amount( value1.amount, precision_norm, value1.symbol.precision() );
    const int64_t amount2 = mul_amount( supply.amount, precision_norm, supply.symbol.precision() );
    return static_cast<double>( safemath::add(amount0, amount1) ) / amount2;
}

// calculate last price per trade
double curve::calculate_price( const asset value0, const asset value1 )
{
    const uint8_t precision_norm = max( value0.symbol.precision(), value1.symbol.precision() );
    const int64_t amount0 = mul_amount( value0.amount, precision_norm, value0.symbol.precision() );
    const int64_t amount1 = mul_amount( value1.amount, precision_norm, value1.symbol.precision() );
    return static_cast<double>(amount0) / amount1;
}

// Memo schemas
// ============
// Swap: `swap,<min_return>,<pair_ids>` (ex: "swap,0,SXA" )
// Deposit: `deposit,<pair_id>` (ex: "deposit,SXA")
// Withdrawal: `` (empty)
curve::memo_schema curve::parse_memo( const string memo )
{
    if(memo == "") return {};

    // split memo into parts
    const vector<string> parts = sx::utils::split(memo, ",");
    check(parts.size() <= 3, ERROR_INVALID_MEMO );

    // memo result
    memo_schema result;
    result.action = sx::utils::parse_name(parts[0]);
    result.min_return = 0;

    // swap action
    if ( result.action == "swap"_n ) {
        result.pair_ids = parse_memo_pair_ids( parts[2] );
        check( sx::utils::is_digit( parts[1] ), ERROR_INVALID_MEMO );
        result.min_return = std::stoll( parts[1] );
        check( result.min_return >= 0, ERROR_INVALID_MEMO );
        check( result.pair_ids.size() >= 1, ERROR_INVALID_MEMO );

    // deposit action
    } else if ( result.action == "deposit"_n ) {
        result.pair_ids = parse_memo_pair_ids( parts[1] );
        check( result.pair_ids.size() == 1, ERROR_INVALID_MEMO );
    }
    return result;
}

// Memo schemas
// ============
// Single: `<pair_id>` (ex: "SXA")
// Multiple: `<pair_id>-<pair_id>` (ex: "SXA-SXB")
vector<symbol_code> curve::parse_memo_pair_ids( const string memo )
{
    curve::pairs_table _pairs( get_self(), get_self().value );

    set<symbol_code> duplicates;
    vector<symbol_code> pair_ids;
    for ( const string str : sx::utils::split(memo, "-") ) {
        const symbol_code symcode = sx::utils::parse_symbol_code( str );
        check( symcode.raw(), ERROR_INVALID_MEMO );
        check( _pairs.find( symcode.raw() ) != _pairs.end(), "curve::parse_memo_pair_ids: `pair_id` does not exist");
        pair_ids.push_back( symcode );
        check( !duplicates.count( symcode ), "curve::parse_memo_pair_ids: invalid duplicate `pair_ids`");
        duplicates.insert( symcode );
    }
    return pair_ids;
}

[[eosio::action]]
void curve::calculate( const uint64_t amount, const uint64_t reserve_in, const uint64_t reserve_out, const uint64_t amplifier, const uint64_t fee )
{
    const uint64_t out = Curve::get_amount_out( amount, reserve_in, reserve_out, amplifier, fee );
    check(false, "current get_amount_out(amount: " + to_string(amount) + ", amp: " + to_string(amplifier) + "  ): " + to_string(out) );
}

} // namespace sx