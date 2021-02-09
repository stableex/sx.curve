#!/usr/bin/env bats

@test "invalid ramp" {

  run cleos push action curve.sx ramp '["AB", 0, 1]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "target amplifier should be" ]]

  run cleos push action curve.sx ramp '["AB", 2000000, 1]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "target amplifier should be" ]]

  run cleos push action curve.sx ramp '["AB", 100, 0]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "should be above" ]]

  run cleos push action curve.sx ramp '["AD", 100, 0]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "does not exist in" ]]

}
@test "ramp AB amplifier 20->200" {

  amp_start=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].amplifier')
  run cleos push action curve.sx ramp '["AB", 200, 1]' -p curve.sx
  if [[ $output =~ "86400 seconds" ]]; then
      skip "can't run this test in production configuration: MIN_RAMP_TIME==86400 seconds"
  fi
  [ $status -eq 0 ]

  amp=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].amplifier')
  [ "$amp" = "$amp_start" ]
  sleep 5

  run cleos transfer myaccount curve.sx "100.0000 A" "swap,0,AB"
  [ $status -eq 0 ]
  amp=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[0].amplifier')
  [ "$amp" != "$amp_start" ]

}

@test "ramp AC amplifier 200->100" {

  amp_start=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].amplifier')
  run cleos push action curve.sx ramp '["AC", 100, 1]' -p curve.sx
  if [[ $output =~ "86400 seconds" ]]; then
      skip "can't run this test in production configuration: MIN_RAMP_TIME==86400 seconds"
  fi
  [ $status -eq 0 ]

  amp=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].amplifier')
  [ "$amp" = "$amp_start" ]
  sleep 5

  run cleos transfer myaccount curve.sx "100.0000 A" "swap,0,AC"
  [ $status -eq 0 ]
  amp=$(cleos get table curve.sx curve.sx pairs | jq -r '.rows[1].amplifier')
  [ "$amp" != "$amp_start" ]
}

@test "stop ramp" {

  amp=$(cleos get table curve.sx curve.sx ramp | jq -r '.rows[0].target_amplifier')
  if [[ "$amp" != "200" ]]; then
      skip "no ramp set - production configuration?"
  fi

  run cleos push action curve.sx stopramp '["BC"]' -p curve.sx
  [ $status -eq 1 ]
  [[ "$output" =~ "does not exist in" ]]

  run cleos push action curve.sx stopramp '["AB"]' -p curve.sx
  [ $status -eq 0 ]

  amp=$(cleos get table curve.sx curve.sx ramp | jq -r '.rows[0].target_amplifier')
  [ -n "$amp" ]

}
