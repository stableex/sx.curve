#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

#include <optional>

using namespace eosio;

namespace sx {
class [[eosio::contract("curve.sx")]] curvetest : public eosio::contract {
public:
    using contract::contract;

    [[eosio::action]]
    void test( const uint64_t amount );

    // /**
    //  * ## TABLE `settings`
    //  *
    //  * - `{uint8_t} fee` - curve fee (pips 1/100 of 1%)
    //  *
    //  * ### example
    //  *
    //  * ```json
    //  * {
    //  *   "fee": 0
    //  * }
    //  * ```
    //  */
    // struct [[eosio::table("settings")]] settings_row {
    //     uint8_t             fee = 0;
    // };
    // typedef eosio::singleton< "settings"_n, settings_row > settings;

    // /**
    //  * ## TABLE `pairs`
    //  *
    //  * - `{extended_symbol} base` - base asset
    //  * - `{extended_symbol} quote` - quote asset
    //  *
    //  * ### example
    //  *
    //  * ```json
    //  * {
    //  *   "base": {"symbol": "4,USDT", "contract": "tethertether"},
    //  *   "quote": {"symbol": "4,SX", "contract": "token.sx"}
    //  * }
    //  * ```
    //  */
    // struct [[eosio::table("pairs")]] pairs_row {
    //     extended_symbol     base;
    //     extended_symbol     quote;

    //     uint64_t primary_key() const { return base.get_symbol().code().raw(); }
    // };
    // typedef eosio::multi_index< "pairs"_n, pairs_row > pairs;

    // [[eosio::action]]
    // void setsettings( const std::optional<sx::nav::settings_row> settings );

    // [[eosio::action]]
    // void setpair( const extended_symbol base, const std::optional<extended_symbol> quote );

    // /**
    //  * Notify contract when any token transfer notifiers relay contract
    //  */
    // [[eosio::on_notify("*::transfer")]]
    // void on_transfer( const name from, const name to, const asset quantity, const std::string memo );

    // /**
    //  * ## STATIC `get_amount_out`
    //  *
    //  * Given an input amount of an asset and pair reserves, returns the maximum output amount of the other asset
    //  *
    //  * ### params
    //  *
    //  * - `{name} contract` - contract name
    //  * - `{asset} amount_in` - amount input
    //  *
    //  * ## returns
    //  *
    //  * - `{extended_asset}` - out amount
    //  *
    //  * ### example
    //  *
    //  * ```c++
    //  * // Inputs
    //  * const name contract = "nav.sx"_n;
    //  * const asset amount_in = asset{10000, symbol{"USDT", 4}};
    //  *
    //  * // Calculation
    //  * const asset amount_out = sx::nav::get_amount_out( contract, amount_in );
    //  * // => {"quantity": "1.0000 SX", "contract": "token.sx"}
    //  * ```
    //  */
    // static extended_asset get_amount_out( const name contract, const asset amount_in )
    // {
    //     sx::nav::settings _settings( contract, contract.value );
    //     sx::nav::pairs _pairs( contract, contract.value );
    //     check( _settings.exists(), "contract is under maintenance");

    //     // params
    //     const uint8_t fee = _settings.get().fee;
    //     const extended_symbol quote = _pairs.get(amount_in.symbol.code().raw(), "base symbol does not exist").quote;

    //     // calculations
    //     const int64_t fee_amount = amount_in.amount * fee / 10000;
    //     const int64_t out = amount_in.amount - fee_amount;
    //     return { out, quote };
    // }

private:
    // void transfer( const name from, const name to, const extended_asset value, const string memo );
    // void retire( const extended_asset value, const string memo );
    // void issue( const extended_asset value, const string memo );
};
}