#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <sx.curve/curve.hpp>

#include <optional>

using namespace eosio;
using namespace std;

static constexpr uint8_t MAX_PRECISION = 8;
static constexpr int64_t asset_mask{(1LL << 62) - 1};
static constexpr int64_t asset_max{ asset_mask }; //  4611686018427387903
static constexpr name TOKEN_CONTRACT = "lptoken.sx"_n;

namespace sx {
class [[eosio::contract("curve.sx")]] curve : public eosio::contract {
public:
    using contract::contract;

    static constexpr name id = "curve.sx"_n;
    static constexpr name code = "curve.sx"_n;

    /**
     * ## TABLE `config`
     *
     * - `{name} status` - contract status ("ok", "paused")
     * - `{uint8_t} trading_fee` - trading fee (pips 1/100 of 1%)
     * - `{uint8_t} protocol_fee` - protocol fee (pips 1/100 of 1%)
     * - `{name} fee_account` - transfer protocol fees to account
     *
     * ### example
     *
     * ```json
     * {
     *   "status": "ok",
     *   "trade_fee": 4,
     *   "protocol_fee": 0,
     *   "fee_account": "fee.sx"
     * }
     * ```
     */
    struct [[eosio::table("config")]] config_row {
        name                status = "ok"_n;
        uint8_t             trade_fee = 4;
        uint8_t             protocol_fee = 0;
        name                fee_account = "fee.sx"_n;
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
     *   "liquidity": {"quantity": "2000.00000000 AB", "contract": "curve.sx"},
     *   "amplifier": 450,
     *   "virtual_price": 1.0,
     *   "price0_last": 1.0,
     *   "price1_last": 1.0,
     *   "volume0": "100.0000 A",
     *   "volume1": "100.0000 B",
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
        time_point_sec      last_updated;

        uint64_t primary_key() const { return id.raw(); }
        uint128_t by_reserves() const { return compute_by_symcodes( reserve0.quantity.symbol.code(), reserve1.quantity.symbol.code() ); }
    };
    typedef eosio::multi_index< "pairs"_n, pairs_row,
        indexed_by<"byreserves"_n, const_mem_fun<pairs_row, uint128_t, &pairs_row::by_reserves>>
    > pairs_table;

    static uint128_t compute_by_symcodes( const symbol_code symcode0, const symbol_code symcode1 ) {
        return ((uint128_t) symcode0.raw()) << 64 | symcode1.raw();
    }

    struct [[eosio::table("backup")]] backup_row {
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
    typedef eosio::multi_index< "backup"_n, backup_row> backup_table;

    [[eosio::action]]
    void setconfig( const std::optional<sx::curve::config_row> config );

    [[eosio::action]]
    void createpair( const name creator, const symbol_code pair_id, const extended_symbol reserve0, const extended_symbol reserve1, const uint64_t amplifier );

    [[eosio::action]]
    void deposit( const name owner, const symbol_code pair_id );

    [[eosio::action]]
    void cancel( const name owner, const symbol_code pair_id );

    [[eosio::on_notify("*::transfer")]]
    void on_transfer( const name from, const name to, const asset quantity, const std::string memo );

    // MAINTENANCE (TESTING ONLY)
    [[eosio::action]]
    void reset( const name table );

    [[eosio::action]]
    void backup();

    [[eosio::action]]
    void copy();

    [[eosio::action]]
    void update();

    [[eosio::action]]
    void test( const uint64_t amount, const uint64_t reserve_in, const uint64_t reserve_out, const uint64_t amplifier, const uint64_t fee );

    /**
     * ## STATIC `get_amount_out`
     *
     * Calculate return for converting {in} amount via {pair_id} pool
     *
     * ### params
     *
     * - `{asset} in` - input token quantity
     * - `{symbol_code} pair_id` - pair id
     *
     * ### returns
     *
     * - `{asset}` - calculated return
     *
     * ### example
     *
     * ```c++
     * const asset in = asset {10'0000, {"A", 4}};
     * const symbol_code pair_id = symbol_code {"SXA"};
     *
     * const asset out  = sx::curve::get_amount_out( in, pair_id );
     * => 10.1000 B
     * ```
     */
    static asset get_amount_out( const asset in, const symbol_code pair_id )
    {
        sx::curve::config_table _config( sx::curve::code, sx::curve::code.value );
        sx::curve::pairs_table _pairs( sx::curve::code, sx::curve::code.value );
        check( _config.exists(), "Curve.sx: contract is under maintenance");

        // get configs
        auto config = _config.get();
        auto pairs = _pairs.get( pair_id.raw(), "Curve.sx: invalid pair id" );

        // inverse reserves based on input quantity
        if (pairs.reserve0.quantity.symbol != in.symbol) std::swap(pairs.reserve0, pairs.reserve1);
        eosio::check( pairs.reserve0.quantity.symbol == in.symbol, "Curve.sx: no such reserve in pairs");

        // normalize inputs to max precision
        const uint8_t precision_in = pairs.reserve0.quantity.symbol.precision();
        const uint8_t precision_out = pairs.reserve1.quantity.symbol.precision();
        const int64_t amount_in = mul_amount( in.amount, MAX_PRECISION, precision_in );
        const int64_t reserve_in = mul_amount( pairs.reserve0.quantity.amount, MAX_PRECISION, precision_in );
        const int64_t reserve_out = mul_amount( pairs.reserve1.quantity.amount, MAX_PRECISION, precision_out );
        const uint64_t amplifier = pairs.amplifier;
        const uint8_t fee = config.trade_fee + config.protocol_fee;

        // calculate out
        const int64_t out = div_amount( Curve::get_amount_out( amount_in, reserve_in, reserve_out, amplifier, fee ), MAX_PRECISION, precision_out );

        return { out, pairs.reserve1.quantity.symbol };
    }

    static int64_t mul_amount( const int64_t amount, const uint8_t precision0, const uint8_t precision1 )
    {
        return amount * pow(10, precision0 - precision1 );
    }

    static int64_t div_amount( const int64_t amount, const uint8_t precision0, const uint8_t precision1 )
    {
        return amount / pow(10, precision0 - precision1 );
    }

private:
    // token helpers
    void create( const extended_symbol value );
    void transfer( const name from, const name to, const extended_asset value, const string memo );
    void retire( const extended_asset value, const string memo );
    void issue( const extended_asset value, const string memo );

    // exchange {ext_in} and send to receiver
    void convert(const extended_asset ext_in, const extended_asset ext_min_out, name receiver);

    // add liquidity {value} to pool {id} for {owner}
    void add_liquidity( const symbol_code id, const name owner, const extended_asset value );

    // utils
    pair<extended_asset, name> parse_memo(const string memo);
    double calculate_price( const asset value0, const asset value1 );
    double calculate_virtual_price( const asset value0, const asset value1, const asset supply );

    // find pair_id based on symbol_code of incoming tokens and memo
    symbol_code find_pair_id( const symbol_code symcode_in, const symbol_code symcode_memo );

    // find all possible paths to trade symcode_in to memo symcode, include 2-hops
    vector<vector<symbol_code>> find_trade_paths( const symbol_code symcode_in, const symbol_code symcode_out );

    // calculate return for trade via {path}, finalize it if {finalize}==true
    extended_asset apply_trade( const extended_asset ext_in, const vector<symbol_code>& path, bool finalize = false );

    // maintenance
    template <typename T>
    void clear_table( T& table );
};
}