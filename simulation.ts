const zip = (arr1: number[], arr2: number[]) => arr1.map((k, i) => [k, arr2[i]]);
const range = (num: number) => [...Array(num).keys()];
const sum = (arr1: number[]) => arr1.reduce((a, b) => a + b, 0);

/**
 * Typescript model of Curve pool math.
 */
class Curve {
    public A: number;
    public n: number;
    public fee: number;
    public p: number[];
    public x: number[];
    public tokens: number | null;

    /**
     * A: Amplification coefficient
     * D: Total deposit size
     * n: number of currencies
     * p: target prices
     */
    constructor(A: number, D: number[] | number, n: number, p: number[]|null = null, tokens: number| null = null) {
        this.A = A // actually A * n ** (n - 1) because it's an invariant
        this.n = n
        this.fee = 10 ** 7
        this.p = p ? p : Array(n).fill(10 ** 18);
        this.x = Array.isArray(D) ? D : this.p.map(_p => Math.floor(D / n) * 10 ** Math.floor(18 / _p));
        this.tokens = tokens;
    }

    public xp(): number[] {
        return zip(this.x, this.p).map(([x, p]) => Math.floor(x * p / 10 ** 18))
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
        const fee = Math.floor(dy * this.fee / 10 ** 10)
        if (dy > 0) throw new Error();
        this.x[i] = Math.floor(x * 10 ** 18 / this.p[i]);
        this.x[j] = Math.floor((y + fee) * 10 ** 18 / this.p[j])
        return dy - fee
    }
}

const reserveA = 62755905
const reserveB = 78805744
const reserveC = 26929921
const total = reserveA + reserveB + reserveC
const c = new Curve(200, total, 3)
// console.log(c.xp());
// console.log(c.D());
// console.log(c.y(1, 2, 3));
console.log(c.exchange(100, 100, 100));

// class Curve:

//     def remove_liquidity_imbalance(self, amounts):
//         _fee = self.fee * self.n // (4 * (self.n - 1))

//         old_balances = self.x
//         new_balances = self.x[:]
//         D0 = self.D()
//         for i in range(self.n):
//             new_balances[i] -= amounts[i]
//         self.x = new_balances
//         D1 = self.D()
//         self.x = old_balances
//         fees = [0] * self.n
//         for i in range(self.n):
//             ideal_balance = D1 * old_balances[i] // D0
//             difference = abs(ideal_balance - new_balances[i])
//             fees[i] = _fee * difference // 10 ** 10
//             new_balances[i] -= fees[i]
//         self.x = new_balances
//         D2 = self.D()
//         self.x = old_balances

//         token_amount = (D0 - D2) * self.tokens // D0

//         return token_amount

//     def calc_withdraw_one_coin(self, token_amount, i):
//         xp = self.xp()
//         if self.fee:
//             fee = self.fee - self.fee * xp[i] // sum(xp) + 5 * 10 ** 5
//         else:
//             fee = 0

//         D0 = self.D()
//         D1 = D0 - token_amount * D0 // self.tokens
//         dy = xp[i] - self.y_D(i, D1)

//         return dy - dy * fee // 10 ** 10

