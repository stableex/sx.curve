#!/usr/bin/env bats

@test "create AB" {
  run cleos push action curve.sx createpair '["curve.sx", "AB", ["4,A", "eosio.token"], ["4,B", "eosio.token"], 20]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].id')
  echo "Output: $output"
  [ $result = AB ]
}

@test "create AC" {
  run cleos push action curve.sx createpair '["curve.sx", "AC", ["4,A", "eosio.token"], ["9,C", "eosio.token"], 200]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].id')
  echo "Output: $output"
  [ $result = AC ]
}

@test "create BC" {
  run cleos push action curve.sx createpair '["curve.sx", "BC", ["4,B", "eosio.token"], ["9,C", "eosio.token"], 100]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].id')
  echo "Output: $output"
  [ $result = BC ]
}

@test "create CAB" {
  run cleos push action curve.sx createpair '["curve.sx", "CAB", ["4,AB", "lptoken.sx"], ["9,C", "eosio.token"], 20]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[3].id')
  echo "Output: $output"
  [ $result = CAB ]
}

@test "create DE" {
  run cleos push action curve.sx createpair '["curve.sx", "DE", ["6,D", "eosio.token"], ["6,E", "eosio.token"], 200]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[3].id')
  echo "Output: $output"
  [ $result = DE ]
}

@test "invalid pairs" {
  run cleos push action curve.sx createpair '["curve.sx", "AB", ["4,A", "eosio.token"], ["4,B", "eosio.token"], 20]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "already exists" ]]

  run cleos push action curve.sx createpair '["curve.sx", "AB", ["4,A", "some.token"], ["4,B", "eosio.token"], 20]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "contract does not exists" ]]

  run cleos push action curve.sx createpair '["curve.sx", "AB", ["5,A", "eosio.token"], ["4,B", "eosio.token"], 20]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "symbol mismatch" ]]

  run cleos push action curve.sx createpair '["curve.sx", "AD", ["4,A", "eosio.token"], ["6,D", "eosio.token"], 2000000]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "invalid amplifier" ]]
}