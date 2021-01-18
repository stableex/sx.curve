const zip = (arr1: number[], arr2: number[]) => arr1.map((k, i) => [k, arr2[i]]);
const range = (num: number) => [...Array(num).keys()];
const sum = (arr1: number[]) => arr1.reduce((a, b) => a + b, 0);
const sumBigInt = (arr1: bigint[]) => arr1.reduce((a, b) => a + b, 0n);

// GLOBAL SETTINGS
const FEE_DENOMINATOR = 10n ** 10n
const PRECISION = 10n ** 8n  // The precision to convert to
const FEE = 4n * 10n ** 6n;

/**
 * Typescript model of Curve pool math.
 */
class Curve {
    public amplifier: bigint;
    public reserve0: bigint;
    public reserve1: bigint;
    public reserves: bigint[];

    /**
     * A: Amplification coefficient
     * D: Total deposit size
     * n: number of currencies
     * p: target prices
     */
    constructor( amplifier: bigint, reserve0: bigint, reserve1: bigint ) {
        this.amplifier = amplifier;
        this.reserve0 = reserve0;
        this.reserve1 = reserve1;
        this.reserves = [reserve0, reserve1];
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
    public D(): bigint {
        const sum = this.reserve0 + this.reserve1;
        let previous = sum
        for ( const reserve of this.reserves ) {
            previous = previous * sum / (2n * reserve)
        }
        return (this.amplifier * 2n * sum + previous * 2n) * sum / ((this.amplifier * 2n - 1n) * sum + (2n + 1n) * previous)
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
    public y( amount: bigint ): bigint {
        const D = this.D()
        let c = D
        c = c * D / (amount * 2n)
        c = c * D / (this.amplifier * 4n)
        // return D;
        const b = amount + (D / (this.amplifier * 2n)) - D
        // return b;
        let y_prev = 0n
        let y = D
        while (Math.abs(Number(y - y_prev)) > 1) {
            y_prev = y
            y = (y ** 2n + c) / (2n * y + b)
        }
        return y
    }

    public exchange( amount_in: bigint ): bigint {
        const reserve_in = this.reserve0 + amount_in;
        const numerator = this.y(reserve_in);
        const out = this.reserve1 - numerator;
        const fee = out * FEE / FEE_DENOMINATOR;

        if (!(out > 0)) throw new Error("exchange amount too small");
        return out - fee;
    }
}

const reserve0 = 3432247548n
const reserve1 = 6169362700n
const amplifier = 450n
// const reserve0 = 8000000n
// const reserve1 = 12000000n
// const deposits = [reserve0, reserve1]
const c = new Curve(amplifier, reserve0, reserve1)
// console.log("xp()", c.xp());
// console.log("D()", c.D());
console.log("y(0,1,100000)", c.y(100000n));
console.log("exchange", c.exchange(100000n));
