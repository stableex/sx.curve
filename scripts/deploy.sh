#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# create account
cleos create account eosio curve.sx EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
# cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio myaccount EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

# contract
cleos set contract curve.sx . curve.sx.wasm curve.sx.abi
# cleos set contract eosio.token ../eosio.token eosio.token.wasm eosio.token.abi

# @eosio.code permission
cleos set account permission curve.sx active --add-code

# # create EOS token
# cleos push action eosio.token create '["eosio", "100000000.0000 EOS"]' -p eosio.token
# cleos push action eosio.token issue '["eosio", "5000000.0000 EOS", "init"]' -p eosio