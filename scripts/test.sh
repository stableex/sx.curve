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

# swap
cleos transfer myaccount curve.sx "100.0000 A" "AB"
cleos transfer myaccount curve.sx "100.0000 B" "AB"
