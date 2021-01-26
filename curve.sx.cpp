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
    auto config = _config.get();

    // TEMP - DURING TESTING PERIOD
    check( from.suffix() == "sx"_n || from == "myaccount"_n, "account must be *.sx during testing period");

    // user input params
    const name contract = get_first_receiver();
    auto [ min_ext_out, receiver ] = parse_memo(memo);
    receiver = receiver.value ? receiver : from;
    const symbol_code memo_symcode = min_ext_out.quantity.symbol.code();
    const extended_asset ext_in {quantity, contract};

    // find all possible trade paths
    auto paths = find_trade_paths( quantity.symbol.code(), memo_symcode );
    check(paths.size(), "no path for a trade");

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

    check(best_out.quantity.amount, "no matching trade");
    check(min_ext_out.contract.value == 0 || min_ext_out.contract == best_out.contract, "reserve_out vs memo contract mismatch");
    check(min_ext_out.quantity.amount == 0 || min_ext_out.quantity.symbol == best_out.quantity.symbol, "return vs memo symbol precision mismatch");
    check(min_ext_out.quantity.amount == 0 || min_ext_out.quantity.amount <= best_out.quantity.amount, "return is not enough");

    // execute the trade by updating all involved pools
    best_out = apply_trade(ext_in, best_path, true);

    // transfer amount to receiver
    transfer( get_self(), receiver, best_out, "swap" );
}

pair<extended_asset, name> sx::curve::parse_memo(string memo){

    name receiver;
    auto arr = sx::utils::split(memo, ",");

    check(arr.size() < 3 && arr.size() > 0, "invalid memo format");
    if ( arr.size() == 2 ) {
        receiver = sx::utils::parse_name(arr[1]);
        check(receiver.value, "invalid receiver name in memo");
        check(is_account(receiver), "receiver account does not exist");
    }

    memo = arr[0];

    auto sym_code = sx::utils::parse_symbol_code(memo);
    if (sym_code.is_valid()) return { extended_asset{ asset{0, symbol{sym_code, 0} }, ""_n}, receiver };

    auto quantity = sx::utils::parse_asset(memo);
    if (quantity.is_valid()) return { extended_asset{quantity, ""_n}, receiver };

    auto ext_out = sx::utils::parse_extended_asset(memo);
    if ( ext_out.contract.value ) check( is_account( ext_out.contract ), "extended asset contract account does not exist");
    if ( ext_out.quantity.is_valid()) return { ext_out, receiver };

    check(false, "invalid memo");
    return {};
}

// find pair_id based on symbol_code of incoming tokens and memo
symbol_code sx::curve::find_pair_id( const symbol_code symcode_in, const symbol_code symcode_memo )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    // find by input quantity
    auto itr0 = _pairs.find( symcode_in.raw() );
    if ( itr0 != _pairs.end() ) return itr0->id;

    // find by memo symbol
    auto itr1 = _pairs.find( symcode_memo.raw() );
    if ( itr1 != _pairs.end() && (itr1->reserve0.quantity.symbol.code()==symcode_in || itr1->reserve1.quantity.symbol.code()==symcode_in))
        return itr1->id;

    // find by combination of input quantity & memo symbol
    auto _pairs_by_reserves = _pairs.get_index<"byreserves"_n>();
    auto itr = _pairs_by_reserves.find( compute_by_symcodes( symcode_in, symcode_memo ) );
    if ( itr != _pairs_by_reserves.end() ) return itr->id;

    itr = _pairs_by_reserves.find( compute_by_symcodes( symcode_memo, symcode_in ) );
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
void sx::curve::setpair( const symbol_code id, const extended_asset reserve0, const extended_asset reserve1, const uint64_t amplifier )
{
    require_auth( get_self() );
    sx::curve::pairs_table _pairs( get_self(), get_self().value );

    // reserve params
    const name contract0 = reserve0.contract;
    const name contract1 = reserve1.contract;
    const symbol sym0 = reserve0.quantity.symbol;
    const symbol sym1 = reserve1.quantity.symbol;

    // normalize reserves
    const int64_t amount0 = mul_amount(reserve0.quantity.amount, MAX_PRECISION, sym0.precision());
    const int64_t amount1 = mul_amount(reserve1.quantity.amount, MAX_PRECISION, sym1.precision());

    // check reserves
    check( is_account( contract0 ), "reserve0 contract does not exists");
    check( is_account( contract1 ), "reserve1 contract does not exists");
    check( token::get_supply( contract0, sym0.code() ).symbol == sym0, "reserve0 symbol mismatch" );
    check( token::get_supply( contract1, sym1.code() ).symbol == sym1, "reserve1 symbol mismatch" );
    check( amount0 == amount1, "reserve0 & reserve1 normalized amount must match");

    // create liquidity token
    const extended_asset liquidity = { asset{ amount0 + amount1, { id, MAX_PRECISION }}, get_self() };

    // pairs content
    auto insert = [&]( auto & row ) {
        row.id = id;
        row.reserve0 = reserve0;
        row.reserve1 = reserve1;
        row.liquidity = liquidity;
        row.amplifier = amplifier;
        row.volume0 = { 0, sym0 };
        row.volume1 = { 0, sym1 };
        row.last_updated = current_time_point();
    };

    // create/modify pairs
    auto itr = _pairs.find( id.raw() );
    if ( itr == _pairs.end() ) _pairs.emplace( get_self(), insert );
    else check( false, "`setpair` cannot modify, must first `delete` pair");
}

// find all possible paths to trade symcode_in to memo symcode, include 2-hops
vector<vector<symbol_code>> sx::curve::find_trade_paths( const symbol_code symcode_in, const symbol_code symcode_memo )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    check( symcode_in != symcode_memo, "memo symbol must not match quantity symbol");

    vector<vector<symbol_code>> paths;

    //if direct path exists
    auto direct = find_pair_id(symcode_in, symcode_memo);
    if (direct.is_valid()) paths.push_back({ direct });

    //then - try to find via 2 hops
    if (_pairs.find(symcode_memo.raw()) != _pairs.end()) return paths;    // LP token memo only for direct trades

    for (const auto& row : _pairs) {
        symbol_code sc1 = row.reserve0.quantity.symbol.code();
        symbol_code sc2 = row.reserve1.quantity.symbol.code();

        if (sc1 != symcode_in) std::swap(sc1, sc2);
        if (sc1 != symcode_in || row.id == direct) continue;         // if this row doesn't contain our in symbol or it's a direct path - skip
        auto hop2 = find_pair_id(sc2, symcode_memo);
        if (hop2.is_valid()) paths.push_back({row.id, hop2});
    }

    return paths;
}

extended_asset sx::curve::apply_trade( const extended_asset ext_in, const vector<symbol_code> path, const bool finalize /*=false*/ )
{
    sx::curve::pairs_table _pairs( get_self(), get_self().value );
    extended_asset ext_quantity = ext_in;
    check( path.size(), "path is empty");
    for (auto pair_id : path) {
        const auto& pairs = _pairs.get( pair_id.raw(), "pair id does not exist");
        const bool is_in = pairs.reserve0.quantity.symbol == ext_quantity.quantity.symbol;
        const extended_asset reserve_in = is_in ? pairs.reserve0 : pairs.reserve1;
        const extended_asset reserve_out = is_in ? pairs.reserve1 : pairs.reserve0;

        if (reserve_in.get_extended_symbol() != ext_quantity.get_extended_symbol()) {
            check(!finalize, "incoming currency/reserves contract mismatch");
            return {};
        }

        // calculate out
        const extended_asset ext_out = { get_amount_out( ext_quantity.quantity, pair_id ), reserve_out.contract };

        if (finalize) {
            // modify reserves
            _pairs.modify( pairs, get_self(), [&]( auto & row ) {
                if ( is_in ) {
                    row.reserve0.quantity += ext_quantity.quantity;
                    row.reserve1.quantity -= ext_out.quantity;
                } else {
                    row.reserve0.quantity -= ext_out.quantity;
                    row.reserve1.quantity += ext_quantity.quantity;
                }
                // calculate last price
                const double price = calculate_price( ext_quantity.quantity, ext_out.quantity );
                row.virtual_price = calculate_virtual_price( row.reserve0.quantity, row.reserve1.quantity, row.liquidity.quantity );
                row.price0_last = is_in ? 1 / price : price;
                row.price1_last = is_in ? price : 1 / price;

                // calculate incoming culmative trading volume
                if ( is_in ) row.volume0 += ext_quantity.quantity;
                if ( !is_in ) row.volume1 += ext_quantity.quantity;
                row.last_updated = current_time_point();
            });
        }
        ext_quantity = ext_out;
    }

    return ext_quantity;
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
