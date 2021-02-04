#!/usr/bin/env bats

load bats.global

@test "deposit AB" {
  run cleos transfer liquidity.sx curve.sx "$((AB_LIQ)).0000 A" "AB"
  run cleos transfer liquidity.sx curve.sx "$((AB_LIQ)).0000 B" "AB"

  result=$(cleos get table curve.sx AB orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "$((AB_LIQ)).0000 A" ]
  result=$(cleos get table curve.sx AB orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "$((AB_LIQ)).0000 B" ]

  run cleos push action curve.sx deposit '["liquidity.sx", "AB"]' -p liquidity.sx

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].reserve0.quantity')
  [ "$result" = "$((AB_LIQ)).0000 A" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].reserve1.quantity')
  [ "$result" = "$((AB_LIQ)).0000 B" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].liquidity.quantity')
  [ "$result" = "$((2*AB_LIQ)).0000 AB" ]
  result=$(cleos get currency balance lptoken.sx liquidity.sx)
  [ "$result" = "$((2*AB_LIQ)).0000 AB" ]
}


@test "deposit with excess BC" {
  run cleos transfer liquidity.sx curve.sx "$((BC_LIQ+100)).0000 B" "BC"
  run cleos transfer liquidity.sx curve.sx "$((BC_LIQ)).000000000 C" "BC"

  result=$(cleos get table curve.sx BC orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "$((BC_LIQ+100)).0000 B" ]
  result=$(cleos get table curve.sx BC orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "$((BC_LIQ)).000000000 C" ]

  result=$(cleos get currency balance eosio.token liquidity.sx B)
  [ "$result" = "$((B_LP_TOTAL-AB_LIQ-BC_LIQ-100)).0000 B" ]

  run cleos push action curve.sx deposit '["liquidity.sx", "BC"]' -p liquidity.sx

  result=$(cleos get currency balance eosio.token liquidity.sx B)
  [ "$result" = "$((B_LP_TOTAL-AB_LIQ-BC_LIQ)).0000 B" ]

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].reserve0.quantity')
  [ "$result" = "$((BC_LIQ)).0000 B" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].reserve1.quantity')
  [ "$result" = "$((BC_LIQ)).000000000 C" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[2].liquidity.quantity')
  echo "actual BC liq: $result"
  [ "$result" = "$((2*BC_LIQ)).000000000 BC" ]
  result=$(cleos get currency balance lptoken.sx liquidity.sx BC)
  [ "$result" = "$((2*BC_LIQ)).000000000 BC" ]
}


@test "deposit AC" {
  run cleos transfer liquidity.sx curve.sx "$((AC_LIQ)).0000 A" "AC"
  run cleos transfer liquidity.sx curve.sx "$((AC_LIQ)).000000000 C" "AC"

  result=$(cleos get table curve.sx AC orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "$((AC_LIQ)).0000 A" ]
  result=$(cleos get table curve.sx AC orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "$((AC_LIQ)).000000000 C" ]

  run cleos push action curve.sx deposit '["liquidity.sx", "AC"]' -p liquidity.sx
  [ $status -eq 0 ]

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve0.quantity')
  [ "$result" = "$((AC_LIQ)).0000 A" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve1.quantity')
  [ "$result" = "$((AC_LIQ)).000000000 C" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].liquidity.quantity')
  [ "$result" = "$((2*AC_LIQ)).000000000 AC" ]
  result=$(cleos get currency balance eosio.token liquidity.sx C)
  [ "$result" = "$((C_LP_TOTAL-AC_LIQ-BC_LIQ)).000000000 C" ]
  result=$(cleos get currency balance lptoken.sx liquidity.sx AC)
  [ "$result" = "$((2*AC_LIQ)).000000000 AC" ]
}


@test "invalid deposits" {
  a_balance=$(cleos get currency balance eosio.token liquidity.sx A)

  run cleos transfer liquidity.sx curve.sx "1000.0000 A" "AC"
  [ $status -eq 0 ]

  run cleos transfer liquidity.sx curve.sx "1000.0000 A" "ABB"
  [[ "$output" =~ "no path for exchange" ]]
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["liquidity.sx", "AC"]' -p liquidity.sx
  echo "$output"
  [[ "$output" =~ "one of the currencies not provided" ]]
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["myaccount", "AC"]' -p myaccount
  echo "$output"
  [[ "$output" =~ "no deposits for this user" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "AC"
  echo "$output"
  [[ "$output" =~ "invalid extended symbol when adding liquidity" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "1000.0000 A" "AB" --contract fake.token
  echo "$output"
  [[ "$output" =~ "invalid extended symbol when adding liquidity" ]]
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["liquidity.sx", "BC"]' -p liquidity.sx
  echo "$output"
  [[ "$output" =~ "no deposits for this user" ]]
  [ $status -eq 1 ]

  run cleos push action curve.sx cancel '["liquidity.sx", "AB"]' -p liquidity.sx
  echo "$output"
  [[ "$output" =~ "no deposits for this user in this pool" ]]
  [ $status -eq 1 ]

  run cleos push action curve.sx cancel '["liquidity.sx", "AC"]' -p liquidity.sx
  [ $status -eq 0 ]

  a_new_balance=$(cleos get currency balance eosio.token liquidity.sx A)
  [ "$a_balance" = "$a_new_balance" ]

}

@test "withdraw half AC" {
  run cleos transfer liquidity.sx curve.sx "$((AC_LIQ)).0000 AC" "" --contract lptoken.sx

  a_balance=$(cleos get currency balance eosio.token liquidity.sx A)
  [ "$a_balance" = "$((A_LP_TOTAL-AB_LIQ-AC_LIQ/2)).0000 A" ]
  ac_balance=$(cleos get currency balance lptoken.sx liquidity.sx AC)
  [ "$ac_balance" = "$((AC_LIQ)).000000000 AC" ]

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].liquidity.quantity')
  [ "$result" = "$((AC_LIQ)).000000000 AC" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve0.quantity')
  [ "$result" = "$((AC_LIQ/2)).0000 A" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].reserve1.quantity')
  [ "$result" = "$((AC_LIQ/2)).000000000 C" ]
}
