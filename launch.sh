#!/bin/bash

PIDS=()

cleanup() {
    echo "Stopping all players..."
    for pid in "${PIDS[@]}"; do
        kill "$pid" 2>/dev/null
    done
    exit 0
}

trap cleanup SIGINT

BASE_PORT=32001

for i in {1..4}; do
    PLAYER="player$i"
    PORT=$((BASE_PORT + i - 1))

    ./sh13 localhost 32000 localhost "$PORT" "$PLAYER" &
    PIDS+=($!)
done

wait

