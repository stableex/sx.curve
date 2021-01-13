#!/bin/bash

# build
eosio-cpp curve.sx.cpp -I ../
cleos set contract curve.sx . curve.sx.wasm curve.sx.abi
