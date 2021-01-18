import { get_amount_out } from "../curve";

test("get_amount_out", () => {
    const amount_in = 100000n
    const reserve_in = 3432247548n
    const reserve_out = 6169362700n
    const amplifier = 450n
    const amount_out = get_amount_out( amount_in, reserve_in, reserve_out, amplifier )
    expect( amount_out ).toBe( 100110n );
});