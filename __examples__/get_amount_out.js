
// calculations
const fee = 30n;
const amount_in = 10000n;
const reserve_in = 100669664n;
const reserve_out = 3774590382732755n;

const amount_in_with_fee = amount_in * (10000n - fee);
const numerator = amount_in_with_fee * reserve_out;
const denominator = (reserve_in * 10000n) + amount_in_with_fee;
const amount_out = numerator / denominator;
console.log(amount_out);
// => 10000n
