#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)
# settings
cleos push action curve.sx setfee '[10, 10, "fee.sx"]' -p curve.sx
cleos push action curve.sx setstatus '["ok"]' -p curve.sx

# set pair
cleos -v push action curve.sx createpair '["AB", ["4,A", "eosio.token"], ["4,B", "eosio.token"], 20]' -p curve.sx
cleos -v push action curve.sx createpair '["BC", ["4,B", "eosio.token"], ["9,C", "eosio.token"], 100]' -p curve.sx
cleos -v push action curve.sx createpair '["AC", ["4,A", "eosio.token"], ["9,C", "eosio.token"], 200]' -p curve.sx

# add liquidity to pairs
cleos transfer myaccount curve.sx "1000.0000 A" "AB"
cleos transfer myaccount curve.sx "1000.0000 B" "AB"
cleos push action curve.sx deposit '["myaccount", "AB"]' -p myaccount

cleos transfer myaccount curve.sx "1000.0000 B" "BC"
cleos transfer myaccount curve.sx "1000.0000 C" "BC"
cleos push action curve.sx deposit '["myaccount", "BC"]' -p myaccount

cleos transfer myaccount curve.sx "1000.0000 A" "AC"
cleos transfer myaccount curve.sx "1000.0000 C" "AC"
cleos push action curve.sx deposit '["myaccount", "AC"]' -p myaccount
