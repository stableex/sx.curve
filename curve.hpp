#pragma once

#include <sx.safemath/safemath.hpp>
#include <math.h>

class curve
{
public:
    // GLOBAL SETTINGS
    static constexpr uint64_t FEE_DENOMINATOR = 10000000000; // 10 ** 10
    static constexpr uint64_t PRECISION = 100000000; // 10 ** 8;  // The precision to convert to
    static constexpr uint64_t FEE = 4000000; // 4 * 10 ** 6;

    // SIMPLE params
    uint16_t amplifier;
    uint64_t reserve0;
    uint64_t reserve1;
    std::vector<uint64_t> reserves;

    /**
     * A: Amplification coefficient
     * D: Total deposit size
     * n: number of currencies
     * p: target prices
     */
    explicit curve( const uint16_t amplifier, const uint64_t reserve0, const uint64_t reserve1 )
    :amplifier(amplifier), reserve0{reserve0}, reserve1{reserve1} {
        reserves = { reserve0, reserve1 };
    }

    uint64_t D () const {
        const uint128_t sum = reserve0 + reserve1;
        uint128_t previous = sum;
        for ( const uint128_t reserve : reserves ) {
            previous = previous * sum / (2 * reserve);
        }
        return (amplifier * 2 * sum + previous * 2) * sum / ((amplifier * 2 - 1) * sum + (2 + 1) * previous);
    }
};
