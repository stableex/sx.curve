#!/usr/bin/env bats

@test "create AB" {
  run cleos push action curve.sx createpair '["curve.sx", "AB", ["4,A", "eosio.token"], ["4,B", "eosio.token"], 20]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].id')
  echo "Output: $output"
  [ $result = AB ]
}

@test "duplicate pair" {
  run cleos push action curve.sx createpair '["curve.sx", "AB", ["4,A", "eosio.token"], ["4,B", "eosio.token"], 20]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 1 ]
}

@test "create AC" {
  run cleos push action curve.sx createpair '["curve.sx", "AC", ["4,A", "eosio.token"], ["8,C", "eosio.token"], 200]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].id')
  echo "Output: $output"
  [ $result = AC ]
}

@test "create BC" {
  run cleos push action curve.sx createpair '["curve.sx", "BC", ["4,B", "eosio.token"], ["8,C", "eosio.token"], 100]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].id')
  echo "Output: $output"
  [ $result = BC ]
}

