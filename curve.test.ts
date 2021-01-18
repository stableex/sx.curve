import { get_amount_out } from "./curve";

test("get_amount_out #1 (pass)", () => {
    const amount_in = 100000n
    const reserve_in = 3432247548n
    const reserve_out = 6169362700n
    const amplifier = 450n

    expect( get_amount_out( amount_in, reserve_in, reserve_out, amplifier ) ).toBe( 100110n );
});

test("get_amount_out #2 (pass)", () => {
    const reserve_in = 5862496056n
    const reserve_out = 6260058778n
    const amplifier = 450n
    const fee = 4n

    expect( get_amount_out( 10000000n, reserve_in, reserve_out, amplifier, fee ) ).toBe( 9997422n );
    expect( get_amount_out( 10000000n, reserve_out, reserve_in, amplifier, fee ) ).toBe( 9994508n );
    expect( get_amount_out( 10000000000n, reserve_in, reserve_out, amplifier, fee ) ).toBe( 6249264902n );
    expect( get_amount_out( 10000000000n, reserve_out, reserve_in, amplifier, fee ) ).toBe( 5852835188n );
});
