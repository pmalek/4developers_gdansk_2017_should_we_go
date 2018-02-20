#!/bin/bash

cleanup() {
  exit 1
}

trap "cleanup" SIGTERM SIGINT

COUNT=20000

if [[ ${1} -lt 2 || ${1} -ge 65000 ]]; then
  echo "usage: test.sh <port>"
  echo "you passed in '$1'"
  exit 1
fi

for i in {4..13}; do
  C=$((1<<$i))
  RET=$( ab -c $C -n $COUNT http://localhost:$1/ 2>&1 \
    | egrep -o "Requests per second:.*" \
    | tr -d '[:space:]' \
    | cut -d':' -f2 \
    | egrep -o "[0-9.]*")
  printf "%d %.0f\n" $C $RET
done
