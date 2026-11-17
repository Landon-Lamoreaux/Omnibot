#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
export ROOT_DIR=$SCRIPT_DIR
cd "$ROOT_DIR" || exit

export DISPLAY=:0

if [ "$SIMULATION" = true ]; then
  xhost + > /dev/null
else
  xhost +local:root > /dev/null
fi

docker compose -p nav down
docker compose \
    -p nav \
    -f compose/nav.yaml \
    -f compose/nav.sim.yaml \
    "${ENV_FLAGS[@]}" \
    up --build --attach navigation --exit-code-from navigation
EXIT_CODE=$?

if [ "$SIMULATION" = true ]; then
  xhost - > /dev/null
else
  xhost -local:root > /dev/null
fi

docker compose -p nav down --volumes
exit $EXIT_CODE