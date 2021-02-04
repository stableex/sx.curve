#!/usr/bin/env bats

@test "initialized" {
  run cleos transfer myaccount curve.sx "1000.0000 A" ""
  echo "Output: $output"
  [ $status -eq 1 ]
  [[ "$output" =~ "initialized" ]]
}

@test "set config" {
  run cleos push action curve.sx setfee '[4, 0, "fee.sx"]' -p curve.sx
  run cleos push action curve.sx setstatus '["ok"]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]
}

@test "config.status = ok" {
  result=$(cleos get table curve.sx curve.sx config | jq -r '.rows[0].status')
  echo $result
  [ $result = "ok" ]
}
