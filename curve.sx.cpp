#include <eosio.token/eosio.token.hpp>
#include <sx.utils/utils.hpp>
#include <sx.rex/rex.hpp>

#include "curve.sx.hpp"
#include "src/maintenance.cpp"

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

    // config
    sx::curve::config_table _config( get_self(), get_self().value );
    check( _config.exists(), "contract is under maintenance");

    // TEMP - DURING TESTING PERIOD
    check( from.suffix() == "sx"_n || from == "myaccount"_n, "account must be *.sx during testing period");

    // user input params
    auto [ ext_min_out, receiver ] = parse_memo(memo);
    receiver = receiver.value ? receiver : from;
    const symbol_code symcode_memo = ext_min_out.quantity.symbol.code();
    const extended_asset ext_in {quantity, get_first_receiver()};

    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    auto itr = _pairs.find(symcode_memo.raw());

    // withdraw liquidity
    if ( _pairs.find( quantity.symbol.code().raw() ) != _pairs.end() ) {
        withdraw_liquidity( from, ext_in );
    // add liquidity
    } else if (itr != _pairs.end()) {
        // add liquidity
        check( memo == symcode_memo.to_string(), "memo should contain liquidity symbol code only");
        check( from == receiver, "receiver cannot be defined when adding liquidity" );
        add_liquidity( from, symcode_memo, ext_in );
    }
    // swap convert (ex: USDT => USN)
    else {
        convert(ext_in, ext_min_out, receiver);
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
    check( _config.exists(), "contract is under maintenance");
    auto config = _config.get();

    // get current order & pairs
    auto & pairs = _pairs.get( pair_id.raw(), "pair does not exist");
    auto itr = _orders.find( owner.value );
    check( itr != _orders.end(), "no deposits for this user");
    check((pairs.reserve0.quantity.amount && pairs.reserve1.quantity.amount) || (pairs.reserve0.quantity.amount==0 && pairs.reserve0.quantity.amount==0), "invalid pair reserves");

    const symbol sym0 = pairs.reserve0.quantity.symbol;
    const symbol sym1 = pairs.reserve1.quantity.symbol;

    // calculate total deposits based on reserves
    const int64_t supply = mul_amount(pairs.liquidity.quantity.amount, MAX_PRECISION, pairs.liquidity.quantity.symbol.precision());
    const int64_t reserve0 = pairs.reserve0.quantity.amount ? mul_amount(pairs.reserve0.quantity.amount, MAX_PRECISION, sym0.precision()) : 1;
    const int64_t reserve1 = pairs.reserve1.quantity.amount ? mul_amount(pairs.reserve1.quantity.amount, MAX_PRECISION, sym1.precision()) : 1;
    const int64_t reserves = reserve0 + reserve1;
    const double reserve_ratio0 = double(reserve0) / reserves;
    const double reserve_ratio1 = double(reserve1) / reserves;

    // get owner order and calculate payment
    const int64_t amount0 = mul_amount(itr->quantity0.quantity.amount, MAX_PRECISION, sym0.precision());
    const int64_t amount1 = mul_amount(itr->quantity1.quantity.amount, MAX_PRECISION, sym1.precision());
    const int64_t payment = amount0 + amount1;
    const double amount_ratio0 = double(amount0) / payment;
    const double amount_ratio1 = double(amount1) / payment;

    //actual amounts to deposit
    int64_t deposit0 = amount0;
    int64_t deposit1 = amount1;

    print( "\nexisting supply: " + to_string(supply) + "\n");
    print( "existing reserve0: " + to_string(reserve0) + "\n");
    print( "existing reserve1: " + to_string(reserve1) + "\n");
    print( "reserve_ratio0: " + to_string(reserve_ratio0) + "\n");
    print( "reserve_ratio1: " + to_string(reserve_ratio1) + "\n");
    print( "reserves: " + to_string(reserves) + "\n");
    print( "payment: " + to_string(payment) + "\n");
    print( "amount_ratio0: " + to_string(amount_ratio0) + "\n");
    print( "amount_ratio1: " + to_string(amount_ratio1) + "\n");
    print( "incoming amount0: " + to_string(amount0) + "\n");
    print( "incoming amount1: " + to_string(amount1) + "\n");

    if (amount_ratio0 <= reserve_ratio0) deposit1 = ((uint128_t) amount0) * reserve1 / reserve0;
    else deposit0 = ((uint128_t) amount1) * reserve0 / reserve1;

    print( "to deposit0: " + to_string(deposit0) + "\n");
    print( "to deposit1: " + to_string(deposit1) + "\n");

    if (deposit0 < amount0) {
        const int64_t excess_amount = div_amount(amount0, MAX_PRECISION, sym0.precision()) - div_amount(deposit0, MAX_PRECISION, sym0.precision());
        const extended_asset excess = { excess_amount, pairs.reserve0.get_extended_symbol() };
        transfer( get_self(), owner, excess, "excess");
        print("\nSending excess ", excess, " to ", owner);
    }
    if (deposit1 < amount1) {
        const int64_t excess_amount = div_amount(amount1, MAX_PRECISION, sym1.precision()) - div_amount(deposit1, MAX_PRECISION, sym1.precision());
        const extended_asset excess = { excess_amount, pairs.reserve1.get_extended_symbol() };
        transfer( get_self(), owner, excess, "excess");
        print("\nSending excess ", excess, " to ", owner);
    }

    const extended_asset ext_deposit0 = { div_amount(deposit0, MAX_PRECISION, sym0.precision()), pairs.reserve0.get_extended_symbol()};
    const extended_asset ext_deposit1 = { div_amount(deposit1, MAX_PRECISION, sym1.precision()), pairs.reserve1.get_extended_symbol()};

    // issue liquidity
    const int64_t issued_amount = rex::issue(deposit0 + deposit1, reserves, supply, 1);
    const extended_asset issued = { div_amount(issued_amount, MAX_PRECISION, pairs.liquidity.quantity.symbol.precision()), pairs.liquidity.get_extended_symbol()};

    print( "\nDepositing: ", ext_deposit0, " + ", ext_deposit1);
    print( "\nIssuing ", issued, " and sending to ", owner);

    // add liquidity deposits & newly issued liquidity
    _pairs.modify(pairs, get_self(), [&]( auto & row ) {
        row.reserve0 += ext_deposit0;
        row.reserve1 += ext_deposit1;
        row.liquidity += issued;
    });

    // issue & transfer to owner
    issue( issued, "deposit" );
    transfer( get_self(), owner, issued, "deposit");

    // delete any remaining liquidity deposit order
    // check(false, "see dev print");
    _orders.erase( itr );
}

// returns any remaining orders to owner account
[[eosio::action]]
void sx::curve::cancel( const name owner, const symbol_code pair_id )
{
    if ( !has_auth( get_self() )) require_auth( owner );

    sx::curve::orders_table _orders( get_self(), pair_id.raw() );
    auto itr = _orders.find( owner.value );
    check( itr != _orders.end(), "no deposits for this user in this pool");
    if ( itr->quantity0.quantity.amount ) transfer( get_self(), owner, itr->quantity0, "cancel");
    if ( itr->quantity1.quantity.amount ) transfer( get_self(), owner, itr->quantity1, "cancel");

    _orders.erase( itr );
}

void sx::curve::withdraw_liquidity( const name owner, const extended_asset value )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    // get current pairs
    auto & pairs = _pairs.get( value.quantity.symbol.code().raw(), "pairs does not exist");

    // extended symbols
    const extended_symbol ext_sym0 = pairs.reserve0.get_extended_symbol();
    const extended_symbol ext_sym1 = pairs.reserve1.get_extended_symbol();
    const symbol sym0 = pairs.reserve0.quantity.symbol;
    const symbol sym1 = pairs.reserve1.quantity.symbol;

    // calculate total deposits based on reserves
    const int64_t supply = mul_amount(pairs.liquidity.quantity.amount, MAX_PRECISION, pairs.liquidity.quantity.symbol.precision());
    const int64_t reserve0 = pairs.reserve0.quantity.amount ? mul_amount(pairs.reserve0.quantity.amount, MAX_PRECISION, sym0.precision()) : 1;
    const int64_t reserve1 = pairs.reserve1.quantity.amount ? mul_amount(pairs.reserve1.quantity.amount, MAX_PRECISION, sym1.precision()) : 1;
    const int64_t reserves = reserve0 + reserve1;
    const double reserve_ratio0 = double(reserve0) / reserves;
    const double reserve_ratio1 = double(reserve1) / reserves;

    // calculate withdraw amounts
    const int64_t payment = mul_amount(value.quantity.amount, MAX_PRECISION, value.quantity.symbol.precision());
    const int64_t retire_amount = rex::retire( payment, reserves, supply );

    // // get owner order and calculate payment
    const int64_t amount0 = retire_amount * reserve_ratio0;
    const int64_t amount1 = retire_amount * reserve_ratio1;
    const extended_asset out0 = { div_amount(amount0, MAX_PRECISION, sym0.precision()), ext_sym0 };
    const extended_asset out1 = { div_amount(amount1, MAX_PRECISION, sym1.precision()), ext_sym1 };
    check( out0.quantity.amount && out1.quantity.amount, "withdraw amount too small");

    print( "\nexisting supply: ", supply, "\n");
    print( "existing reserve0: ", reserve0, "\n");
    print( "existing reserve1: ", reserve1, "\n");
    print( "reserve_ratio0: ", reserve_ratio0, "\n");
    print( "reserve_ratio1: ", reserve_ratio1, "\n");
    print( "reserves: ", reserves, "\n");
    print( "payment: ", payment, "\n");
    print( "retire_amount: ", retire_amount, "\n");
    print( "outgoing amount0: ", amount0, "\n");
    print( "outgoing amount1: ", amount1, "\n");
    print( "outgoing out0: ", out0, "\n");
    print( "outgoing out1: ", out1, "\n");

    print( "\nDepositing: ", value);
    print( "\nWithdrawing ", out0, " + ", out1, " to ", owner);

    // add liquidity deposits & newly issued liquidity
    _pairs.modify(pairs, get_self(), [&]( auto & row ) {
        row.reserve0 -= out0;
        row.reserve1 -= out1;
        row.liquidity -= value;
    });

    // issue & transfer to owner
    retire( value, "withdraw" );
    transfer( get_self(), owner, out0, "withdraw");
    transfer( get_self(), owner, out1, "withdraw");
}

void sx::curve::add_liquidity( const name owner, const symbol_code pair_id, const extended_asset value )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    sx::curve::orders_table _orders( get_self(), pair_id.raw() );

    // get current order & pairs
    auto pairs = _pairs.get( pair_id.raw(), "pairs does not exist");
    auto itr = _orders.find( owner.value );

    // extended symbols
    const extended_symbol ext_sym_in = value.get_extended_symbol();
    const extended_symbol ext_sym0 = pairs.reserve0.get_extended_symbol();
    const extended_symbol ext_sym1 = pairs.reserve1.get_extended_symbol();

    // initialize quantities
    auto insert = [&]( auto & row ) {
        row.owner = owner;
        row.quantity0 = { itr == _orders.end() ? 0 : itr->quantity0.quantity.amount, ext_sym0 };
        row.quantity1 = { itr == _orders.end() ? 0 : itr->quantity1.quantity.amount, ext_sym1 };

        // add & validate deposit
        if ( ext_sym_in == ext_sym0 ) row.quantity0 += value;
        else if ( ext_sym_in == ext_sym1 ) row.quantity1 += value;
        else check( false, "invalid extended symbol when adding liquidity");
    };

    // create/modify order
    if ( itr == _orders.end() ) _orders.emplace( get_self(), insert );
    else _orders.modify( itr, get_self(), insert );
}

void sx::curve::convert(const extended_asset ext_in, const extended_asset ext_min_out, name receiver) {

    update_amplifiers();

    // find all possible trade paths
    auto paths = find_trade_paths( ext_in.quantity.symbol.code(), ext_min_out.quantity.symbol.code() );
    check(paths.size(), "no path for exchange");

    // choose trade path that gets the best return
    auto best_path = paths[0];
    extended_asset best_out;
    for (const auto& path: paths) {
        auto out = apply_trade(ext_in, path);
        if(out.quantity.amount > best_out.quantity.amount) {
            best_path = path;
            best_out = out;
        }
    }
    check(best_out.quantity.amount, "no matching exchange");
    check(ext_min_out.contract.value == 0 || ext_min_out.contract == best_out.contract, "reserve_out vs memo contract mismatch");
    check(ext_min_out.quantity.amount == 0 || ext_min_out.quantity.symbol == best_out.quantity.symbol, "return vs memo symbol precision mismatch");
    check(ext_min_out.quantity.amount == 0 || ext_min_out.quantity.amount <= best_out.quantity.amount, "return is not enough");

    // execute the trade by updating all involved pools
    best_out = apply_trade(ext_in, best_path, true);

    // transfer amount to receiver
    transfer( get_self(), receiver, best_out, "swap" );
}

pair<extended_asset, name> sx::curve::parse_memo(const string memo){

    name receiver;
    auto rmemo = sx::utils::split(memo, ",");

    // check(rmemo.size() < 3 && rmemo.size() > 0, "invalid memo format");
    if ( rmemo.size() == 2 ) {
        receiver = sx::utils::parse_name(rmemo[1]);
        check(receiver.value, "invalid receiver name in memo");
        check(is_account(receiver), "receiver account does not exist");
    }

    auto sym_code = sx::utils::parse_symbol_code(rmemo[0]);
    if (sym_code.is_valid()) return { extended_asset{ asset{0, symbol{sym_code, 0} }, ""_n}, receiver };

    auto quantity = sx::utils::parse_asset(rmemo[0]);
    if (quantity.is_valid()) return { extended_asset{quantity, ""_n}, receiver };

    auto ext_out = sx::utils::parse_extended_asset(rmemo[0]);
    if ( ext_out.contract.value ) check( is_account( ext_out.contract ), "extended asset contract account does not exist");
    if ( ext_out.quantity.is_valid()) return { ext_out, receiver };

    // check(false, "invalid memo");
    return {};
}

// find pair_id based on symbol_code of incoming and target tokens
symbol_code sx::curve::find_pair_id( const symbol_code symcode_in, const symbol_code symcode_out )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    // find by combination of input quantity & memo symbol
    auto _pairs_by_reserves = _pairs.get_index<"byreserves"_n>();
    auto itr = _pairs_by_reserves.find( compute_by_symcodes( symcode_in, symcode_out ) );
    if ( itr != _pairs_by_reserves.end() ) return itr->id;

    itr = _pairs_by_reserves.find( compute_by_symcodes( symcode_out, symcode_in ) );
    if ( itr != _pairs_by_reserves.end() ) return itr->id;

    // nothing found - return empty
    return {};
}

[[eosio::action]]
void sx::curve::setconfig( const std::optional<sx::curve::config_row> config )
{
    require_auth( get_self() );
    sx::curve::config_table _config( get_self(), get_self().value );

    // clear table if setting is `null`
    if ( !config ) return _config.remove();

    _config.set( *config, get_self() );
}

[[eosio::action]]
void sx::curve::createpair( const name creator, const symbol_code pair_id, const extended_symbol reserve0, const extended_symbol reserve1, const uint64_t amplifier )
{
    if ( !has_auth( get_self() )) check(false, "`createpair` is currently disabled at the moment");

    sx::curve::pairs_table _pairs( get_self(), get_self().value );

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
    check( !find_pair_id( sym0.code(), sym1.code() ).is_valid(), "pair with these reserves already exists" );
    check( _pairs.find( pair_id.raw() ) == _pairs.end(), "pair id already exists" );

    // create liquidity token
    const extended_symbol liquidity = {{ pair_id, max(sym0.precision(), sym1.precision())}, TOKEN_CONTRACT };
    create( liquidity );

    // create pair
    _pairs.emplace( get_self(), [&]( auto & row ) {
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

// find all possible paths to exchange symcode_in to memo symcode, include 2-hops
vector<vector<symbol_code>> sx::curve::find_trade_paths( const symbol_code symcode_in, const symbol_code symcode_out )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    check( symcode_in != symcode_out, "conversion target symbol must not match incoming symbol");

    // find first valid hop
    vector<vector<symbol_code>> paths;
    vector<pair<symbol_code, symbol_code>> hop_one;     // {pair id, first hop symbol_code}
    for (const auto& row : _pairs) {
        symbol_code sc1 = row.reserve0.quantity.symbol.code();
        symbol_code sc2 = row.reserve1.quantity.symbol.code();

        if (sc1 != symcode_in) std::swap(sc1, sc2);
        if (sc1 != symcode_in) continue;
        if (sc2 == symcode_out) paths.push_back({row.id});  //direct path
        else hop_one.push_back({row.id, sc2});
    }

    // find all possible second hops
    for(const auto& [id1, sc_in] : hop_one) {
        auto id2 = find_pair_id(sc_in, symcode_out);
        if(id2.is_valid()) paths.push_back({id1, id2});
    }

    return paths;
}

extended_asset sx::curve::apply_trade( const extended_asset ext_quantity, const vector<symbol_code>& path, const bool finalize /*=false*/ )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    extended_asset ext_out, ext_in = ext_quantity;
    check( path.size(), "exchange path is empty");
    for (auto pair_id : path) {
        const auto& pairs = _pairs.get( pair_id.raw(), "pair id does not exist");
        const bool is_in = pairs.reserve0.quantity.symbol == ext_in.quantity.symbol;
        const extended_asset reserve_in = is_in ? pairs.reserve0 : pairs.reserve1;
        const extended_asset reserve_out = is_in ? pairs.reserve1 : pairs.reserve0;

        if (reserve_in.get_extended_symbol() != ext_in.get_extended_symbol()) {
            check(!finalize, "incoming currency/reserves contract mismatch");
            return {};
        }
        if (reserve_in.quantity.amount == 0 || reserve_out.quantity.amount == 0) {
            check(!finalize, "empty pool reserves");
            return {};
        }

        // calculate out
        ext_out = { get_amount_out( ext_in.quantity, pair_id ), reserve_out.contract };

        if (finalize) {
            // modify reserves
            _pairs.modify( pairs, get_self(), [&]( auto & row ) {
                if ( is_in ) {
                    row.reserve0.quantity += ext_in.quantity;
                    row.reserve1.quantity -= ext_out.quantity;
                    row.volume0 += ext_in.quantity;
                } else {
                    row.reserve1.quantity += ext_in.quantity;
                    row.reserve0.quantity -= ext_out.quantity;
                    row.volume1 += ext_in.quantity;
                }
                // calculate last price
                const double price = calculate_price( ext_in.quantity, ext_out.quantity );
                row.virtual_price = calculate_virtual_price( row.reserve0.quantity, row.reserve1.quantity, row.liquidity.quantity );
                row.price0_last = is_in ? 1 / price : price;
                row.price1_last = is_in ? price : 1 / price;
                row.last_updated = current_time_point();
            });
        }
        ext_in = ext_out;
    }

    return ext_out;
}

double sx::curve::calculate_virtual_price( const asset value0, const asset value1, const asset supply )
{
    const int64_t amount0 = mul_amount( value0.amount, MAX_PRECISION, value0.symbol.precision() );
    const int64_t amount1 = mul_amount( value1.amount, MAX_PRECISION, value1.symbol.precision() );
    const int64_t amount2 = mul_amount( supply.amount, MAX_PRECISION, supply.symbol.precision() );
    return static_cast<double>( amount0 + amount1 ) / amount2;
}

double sx::curve::calculate_price( const asset value0, const asset value1 )
{
    const int64_t amount0 = mul_amount( value0.amount, MAX_PRECISION, value0.symbol.precision() );
    const int64_t amount1 = mul_amount( value1.amount, MAX_PRECISION, value1.symbol.precision() );
    return static_cast<double>(amount0) / amount1;
}

void sx::curve::create( const extended_symbol value )
{
    eosio::token::create_action create( value.get_contract(), { value.get_contract(), "active"_n });
    create.send( get_self(), asset{ asset_max, value.get_symbol() } );
}

void sx::curve::issue( const extended_asset value, const string memo )
{
    eosio::token::issue_action issue( value.contract, { get_self(), "active"_n });
    issue.send( get_self(), value.quantity, memo );
}

void sx::curve::retire( const extended_asset value, const string memo )
{
    eosio::token::retire_action retire( value.contract, { get_self(), "active"_n });
    retire.send( value.quantity, memo );
}

void sx::curve::transfer( const name from, const name to, const extended_asset value, const string memo )
{
    eosio::token::transfer_action transfer( value.contract, { from, "active"_n });
    transfer.send( from, to, value.quantity, memo );
}


[[eosio::action]]
void sx::curve::adjustampl( const symbol_code pair_id, uint64_t new_value, uint64_t minutes )
{
    require_auth( get_self() );

    sx::curve::amplifier_table _ampl_table( get_self(), get_self().value );
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    auto pair = _pairs.get(pair_id.raw(), "pair does not exist");

    check(minutes > 0, "minutes should be > 0");

    auto insert = [&]( auto & row ) {
        row.id = pair_id;
        row.target_value = new_value;
        row.start_value = pair.amplifier;
        row.start_ts =  current_time_point();
        row.end_ts = current_time_point() + eosio::minutes(minutes);    //replace with hours in production
        row.last_updated = current_time_point();
    };

    auto itr = _ampl_table.find(pair_id.raw());
    if ( itr == _ampl_table.end() ) _ampl_table.emplace( get_self(), insert );
    else _ampl_table.modify( itr, get_self(), insert );
}

void sx::curve::update_amplifiers( )
{
    sx::curve::amplifier_table _ampl_table( get_self(), get_self().value );
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    time_point_sec now = current_time_point();
    auto itr = _ampl_table.begin();
    while ( itr != _ampl_table.end() ) {
        auto pairs_itr = _pairs.find(itr->id.raw());
        if(pairs_itr == _pairs.end()){
            itr = _ampl_table.erase(itr);   //orphane record, i.e. pair had been removed - just erase it
            continue;
        }

        auto total = itr->end_ts.utc_seconds - itr->start_ts.utc_seconds;
        auto since = now.utc_seconds - itr->start_ts.utc_seconds;
        auto new_value =  itr->start_value + (int64_t(itr->target_value) - int64_t(itr->start_value)) * since / total;
        if(now.utc_seconds - itr->last_updated.utc_seconds < 30) return;    //minimum update frequency in seconds
        if(now >= itr->end_ts) new_value = itr->target_value;

        _pairs.modify( pairs_itr, get_self(), [&]( auto & row ) {
            row.amplifier = new_value;
        });

        _ampl_table.modify( itr, get_self(), [&]( auto & row ) {
            row.last_updated = current_time_point();
        });

        if(itr->end_ts <= now) itr = _ampl_table.erase(itr);
        else ++itr;
    }
}