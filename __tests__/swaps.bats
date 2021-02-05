
#!/usr/bin/env bats

load bats.global

@test "sample swaps" {
  a_balance=$(cleos get currency balance eosio.token myaccount A)
  b_balance=$(cleos get currency balance eosio.token myaccount B)
  c_balance=$(cleos get currency balance eosio.token myaccount C)
  [ "$a_balance" = "1000000.0000 A" ]
  [ "$b_balance" = "1000000.0000 B" ]
  [ "$c_balance" = "1000000.000000000 C" ]

  run cleos transfer myaccount curve.sx "100.0000 A" "B"
  [ $status -eq 0 ]
  [[ "$output" =~ "99.9594 B" ]]

  run cleos transfer myaccount curve.sx "10000.0000 A" "B"
  [ $status -eq 0 ]
  [[ "$output" =~ "9989.9337 B" ]]

  run cleos transfer myaccount curve.sx "1000.0000 A" "B"
  echo "status: $output"
  [ $status -eq 0 ]
  [[ "$output" =~ "999.0018 B" ]]
}

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


@test "valid transfers" {
  run cleos transfer myaccount curve.sx "100.0000 A" "B"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "A"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 B" "90.0000 A"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 A" "90.0000 B@eosio.token"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 A" "90.000000000 C@eosio.token"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.00000000 C" "90.0000 A"
  echo "$output"
  [ $status -eq 0 ]

  run cleos transfer myaccount curve.sx "100.0000 C" "A,myaccount2"
  echo "$output"
  [ $status -eq 0 ]
  [[ "$output" =~ "\"to\":\"myaccount2\"" ]]
}

@test "swap with protocol fee" {
  run cleos push action curve.sx setfee '[4, 1, "fee.sx"]' -p curve.sx
  [ $status -eq 0 ]

  fee_balance=$(cleos get currency balance eosio.token fee.sx)
  [ "$fee_balance" = "" ]

  run cleos transfer myaccount curve.sx "100.0000 A" "B"
  [ $status -eq 0 ]
  [[ "$output" =~ "99.8445 B" ]]

  run cleos transfer myaccount curve.sx "100.0000 B" "A"
  [[ "$output" =~ "100.0648 A" ]]
  [ $status -eq 0 ]

  fee_balance=$(cleos get currency balance eosio.token fee.sx A)
  [ "$fee_balance" = "0.0100 A" ]

  fee_balance=$(cleos get currency balance eosio.token fee.sx B)
  [ "$fee_balance" = "0.0100 B" ]

  run cleos push action curve.sx setfee '[4, 0, "fee.sx"]' -p curve.sx
  [ $status -eq 0 ]
}


@test "50 random swaps" {
  symbols="ABC"
  for i in {0..50}
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

}

