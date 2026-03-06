#!/bin/bash
# Test if CGI is non-blocking by sending:
# 1) A slow CGI request (infinite.py — should timeout after ~30s)
# 2) A fast static file request at the same time
# If CGI is non-blocking, the fast request should return immediately.
# If CGI is blocking, the fast request will be delayed until the slow CGI finishes/times out.

echo "=== CGI Non-Blocking Test ==="
echo ""

# Test 1: First, verify the normal CGI works
echo "[Test 1] Quick CGI sanity check (hello.py)..."
START=$(date +%s%N)
RESULT=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:8080/cgi-bin/hello.py)
END=$(date +%s%N)
ELAPSED=$(( (END - START) / 1000000 ))
echo "  Status: $RESULT, Time: ${ELAPSED}ms"
echo ""

# Test 2: Send slow CGI request (infinite.py) in background, then immediately request a static file
echo "[Test 2] Non-blocking test: slow CGI + fast static request in parallel"
echo "  Sending request to infinite.py (will timeout)..."

# Start slow CGI request in background
curl -s -o /dev/null -w "  Slow CGI status: %{http_code}\n" --max-time 35 http://localhost:8080/cgi-bin/infinite.py &
SLOW_PID=$!

# Give the slow request a moment to reach the server
sleep 0.3

echo "  Sending fast static request while slow CGI is running..."
START=$(date +%s%N)
FAST_RESULT=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:8080/)
END=$(date +%s%N)
FAST_ELAPSED=$(( (END - START) / 1000000 ))
echo "  Fast request status: $FAST_RESULT, Time: ${FAST_ELAPSED}ms"

if [ "$FAST_ELAPSED" -lt 2000 ]; then
    echo "  ✅ PASS: Fast request completed quickly (${FAST_ELAPSED}ms) — CGI appears NON-BLOCKING"
else
    echo "  ❌ FAIL: Fast request took ${FAST_ELAPSED}ms — CGI appears BLOCKING"
fi

echo ""
echo "  Waiting for slow CGI to finish/timeout..."
wait $SLOW_PID 2>/dev/null
echo ""

# Test 3: Multiple concurrent fast requests while slow CGI is running
echo "[Test 3] Multiple parallel fast requests during slow CGI"
curl -s -o /dev/null -w "  Slow CGI status: %{http_code}\n" --max-time 35 http://localhost:8080/cgi-bin/infinite.py &
SLOW_PID=$!
sleep 0.3

ALL_FAST=true
for i in 1 2 3 4 5; do
    START=$(date +%s%N)
    RESULT=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 http://localhost:8080/)
    END=$(date +%s%N)
    ELAPSED=$(( (END - START) / 1000000 ))
    echo "  Request $i: status=$RESULT, time=${ELAPSED}ms"
    if [ "$ELAPSED" -gt 2000 ]; then
        ALL_FAST=false
    fi
done

if $ALL_FAST; then
    echo "  ✅ PASS: All fast requests completed quickly"
else
    echo "  ❌ FAIL: Some fast requests were delayed"
fi

echo ""
echo "  Waiting for slow CGI to finish/timeout..."
wait $SLOW_PID 2>/dev/null

echo ""
echo "=== Tests Complete ==="
