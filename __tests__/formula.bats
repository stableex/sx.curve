#!/usr/bin/env bats

amplifier=450
reserve1=5862496056
reserve2=6260058778
fee=4

@test "curve formula #1" {
  run cleos push action curve.sx calculate "[10000000, $reserve1, $reserve2, $amplifier, $fee]" -p curve.sx
  echo "Output: $output"
  [ $status -eq 1 ]
  [[ "$output" =~ "9997422" ]]
}

@test "curve formula #2" {
  run cleos push action curve.sx calculate "[10000000, $reserve2, $reserve1, $amplifier, $fee]" -p curve.sx
  echo "Output: $output"
  [ $status -eq 1 ]
  [[ "$output" =~ "9994508" ]]
}

@test "curve formula #3" {
  run cleos push action curve.sx calculate "[10000000000, $reserve1, $reserve2, $amplifier, $fee]" -p curve.sx
  echo "Output: $output"
  [ $status -eq 1 ]
  [[ "$output" =~ "6249264902" ]]
}

@test "curve formula #4" {
  run cleos push action curve.sx calculate "[10000000000, $reserve2, $reserve1, $amplifier, $fee]" -p curve.sx
  echo "Output: $output"
  [ $status -eq 1 ]
  [[ "$output" =~ "5852835188" ]]
}

@test "curve formula #5" {
  run cleos push action curve.sx calculate "[10000000, 1000000000000000000, 1000000000000000000, 5, $fee]" -p curve.sx
  echo "Output: $output"
  [ $status -eq 1 ]
  [[ "$output" =~ "9996000" ]]
}

@test "curve formula #6" {
  run cleos push action curve.sx calculate "[10000000, 4000000000000000000, 4000000000000000000, 2, $fee]" -p curve.sx
  echo "Output: $output"
  [ $status -eq 1 ]
  [[ "$output" =~ "d1 overflow" ]]
}

@test "curve formula #7" {
  run cleos push action curve.sx calculate "[10000000, 9500000000000000000, 9500000000000000000, $amplifier, $fee]" -p curve.sx
  echo "Output: $output"
  [ $status -eq 1 ]
  [[ "$output" =~ "invalid reserves" ]]
}
