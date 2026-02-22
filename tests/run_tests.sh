#!/bin/bash
# =============================================================================
# WebServ Test Suite
# Tests based on the 42 School WebServ evaluation criteria
# =============================================================================

set -o pipefail

# ---- Configuration ----
WEBSERV_BIN="./webserv"
CONFIG_FILE="Configs/default.conf"
HOST="localhost"
PORT1=8080
PORT2=8081
BASE_URL="http://${HOST}:${PORT1}"
BASE_URL2="http://${HOST}:${PORT2}"
SERVER_PID=""
PASS=0
FAIL=0
SKIP=0
TOTAL=0

# ---- Colors ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# ---- Helpers ----
log_header() {
    echo ""
    echo -e "${BOLD}${BLUE}=====================================================================${NC}"
    echo -e "${BOLD}${BLUE}  $1${NC}"
    echo -e "${BOLD}${BLUE}=====================================================================${NC}"
}

log_subheader() {
    echo ""
    echo -e "${BOLD}--- $1 ---${NC}"
}

assert_status() {
    local test_name="$1"
    local expected="$2"
    local actual="$3"

    TOTAL=$((TOTAL + 1))
    if [ "$actual" = "$expected" ]; then
        echo -e "  ${GREEN}✓ PASS${NC}: $test_name (HTTP $actual)"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗ FAIL${NC}: $test_name (expected HTTP $expected, got HTTP $actual)"
        FAIL=$((FAIL + 1))
    fi
}

assert_body_contains() {
    local test_name="$1"
    local expected_text="$2"
    local body="$3"

    TOTAL=$((TOTAL + 1))
    if echo "$body" | grep -q "$expected_text"; then
        echo -e "  ${GREEN}✓ PASS${NC}: $test_name (body contains '$expected_text')"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗ FAIL${NC}: $test_name (body does not contain '$expected_text')"
        FAIL=$((FAIL + 1))
    fi
}

assert_header_contains() {
    local test_name="$1"
    local expected_header="$2"
    local headers="$3"

    TOTAL=$((TOTAL + 1))
    if echo "$headers" | grep -qi "$expected_header"; then
        echo -e "  ${GREEN}✓ PASS${NC}: $test_name (header contains '$expected_header')"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗ FAIL${NC}: $test_name (header does not contain '$expected_header')"
        FAIL=$((FAIL + 1))
    fi
}

assert_not_crash() {
    local test_name="$1"

    TOTAL=$((TOTAL + 1))
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        echo -e "  ${GREEN}✓ PASS${NC}: $test_name (server still running)"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}✗ FAIL${NC}: $test_name (server crashed!)"
        FAIL=$((FAIL + 1))
    fi
}

# ---- Server Management ----
start_server() {
    local config="$1"
    if [ -z "$config" ]; then
        config="$CONFIG_FILE"
    fi

    # Make sure no old server is running
    stop_server

    "$WEBSERV_BIN" "$config" &>/dev/null &
    SERVER_PID=$!
    sleep 2

    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo -e "${RED}ERROR: Server failed to start with config: $config${NC}"
        return 1
    fi
    return 0
}

stop_server() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
    fi
    SERVER_PID=""
}

cleanup() {
    stop_server
    # Clean up test files
    rm -f /tmp/webserv_test_upload.txt
    rm -f /tmp/webserv_test_large.txt
    rm -f /tmp/webserv_test_download.txt
    rm -f /tmp/webserv_test_headers.txt
    rm -f /tmp/webserv_test_body.txt
}

trap cleanup EXIT

# =============================================================================
# TESTS BEGIN HERE
# =============================================================================

log_header "WEBSERV TEST SUITE"
echo "  Binary:  $WEBSERV_BIN"
echo "  Config:  $CONFIG_FILE"
echo "  Server1: $BASE_URL"
echo "  Server2: $BASE_URL2"

# =========================================================================
# 1. COMPILATION TEST
# =========================================================================
log_header "1. COMPILATION"

TOTAL=$((TOTAL + 1))
if [ -x "$WEBSERV_BIN" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Binary exists and is executable"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Binary does not exist or is not executable"
    FAIL=$((FAIL + 1))
    echo "Run 'make' first to compile the project."
    exit 1
fi

# =========================================================================
# 2. SERVER STARTUP
# =========================================================================
log_header "2. SERVER STARTUP"

start_server "$CONFIG_FILE"
TOTAL=$((TOTAL + 1))
if kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server started successfully (PID: $SERVER_PID)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server failed to start"
    FAIL=$((FAIL + 1))
    echo "Cannot continue without a running server."
    exit 1
fi

# Verify both ports are listening
TOTAL=$((TOTAL + 1))
if curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}/" | grep -q "200"; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server responding on port $PORT1"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server not responding on port $PORT1"
    FAIL=$((FAIL + 1))
fi

TOTAL=$((TOTAL + 1))
if curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL2}/" | grep -q "200"; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server responding on port $PORT2"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server not responding on port $PORT2"
    FAIL=$((FAIL + 1))
fi

# =========================================================================
# 3. BASIC GET REQUESTS
# =========================================================================
log_header "3. BASIC GET REQUESTS"

log_subheader "3.1 Static file serving"

# GET / should return 200 with index.html
STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 5 "${BASE_URL}/")
assert_status "GET / returns 200" "200" "$STATUS"

BODY=$(cat /tmp/webserv_test_body.txt)
assert_body_contains "GET / serves index.html content" "Welcome to WebServ" "$BODY"

# GET / on second server
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL2}/")
assert_status "GET / on port $PORT2 returns 200" "200" "$STATUS"

log_subheader "3.2 Non-existent resources"

# GET /nonexistent should return 404
STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 5 "${BASE_URL}/nonexistent")
assert_status "GET /nonexistent returns 404" "404" "$STATUS"

BODY=$(cat /tmp/webserv_test_body.txt)
assert_body_contains "404 response has error page content" "404" "$BODY"

# GET /very/deep/nonexistent/path should return 404
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}/very/deep/nonexistent/path")
assert_status "GET deep nonexistent path returns 404" "404" "$STATUS"

log_subheader "3.3 Response headers"

HEADERS=$(curl -s -D - -o /dev/null --max-time 5 "${BASE_URL}/")
assert_header_contains "Response has Server header" "Server:" "$HEADERS"
assert_header_contains "Response has Content-Type header" "Content-Type:" "$HEADERS"
assert_header_contains "Response has Content-Length header" "Content-Length:" "$HEADERS"
assert_header_contains "Response has Date header" "Date:" "$HEADERS"

# =========================================================================
# 4. HTTP METHODS
# =========================================================================
log_header "4. HTTP METHODS"

log_subheader "4.1 Method restrictions"

# POST on port 8081 should be 405 (only GET allowed)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X POST "${BASE_URL2}/")
assert_status "POST on GET-only server returns 405" "405" "$STATUS"

# DELETE on root (not allowed - only GET POST)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X DELETE "${BASE_URL}/")
assert_status "DELETE on root (GET/POST only) returns 405" "405" "$STATUS"

# DELETE on port 8081 should be 405
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X DELETE "${BASE_URL2}/")
assert_status "DELETE on GET-only server returns 405" "405" "$STATUS"

log_subheader "4.2 Unknown/unsupported methods"

# PATCH should not crash the server
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X PATCH "${BASE_URL}/")
assert_not_crash "Server survives PATCH request"
# It should return an error (405 or 501)
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "405" ] || [ "$STATUS" = "501" ] || [ "$STATUS" = "400" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: PATCH returns error status ($STATUS)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: PATCH returned unexpected status ($STATUS)"
    FAIL=$((FAIL + 1))
fi

# OPTIONS method
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X OPTIONS "${BASE_URL}/")
assert_not_crash "Server survives OPTIONS request"

# PUT method
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X PUT "${BASE_URL}/")
assert_not_crash "Server survives PUT request"

# HEAD method
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -I "${BASE_URL}/")
assert_not_crash "Server survives HEAD request"

# =========================================================================
# 5. ERROR PAGES
# =========================================================================
log_header "5. CUSTOM ERROR PAGES"

# 404 error page
STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 5 "${BASE_URL}/nonexistent")
assert_status "Custom 404 returns 404" "404" "$STATUS"

BODY=$(cat /tmp/webserv_test_body.txt)
assert_body_contains "Custom 404 page has HTML content" "html" "$BODY"

# 405 error page
STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 5 -X DELETE "${BASE_URL}/")
assert_status "Custom 405 returns 405" "405" "$STATUS"

BODY=$(cat /tmp/webserv_test_body.txt)
assert_body_contains "Custom 405 page has HTML content" "html" "$BODY"

# =========================================================================
# 6. FILE UPLOAD AND DOWNLOAD (POST / GET / DELETE cycle)
# =========================================================================
log_header "6. FILE UPLOAD / DOWNLOAD / DELETE"

log_subheader "6.1 Upload a file via POST"

echo "Hello WebServ Upload Test $(date)" > /tmp/webserv_test_upload.txt

# POST to /upload/testfile.txt
STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 5 \
    -X POST \
    -H "Content-Type: application/octet-stream" \
    --data-binary @/tmp/webserv_test_upload.txt \
    "${BASE_URL}/upload/testfile.txt")
assert_status "POST /upload/testfile.txt returns 201" "201" "$STATUS"

log_subheader "6.2 Download the uploaded file via GET"

# Check if the file was created
TOTAL=$((TOTAL + 1))
if [ -f "./www/uploads/testfile.txt" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Uploaded file exists on disk"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Uploaded file not found on disk"
    FAIL=$((FAIL + 1))
fi

log_subheader "6.3 Delete the uploaded file via DELETE"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 \
    -X DELETE \
    "${BASE_URL}/upload/testfile.txt")
assert_status "DELETE /upload/testfile.txt returns 200" "200" "$STATUS"

# Verify file is gone
TOTAL=$((TOTAL + 1))
if [ ! -f "./www/uploads/testfile.txt" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Deleted file no longer exists"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: File still exists after DELETE"
    FAIL=$((FAIL + 1))
fi

log_subheader "6.4 Delete non-existent file"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 \
    -X DELETE \
    "${BASE_URL}/upload/nonexistent_file.txt")
assert_status "DELETE non-existent file returns 404" "404" "$STATUS"

# =========================================================================
# 7. CLIENT BODY SIZE LIMIT
# =========================================================================
log_header "7. CLIENT BODY SIZE LIMIT"

# The default config has client_max_body_size 10m for server 1, 5m for server 2
# Test with a body that exceeds the limit

# Generate a body larger than 5MB for port 8081
log_subheader "7.1 Body within limit"

SMALL_BODY="This is a small body that should be accepted"
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 \
    -X POST \
    -H "Content-Type: text/plain" \
    --data "$SMALL_BODY" \
    "${BASE_URL}/")
# POST to / is allowed but has no upload_store, should return 403
TOTAL=$((TOTAL + 1))
if [ "$STATUS" != "413" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Small body not rejected as too large (HTTP $STATUS)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Small body incorrectly rejected as too large (HTTP $STATUS)"
    FAIL=$((FAIL + 1))
fi

log_subheader "7.2 Body exceeding limit"

# Create a body larger than the 5m limit to test 413 (using a file to avoid arg limits)
LARGE_BODY_SIZE_BYTES=6000000  # 6MB, exceeds the 5MB limit on PORT2
python3 -c "print('x' * $LARGE_BODY_SIZE_BYTES)" > /tmp/webserv_test_large.txt
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 \
    -X POST \
    -H "Content-Type: text/plain" \
    --data-binary @/tmp/webserv_test_large.txt \
    "${BASE_URL2}/")
assert_status "Body exceeding 5m limit returns 413" "413" "$STATUS"

# =========================================================================
# 8. REDIRECT
# =========================================================================
log_header "8. REDIRECTS"

# /redirect should return 301
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}/redirect")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "301" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: GET /redirect returns 301"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: GET /redirect expected 301, got $STATUS"
    FAIL=$((FAIL + 1))
fi

# Check redirect Location header
HEADERS=$(curl -s -D - -o /dev/null --max-time 5 "${BASE_URL}/redirect")
TOTAL=$((TOTAL + 1))
if echo "$HEADERS" | grep -qi "Location:"; then
    echo -e "  ${GREEN}✓ PASS${NC}: Redirect response has Location header"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Redirect response missing Location header"
    FAIL=$((FAIL + 1))
fi

# Follow redirect
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -L "${BASE_URL}/redirect")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "200" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Following redirect results in 200"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Following redirect expected 200, got $STATUS"
    FAIL=$((FAIL + 1))
fi

# =========================================================================
# 9. AUTOINDEX (Directory listing)
# =========================================================================
log_header "9. AUTOINDEX (Directory Listing)"

STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 5 "${BASE_URL}/autoindex/")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "200" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: GET /autoindex/ returns 200"
    PASS=$((PASS + 1))
    BODY=$(cat /tmp/webserv_test_body.txt)
    assert_body_contains "Autoindex lists readme.txt" "readme.txt" "$BODY"
    assert_body_contains "Autoindex lists test.txt" "test.txt" "$BODY"
    assert_body_contains "Autoindex has 'Index of' heading" "Index of" "$BODY"
else
    echo -e "  ${RED}✗ FAIL${NC}: GET /autoindex/ expected 200, got $STATUS"
    FAIL=$((FAIL + 1))
fi

# =========================================================================
# 10. CGI
# =========================================================================
log_header "10. CGI"

log_subheader "10.1 CGI GET request"

STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 10 "${BASE_URL}/cgi-bin/hello.py")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "200" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: CGI GET returns 200"
    PASS=$((PASS + 1))
    BODY=$(cat /tmp/webserv_test_body.txt)
    assert_body_contains "CGI output contains 'Hello from CGI'" "Hello from CGI" "$BODY"
    assert_body_contains "CGI output contains REQUEST_METHOD" "Request Method" "$BODY"
else
    echo -e "  ${RED}✗ FAIL${NC}: CGI GET expected 200, got $STATUS"
    FAIL=$((FAIL + 1))
fi

log_subheader "10.2 CGI GET with query string"

STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 10 "${BASE_URL}/cgi-bin/hello.py?name=test&value=42")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "200" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: CGI GET with query string returns 200"
    PASS=$((PASS + 1))
    BODY=$(cat /tmp/webserv_test_body.txt)
    assert_body_contains "CGI output contains query string" "name=test" "$BODY"
else
    echo -e "  ${RED}✗ FAIL${NC}: CGI GET with query string expected 200, got $STATUS"
    FAIL=$((FAIL + 1))
fi

log_subheader "10.3 CGI POST request"

STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 10 \
    -X POST \
    -H "Content-Type: text/plain" \
    --data "test body data" \
    "${BASE_URL}/cgi-bin/hello.py")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "200" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: CGI POST returns 200"
    PASS=$((PASS + 1))
    BODY=$(cat /tmp/webserv_test_body.txt)
    assert_body_contains "CGI POST output contains 'POST'" "POST" "$BODY"
else
    echo -e "  ${RED}✗ FAIL${NC}: CGI POST expected 200, got $STATUS"
    FAIL=$((FAIL + 1))
fi

log_subheader "10.4 CGI non-existent script"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 "${BASE_URL}/cgi-bin/nonexistent.py")
assert_status "Non-existent CGI script returns 404" "404" "$STATUS"

log_subheader "10.5 CGI error script"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 10 "${BASE_URL}/cgi-bin/error.py")
assert_not_crash "Server survives CGI error script"
TOTAL=$((TOTAL + 1))
if [ "$STATUS" != "000" ] && [ -n "$STATUS" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server returns a response for CGI error (HTTP $STATUS)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server did not respond to CGI error request"
    FAIL=$((FAIL + 1))
fi

log_subheader "10.6 CGI POST with echo script"

STATUS=$(curl -s -o /tmp/webserv_test_body.txt -w "%{http_code}" --max-time 10 \
    -X POST \
    -H "Content-Type: text/plain" \
    --data "echo test data" \
    "${BASE_URL}/cgi-bin/echo.py")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" = "200" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: CGI echo POST returns 200"
    PASS=$((PASS + 1))
    BODY=$(cat /tmp/webserv_test_body.txt)
    assert_body_contains "CGI echo contains POST method" "POST" "$BODY"
    assert_body_contains "CGI echo contains body data" "echo test data" "$BODY"
else
    echo -e "  ${RED}✗ FAIL${NC}: CGI echo POST expected 200, got $STATUS"
    FAIL=$((FAIL + 1))
fi

# =========================================================================
# 11. MULTIPLE SERVERS / PORTS
# =========================================================================
log_header "11. MULTIPLE SERVERS / PORTS"

log_subheader "11.1 Different content on different ports"

BODY1=$(curl -s --max-time 5 "${BASE_URL}/")
BODY2=$(curl -s --max-time 5 "${BASE_URL2}/")

# Both should serve content
TOTAL=$((TOTAL + 1))
if [ -n "$BODY1" ] && [ -n "$BODY2" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Both servers serve content"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: One or both servers returned empty content"
    FAIL=$((FAIL + 1))
fi

log_subheader "11.2 Different method restrictions per server"

# Port 8080 allows POST, port 8081 does not
STATUS1=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X POST -d "test" "${BASE_URL}/")
STATUS2=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 -X POST -d "test" "${BASE_URL2}/")

TOTAL=$((TOTAL + 1))
if [ "$STATUS2" = "405" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Port $PORT2 correctly rejects POST (405)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Port $PORT2 should reject POST with 405, got $STATUS2"
    FAIL=$((FAIL + 1))
fi

TOTAL=$((TOTAL + 1))
if [ "$STATUS1" != "405" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Port $PORT1 accepts POST (HTTP $STATUS1)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Port $PORT1 should accept POST but got 405"
    FAIL=$((FAIL + 1))
fi

# =========================================================================
# 12. ROBUSTNESS / EDGE CASES
# =========================================================================
log_header "12. ROBUSTNESS AND EDGE CASES"

log_subheader "12.1 Malformed requests"

# Empty request
TOTAL=$((TOTAL + 1))
RESULT=$(echo "" | nc -w 2 127.0.0.1 "$PORT1" 2>/dev/null)
if kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server survives empty request"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server crashed on empty request"
    FAIL=$((FAIL + 1))
fi

# Incomplete HTTP request
TOTAL=$((TOTAL + 1))
echo -ne "GET / HTTP/1.1\r\n" | nc -w 2 127.0.0.1 "$PORT1" >/dev/null 2>&1
if kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server survives incomplete request"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server crashed on incomplete request"
    FAIL=$((FAIL + 1))
fi

# Request with invalid HTTP version
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 --http1.0 "${BASE_URL}/")
assert_not_crash "Server survives HTTP/1.0 request"

log_subheader "12.2 Long URL"

LONG_PATH=$(python3 -c "print('/a' * 500)")
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}${LONG_PATH}")
assert_not_crash "Server survives very long URL"
TOTAL=$((TOTAL + 1))
if [ -n "$STATUS" ] && [ "$STATUS" != "000" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server responds to long URL (HTTP $STATUS)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server did not respond to long URL"
    FAIL=$((FAIL + 1))
fi

log_subheader "12.3 Many headers"

EXTRA_HEADERS=""
for i in $(seq 1 50); do
    EXTRA_HEADERS="$EXTRA_HEADERS -H \"X-Custom-Header-$i: value$i\""
done
STATUS=$(eval curl -s -o /dev/null -w "%{http_code}" --max-time 5 $EXTRA_HEADERS "${BASE_URL}/")
assert_not_crash "Server survives request with many headers"

log_subheader "12.4 Concurrent connections"

# Send 10 concurrent requests
CURL_PIDS=""
for i in $(seq 1 10); do
    curl -s -o /dev/null --max-time 5 "${BASE_URL}/" &
    CURL_PIDS="$CURL_PIDS $!"
done
# Wait for all curls to finish
for cpid in $CURL_PIDS; do
    wait "$cpid" 2>/dev/null
done
assert_not_crash "Server survives 10 concurrent connections"

log_subheader "12.5 Path traversal prevention"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}/../../../etc/passwd")
TOTAL=$((TOTAL + 1))
if [ "$STATUS" != "200" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Path traversal blocked (HTTP $STATUS)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Path traversal not blocked - security issue!"
    FAIL=$((FAIL + 1))
fi

# =========================================================================
# 13. KEEP-ALIVE / CONNECTION HANDLING
# =========================================================================
log_header "13. CONNECTION HANDLING"

# Test keep-alive
HEADERS=$(curl -s -D - -o /dev/null --max-time 5 -H "Connection: keep-alive" "${BASE_URL}/")
assert_header_contains "Keep-alive response has Connection header" "Connection:" "$HEADERS"

# Test Connection: close
HEADERS=$(curl -s -D - -o /dev/null --max-time 5 -H "Connection: close" "${BASE_URL}/")
assert_header_contains "Close response has Connection header" "Connection:" "$HEADERS"

# Multiple requests on same connection (curl handles this with keep-alive)
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}/" "${BASE_URL}/")
assert_not_crash "Server handles multiple requests on one connection"

# =========================================================================
# 14. CONTENT-TYPE HANDLING
# =========================================================================
log_header "14. CONTENT-TYPE HANDLING"

# HTML content type
HEADERS=$(curl -s -D - -o /dev/null --max-time 5 "${BASE_URL}/")
assert_header_contains "HTML file has text/html content type" "text/html" "$HEADERS"

# CSS content type
HEADERS=$(curl -s -D - -o /dev/null --max-time 5 "${BASE_URL}/style.css")
TOTAL=$((TOTAL + 1))
if echo "$HEADERS" | grep -qi "text/css"; then
    echo -e "  ${GREEN}✓ PASS${NC}: CSS file has text/css content type"
    PASS=$((PASS + 1))
elif echo "$HEADERS" | grep -qi "200"; then
    echo -e "  ${YELLOW}⚠ SKIP${NC}: CSS file served but Content-Type may vary"
    SKIP=$((SKIP + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: CSS file not served correctly"
    FAIL=$((FAIL + 1))
fi

# =========================================================================
# 15. CHUNKED TRANSFER ENCODING
# =========================================================================
log_header "15. CHUNKED TRANSFER ENCODING"

# Send chunked request body
STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 \
    -X POST \
    -H "Transfer-Encoding: chunked" \
    --data-binary "test chunk data" \
    "${BASE_URL}/upload/chunked_test.txt")
assert_not_crash "Server handles chunked transfer encoding"

# Clean up if file was created
rm -f ./www/uploads/chunked_test.txt

# =========================================================================
# 16. CONFIGURATION VALIDATION
# =========================================================================
log_header "16. CONFIGURATION VALIDATION"

log_subheader "16.1 Invalid configuration"

stop_server

# Test with empty config
echo "" > /tmp/webserv_empty.conf
TOTAL=$((TOTAL + 1))
"$WEBSERV_BIN" /tmp/webserv_empty.conf &>/tmp/webserv_test_err.txt &
EMPTY_PID=$!
sleep 2
if kill -0 "$EMPTY_PID" 2>/dev/null; then
    # Server started with empty config - might be OK
    echo -e "  ${YELLOW}⚠ SKIP${NC}: Server started with empty config (implementation-dependent)"
    kill "$EMPTY_PID" 2>/dev/null
    wait "$EMPTY_PID" 2>/dev/null
    SKIP=$((SKIP + 1))
else
    echo -e "  ${GREEN}✓ PASS${NC}: Server rejected empty config"
    PASS=$((PASS + 1))
fi

# Test with non-existent config file
TOTAL=$((TOTAL + 1))
"$WEBSERV_BIN" /tmp/nonexistent_config.conf &>/tmp/webserv_test_err.txt &
NOFILE_PID=$!
sleep 2
if kill -0 "$NOFILE_PID" 2>/dev/null; then
    echo -e "  ${RED}✗ FAIL${NC}: Server started with non-existent config"
    kill "$NOFILE_PID" 2>/dev/null
    wait "$NOFILE_PID" 2>/dev/null
    FAIL=$((FAIL + 1))
else
    echo -e "  ${GREEN}✓ PASS${NC}: Server rejected non-existent config file"
    PASS=$((PASS + 1))
fi

# Restart server with default config for remaining tests
start_server "$CONFIG_FILE"

# =========================================================================
# 17. STRESS TEST (lightweight)
# =========================================================================
log_header "17. STRESS TEST (Lightweight)"

log_subheader "17.1 Rapid sequential requests"

SUCCESS_COUNT=0
FAIL_COUNT=0
for i in $(seq 1 50); do
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}/")
    if [ "$STATUS" = "200" ]; then
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
done

TOTAL=$((TOTAL + 1))
# Allow up to 4% failure rate (48/50 = 96% success threshold)
MIN_SEQUENTIAL_SUCCESS=48
if [ "$SUCCESS_COUNT" -ge $MIN_SEQUENTIAL_SUCCESS ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: $SUCCESS_COUNT/50 rapid requests succeeded (${FAIL_COUNT} failed)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Only $SUCCESS_COUNT/50 rapid requests succeeded"
    FAIL=$((FAIL + 1))
fi

assert_not_crash "Server survives rapid sequential requests"

log_subheader "17.2 Concurrent burst"

SUCCESS_COUNT=0
TMPDIR=$(mktemp -d)
CURL_PIDS=""
for i in $(seq 1 20); do
    curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}/" > "${TMPDIR}/result_${i}" &
    CURL_PIDS="$CURL_PIDS $!"
done
for cpid in $CURL_PIDS; do
    wait "$cpid" 2>/dev/null
done

for i in $(seq 1 20); do
    STATUS=$(cat "${TMPDIR}/result_${i}" 2>/dev/null)
    if [ "$STATUS" = "200" ]; then
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    fi
done
rm -rf "$TMPDIR"

TOTAL=$((TOTAL + 1))
# Allow up to 10% failure rate (18/20 = 90% success threshold)
MIN_CONCURRENT_SUCCESS=18
if [ "$SUCCESS_COUNT" -ge $MIN_CONCURRENT_SUCCESS ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: $SUCCESS_COUNT/20 concurrent requests succeeded"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Only $SUCCESS_COUNT/20 concurrent requests succeeded"
    FAIL=$((FAIL + 1))
fi

assert_not_crash "Server survives concurrent burst"

# =========================================================================
# 18. CGI INFINITE LOOP (potentially destructive - run last)
# =========================================================================
log_header "18. CGI INFINITE LOOP (Timeout Handling)"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 35 "${BASE_URL}/cgi-bin/infinite.py")
TOTAL=$((TOTAL + 1))
if kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server survives CGI infinite loop"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server crashed during CGI infinite loop"
    FAIL=$((FAIL + 1))
fi
TOTAL=$((TOTAL + 1))
if [ -n "$STATUS" ] && [ "$STATUS" != "000" ]; then
    echo -e "  ${GREEN}✓ PASS${NC}: Server handles CGI timeout gracefully (HTTP $STATUS)"
    PASS=$((PASS + 1))
else
    echo -e "  ${RED}✗ FAIL${NC}: Server did not respond to CGI timeout (HTTP $STATUS)"
    FAIL=$((FAIL + 1))
fi

# Restart server if it crashed during the infinite loop test
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "  ${YELLOW}  (Restarting server for remaining tests...)${NC}"
    start_server "$CONFIG_FILE"
fi

# =========================================================================
# 19. FINAL SERVER HEALTH CHECK
# =========================================================================
log_header "19. FINAL SERVER HEALTH CHECK"

assert_not_crash "Server still running after all tests"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL}/")
assert_status "Server still responds correctly" "200" "$STATUS"

STATUS=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "${BASE_URL2}/")
assert_status "Second server still responds correctly" "200" "$STATUS"

# =========================================================================
# SUMMARY
# =========================================================================
log_header "TEST SUMMARY"

echo ""
echo -e "  ${GREEN}Passed:  $PASS${NC}"
echo -e "  ${RED}Failed:  $FAIL${NC}"
echo -e "  ${YELLOW}Skipped: $SKIP${NC}"
echo -e "  ${BOLD}Total:   $TOTAL${NC}"
echo ""

SCORE=$((PASS * 100 / TOTAL))
if [ "$FAIL" -eq 0 ]; then
    echo -e "  ${GREEN}${BOLD}★ ALL TESTS PASSED! ★${NC}"
elif [ "$SCORE" -ge 90 ]; then
    echo -e "  ${GREEN}${BOLD}Score: $SCORE% - Excellent${NC}"
elif [ "$SCORE" -ge 70 ]; then
    echo -e "  ${YELLOW}${BOLD}Score: $SCORE% - Good${NC}"
else
    echo -e "  ${RED}${BOLD}Score: $SCORE% - Needs Improvement${NC}"
fi
echo ""

# Clean up
stop_server

# Exit with appropriate code
if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
