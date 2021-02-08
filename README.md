# **`SX Curve`**

> Peripheral EOSIO smart contracts for interacting with SX Curve

## Quickstart

### `convert`

> memo schema: `swap,<min_return>,<pair_ids>`

```bash
$ cleos transfer myaccount curve.sx "10.0000 USDT" "swap,0,SXA" --contract tethertether
# => receive "10.1000 USN@danchortoken"
```

### `deposit`

> memo schema: `deposit,<pair_id>`

```bash
$ cleos transfer myaccount curve.sx "10.0000 USDT" "deposit,SXA" --contract tethertether
$ cleos transfer myaccount curve.sx "10.0000 USN" "deposit,SXA" --contract danchortoken
$ cleos push action curve.sx deposit '["myaccount", "SXA"]' -p myaccount
# => receive "20.0000 SXA@lptoken.sx"
```

### `withdraw`

> memo schema: `N/A`

```bash
$ cleos transfer myaccount curve.sx "20.0000 SXA" "" --contract lptoken.sx
# => receive "10.0000 USDT@tethertether" + "10.0000 USN@danchortoken"
```

### C++

```c++
#include "curve.sx.hpp"

// User Inputs
const asset in = asset{10'0000, {"USDT", 4}};
const symbol_code pair_id = symbol_code{"SXA"};

// Calculated Output
const asset out = sx::curve::get_amount_out( in, pair_id );
//=> "10.1000 USN"
```

## Dependencies

- [sx.utils](https://github.com/stableex/sx.utils)
- [sx.rex](https://github.com/stableex/sx.rex)
- [eosio.token](https://github.com/EOSIO/eosio.contracts)

## Testing

**Requirements:**

- [**Bats**](https://github.com/sstephenson/bats) - Bash Automated Testing System
- [**EOSIO**](https://github.com/EOSIO/eos) - `nodeos` is the core service daemon & `cleos` command line tool
- [**Blanc**](https://github.com/turnpike/blanc) - Toolchain for WebAssembly-based Blockchain Contracts

```bash
$ ./scripts/build.sh
$ ./scripts/restart.sh
$ ./test.sh
```
