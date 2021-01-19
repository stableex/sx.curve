#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# Inputs
amplifier=450
reserve_in=5862496056
reserve_out=6260058778
fee=4

cleos -v push action curve.sx test "[10000000, $reserve_in, $reserve_out, $amplifier, $fee]" -p curve.sx
cleos -v push action curve.sx test "[10000000, $reserve_out, $reserve_in, $amplifier, $fee]" -p curve.sx
cleos -v push action curve.sx test "[10000000000, $reserve_in, $reserve_out, $amplifier, $fee]" -p curve.sx
cleos -v push action curve.sx test "[10000000000, $reserve_out, $reserve_in, $amplifier, $fee]" -p curve.sx

# settings
cleos push action curve.sx setsettings '[[4]]' -p curve.sx

# set pair
cleos -v push action curve.sx setpair '["AB", ["1000.0000 A", "eosio.token"], ["1000.0000 B", "eosio.token"], 100]' -p curve.sx

NOCOLOR='\033[0m'
RED='\033[0;31m'
LIGHTGREEN='\033[1;32m'

# swap must pass
printf "\nTransactions below must ${LIGHTGREEN}PASS${NOCOLOR}\n\n"
cleos transfer myaccount curve.sx "100.0000 A" "AB"
cleos transfer myaccount curve.sx "100.0000 B" "AB"
cleos transfer myaccount curve.sx "100.0000 A" "B"
cleos transfer myaccount curve.sx "100.0000 B" "A"
cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A"
cleos transfer myaccount curve.sx "100.0000 A" "90.0000 B@eosio.token"

# swap must fail
printf "\nTransactions below must ${RED}FAIL${NOCOLOR}\n\n"
cleos transfer myaccount curve.sx "100.0000 A" ""
cleos transfer myaccount curve.sx "100.0000 A" "BA"
cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A@curve.sx"
cleos transfer myaccount curve.sx "100.0000 B" "90 A@eosio.token"
cleos transfer myaccount curve.sx "100.0000 A" "10 B"
cleos transfer myaccount curve.sx "100.0000 B" "120.0000 A"