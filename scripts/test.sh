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
cleos push action curve.sx setconfig '[["ok", 4, 0, "fee.sx"]]' -p curve.sx

# set pair
cleos -v push action curve.sx setpair '["AB", ["1000.0000 A", "eosio.token"], ["1000.0000 B", "eosio.token"], 20]' -p curve.sx
cleos -v push action curve.sx setpair '["BC", ["1000.0000 B", "eosio.token"], ["1000.00000000 C", "eosio.token"], 100]' -p curve.sx
cleos -v push action curve.sx setpair '["AC", ["1000.0000 A", "eosio.token"], ["1000.00000000 C", "eosio.token"], 200]' -p curve.sx

NOCOLOR='\033[0m'
RED='\033[0;31m'
LIGHTGREEN='\033[1;32m'

# swap must fail
printf "\nTransactions below must ${RED}FAIL${NOCOLOR}\n\n"
cleos transfer myaccount curve.sx "100.0000 A" ""
cleos transfer myaccount curve.sx "100.0000 A" "BA"
cleos transfer myaccount curve.sx "100.0000 B" "AC"
cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A@curve.sx"
cleos transfer myaccount curve.sx "100.0000 B" "90 A@eosio.token"
cleos transfer myaccount curve.sx "100.0000 A" "10 B"
cleos transfer myaccount curve.sx "100.0000 B" "120.0000 A"
cleos transfer myaccount curve.sx "100.0000 A" "90.000000 C@eosio.token"
cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A,BadUserName"
cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A, myaccount"
cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A,myaccount,curve.sx"
cleos transfer myaccount curve.sx "100.0000 A" "AC,curve.sx"
cleos transfer myaccount curve.sx "100.0000 A" "AC,nonexistuser"
cleos transfer myaccount curve.sx "100.0000 A" "B" --contract fake.token

# swap must pass
printf "\nTransactions below must ${LIGHTGREEN}PASS${NOCOLOR}\n\n"
cleos transfer myaccount curve.sx "100.0000 A" "B"
cleos transfer myaccount curve.sx "100.0000 B" "A"
cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A"
cleos transfer myaccount curve.sx "100.0000 A" "90.0000 B@eosio.token"
cleos transfer myaccount curve.sx "100.0000 A" "90.00000000 C@eosio.token"
cleos transfer myaccount curve.sx "100.00000000 C" "90.0000 A"
cleos transfer myaccount curve.sx "100.0000 C" "A,myaccount"
cleos transfer myaccount curve.sx "100.0000 B" "C"
cleos transfer myaccount curve.sx "100.0000 A" "C"

# add liquidity
cleos transfer myaccount curve.sx "100.0000 A" "AB"
cleos transfer myaccount curve.sx "100.0000 B" "AB"