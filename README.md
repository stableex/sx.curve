# **`SX Curve`**

> SX Curve is an amplified AMM (automated market maker) swap liquidity pool designed efficiently for stable currencies and/or highly correlated assets.

## Audits

- <a href="https://www.knownsec.com"><img height=30px src="https://user-images.githubusercontent.com/550895/120322175-1fe4fd00-c2b2-11eb-96bb-402dec711e38.png" /> KnownSec</a> (2021-05)
- <a href="https://s3.eu-central-1.wasabisys.com/audit-certificates/Smart%20Contract%20Audit%20Certificate%20-%20Sx.curve-final.pdf"><img height=30px src="https://sentnl.io/static/media/sentnl-logo.e5ce1ec4.png" /> Sentnl Audit</a> (2020-03)

## SHA256 Checksum

**Local**
```bash
$ git checkout v1.0.1
$ eosio-cpp curve.sx.cpp -I include
$ shasum -a 256 curve.sx.wasm
abb4b445cac42c9e0a1ceac33dc7e9c36a1452eb89bdff05b907cf3936b2ff9c  curve.sx.wasm
```

**EOS Mainnet**
```bash
$ cleos -u https://api.eosn.io get code curve.sx
code hash: abb4b445cac42c9e0a1ceac33dc7e9c36a1452eb89bdff05b907cf3936b2ff9c
```

## Quickstart

### `convert`

> memo schema: `swap,<min_return>,<pair_ids>`

```bash
$ cleos transfer myaccount curve.sx "10.0000 USDT" "swap,0,SXA" --contract tethertether
# => receive "10.0000 USN@danchortoken"
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
//=> "10.0000 USN"
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
