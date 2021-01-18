#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include <eosio/check.hpp>
#include <uint128_t/uint128_t.cpp>

#include "curve.hpp"

TEST_CASE( "get_amount_out #1 (pass)" ) {
    // Inputs
    const uint64_t amount = 100000;
    const uint64_t amplifier = 450;
    const uint64_t reserve_in = 3432247548;
    const uint64_t reserve_out = 6169362700;
    const uint64_t fee = 4;

    REQUIRE( curve::get_amount_out(amount, reserve_in, reserve_out, amplifier, fee) == 100110 );
}
