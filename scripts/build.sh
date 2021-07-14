#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# build
# eosio-cpp curve.sx.cpp -I include
blanc++ curve.sx.cpp -I include
cleos set contract curve.sx . curve.sx.wasm curve.sx.abi
