#!/usr/bin/env bats

@test "deposit AB" {
  run cleos transfer myaccount curve.sx "1000.0000 A" "AB"
  run cleos transfer myaccount curve.sx "1000.0000 B" "AB"

  result=$(cleos get table curve.sx AB orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "1000.0000 A" ]
  result=$(cleos get table curve.sx AB orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "1000.0000 B" ]

  run cleos push action curve.sx deposit '["myaccount", "AB"]' -p myaccount

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
  run cleos transfer myaccount curve.sx "1000.0000000000 C" "BC"

  result=$(cleos get table curve.sx BC orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "1100.0000 B" ]
  result=$(cleos get table curve.sx BC orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "1000.0000000000 C" ]

  result=$(cleos get currency balance eosio.token myaccount B)
  [ "$result" = "7900.0000 B" ]

  run cleos push action curve.sx deposit '["myaccount", "BC"]' -p myaccount

  result=$(cleos get currency balance eosio.token myaccount B)
  [ "$result" = "8000.0000 B" ]

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].reserve0.quantity')
  [ "$result" = "1000.0000 B" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].reserve1.quantity')
  [ "$result" = "1000.0000000000 C" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].liquidity.quantity')
  echo "actual BC liq: $result"
  [ "$result" = "2000.0000000000 BC" ]
  result=$(cleos get currency balance lptoken.sx myaccount BC)
  [ "$result" = "2000.0000000000 BC" ]
}


@test "deposit AC" {
  run cleos transfer myaccount curve.sx "1000.0000 A" "AC"
  run cleos transfer myaccount curve.sx "1000.0000000000 C" "AC"

  result=$(cleos get table curve.sx AC orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "1000.0000 A" ]
  result=$(cleos get table curve.sx AC orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "1000.0000000000 C" ]

  run cleos push action curve.sx deposit '["myaccount", "AC"]' -p myaccount

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve0.quantity')
  [ "$result" = "1000.0000 A" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve1.quantity')
  [ "$result" = "1000.0000000000 C" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].liquidity.quantity')
  [ "$result" = "2000.0000000000 AC" ]
  result=$(cleos get currency balance eosio.token myaccount C)
  [ "$result" = "8000.0000000000 C" ]
  result=$(cleos get currency balance lptoken.sx myaccount AC)
  [ "$result" = "2000.0000000000 AC" ]
}


@test "invalid deposits" {
  a_balance=$(cleos get currency balance eosio.token myaccount A)

  run cleos transfer myaccount curve.sx "1000.0000 A" "AC"

  run cleos transfer myaccount curve.sx "1000.0000 A" "ABB"
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["myaccount", "AC"]' -p myaccount
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["myaccount", "BC"]' -p myaccount
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["curve.sx", "AC"]' -p curve.sx
  [ $status -eq 1 ]

  run cleos push action curve.sx cancel '["myaccount", "AB"]' -p myaccount
  [ $status -eq 1 ]

  run cleos push action curve.sx cancel '["myaccount", "AC"]' -p myaccount

  a_new_balance=$(cleos get currency balance eosio.token myaccount A)
  [ "$a_balance" = "$a_new_balance" ]

}

@test "withdrawal" {
  run cleos transfer myaccount curve.sx "400.0000 AC" "" --contract lptoken.sx

  a_balance=$(cleos get currency balance eosio.token myaccount A)
  [ "$a_balance" = "8200.0000 A" ]
  ac_balance=$(cleos get currency balance lptoken.sx myaccount AC)
  [ "$ac_balance" = "1600.0000000000 AC" ]

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].liquidity.quantity')
  [ "$result" = "1600.0000000000 AC" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve0.quantity')
  [ "$result" = "800.0000 A" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve1.quantity')
  [ "$result" = "800.0000000000 C" ]
}
