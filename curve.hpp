#pragma once

#include <sx.safemath/safemath.hpp>
#include <math.h>

class Curve
{
public:
    // GLOBAL SETTINGS
    static constexpr uint64_t FEE_DENOMINATOR = 10000000000; // 10 ** 10
    static constexpr uint64_t PRECISION = 100000000; // 10 ** 8;  // The precision to convert to
    static constexpr uint64_t FEE = 4000000; // 4 * 10 ** 6;

    // SIMPLE params
    int16_t amplifier;
    uint64_t reserve0;
    uint64_t reserve1;
    std::vector<uint64_t> reserves;

    /**
     * A: Amplification coefficient
     * D: Total deposit size
     * n: number of currencies
     * p: target prices
     */
    explicit Curve( const int16_t amplifier, const uint64_t reserve0, const uint64_t reserve1 )
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

    /**
     * Calculate x[j] if one makes x[i] = x
     *
     * Done by solving quadratic equation iteratively.
     * x_1**2 + x1 * (sum' - (A*n**n - 1) * D / (A * n**n)) = D ** (n + 1) / (n ** (2 * n) * prod' * A)
     * x_1**2 + b*x_1 = c
     *
     * x_1 = (x_1**2 + c) / (2*x_1 + b)
     */
    uint128_t y( const int64_t amount ) const {
        const int64_t _D = D();
        uint128_t c = _D;
        c = (c * _D) / (amount * 2);
        c = (c * _D) / (amplifier * 4);
        const int64_t b = amount + (_D / (amplifier * 2)) - _D;
        uint64_t y_prev = 0;
        uint64_t _y = _D;
        while (abs(int(_y - y_prev)) > 1) {
            y_prev = _y;
            // _y = uint128_t(uint64_t(pow(_y, 2)));
            _y = (uint128_t(uint64_t(pow(_y, 2))) + c) / (2 * _y + b);
            // _y = (pow(_y, 2) + c) / (2 * _y + b);
        }
        return _y;
    }
};
