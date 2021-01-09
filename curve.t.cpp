#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include <eosio/check.hpp>
#include <uint128_t/uint128_t.cpp>

#include "curve.hpp"

TEST_CASE( "get_amount_out #2 (pass)" ) {
    // Inputs
    const uint16_t amplifier = 450;
    const uint64_t reserve0 = 3432247548;
    const uint64_t reserve1 = 6169362700;
    // const std::vector<uint64_t> reserves = { reserve0, reserve1 };
    const curve c = curve{ amplifier, reserve0, reserve1 };

    REQUIRE( c.amplifier == amplifier );
    // REQUIRE( c.x.size() == 2 );
    // REQUIRE( c.x[0] == reserve0 );
    // REQUIRE( c.x[1] == reserve1 );
    // REQUIRE( c.xp()[1] == reserve1 );
    REQUIRE( c.reserve0 == 3432247548 );
    REQUIRE( c.D() == 9600668971 );
}
