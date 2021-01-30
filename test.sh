#!/bin/bash

cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

bats ./__tests__/system.bats
bats ./__tests__/config.bats
bats ./__tests__/formula.bats
bats ./__tests__/create_pairs.bats
bats ./__tests__/liquidity.bats