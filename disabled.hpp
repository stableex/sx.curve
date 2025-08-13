#pragma once

#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>

#include <optional>

using namespace eosio;
using namespace std;

class [[eosio::contract("curve.sx")]] curve : public eosio::contract {
public:
    using contract::contract;

    [[eosio::on_notify("*::transfer")]]
    void on_transfer( const name from, const name to, const asset quantity, const string memo )
    {
        if ( from == get_self() ) return; // ignore self transfers

        check(false, "curve::on_transfer: this contract is now disabled");
    }

    /**
     * ## TABLE `config`
     *
     * - `{name} status` - contract status ("ok", "testing", "maintenance")
     * - `{uint8_t} trade_fee` - trading fee (pips 1/100 of 1%)
     * - `{uint8_t} protocol_fee` - trading fee (pips 1/100 of 1%)
     * - `{name} fee_account` - protocol fee are transfered to account
     * - `{name} token_contract` - LP token contract account
     * - `{vector<name>} notifiers` - accounts to be notified via inline action
     *
     * ### example
     *
     * ```json
     * {
     *   "status": "ok",
     *   "trade_fee": 5,
     *   "protocol_fee": 0,
     *   "fee_account": "fee.sx",
     *   "token_contract": "lptoken.sx",
     *   "notifiers": ["stats.sx"]
     * }
     * ```
     */
    struct [[eosio::table("config")]] config_row {
        name                status = "testing"_n;
        uint8_t             trade_fee = 5;
        uint8_t             protocol_fee = 0;
        name                fee_account;
        name                token_contract;
        vector<name>        notifiers;
    };
    typedef eosio::singleton< "config"_n, config_row > config_table;

    /**
     * ## TABLE `orders`
     *
     * *scope*: `pair_id` (symbol_code)
     *
     * - `{name} owner` - owner account
     * - `{extended_asset} quantity0` - quantity asset
     * - `{extended_asset} quantity1` - quantity asset
     *
     * ### example
     *
     * ```json
     * {
     *   "owner": "myaccount",
     *   "quantity0": {"contract": "eosio.token", "quantity": "1000.0000 A"},
     *   "quantity1": {"contract": "eosio.token", "quantity": "1000.0000 B"},
     * }
     * ```
     */
    struct [[eosio::table("orders")]] orders_row {
        name                owner;
        extended_asset      quantity0;
        extended_asset      quantity1;

        uint64_t primary_key() const { return owner.value; }
    };
    typedef eosio::multi_index< "orders"_n, orders_row> orders_table;

    /**
     * ## TABLE `pairs`
     *
     * - `{symbol_code} id` - pair id
     * - `{extended_asset} reserve0` - reserve0 asset
     * - `{extended_asset} reserve1` - reserve1 asset
     * - `{extended_asset} liquidity` - liquidity asset
     * - `{uint64_t} amplifier` - amplifier
     * - `{double} price0_last` - last price for reserve0
     * - `{double} price1_last` - last price for reserve1
     * - `{asset} volume0` - cumulative incoming trading volume for reserve0
     * - `{asset} volume1` - cumulative incoming trading volume for reserve1
     * - `{uint64_t} trades` - cumulative trades count
     * - `{time_point_sec} last_updated` - last updated timestamp
     *
     * ### example
     *
     * ```json
     * {
     *   "id": "AB",
     *   "reserve0": {"quantity": "1000.0000 A", "contract": "eosio.token"},
     *   "reserve1": {"quantity": "1000.0000 B", "contract": "eosio.token"},
     *   "liquidity": {"quantity": "2000.00000000 AB", "contract": "curve.sx"},
     *   "amplifier": 450,
     *   "virtual_price": 1.0,
     *   "price0_last": 1.0,
     *   "price1_last": 1.0,
     *   "volume0": "100.0000 A",
     *   "volume1": "100.0000 B",
     *   "trades": 123,
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
        double              virtual_price;
        double              price0_last;
        double              price1_last;
        asset               volume0;
        asset               volume1;
        uint64_t            trades;
        time_point_sec      last_updated;

        uint64_t primary_key() const { return id.raw(); }
    };
    typedef eosio::multi_index< "pairs"_n, pairs_row> pairs_table;

    /**
     * ## TABLE `ramp`
     *
     * - `{symbol_code} id` - pair id
     * - `{uint64_t} start_amplifier` - start amplifier when `ramp` action is initialized
     * - `{uint64_t} target_amplifier` - target amplifier when end time is reached
     * - `{time_point_sec} start_time` - start time when `ramp` action is initialized
     * - `{time_point_sec} end_time` - end time when target amplifier will be reached
     *
     * ### example
     *
     * ```json
     * {
     *   "id": "AB",
     *   "start_amplifier": 100,
     *   "target_amplifier": 200,
     *   "start_time": "2021-02-03T00:00:00"
     *   "end_time": "2021-02-04T00:00:00"
     * }
     * ```
     */
    struct [[eosio::table("ramp")]] ramp_row {
        symbol_code         pair_id;
        uint64_t            start_amplifier;
        uint64_t            target_amplifier;
        time_point_sec      start_time;
        time_point_sec      end_time;

        uint64_t primary_key() const { return pair_id.raw(); }
    };
    typedef eosio::multi_index< "ramp"_n, ramp_row> ramp_table;
};