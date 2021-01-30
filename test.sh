#!/bin/bash

cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

bats ./scripts/bats/system.bats
bats ./scripts/bats/config.bats
bats ./scripts/bats/formula.bats
bats ./scripts/bats/create_pairs.bats
bats ./scripts/bats/liquidity.bats