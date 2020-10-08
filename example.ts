function curve(amount, reserveA, reserveB, amplifier) {
    /** StableSwap (curve) formula based on Barak's implenantation with Aplifier (no fee) **/
    /** this formula is only valid for 50%/50% pools **/
    const total = reserveA + reserveB;

    return (
    (4 * amplifier * (reserveA + amount)
    + 8 * amplifier * reserveB
    + total
    - 4 * amplifier * total
    - Math.pow
    (
        (
        4 * amplifier * total * total * total
        / (reserveA + amount)
        + 16 * amplifier * amplifier * total * total
        + 16 * amplifier * amplifier * (reserveA + amount) * (reserveA + amount)
        +  8 * amplifier * total * (reserveA + amount)
        +  total * total
        -  32 * amplifier * amplifier * total * (reserveA + amount)
        -  8 * amplifier * total * total
        )
        ,0.5
        ))
        / (8 * amplifier));
}

for ( const amplifier of [10, 50, 100, 200]) {
    const reserveA = 10000;
    const reserveB = 10000;
    console.log("amplifier:", amplifier)
    console.log("reserveA:", reserveA)
    console.log("reserveB:", reserveB)
    console.log('inputs')
    for ( const input of [5000, 10000, 20000, 30000]) {
        console.log("- ",input, "=>", curve(input, reserveA, reserveB, amplifier));
    }
}
