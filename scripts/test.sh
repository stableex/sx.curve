#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# init SXEOS from vaults & send to curve.sx
# cleos transfer eosio curve.sx "0.0100 EOS"

# settings
# cleos push action curve.sx setsettings '[[["basic.sx"], 20]]' -p curve.sx

cleos push action curve.sx test '[100000]' -p curve.sx
