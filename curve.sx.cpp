// #include <eosio.token/eosio.token.hpp>

#include "curve.sx.hpp"
#include "curve.hpp"

using namespace std;

[[eosio::action]]
void sx::curvetest::test( const uint64_t amount, const uint64_t reserve_in, const uint64_t reserve_out, const uint64_t amplifier, const uint64_t fee )
{
    print ("curve::get_amount_out(",amount,"): ", curve::get_amount_out( amount, reserve_in, reserve_out, amplifier, fee ), "\n");

    check(false, "see print");
}

// /**
//  * Notify contract when any token transfer notifiers relay contract
//  */
// [[eosio::on_notify("*::transfer")]]
// void sx::nav::on_transfer( const name from, const name to, const asset quantity, const string memo )
// {
//     // authenticate incoming `from` account
//     require_auth( from );

//     // settings
//     sx::nav::settings _settings( get_self(), get_self().value );
//     sx::nav::pairs _pairs( get_self(), get_self().value );
//     check( _settings.exists(), "contract is under maintenance");
//     auto settings = _settings.get();

//     // TEMP - DURING TESTING PERIOD
//     // check( from.suffix() == "sx"_n || from == "eosnationinc"_n, "account must be *.sx");

//     // ignore transfers
//     if ( to != get_self() || memo == get_self().to_string() || from == "eosio.ram"_n) return;

//     // check incoming transfer
//     const name contract = get_first_receiver();
//     const auto pairs = _pairs.get(quantity.symbol.code().raw(), "base symbol does not exist");
//     check( pairs.base.get_contract() == contract, "base contract mismatch");
//     check( pairs.base.get_symbol() == quantity.symbol, "base symbol mismatch");

//     // calculate out
//     const extended_asset out = sx::nav::get_amount_out( get_self(), quantity );
//     const asset balance = token::get_balance( pairs.quote.get_contract(), get_self(), pairs.quote.get_symbol().code() );

//     // issue if SX tokens
//     if ( pairs.quote.get_contract() == "token.sx"_n ) issue( out, "issue" );
//     else check( balance >= out.quantity, "insufficient quote balance");

//     // transfer amount to sender
//     transfer( get_self(), from, out, "nav" );

//     // retire if SX token
//     if ( contract == "token.sx"_n ) retire( { quantity, contract }, "retire" );
// }

// [[eosio::action]]
// void sx::nav::setsettings( const std::optional<sx::nav::settings_row> settings )
// {
//     require_auth( get_self() );
//     sx::nav::settings _settings( get_self(), get_self().value );

//     // clear table if setting is `null`
//     if ( !settings ) return _settings.remove();

//     _settings.set( *settings, get_self() );
// }

// [[eosio::action]]
// void sx::nav::setpair( const extended_symbol base, const std::optional<extended_symbol> quote )
// {
//     require_auth( get_self() );
//     check( base.get_symbol().precision() == quote->get_symbol().precision(), "base & quote symbol precision must match");

//     sx::nav::pairs _pairs( get_self(), get_self().value );
//     auto itr = _pairs.find( base.get_symbol().code().raw() );

//     // create
//     if ( itr == _pairs.end() ) {
//         _pairs.emplace( get_self(), [&]( auto& row ) {
//             row.base = base;
//             row.quote = *quote;
//         });
//     // erase
//     } else if (!quote) {
//         _pairs.erase( itr );
//     // modify
//     } else {
//         _pairs.modify( itr, get_self(), [&]( auto& row ) {
//             row.base = base;
//             row.quote = *quote;
//         });
//     }
// }

// void sx::nav::issue( const extended_asset value, const string memo )
// {
//     eosio::token::issue_action issue( value.contract, { get_self(), "active"_n });
//     issue.send( get_self(), value.quantity, memo );
// }

// void sx::nav::retire( const extended_asset value, const string memo )
// {
//     eosio::token::retire_action retire( value.contract, { get_self(), "active"_n });
//     retire.send( value.quantity, memo );
// }

// void sx::nav::transfer( const name from, const name to, const extended_asset value, const string memo )
// {
//     eosio::token::transfer_action transfer( value.contract, { from, "active"_n });
//     transfer.send( from, to, value.quantity, memo );
// }
