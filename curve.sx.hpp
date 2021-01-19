#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

#include <sx.curve/curve.hpp>

#include <optional>

using namespace eosio;
using namespace std;

namespace sx {
class [[eosio::contract("curve.sx")]] curve : public eosio::contract {
public:
    using contract::contract;

    [[eosio::action]]
    void test( const uint64_t amount, const uint64_t reserve_in, const uint64_t reserve_out, const uint64_t amplifier, const uint64_t fee );

    /**
     * ## TABLE `settings`
     *
     * - `{uint8_t} fee` - trading fee (pips 1/100 of 1%)
     *
     * ### example
     *
     * ```json
     * {
     *   "fee": 4
     * }
     * ```
     */
    struct [[eosio::table("settings")]] settings_row {
        uint8_t             fee = 4;
    };
    typedef eosio::singleton< "settings"_n, settings_row > settings;

    /**
     * ## TABLE `pairs`
     *
     * - `{symbol_code} id` - pair symbol id
     * - `{extended_asset} reserve0` - reserve0 asset
     * - `{extended_asset} reserve1` - reserve1 asset
     * - `{extended_asset} liquidity` - liquidity asset
     * - `{uint64_t} amplifier` - amplifier
     * - `{double} price0_last` - last price for reserve0
     * - `{double} price1_last` - last price for reserve1
     * - `{uint64_t} volume0` - cumulative incoming trading volume for reserve0
     * - `{uint64_t} volume1` - cumulative incoming trading volume for reserve1
     * - `{time_point_sec} last_updated` - last updated timestamp
     *
     * ### example
     *
     * ```json
     * {
     *   "id": "AB",
     *   "reserve0": {"quantity": "1000.0000 A", "contract": "eosio.token"},
     *   "reserve1": {"quantity": "1000.0000 B", "contract": "eosio.token"},
     *   "liquidity": {"quantity": "20000000.0000 AB", "contract": "curve.sx"},
     *   "amplifier": 450,
     *   "price0_last": 1.0,
     *   "price1_last": 1.0,
     *   "volume0": 1000000,
     *   "volume1": 1000000,
     *   "last_updated": "2020-11-23T00:00:00"
     * }
     * ```
     */
    struct [[eosio::table("pairs")]] pairs_row {
        symbol_code         id;
        extended_asset      reserve0;
        extended_asset      reserve1;
        extended_asset      liquidity;
        uint64_t            amplifier;
        double              price0_last;
        double              price1_last;
        uint64_t            volume0;
        uint64_t            volume1;
        time_point_sec      last_updated;

        uint64_t primary_key() const { return id.raw(); }
    };
    typedef eosio::multi_index< "pairs"_n, pairs_row > pairs;

    [[eosio::action]]
    void setsettings( const std::optional<sx::curve::settings_row> settings );

    [[eosio::action]]
    void setpair( const symbol_code id, const extended_asset reserve0, const extended_asset reserve1, const uint64_t amplifier );

    [[eosio::action]]
    void reset();

    /**
     * Notify contract when any token transfer notifiers relay contract
     */
    [[eosio::on_notify("*::transfer")]]
    void on_transfer( const name from, const name to, const asset quantity, const std::string memo );

    /**
     * ## STATIC `get_amount_out`
     *
     * Given an input amount, reserves pair and amplifier, returns the output amount of the other asset based on Curve formula
     * Whitepaper: https://www.curve.fi/stableswap-paper.pdf
     * Python implementation: https://github.com/curvefi/curve-contract/blob/master/tests/simulation.py
     *
     * ### params
     *
     * - `{uint64_t} amount_in` - amount input
     * - `{uint64_t} reserve_in` - reserve input
     * - `{uint64_t} reserve_out` - reserve output
     * - `{uint64_t} amplifier` - amplifier
     * - `{uint8_t} [fee=4]` - (optional) trade fee (pips 1/100 of 1%)
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const uint64_t amount_in = 100000;
     * const uint64_t reserve_in = 3432247548;
     * const uint64_t reserve_out = 6169362700;
     * cont uint64_t amplifier = 450;
     * const uint8_t fee = 4;
     *
     * // Calculation
     * const uint64_t amount_out = curve::get_amount_out( amount_in, reserve_in, reserve_out, amplifier, fee );
     * // => 100110
     * ```
     */
    static uint64_t get_amount_out( const uint64_t amount_in, const uint64_t reserve_in, const uint64_t reserve_out, const uint64_t amplifier, const uint8_t fee = 4 )
    {
        eosio::check(amount_in > 0, "SX.Curve: INSUFFICIENT_INPUT_AMOUNT");
        eosio::check(amplifier > 0, "SX.Curve: WRONG_AMPLIFIER");
        eosio::check(reserve_in > 0 && reserve_out > 0, "SX.Curve: INSUFFICIENT_LIQUIDITY");
        eosio::check(fee <= 100, "SX.Curve: FEE_TOO_HIGH");

        // calculate invariant D by solving quadratic equation:
        // A * sum * n^n + D = A * D * n^n + D^(n+1) / (n^n * prod), where n==2
        const uint64_t sum = reserve_in + reserve_out;
        uint128_t D = sum, D_prev = 0;
        while (D != D_prev) {
            uint128_t prod1 = D * D / (reserve_in * 2) * D / (reserve_out * 2);
            D_prev = D;
            D = 2 * D * (amplifier * sum + prod1) / ((2 * amplifier - 1) * D + 3 * prod1);
        }

        // calculate x - new value for reserve_out by solving quadratic equation iteratively:
        // x^2 + x * (sum' - (An^n - 1) * D / (An^n)) = D ^ (n + 1) / (n^(2n) * prod' * A), where n==2
        // x^2 + b*x = c
        const int64_t b = (reserve_in + amount_in) + (D / (amplifier * 2)) - D;
        const uint128_t c = D * D / ((reserve_in + amount_in) * 2) * D / (amplifier * 4);
        uint128_t x = D, x_prev = 0;
        while (x != x_prev) {
            x_prev = x;
            x = (x * x + c) / (2 * x + b);
        }

        check(reserve_out > x, "SX.Curve: INSUFFICIENT_RESERVE_OUT");
        const uint64_t amount_out = reserve_out - (uint64_t)x;

        return amount_out - fee * amount_out / 10000;
    }

private:
    void transfer( const name from, const name to, const extended_asset value, const string memo );
    void retire( const extended_asset value, const string memo );
    void issue( const extended_asset value, const string memo );

    // maintenance
    template <typename T>
    void clear_table( T& table );
};
}