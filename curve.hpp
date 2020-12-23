#pragma once

#include <sx.safemath/safemath.hpp>

#include <math.h>

namespace uniswap {
    /**
     * ## STATIC `get_amount_out`
     *
     * Given an input amount of an asset and pair reserves, returns the maximum output amount of the other asset
     *
     * ### params
     *
     * - `{uint64_t} amount_in` - amount input
     * - `{uint64_t} reserve_in` - reserve input
     * - `{uint64_t} reserve_out` - reserve output
     * - `{uint8_t} [fee=30]` - (optional) trade fee (pips 1/100 of 1%)
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const uint64_t amount_in = 10000;
     * const uint64_t reserve_in = 45851931234;
     * const uint64_t reserve_out = 125682033533;
     * const uint8_t fee = 30;
     *
     * // Calculation
     * const uint64_t amount_out = curve::get_amount_out( amount_in, reserve_in, reserve_out, fee );
     * // => 27328
     * ```
     */
    static uint64_t get_amount_out( const uint64_t amount_in, const uint64_t reserve_in, const uint64_t reserve_out, const uint8_t fee = 30, const double amplifier = 200 )
    {
        // checks
        eosio::check(amount_in > 0, "SX.Curve: INSUFFICIENT_INPUT_AMOUNT");
        eosio::check(reserve_in > 0 && reserve_out > 0, "SX.Curve: INSUFFICIENT_LIQUIDITY");

        // calculations
        const double total = reserve_in + reserve_out;
        const double numerator = (
            4 * amplifier * (reserve_in + amount_in)
            + 8 * amplifier * reserve_out
            + total
            - 4 * amplifier * total
            - pow
                (
                    (
                    4 * amplifier * total * total * total
                    / (reserve_in + amount_in)
                    + 16 * amplifier * amplifier * total * total
                    + 16 * amplifier * amplifier * (reserve_in + amount_in) * (reserve_in + amount_in)
                    +  8 * amplifier * total * (reserve_in + amount_in)
                    +  total * total
                    -  32 * amplifier * amplifier * total * (reserve_in + amount_in)
                    -  8 * amplifier * total * total
                    )
                    ,0.5
                )
        );
        const double denominator = (8 * amplifier);
        const uint64_t amount_out = numerator / denominator;

        // const uint128_t amount_in_with_fee = static_cast<uint128_t>(amount_in) * (10000 - fee);
        // const uint128_t numerator = amount_in_with_fee * reserve_out;
        // const uint128_t denominator = (static_cast<uint128_t>(reserve_in) * 10000) + amount_in_with_fee;
        // const uint64_t amount_out = numerator / denominator;

        return amount_out;
    }

    /**
     * ## STATIC `get_amount_in`
     *
     * Given an output amount of an asset and pair reserves, returns a required input amount of the other asset.
     *
     * ### params
     *
     * - `{uint64_t} amount_out` - amount input
     * - `{uint64_t} reserve_in` - reserve input
     * - `{uint64_t} reserveOut` - reserve output
     * - `{uint8_t} [fee=30]` - (optional) trading fee (pips 1/100 of 1%)
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const uint64_t amount_out = 27328;
     * const uint64_t reserve_in = 45851931234;
     * const uint64_t reserve_out = 125682033533;
     * const uint8_t fee = 30;
     *
     * // Calculation
     * const uint64_t amount_in = curve::get_amount_in( amount_out, reserve_in, reserve_out, fee );
     * // => 10000
     * ```
     */
    static uint64_t get_amount_in( const uint64_t amount_out, const uint64_t reserve_in, const uint64_t reserve_out, const uint8_t fee = 30 )
    {
        // checks
        eosio::check(amount_out > 0, "SX.Uniswap: INSUFFICIENT_OUTPUT_AMOUNT");
        eosio::check(reserve_in > 0 && reserve_out > 0, "SX.Uniswap: INSUFFICIENT_LIQUIDITY");

        const uint128_t numerator = static_cast<uint128_t>(reserve_in) * amount_out * 10000;
        const uint128_t denominator = static_cast<uint128_t>(reserve_out - amount_out) * (10000 - fee);
        const uint64_t amount_in = (numerator / denominator) + 1;

        return amount_in;
    }

    /**
     * ## STATIC `quote`
     *
     * Given some amount of an asset and pair reserves, returns an equivalent amount of the other asset
     *
     * ### params
     *
     * - `{uint64_t} amount_a` - amount A
     * - `{uint64_t} reserve_a` - reserve A
     * - `{uint64_t} reserve_b` - reserve B
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const uint64_t amount_a = 10000;
     * const uint64_t reserve_a = 45851931234;
     * const uint64_t reserve_b = 125682033533;
     *
     * // Calculation
     * const uint64_t amount_b = curve::quote( amount_a, reserve_a, reserve_b );
     * // => 27410
     * ```
     */
    static uint64_t quote( const uint64_t amount_a, const uint64_t reserve_a, const uint64_t reserve_b )
    {
        eosio::check(amount_a > 0, "SX.Uniswap: INSUFFICIENT_AMOUNT");
        eosio::check(reserve_a > 0 && reserve_b > 0, "SX.Uniswap: INSUFFICIENT_LIQUIDITY");
        const uint64_t amount_b = safemath::mul(amount_a, reserve_b) / reserve_a;
        return amount_b;
    }
}