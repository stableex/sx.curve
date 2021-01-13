#!/bin/bash

nodeos -p eosio -e \
  --data-dir ./nodeos/data \
  --config-dir ./nodeos/config  \
  --plugin eosio::http_plugin \
  --plugin eosio::chain_plugin \
  --plugin eosio::chain_api_plugin \
  --plugin eosio::producer_plugin \
  --plugin eosio::producer_api_plugin \
  --plugin eosio::history_plugin \
  --plugin eosio::history_api_plugin \
  --plugin eosio::state_history_plugin \
  --http-server-address=0.0.0.0:8888 \
  --access-control-allow-origin=* \
  --http-validate-host=false \
  --max-transaction-time=1000 \
  --replay-blockchain \
  --hard-replay-blockchain \
  --verbose-http-errors \
  --disable-replay-opts \
  --delete-all-blocks \
  --producer-name eosio \
  --contracts-console \
  --filter-on=* \
  --filter-out=eosio:onblock: \
  --trace-history \
  --chain-state-history 2>stderr &
