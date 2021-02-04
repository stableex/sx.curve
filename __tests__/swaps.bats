
#!/usr/bin/env bats

load bats.global

@test "invalid transfers" {
  run cleos transfer myaccount curve.sx "100.0000 A" ""
  echo "$output"
  [[ "$output" =~ "memo should contain target currency" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 A" "BA"
  echo "$output"
  [[ "$output" =~ "no path for exchange" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A@curve.sx"
  echo "$output"
  [[ "$output" =~ "reserve_out vs memo contract mismatch" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 B" "90 A@eosio.token"
  echo "$output"
  [[ "$output" =~ "return vs memo symbol precision mismatch" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 A" "10 B"
  echo "$output"
  [[ "$output" =~ "return vs memo symbol precision mismatch" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 B" "120.0000 A"
  echo "$output"
  [[ "$output" =~ "return is not enough" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 A" "90.000000 C@eosio.token"
  echo "$output"
  [[ "$output" =~ "return vs memo symbol precision mismatch" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A,BadUserName"
  echo "$output"
  [[ "$output" =~ "invalid receiver name in memo" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A, myaccount"
  echo "$output"
  [[ "$output" =~ "invalid receiver name in memo" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A,myaccount,liquidity.sx"
  echo "$output"
  [[ "$output" =~ "invalid memo format" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 A" "AC,curve.sx"
  echo "$output"
  [[ "$output" =~ "memo should contain liquidity symbol code only" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 A" "AC,nonexistuser"
  echo "$output"
  [[ "$output" =~ "receiver account does not exist" ]]
  [ $status -eq 1 ]
  run cleos transfer myaccount curve.sx "100.0000 A" "B" --contract fake.token
  echo "$output"
  [[ "$output" =~ "no matching exchange" ]]
  [ $status -eq 1 ]
}


@test "100 random swaps and withdraw all" {
  symbols="ABC"
  ac_balance=$(cleos get currency balance lptoken.sx liquidity.sx AC)
  ab_balance=$(cleos get currency balance lptoken.sx liquidity.sx AB)
  bc_balance=$(cleos get currency balance lptoken.sx liquidity.sx BC)

  for i in {0..100}
  do
    rnd=$((RANDOM % 10000))
    curr1="${symbols:$(( RANDOM % ${#symbols} )):1}"
    decimals=$((RANDOM % 10000))
    if [[ "$curr1" = "C" ]]
    then
      decimals=$((RANDOM % 10000))8$((RANDOM % 10000))
    fi
    curr2=$curr1
    until [ $curr2 != $curr1 ]
    do
      curr2="${symbols:$(( RANDOM % ${#symbols} )):1}"
    done
    tokens="$rnd.$decimals $curr1"
    echo "swapping $tokens => $curr2"
    run cleos transfer myaccount curve.sx "$tokens" "$curr2"
    [ $status -eq 0 ]
  done

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
