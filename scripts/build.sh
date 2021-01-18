#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# build
eosio-cpp curve.sx.cpp -I include -I ../
cleos set contract curve.sx . curve.sx.wasm curve.sx.abi
