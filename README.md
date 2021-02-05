# **`SX Curve`**

> Peripheral EOSIO smart contracts for interacting with SX Curve

## Quickstart

### `cleos`

```bash
$ cleos transfer myaccount curve.sx "10.0000 USDT" "USN" --contract tethertether
# => receive "10.1000 USN@danchortoken"
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
