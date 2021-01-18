#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# create account
cleos create account eosio curve.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio myaccount EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

# contract
cleos set contract curve.sx . curve.sx.wasm curve.sx.abi
cleos set contract eosio.token ./include/eosio.token eosio.token.wasm eosio.token.abi

# @eosio.code permission
cleos set account permission curve.sx active --add-code

# create tokens
cleos push action eosio.token create '["eosio", "100000000.0000 A"]' -p eosio.token
cleos push action eosio.token issue '["eosio", "5000000.0000 A", "init"]' -p eosio

cleos push action eosio.token create '["eosio", "100000000.0000 B"]' -p eosio.token
cleos push action eosio.token issue '["eosio", "5000000.0000 B", "init"]' -p eosio

# transfer tokens
cleos transfer eosio curve.sx "1000.0000 B" "curve.sx"
cleos transfer eosio curve.sx "1000.0000 A" "curve.sx"
cleos transfer eosio myaccount "10000.0000 B" ""
cleos transfer eosio myaccount "10000.0000 A" ""