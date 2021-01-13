#!/bin/bash

# compile
g++ -std=c++11 -o curve.t.out curve.t.cpp -I __tests__ -I ../

# test
./curve.t.out --success
