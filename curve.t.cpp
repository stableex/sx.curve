#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include <eosio/check.hpp>
#include <uint128_t/uint128_t.cpp>

#include "curve.hpp"

TEST_CASE( "get_amount_out #1 (pass)" ) {
    // Inputs
    const uint64_t amount_in = 1000000;
    const uint64_t reserve_in = 100000000;
    const uint64_t reserve_out = 100000000;

    // Calculation
    const uint64_t amountOut = uniswap::get_amount_out( amount_in, reserve_in, reserve_out );

    REQUIRE( amountOut == 9999 );
}

TEST_CASE( "get_amount_out #2 (pass)" ) {
    // Inputs
    const uint64_t amount_in = 100000;
    const uint64_t reserve_in = 62854618;
    const uint64_t reserve_out = 78909405;

    // Calculation
    const uint64_t amountOut = uniswap::get_amount_out( amount_in, reserve_in, reserve_out, 0, 200 );

    REQUIRE( amountOut == 100079 );
}
