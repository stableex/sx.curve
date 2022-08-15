# **`SX Curve`**

> SX Curve is an amplified AMM (automated market maker) swap liquidity pool designed efficiently for stable currencies and/or highly correlated assets.

## Audits

- <a href="https://github.com/slowmist/Knowledge-Base/blob/b717756a17702e604ab2dc8b072a2f127304b17d/open-report/Smart%20Contract%20Security%20Audit%20Report%20-%20sx.curve.pdf"><img height=30px src="https://user-images.githubusercontent.com/550895/132642025-b4dacacd-e1c1-4a02-9bb1-09ae359eb1f9.png" /> SlowMist Audit</a> (2021-07)
- <a href="https://www.knownsec.com"><img height=30px src="https://user-images.githubusercontent.com/550895/120322175-1fe4fd00-c2b2-11eb-96bb-402dec711e38.png" /> KnownSec</a> (2021-05)
- <a href="https://s3.eu-central-1.wasabisys.com/audit-certificates/Smart%20Contract%20Audit%20Certificate%20-%20Sx.curve-final.pdf"><img height=30px src="https://user-images.githubusercontent.com/550895/132641907-6425e632-1b1b-4015-9b84-b7f26a25ec58.png" /> Sentnl Audit</a> (2021-03)

## SHA256 Checksum

**Local**
```bash
$ git checkout v1.0.3
$ eosio-cpp --version
eosio-cpp version 1.7.0
$ eosio-cpp curve.sx.cpp -I include
$ shasum -a 256 curve.sx.wasm
a9aa7e60901cfc3ffe481099daa7f13f0b9cdba19c64ffd18aa5789932336f8c  curve.sx.wasm
```

**EOS Mainnet**
```bash
$ cleos -u https://eos.greymass.com get code curve.sx
code hash: a9aa7e60901cfc3ffe481099daa7f13f0b9cdba19c64ffd18aa5789932336f8c
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
$ cleos push action curve.sx deposit '["myaccount", "SXA", 200000]' -p myaccount
# => receive "20.0000 SXA@lptoken.sx"
```

### `withdraw`

> memo schema: `N/A`

```bash
$ cleos transfer myaccount curve.sx "20.0000 SXA" "" --contract lptoken.sx
# => receive "10.0000 USDT@tethertether" + "10.0000 USN@danchortoken"
```

### `cancel`

```bash
$ cleos transfer myaccount curve.sx "10.0000 USDT" "deposit,SXA" --contract tethertether
$ cleos push action curve.sx cancel '["myaccount", "SXA"]' -p myaccount
# => receive "10.0000 USDT@tethertether"
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
