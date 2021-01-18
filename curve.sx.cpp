#include <eosio.token/eosio.token.hpp>
#include <sx.utils/utils.hpp>

#include "curve.sx.hpp"
#include "curve.hpp"

using namespace std;

[[eosio::action]]
void sx::curve::test( const uint64_t amount, const uint64_t reserve_in, const uint64_t reserve_out, const uint64_t amplifier, const uint64_t fee )
{
    print ("curve::get_amount_out(",amount,"): ", Curve::get_amount_out( amount, reserve_in, reserve_out, amplifier, fee ), "\n");

    check(false, "see print");
}

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

    // settings
    sx::curve::settings _settings( get_self(), get_self().value );
    sx::curve::pairs _pairs( get_self(), get_self().value );
    check( _settings.exists(), "contract is under maintenance");
    auto settings = _settings.get();

    // TEMP - DURING TESTING PERIOD
    // check( from.suffix() == "sx"_n || from == "eosnationinc"_n, "account must be *.sx");

    // check incoming transfer
    const name contract = get_first_receiver();
    const auto& pairs = _pairs.get( sx::utils::parse_symbol_code(memo).raw(), "pair id does not exist");
    const bool reverse = pairs.reserve0.quantity.symbol == quantity.symbol;
    const extended_asset reserve_in = reverse ? pairs.reserve0 : pairs.reserve1;
    const extended_asset reserve_out = reverse ? pairs.reserve1 : pairs.reserve0;
    check( reserve_in.contract == contract, "reserve_in contract mismatch");
    check( reserve_in.quantity.symbol == quantity.symbol, "reserve_in symbol mismatch");

    // calculate out
    const int64_t amount_out = Curve::get_amount_out( quantity.amount, reserve_in.quantity.amount, reserve_out.quantity.amount, pairs.amplifier, settings.fee );
    const extended_asset out = { amount_out, reserve_out.get_extended_symbol() };

    // modify reserves
    _pairs.modify( pairs, get_self(), [&]( auto & row ) {
        if ( reverse ) {
            row.reserve0.quantity += quantity;
            row.reserve1.quantity -= out.quantity;
        } else {
            row.reserve0.quantity -= out.quantity;
            row.reserve1.quantity += quantity;
        }
        row.last_updated = current_time_point();
    });

    // transfer amount to sender
    transfer( get_self(), from, out, "swap" );
}

[[eosio::action]]
void sx::curve::setsettings( const std::optional<sx::curve::settings_row> settings )
{
    require_auth( get_self() );
    sx::curve::settings _settings( get_self(), get_self().value );

    // clear table if setting is `null`
    if ( !settings ) return _settings.remove();

    _settings.set( *settings, get_self() );
}

[[eosio::action]]
void sx::curve::setpair( const symbol_code id, const extended_asset reserve0, const extended_asset reserve1, const uint64_t amplifier )
{
    require_auth( get_self() );
    sx::curve::pairs _pairs( get_self(), get_self().value );

    // check input
    check( reserve0.quantity.symbol.precision() == reserve1.quantity.symbol.precision(), "reserve0 & reserve0 precision must match");
    check( reserve0.quantity.amount == reserve1.quantity.amount, "reserve0 & reserve0 amount must match");

    // pairs content
    auto insert = [&]( auto & row ) {
        row.id = id;
        row.reserve0 = reserve0;
        row.reserve1 = reserve1;
        row.liquidity = {asset{0, { id, 4 }}, get_self() };
        row.amplifier = amplifier;
        row.last_updated = current_time_point();
    };

    // create/modify pairs
    auto itr = _pairs.find( id.raw() );
    if ( itr == _pairs.end() ) _pairs.emplace( get_self(), insert );
    else _pairs.modify( itr, get_self(), insert );
}

[[eosio::action]]
void sx::curve::reset()
{
    require_auth( get_self() );

    sx::curve::pairs _pairs( get_self(), get_self().value );
    sx::curve::settings _settings( get_self(), get_self().value );

    _settings.remove();
    clear_table( _pairs );
}

template <typename T>
void sx::curve::clear_table( T& table )
{
    auto itr = table.begin();
    while ( itr != table.end() ) {
        itr = table.erase( itr );
    }
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
