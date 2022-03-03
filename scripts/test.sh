#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# variables
CONTRACT=curve.sx
LP_CONTRACT=lptoken.sx

# settings
cleos push action $CONTRACT init "[$LP_CONTRACT]" -p $CONTRACT
cleos push action $CONTRACT setfee '[10, 10, fee.sx]' -p $CONTRACT
cleos push action $CONTRACT setstatus '["ok"]' -p $CONTRACT

# set pair
cleos -v push action $CONTRACT createpair "[$CONTRACT, AB, [\"4,A\", eosio.token], [\"4,B\", eosio.token], 20]" -p $CONTRACT
cleos -v push action $CONTRACT createpair "[$CONTRACT, BC, [\"4,B\", eosio.token], [\"9,C\", eosio.token], 100]" -p $CONTRACT
cleos -v push action $CONTRACT createpair "[$CONTRACT, AC, [\"4,A\", eosio.token], [\"9,C\", eosio.token], 200]" -p $CONTRACT
cleos -v push action $CONTRACT createpair "[$CONTRACT, ABC, [\"4,AB\", $LP_CONTRACT], [\"9,C\", eosio.token], 20]" -p $CONTRACT

# add liquidity to pairs
cleos transfer myaccount $CONTRACT "1000.0000 A" "deposit,AB"
cleos transfer myaccount $CONTRACT "1000.0000 B" "deposit,AB"
cleos push action $CONTRACT deposit '["myaccount", "AB", null]' -p myaccount

cleos transfer myaccount $CONTRACT "1000.0000 B" "deposit,BC"
cleos transfer myaccount $CONTRACT "1000.000000000 C" "deposit,BC"
cleos push action $CONTRACT deposit '["myaccount", "BC", null]' -p myaccount

cleos transfer myaccount $CONTRACT "1000.0000 A" "deposit,AC"
cleos transfer myaccount $CONTRACT "1000.000000000 C" "deposit,AC"
cleos push action $CONTRACT deposit '["myaccount", "AC", null]' -p myaccount

cleos transfer myaccount $CONTRACT "1000.0000 AB" "deposit,ABC" --contract $LP_CONTRACT
cleos transfer myaccount $CONTRACT "1000.000000000 C" "deposit,ABC"
cleos push action $CONTRACT deposit '["myaccount", "ABC", null]' -p myaccount

# swap
cleos transfer myaccount $CONTRACT "100.0000 A" "swap,0,AC"
cleos transfer myaccount $CONTRACT "100.0000 A" "swap,0,AC-BC"
cleos transfer myaccount $CONTRACT "100.000000000 C" "swap,0,ABC"
