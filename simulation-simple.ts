const zip = (arr1: number[], arr2: number[]) => arr1.map((k, i) => [k, arr2[i]]);
const range = (num: number) => [...Array(num).keys()];
const sum = (arr1: number[]) => arr1.reduce((a, b) => a + b, 0);

// GLOBAL SETTINGS
const FEE_DENOMINATOR = 10 ** 10
const PRECISION = 10 ** 18  // The precision to convert to
const FEE = 4 * 10 ** 6;

/**
 * Typescript model of Curve pool math.
 */
class Curve {
    public A: number;
    public n: number;
    public p: number[];
    public x: number[];

    /**
     * A: Amplification coefficient
     * D: Total deposit size
     * n: number of currencies
     * p: target prices
     */
    constructor( A: number, D: number[] ) {
        this.A = A
        this.n = D.length;
        this.p = Array(this.n).fill(PRECISION);
        this.x = D;
    }

    // public xp(): number[] {
    //     return zip(this.x, this.p).map(([x, p]) => Math.floor(x * p / PRECISION))
    // }

    public xp(): number[] {
        return [this.x[0] * this.p[0] / PRECISION, this.x[1] * this.p[1] / PRECISION];
        // return zip(this.x, this.p).map(([x, p]) => Math.floor(x * p / PRECISION))
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
    public D(): number {
        let Dprev = 0
        const xp = this.xp()
        const S = sum(xp);
        let D = S
        const Ann = this.A * this.n
        while (Math.abs(D - Dprev) > 1) {
            let D_P = D
            for ( const x of xp ) {
                D_P = Math.floor(D_P * D / (this.n * x))
            }
            Dprev = D
            D = Math.floor((Ann * S + D_P * this.n) * D / ((Ann - 1) * D + (this.n + 1) * D_P))
        }
        return D
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
    public y(i: number, j: number, x: number): number {
        const D = this.D()
        let xx = this.xp()
        xx[i] = x  // x is quantity of underlying asset brought to 1e18 precision
        xx = range(this.n).filter( k => k != j).map(k => xx[k]);

        let Ann = this.A * this.n
        let c = D
        for (const y of xx ) {
            c = Math.floor(c * D / (y * this.n))
        }
        c = Math.floor(c * D / (this.n * Ann))
        const b = sum(xx) + Math.floor(D / Ann) - D
        let y_prev = 0
        let y = D
        while (Math.abs(y - y_prev) > 1) {
            y_prev = y
            y = Math.floor((y ** 2 + c) / (2 * y + b))
        }
        return y  // the result is in underlying units too
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
    public y_D(i: number, _D: number) {
        let xx = this.xp()
        xx = range(this.n).filter( k => k != i).map(k => xx[k]);
        const S = sum(xx);
        const Ann = this.A * this.n
        let c = _D
        for ( const y of xx ) {
            c = Math.floor(c * _D / (y * this.n))
        }
        c = c * Math.floor(_D / (this.n * Ann))
        const b = S + Math.floor(_D / Ann)
        let y_prev = 0
        let y = _D
        while ( Math.abs(y - y_prev) > 1 ) {
            y_prev = y
            y = Math.floor((y ** 2 + c) / (2 * y + b - _D))
        }
        return y  // the result is in underlying units too
    }

    /**
     * dx and dy are in underlying units
     */
    public dy(i: number, j: number, dx: number) {
        const xp = this.xp()
        return xp[j] - this.y(i, j, xp[i] + dx)
    }

    public exchange(i: number, j: number, dx: number): number {
        const xp = this.xp()
        const x = xp[i] + dx
        const y = this.y(i, j, x)
        const dy = xp[j] - y
        const fee = Math.floor(dy * FEE / FEE_DENOMINATOR)

        console.log("x:", x);
        console.log("xp:", xp);
        console.log("y:", y);
        console.log("dy:", dy);
        console.log("fee:", fee);

        if (!(dy > 0)) throw new Error();
        this.x[i] = Math.floor(x * PRECISION / this.p[i]);
        this.x[j] = Math.floor((y + fee) * PRECISION / this.p[j])
        return dy - fee
    }
}

const amplifier = 450
const reserve0 = 3432247548
const reserve1 = 6169362700
// const reserve0 = 8000000
// const reserve1 = 12000000

const deposits = [reserve0, reserve1]
const c = new Curve(amplifier, deposits)
console.log("xp()", c.xp());
console.log("D()", c.D());
console.log("y(1,2,3)", c.y(1, 2, 3));
console.log("exchange", c.exchange(0, 1, 100000));
