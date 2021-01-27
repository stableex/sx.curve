#!/bin/bash

cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

bats ./scripts/bats/system.bats
bats ./scripts/bats/formula.bats