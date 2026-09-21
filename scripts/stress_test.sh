#!/bin/bash
# stress_test.sh
# Exercise 3 Q2: fire many simultaneous client connections at the server.
#
# xargs -P runs that many client processes at the same time instead of
# one after another, which is what makes this a concurrency test.
#
# Usage (run from the project root):
#   ./scripts/stress_test.sh                    50 connections using c/client
#   ./scripts/stress_test.sh cpp/client_cpp     50 connections using the C++ client
#   ./scripts/stress_test.sh c/client 100       100 connections

CLIENT=${1:-c/client}
COUNT=${2:-50}

if [ ! -x "$CLIENT" ]; then
    echo "Client binary '$CLIENT' not found. Compile it first (see README)."
    exit 1
fi

echo "Starting $COUNT simultaneous connections using $CLIENT"
START=$(date +%s)
OUTPUT_FILE=$(mktemp)

seq 1 "$COUNT" | xargs -P "$COUNT" -I{} "$CLIENT" "client-{}" > "$OUTPUT_FILE" 2>&1

END=$(date +%s)
ELAPSED=$((END - START))

SUCCESS=$(grep -c "Server response" "$OUTPUT_FILE")
FAILED=$((COUNT - SUCCESS))

echo "Finished in ${ELAPSED} seconds"
echo "Successful responses: $SUCCESS of $COUNT"
echo "Failed or dropped:    $FAILED"
rm -f "$OUTPUT_FILE"
