#pragma once

namespace safemath {
    /**
     * ## STATIC `add`
     *
     * ### params
     *
     * - `{uint64_t} x`
     * - `{uint64_t} y`
     *
     * ### example
     *
     * ```c++
     * const uint64_t z = safemath::add(1, 2);
     * //=> 3
     * ```
     */
    static uint64_t add( const uint64_t x, const uint64_t y ) {
        const uint64_t z = x + y;
        eosio::check( z >= x, "safemath-add-overflow"); return z;
    }

    /**
     * ## STATIC `sub`
     *
     * ### params
     *
     * - `{uint64_t} x`
     * - `{uint64_t} y`
     *
     * ### example
     *
     * ```c++
     * const uint64_t z = safemath::sub(3, 2);
     * //=> 1
     * ```
     */
    static uint64_t sub(const uint64_t x, const uint64_t y) {
        const uint64_t z = x - y;
        eosio::check(z <= x, "safemath-sub-overflow"); return z;
    }

    /**
     * ## STATIC `mul`
     *
     * ### params
     *
     * - `{uint64_t} x`
     * - `{uint64_t} y`
     *
     * ### example
     *
     * ```c++
     * const uint128_t z = safemath::mul(2, 2);
     * //=> 4
     * ```
     */
    static uint128_t mul(const uint64_t x, const uint64_t y) {
        const uint128_t z = static_cast<uint128_t>(x) * y;
        eosio::check(y == 0 || z / y == x, "safemath-mul-overflow"); return z;
    }

    /**
     * ## STATIC `div`
     *
     * ### params
     *
     * - `{uint64_t} x`
     * - `{uint64_t} y`
     *
     * ### example
     *
     * ```c++
     * const uint64_t z = safemath::div(4, 2);
     * //=> 2
     * ```
     */
    static uint64_t div(const uint64_t x, const uint64_t y) {
        eosio::check(y > 0, "safemath-divide-zero");
        return x / y;
    }
}