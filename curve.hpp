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

    // CURVE PARAMS
    uint16_t A;
    uint8_t n;
    std::vector<uint64_t> p;
    std::vector<uint64_t> x;

    /**
     * A: Amplification coefficient
     * D: Total deposit size
     * n: number of currencies
     * p: target prices
     */
    explicit curve( const uint16_t A, const std::vector<uint64_t> D )
    :A(A), x{D}
    {
        n = D.size();
        p = { PRECISION, PRECISION };
    }

    std::vector<uint64_t> xp () const {
        return { x[0] * p[0] / PRECISION, x[1] * p[1] / PRECISION };
        // return { x[0], x[1] };
    }

    /**
     * D invariant calculation in non-overflowing integer operations
     * iteratively
     *
     * A * sum(x_i) * n**n + D = A * D * n**n + D**(n+1) / (n**n * prod(x_i))
     *
     * Converging solution:
     * D[j+1] = (A * n**n * sum(x_i) - D[j]**(n+1) / (n**n prod(x_i))) / (A * n**n - 1)
     */
    uint64_t D () const {
        uint64_t Dprev = 0;
        const std::vector<uint64_t> _xp = xp();
        const uint64_t S = _xp[0] + _xp[1];
        uint64_t _D = S;
        const uint64_t Ann = A * n;

        while ( abs(int(_D - Dprev)) > 1 ) {
            uint64_t D_P = _D;
            for ( uint64_t x : _xp ) {
                D_P = floor(D_P * _D / (n * x));
            }
            Dprev = _D;
            _D = floor((Ann * S + D_P * n) * _D / ((Ann - 1) * _D + (n + 1) * D_P));
        }
        return _D;
    }
};
