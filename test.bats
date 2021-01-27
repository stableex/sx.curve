# #!/usr/bin/env bats

@test "cleos get info" {
  result="$(cleos get info | jq -r .server_version_string)"
  echo $result
  [ "$result" = "v2.0.8" ]
}

@test "config.status = ok" {
  result="$(cleos get table curve.sx curve.sx config | jq -r '.rows[0].status')"
  echo $result
  [ "$result" = "ok" ]
}
