/**
 * ## STATIC `get_amount_out`
 *
 * Given an input amount, reserves pair and amplifier, returns the output amount of the other asset based on Curve formula
 * Whitepaper: https://www.curve.fi/stableswap-paper.pdf
 * Python implementation: https://github.com/curvefi/curve-contract/blob/master/tests/simulation.py
 *
 * ### params
 *
 * - `{uint64_t} amount_in` - amount input
 * - `{uint64_t} reserve_in` - reserve input
 * - `{uint64_t} reserve_out` - reserve output
 * - `{uint64_t} amplifier` - amplifier
 * - `{uint8_t} [fee=4]` - (optional) trade fee (pips 1/100 of 1%)
 *
 * ### example
 *
 * ```c++
 * // Inputs
 * const uint64_t amount_in = 100000;
 * const uint64_t reserve_in = 3432247548;
 * const uint64_t reserve_out = 6169362700;
 * cont uint64_t amplifier = 450;
 * const uint8_t fee = 4;
 *
 * // Calculation
 * const uint64_t amount_out = curve::get_amount_out( amount_in, reserve_in, reserve_out, amplifier, fee );
 * // => 100110
 * ```
 */
export function get_amount_out( amount_in: bigint, reserve_in: bigint, reserve_out: bigint, amplifier: bigint, fee = 4n )
{
    if ( !(amount_in > 0 )) throw new Error("SX.Curve: INSUFFICIENT_INPUT_AMOUNT");
    if ( !(amplifier > 0 )) throw new Error("SX.Curve: WRONG_AMPLIFIER");
    if ( !(reserve_in > 0 && reserve_out > 0 )) throw new Error("SX.Curve: INSUFFICIENT_LIQUIDITY");
    if ( !(fee <= 100 )) throw new Error("SX.Curve: FEE_TOO_HIGH");

    // calculate invariant D by solving quadratic equation:
    // A * sum * n^n + D = A * D * n^n + D^(n+1) / (n^n * prod), where n==2
    const sum: bigint = reserve_in + reserve_out;
    let D: bigint = sum, D_prev = 0n;
    while (D != D_prev) {
        let prod1: bigint = D * D / (reserve_in * 2n) * D / (reserve_out * 2n);
        D_prev = D;
        D = 2n * D * (amplifier * sum + prod1) / ((2n * amplifier - 1n) * D + 3n * prod1);
    }

    // calculate x - new value for reserve_out by solving quadratic equation iteratively:
    // x^2 + x * (sum' - (An^n - 1) * D / (An^n)) = D ^ (n + 1) / (n^(2n) * prod' * A), where n==2
    // x^2 + b*x = c
    const b: number = Number(reserve_in + amount_in) + Number(D / (amplifier * 2n)) - Number(D);
    const c: bigint = D * D / ((reserve_in + amount_in) * 2n) * D / (amplifier * 4n);
    let x: bigint = D, x_prev = 0n;
    while (x != x_prev) {
        x_prev = x;
        x = BigInt(Math.floor(Number(x * x + c) / (2 * Number(x) + b)));
    }

    if (!(reserve_out > x)) throw new Error("SX.Curve: INSUFFICIENT_RESERVE_OUT");
    const amount_out: bigint = reserve_out - BigInt(x);

    return amount_out - fee * amount_out / 10000n;
}
