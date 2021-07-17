#!/usr/bin/env bats

load bats.global

@test "deposit AB" {
  run cleos transfer liquidity.sx curve.sx "$((AB_LIQ)).0000 A" "deposit,AB"
  run cleos transfer liquidity.sx curve.sx "$((AB_LIQ)).0000 B" "deposit,AB"

  result=$(cleos get table curve.sx AB orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "$((AB_LIQ)).0000 A" ]
  result=$(cleos get table curve.sx AB orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "$((AB_LIQ)).0000 B" ]

  run cleos push action curve.sx deposit '["liquidity.sx", "AB", null]' -p liquidity.sx

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
  run cleos transfer liquidity.sx curve.sx "$((BC_LIQ+100)).0000 B" "deposit,BC"
  run cleos transfer liquidity.sx curve.sx "$((BC_LIQ)).000000000 C" "deposit,BC"

  result=$(cleos get table curve.sx BC orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "$((BC_LIQ+100)).0000 B" ]
  result=$(cleos get table curve.sx BC orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "$((BC_LIQ)).000000000 C" ]

  result=$(cleos get currency balance eosio.token liquidity.sx B)
  [ "$result" = "$((B_LP_TOTAL-AB_LIQ-BC_LIQ-100)).0000 B" ]

  run cleos push action curve.sx deposit '["liquidity.sx", "BC", null]' -p liquidity.sx

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
  run cleos transfer liquidity.sx curve.sx "$((AC_LIQ)).0000 A" "deposit,AC"
  run cleos transfer liquidity.sx curve.sx "$((AC_LIQ)).000000000 C" "deposit,AC"

  result=$(cleos get table curve.sx AC orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "$((AC_LIQ)).0000 A" ]
  result=$(cleos get table curve.sx AC orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "$((AC_LIQ)).000000000 C" ]

  run cleos push action curve.sx deposit '["liquidity.sx", "AC", null]' -p liquidity.sx
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

  run cleos transfer liquidity.sx curve.sx "1000.0000 A" "deposit,AC"
  [ $status -eq 0 ]

  run cleos transfer liquidity.sx curve.sx "1000.0000 A" "deposit,ABB"
  [[ "$output" =~ "does not exist" ]]
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["liquidity.sx", "AC", null]' -p liquidity.sx
  echo "$output"
  [[ "$output" =~ "one of the deposit is empty" ]]
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["myaccount", "AC", null]' -p myaccount
  echo "$output"
  [[ "$output" =~ "no deposits available for this user" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "deposit,AC"
  echo "$output"
  [[ "$output" =~ "invalid extended symbol" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "1000.0000 A" "deposit,AB" --contract fake.token
  echo "$output"
  [[ "$output" =~ "invalid extended symbol" ]]
  [ $status -eq 1 ]

  run cleos push action curve.sx deposit '["liquidity.sx", "BC", null]' -p liquidity.sx
  echo "$output"
  [[ "$output" =~ "no deposits available for this user" ]]
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

@test "deposit CAB" {
  run cleos transfer liquidity.sx curve.sx "$((CAB_LIQ)).000000000 C" "deposit,CAB"
  run cleos transfer liquidity.sx curve.sx "$((CAB_LIQ)).0000 AB" "deposit,CAB" --contract "lptoken.sx"
  [ $status -eq 0 ]

  result=$(cleos get table curve.sx CAB orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "$((CAB_LIQ)).0000 AB" ]
  result=$(cleos get table curve.sx CAB orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "$((CAB_LIQ)).000000000 C" ]

  run cleos push action curve.sx deposit '["liquidity.sx", "CAB", null]' -p liquidity.sx
  [ $status -eq 0 ]

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[4].reserve0.quantity')
  [ "$result" = "$((CAB_LIQ)).0000 AB" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[4].reserve1.quantity')
  [ "$result" = "$((CAB_LIQ)).000000000 C" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[4].liquidity.quantity')
  [ "$result" = "$((2*CAB_LIQ)).000000000 CAB" ]
  result=$(cleos get currency balance eosio.token liquidity.sx C)
  [ "$result" = "$((C_LP_TOTAL-AC_LIQ/2-BC_LIQ-CAB_LIQ)).000000000 C" ]
  result=$(cleos get currency balance lptoken.sx liquidity.sx CAB)
  [ "$result" = "$((2*CAB_LIQ)).000000000 CAB" ]
}

@test "deposit DE" {
  run cleos transfer liquidity.sx curve.sx "$((DE_LIQ)).000000 D" "deposit,DE"
  run cleos transfer liquidity.sx curve.sx "$((DE_LIQ)).000000 E" "deposit,DE"

  result=$(cleos get table curve.sx DE orders | jq -r '.rows[0].quantity0.quantity')
  [ "$result" = "$((DE_LIQ)).000000 D" ]
  result=$(cleos get table curve.sx DE orders | jq -r '.rows[0].quantity1.quantity')
  [ "$result" = "$((DE_LIQ)).000000 E" ]

  run cleos push action curve.sx deposit '["liquidity.sx", "DE", null]' -p liquidity.sx

  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[3].reserve0.quantity')
  [ "$result" = "$((DE_LIQ)).000000 D" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[3].reserve1.quantity')
  [ "$result" = "$((DE_LIQ)).000000 E" ]
  result=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[3].liquidity.quantity')
  [ "$result" = "$((2*DE_LIQ)).000000 DE" ]
  result=$(cleos get currency balance lptoken.sx liquidity.sx DE)
  [ "$result" = "$((2*DE_LIQ)).000000 DE" ]
}

@test "deposit slippage protection" {
  run cleos transfer liquidity.sx curve.sx "1.0000 A" "deposit,AB"
  run cleos transfer liquidity.sx curve.sx "1.0000 B" "deposit,AB"

  run cleos push action curve.sx deposit '["liquidity.sx", "DE", 9999999999]' -p liquidity.sx
  echo "$output"
  [[ "$output" =~ "deposit amount must exceed" ]]
  [ $status -eq 1 ]
}