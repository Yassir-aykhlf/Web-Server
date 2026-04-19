#!/bin/bash
set -e

# Start server
pkill -9 webserv 2>/dev/null || true
sleep 1
./webserv configs/test.conf > /tmp/server.log 2>&1 &
SERVER_PID=$!
sleep 2

# Test with 100MB body
echo "Testing 100MB POST to CGI..."
timeout 10 bash -c '
{
    printf "POST /directory/youpi.bla HTTP/1.1\r\n"
    printf "Host: 127.0.0.1:1027\r\n"
    printf "Content-Length: 100000000\r\n"
    printf "Connection: close\r\n"
    printf "\r\n"
    head -c 100000000 /dev/zero
} | nc 127.0.0.1 1027 2>&1
' > /tmp/test_response.txt 2>&1 || true

sleep 1
echo "=== Response from server ==="
head -20 /tmp/test_response.txt
echo ""
echo "=== Server logs ==="
tail -50 /tmp/server.log

kill $SERVER_PID 2>/dev/null || true
