
// calculations
const fee = 30n;
const amount_out = 373786282495n;
const reserve_in = 100669664n;
const reserve_out = 3774590382732755n;

const numerator = reserve_in * amount_out * 10000n;
const denominator = (reserve_out - amount_out) * (10000n - fee);
const amount_in = (numerator / denominator) + 1n;

console.log(amount_in);
// => 3734523467747
