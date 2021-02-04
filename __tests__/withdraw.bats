
#!/usr/bin/env bats

@test "Withdraw all" {
  ac_balance=$(cleos get currency balance lptoken.sx liquidity.sx AC)
  ab_balance=$(cleos get currency balance lptoken.sx liquidity.sx AB)
  bc_balance=$(cleos get currency balance lptoken.sx liquidity.sx BC)

  run cleos transfer liquidity.sx curve.sx "$ac_balance" "" --contract lptoken.sx
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].liquidity.quantity')
  [ "$result" = "0.000000000 AC" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve0.quantity')
  [ "$result" = "0.0000 A" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve1.quantity')
  [ "$result" = "0.000000000 C" ]

  run cleos transfer liquidity.sx curve.sx "$ab_balance" "" --contract lptoken.sx
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].liquidity.quantity')
  [ "$result" = "0.0000 AB" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].reserve0.quantity')
  [ "$result" = "0.0000 A" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].reserve1.quantity')
  [ "$result" = "0.0000 B" ]

  run cleos transfer liquidity.sx curve.sx "$bc_balance" "" --contract lptoken.sx
  [ $status -eq 0 ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].liquidity.quantity')
  [ "$result" = "0.000000000 BC" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].reserve0.quantity')
  [ "$result" = "0.0000 B" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].reserve1.quantity')
  [ "$result" = "0.000000000 C" ]
}

@test "remove pairs" {

  run cleos push action curve.sx removepair '["AB"]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]

  run cleos push action curve.sx removepair '["AC"]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]

  run cleos push action curve.sx removepair '["BC"]' -p curve.sx
  echo "Output: $output"
  [ $status -eq 0 ]

  run cleos push action curve.sx removepair '["AD"]' -p curve.sx
  echo "Output: $output"
  [[ "$output" =~ "does not exist" ]]
  [ $status -eq 1 ]
}
