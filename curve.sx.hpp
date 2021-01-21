#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>


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
        uint128_t by_reserves() const { return compute_by_symcodes( reserve0.quantity.symbol.code(), reserve1.quantity.symbol.code() ); }
    };
    typedef eosio::multi_index< "pairs"_n, pairs_row,
        indexed_by<"byreserves"_n, const_mem_fun<pairs_row, uint128_t, &pairs_row::by_reserves>>
    > pairs;

    static uint128_t compute_by_symcodes( const symbol_code symcode0, const symbol_code symcode1 ) {
        return ((uint128_t) symcode0.raw()) << 64 | symcode1.raw();
    }

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

private:
    void transfer( const name from, const name to, const extended_asset value, const string memo );
    void retire( const extended_asset value, const string memo );
    void issue( const extended_asset value, const string memo );

    //helper to parse memo
    pair<extended_asset, name> parse_memo(string memo);

    // sx curve
    symbol_code find_pair_id( const symbol_code symcode0, const symbol_code symcode1 );
    int64_t mul_amount( const int64_t amount, const uint8_t precision0, const uint8_t precision1 );
    int64_t div_amount( const int64_t amount, const uint8_t precision0, const uint8_t precision1 );

    // maintenance
    template <typename T>
    void clear_table( T& table );
};
}