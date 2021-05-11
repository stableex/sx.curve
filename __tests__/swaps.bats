
#!/usr/bin/env bats

load bats.global

@test "sample swaps" {
  a_balance=$(cleos get currency balance eosio.token myaccount A)
  b_balance=$(cleos get currency balance eosio.token myaccount B)
  c_balance=$(cleos get currency balance eosio.token myaccount C)
  d_balance=$(cleos get currency balance eosio.token myaccount D)
  e_balance=$(cleos get currency balance eosio.token myaccount E)
  [ "$a_balance" = "1000000.0000 A" ]
  [ "$b_balance" = "1000000.0000 B" ]
  [ "$c_balance" = "1000000.000000000 C" ]
  [ "$d_balance" = "10000000.000000 D" ]
  [ "$e_balance" = "10000000.000000 E" ]

  run cleos transfer myaccount curve.sx "100.0000 A" "swap,0,AB"
  [ $status -eq 0 ]
  [[ "$output" =~ "99.9594 B" ]]

  run cleos transfer myaccount curve.sx "10000.0000 A" "swap,0,AB"
  [ $status -eq 0 ]
  [[ "$output" =~ "9989.9337 B" ]]

  run cleos transfer myaccount curve.sx "1000.0000 A" "swap,0,AB"
  echo "status: $output"
  [ $status -eq 0 ]
  [[ "$output" =~ "998.3396 B" ]]

  run cleos transfer myaccount curve.sx "1000 C" "swap,0,CAB"
  echo "status: $output"
  [ $status -eq 0 ]
  [[ "$output" =~ "999.1241 AB" ]]

  run cleos transfer myaccount curve.sx "900 AB" "swap,0,CAB" --contract "lptoken.sx"
  echo "status: $output"
  [ $status -eq 0 ]
  [[ "$output" =~ "900.110929195 C" ]]

  run cleos transfer myaccount curve.sx "100.0000 A" "swap,0,AB-BC"
  [ $status -eq 0 ]
  [[ "$output" =~ "99.786482760 C" ]]

  run cleos transfer myaccount curve.sx "100.000000 D" "swap,0,DE"
  [ $status -eq 0 ]
  [[ "$output" =~ "99.960000 E" ]]
}

@test "invalid transfers" {
  run cleos transfer myaccount curve.sx "100.0000 A" ""
  echo "$output"
  [[ "$output" =~ "invalid memo" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 A" "swap,0,BA"
  echo "$output"
  [[ "$output" =~ "does not exist" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "swap,900000,AC-BC"
  echo "$output"
  [[ "$output" =~ "contract mismatch" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "swap,0,AB-BC"
  echo "$output"
  [[ "$output" =~ "contract mismatch" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "swap,1200000,AB"
  echo "$output"
  [[ "$output" =~ "invalid minimum return" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "swap,900000,   AB"
  echo "$output"
  [[ "$output" =~ "invalid memo" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "swap,900000,AB,foo"
  echo "$output"
  [[ "$output" =~ "invalid memo" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 A" "foo,0,AC"
  echo "$output"
  [[ "$output" =~ "invalid memo" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.0000 A" "swap,0,AB" --contract fake.token
  echo "$output"
  [[ "$output" =~ "contract mismatch" ]]
  [ $status -eq 1 ]

  run cleos transfer myaccount curve.sx "100.00000000 C" "swap,900000,AC-AC"
  echo "$output"
  [[ "$output" =~ "invalid duplicate" ]]
  [ $status -eq 1 ]
}

@test "valid transfers" {
  run cleos transfer myaccount curve.sx "100.0000 A" "swap,0,AB"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "swap,0,AB"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "swap,900000,AB"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 A" "swap,900000,AB"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 A" "swap,90000000000,AC"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.00000000 C" "swap,900000,AC-AB"
  echo "$output"
  [ $status -eq 0 ]
  [[ "$output" =~ "{\"pair_id\":\"AC\"" ]]
  [[ "$output" =~ "{\"pair_id\":\"AB\"" ]]
}

@test "swap with protocol fee" {
  run cleos push action curve.sx setfee '[4, 1, "fee.sx"]' -p curve.sx
  [ $status -eq 0 ]

  fee_balance=$(cleos get currency balance eosio.token fee.sx)
  [ "$fee_balance" = "" ]

  run cleos transfer myaccount curve.sx "1000.0000 A" "swap,0,AB"
  echo "$output"
  [ $status -eq 0 ]
  [[ "$output" =~ "0.1000 A" ]]
  [[ "$output" =~ "998.0973 B" ]]

  run cleos transfer myaccount curve.sx "1000.0000 B" "swap,0,AB"
  echo "$output"
  [[ "$output" =~ "0.1000 B" ]]
  [[ "$output" =~ "1000.9048 A" ]]
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "1000.0000 C" "swap,0,CAB"
  echo "$output"
  [[ "$output" =~ "0.100000000 C" ]]
  [[ "$output" =~ "998.9296 AB" ]]
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "1000.000000 D" "swap,0,DE"
  [[ "$output" =~ "0.100000 D" ]]
  [[ "$output" =~ "999.500001 E" ]]
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "1000.0000 AB" "swap,0,CAB" --contract lptoken.sx
  [[ "$output" =~ "0.1000 AB" ]]
  [[ "$output" =~ "1000.070290207 C" ]]
  [ $status -eq 0 ]

  fee_balance=$(cleos get currency balance eosio.token fee.sx A)
  [ "$fee_balance" = "0.1000 A" ]

  fee_balance=$(cleos get currency balance eosio.token fee.sx B)
  [ "$fee_balance" = "0.1000 B" ]

  fee_balance=$(cleos get currency balance eosio.token fee.sx C)
  [ "$fee_balance" = "0.100000000 C" ]

  fee_balance=$(cleos get currency balance eosio.token fee.sx D)
  [ "$fee_balance" = "0.100000 D" ]

  fee_balance=$(cleos get currency balance lptoken.sx fee.sx)
  [ "$fee_balance" = "0.1000 AB" ]

  run cleos push action curve.sx setfee '[4, 0, "fee.sx"]' -p curve.sx
  [ $status -eq 0 ]
}


@test "50 random swaps" {
  symbols="ABCDE"
  pairs=("AB" "BC" "AC" "DE")
  for i in {0..50}
  do
    num=$RANDOM
    curr1="${symbols:$(( RANDOM % ${#symbols} )):1}"
    decimals=$((RANDOM % 10000))
    if [[ "$curr1" = "C" ]]
    then
      decimals=$((RANDOM % 10000))8$((RANDOM % 10000))
    fi
    if [[ ("$curr1" = "D") || ("$curr1" = "E") ]]
    then
      decimals=$((RANDOM % 10000))12
    fi
    pair=${pairs[$((RANDOM % ${#pairs[@]}))]}
    until [[ $pair =~ $curr1 ]]
    do
      pair=${pairs[$((RANDOM % ${#pairs[@]}))]}
    done
    tokens="$num.$decimals $curr1"
    echo "swapping $tokens => $pair"
    run cleos transfer myaccount curve.sx "$tokens" "swap,0,$pair"
    [ $status -eq 0 ]
  done
}

