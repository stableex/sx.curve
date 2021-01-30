#!/usr/bin/env bats

@test "simple deposit AB" {
  run cleos transfer myaccount curve.sx "1000.0000 A" "AB"
  run cleos transfer myaccount curve.sx "1000.0000 B" "AB"
  echo "Output: $output"
  [ $status -eq 0 ]

  result=$(cleos get table curve.sx AB orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "1000.0000 A" ]
  result=$(cleos get table curve.sx AB orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "1000.0000 B" ]

  run cleos push action curve.sx deposit '["myaccount", "BC"]' -p myaccount
  [ $status -eq 1 ]
  echo "Output: $output"

  run cleos push action curve.sx deposit '["curve.sx", "AB"]' -p curve.sx
  [ $status -eq 1 ]
  echo "Output: $output"

  run cleos push action curve.sx deposit '["myaccount", "AB"]' -p myaccount
  [ $status -eq 0 ]
  echo "Output: $output"

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].reserve0.quantity')
  [ "$result" = "1000.0000 A" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].reserve1.quantity')
  [ "$result" = "1000.0000 B" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].liquidity.quantity')
  [ "$result" = "2000.0000 AB" ]
  result=$(cleos get currency balance lptoken.sx myaccount)
  [ "$result" = "2000.0000 AB" ]
}


@test "deposit with excess BC" {
  run cleos transfer myaccount curve.sx "1100.0000 B" "BC"
  run cleos transfer myaccount curve.sx "1000.00000000 C" "BC"
  echo "Output: $output"
  [ $status -eq 0 ]

  result=$(cleos get table curve.sx BC orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "1100.0000 B" ]
  result=$(cleos get table curve.sx BC orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "1000.00000000 C" ]

  result=$(cleos get currency balance eosio.token myaccount B)
  [ "$result" = "7900.0000 B" ]

  run cleos push action curve.sx deposit '["myaccount", "BC"]' -p myaccount
  [ $status -eq 0 ]
  echo "Output: $output"

  result=$(cleos get currency balance eosio.token myaccount B)
  [ "$result" = "8000.0000 B" ]

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].reserve0.quantity')
  [ "$result" = "1000.0000 B" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].reserve1.quantity')
  [ "$result" = "1000.00000000 C" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].liquidity.quantity')
  [ "$result" = "2000.00000000 BC" ]
  result=$(cleos get currency balance lptoken.sx myaccount BC)
  [ "$result" = "2000.00000000 BC" ]
}

