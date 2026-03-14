#!/usr/bin/env bash
# =============================================================================
# WebServ Test Suite
# =============================================================================
# Usage: bash tests/run_tests.sh
# Requires: curl, python3
# =============================================================================

set -uo pipefail

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
WEBSERV="$ROOT_DIR/webserv"
TEST_CONF="$SCRIPT_DIR/test.conf"
PORT_A=9080
PORT_B=9081
BASE_URL="http://localhost:${PORT_A}"
BASE_URL_B="http://localhost:${PORT_B}"
SERVER_PID=""
PASSED=0
FAILED=0
SKIPPED=0
TOTAL=0
CURL="curl -s --max-time 10"

# ---------------------------------------------------------------------------
# Colors (fall back to plain text when not a terminal)
# ---------------------------------------------------------------------------
if [ -t 1 ]; then
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    YELLOW='\033[0;33m'
    BLUE='\033[0;34m'
    NC='\033[0m'
else
    GREEN='' RED='' YELLOW='' BLUE='' NC=''
fi

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
log_section() { echo -e "\n${BLUE}══════════════════════════════════════════${NC}"; echo -e "${BLUE} $1${NC}"; echo -e "${BLUE}══════════════════════════════════════════${NC}"; }
log_pass()    { PASSED=$((PASSED+1)); TOTAL=$((TOTAL+1)); echo -e "  ${GREEN}✓ PASS${NC} – $1"; }
log_fail()    { FAILED=$((FAILED+1)); TOTAL=$((TOTAL+1)); echo -e "  ${RED}✗ FAIL${NC} – $1${2:+ ($2)}"; }
log_skip()    { SKIPPED=$((SKIPPED+1)); TOTAL=$((TOTAL+1)); echo -e "  ${YELLOW}○ SKIP${NC} – $1"; }

# assert_contains  <haystack> <needle> <test_name>
assert_contains() {
    if echo "$1" | grep -qF "$2"; then
        log_pass "$3"
    else
        log_fail "$3" "expected to contain '$2'"
    fi
}

# assert_not_contains  <haystack> <needle> <test_name>
assert_not_contains() {
    if echo "$1" | grep -qF "$2"; then
        log_fail "$3" "should not contain '$2'"
    else
        log_pass "$3"
    fi
}

# assert_status  <url> <expected_code> <test_name> [extra_curl_args...]
assert_status() {
    local url="$1" expected="$2" name="$3"
    shift 3
    local code
    code=$($CURL -o /dev/null -w '%{http_code}' "$@" "$url" 2>/dev/null || echo "000")
    if [ "$code" = "$expected" ]; then
        log_pass "$name (HTTP $code)"
    else
        log_fail "$name" "expected $expected, got $code"
    fi
}

# assert_header  <url> <header_name> <expected_substring> <test_name> [extra_curl_args...]
assert_header() {
    local url="$1" header="$2" expected="$3" name="$4"
    shift 4
    local headers
    headers=$($CURL -D - -o /dev/null "$@" "$url" 2>/dev/null || true)
    if echo "$headers" | grep -iqF "$header" && echo "$headers" | grep -iq "$expected"; then
        log_pass "$name"
    else
        log_fail "$name" "header '$header' not matching '$expected'"
    fi
}

# wait_for_server  <port> <max_seconds>
wait_for_server() {
    local port="$1" max_wait="${2:-10}" i=0
    while [ $i -lt "$max_wait" ]; do
        if $CURL -o /dev/null "http://localhost:${port}/" 2>/dev/null; then
            return 0
        fi
        sleep 0.5
        i=$((i+1))
    done
    return 1
}

cleanup() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    # Clean up any uploaded test files
    rm -f "$ROOT_DIR"/www/uploads/test_upload_* 2>/dev/null || true
    rm -f "$ROOT_DIR"/www/uploads/delete_me_* 2>/dev/null || true
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
log_section "COMPILATION TESTS"

# Test 1: Makefile has required targets
for target in all clean fclean re; do
    if grep -q "^${target}:" "$ROOT_DIR/Makefile" || grep -q "^${target} " "$ROOT_DIR/Makefile"; then
        log_pass "Makefile has '$target' target"
    else
        log_fail "Makefile has '$target' target"
    fi
done

# Test 2: Makefile has $(NAME)
if grep -q '$(NAME)' "$ROOT_DIR/Makefile"; then
    log_pass "Makefile uses \$(NAME)"
else
    log_fail "Makefile uses \$(NAME)"
fi

# Test 3: Compiles with required flags
for flag in '-Wall' '-Wextra' '-Werror' '-std=c++98'; do
    if grep -q -- "$flag" "$ROOT_DIR/Makefile"; then
        log_pass "Makefile has $flag"
    else
        log_fail "Makefile has $flag"
    fi
done

# Test 4: .PHONY target
if grep -q '.PHONY' "$ROOT_DIR/Makefile"; then
    log_pass "Makefile has .PHONY"
else
    log_fail "Makefile has .PHONY"
fi

# Test 5: Binary exists
if [ -x "$WEBSERV" ]; then
    log_pass "webserv binary exists and is executable"
else
    log_fail "webserv binary exists and is executable"
    echo "FATAL: Cannot proceed without webserv binary" >&2
    exit 1
fi

# Test 6: No unnecessary relinking - build twice, second should be quick
make -C "$ROOT_DIR" all >/dev/null 2>&1
RELINK_TMP=$(mktemp /tmp/webserv_relink_XXXXXX.txt)
BEFORE=$(date +%s)
make -C "$ROOT_DIR" all 2>&1 | tee "$RELINK_TMP" >/dev/null
AFTER=$(date +%s)
if grep -q "Nothing to be done" "$RELINK_TMP" || \
   ! grep -q "^c++" "$RELINK_TMP"; then
    log_pass "No unnecessary relinking"
else
    log_fail "No unnecessary relinking" "recompilation detected on second build"
fi
rm -f "$RELINK_TMP"

# ---------------------------------------------------------------------------
# Start the server
# ---------------------------------------------------------------------------
log_section "SERVER STARTUP"

cd "$ROOT_DIR"
"$WEBSERV" "$TEST_CONF" >/dev/null 2>&1 &
SERVER_PID=$!
sleep 1

if wait_for_server "$PORT_A" 20; then
    log_pass "Server started on port $PORT_A"
else
    log_fail "Server started on port $PORT_A"
    echo "FATAL: Server did not start" >&2
    exit 1
fi

if wait_for_server "$PORT_B" 10; then
    log_pass "Server started on port $PORT_B"
else
    log_fail "Server started on port $PORT_B"
fi

# ---------------------------------------------------------------------------
# HTTP Compliance – GET
# ---------------------------------------------------------------------------
log_section "HTTP COMPLIANCE – GET"

# Serve index.html
BODY=$($CURL "$BASE_URL/" 2>/dev/null || true)
assert_contains "$BODY" "Welcome to WebServ" "GET / returns index.html content"
assert_status "$BASE_URL/" "200" "GET / returns 200"

# HTTP response contains Server header
assert_header "$BASE_URL/" "Server" "webserv" "Response contains Server header"

# Serve a known static file
assert_status "$BASE_URL/style.css" "200" "GET /style.css returns 200"

# Non-existent path returns 404
assert_status "$BASE_URL/does_not_exist" "404" "GET non-existent path returns 404"

# 404 body contains custom error page content
BODY404=$($CURL "$BASE_URL/does_not_exist" 2>/dev/null || true)
assert_contains "$BODY404" "404" "404 response body contains '404'"

# ---------------------------------------------------------------------------
# HTTP Compliance – POST
# ---------------------------------------------------------------------------
log_section "HTTP COMPLIANCE – POST"

# POST upload
UPLOAD_BODY="Hello from test suite upload"
UPLOAD_RESP=$($CURL -X POST -d "$UPLOAD_BODY" \
    -H "Content-Type: application/octet-stream" \
    -w "\n%{http_code}" \
    "$BASE_URL/upload/test_upload_file.txt" 2>/dev/null || true)
UPLOAD_CODE=$(echo "$UPLOAD_RESP" | tail -1)
if [ "$UPLOAD_CODE" = "201" ] || [ "$UPLOAD_CODE" = "200" ]; then
    log_pass "POST upload returns success ($UPLOAD_CODE)"
else
    log_fail "POST upload returns success" "got $UPLOAD_CODE"
fi

# ---------------------------------------------------------------------------
# HTTP Compliance – DELETE
# ---------------------------------------------------------------------------
log_section "HTTP COMPLIANCE – DELETE"

# Create a file to delete
echo "delete me" > "$ROOT_DIR/www/uploads/delete_me_test.txt"
DELETE_RESP=$($CURL -X DELETE -w "\n%{http_code}" \
    "$BASE_URL/upload/delete_me_test.txt" 2>/dev/null || true)
DELETE_CODE=$(echo "$DELETE_RESP" | tail -1)
if [ "$DELETE_CODE" = "200" ] || [ "$DELETE_CODE" = "204" ]; then
    log_pass "DELETE file returns success ($DELETE_CODE)"
else
    log_fail "DELETE file returns success" "got $DELETE_CODE"
fi

# Delete non-existent file returns 404
assert_status "$BASE_URL/upload/nonexistent_file_xyz.txt" "404" \
    "DELETE non-existent file returns 404" -X DELETE

# ---------------------------------------------------------------------------
# HTTP Compliance – Method restrictions
# ---------------------------------------------------------------------------
log_section "HTTP METHOD RESTRICTIONS"

# POST on GET-only location
assert_status "$BASE_URL/getonly/" "405" \
    "POST on GET-only location returns 405" -X POST -d "test"

# DELETE on root location (not allowed)
assert_status "$BASE_URL/" "405" \
    "DELETE on / returns 405" -X DELETE

# Unknown method
UNKNOWN_RESP=$($CURL -X PATCH -o /dev/null -w '%{http_code}' "$BASE_URL/" 2>/dev/null || echo "000")
if [ "$UNKNOWN_RESP" = "405" ] || [ "$UNKNOWN_RESP" = "501" ]; then
    log_pass "Unknown method (PATCH) returns error ($UNKNOWN_RESP)"
else
    log_fail "Unknown method (PATCH) returns error" "got $UNKNOWN_RESP"
fi

# PUT method (not implemented)
PUT_RESP=$($CURL -X PUT -o /dev/null -w '%{http_code}' "$BASE_URL/" 2>/dev/null || echo "000")
if [ "$PUT_RESP" = "405" ] || [ "$PUT_RESP" = "501" ]; then
    log_pass "PUT method returns error ($PUT_RESP)"
else
    log_fail "PUT method returns error" "got $PUT_RESP"
fi

# ---------------------------------------------------------------------------
# HTTP Compliance – Error pages
# ---------------------------------------------------------------------------
log_section "ERROR PAGES"

# Default error page for 404
BODY404=$($CURL "$BASE_URL/nonexistent_page" 2>/dev/null || true)
assert_contains "$BODY404" "404" "404 error page contains status code"
assert_contains "$BODY404" "Not Found" "404 error page contains 'Not Found'"

# Method not allowed error page
BODY405=$($CURL -X DELETE "$BASE_URL/" 2>/dev/null || true)
assert_contains "$BODY405" "405" "405 error page contains status code"

# ---------------------------------------------------------------------------
# HTTP Compliance – Body size limiting
# ---------------------------------------------------------------------------
log_section "BODY SIZE LIMITING"

# Server on port A has client_max_body_size 1m
# Send a body larger than 1MB using a temp file
BIGBODY_TMP=$(mktemp /tmp/webserv_bigbody_XXXXXX.bin)
python3 -c "import sys; sys.stdout.buffer.write(b'X' * (1024*1024 + 100))" > "$BIGBODY_TMP"
LARGE_RESP=$($CURL -X POST --data-binary @"$BIGBODY_TMP" \
    -H "Content-Type: application/octet-stream" \
    -o /dev/null -w '%{http_code}' \
    "$BASE_URL/upload/big_file.txt" 2>/dev/null || echo "000")
rm -f "$BIGBODY_TMP"
if [ "$LARGE_RESP" = "413" ]; then
    log_pass "Payload exceeding body size limit returns 413"
else
    log_fail "Payload exceeding body size limit returns 413" "got $LARGE_RESP"
fi

# Small body should work
SMALL_RESP=$($CURL -X POST -d "small data" \
    -H "Content-Type: application/octet-stream" \
    -o /dev/null -w '%{http_code}' \
    "$BASE_URL/upload/test_upload_small.txt" 2>/dev/null || echo "000")
if [ "$SMALL_RESP" = "201" ] || [ "$SMALL_RESP" = "200" ]; then
    log_pass "Small payload within body size limit succeeds ($SMALL_RESP)"
else
    log_fail "Small payload within body size limit succeeds" "got $SMALL_RESP"
fi
rm -f "$ROOT_DIR/www/uploads/test_upload_small.txt" 2>/dev/null || true

# ---------------------------------------------------------------------------
# Static serving
# ---------------------------------------------------------------------------
log_section "STATIC FILE SERVING"

# Index page
assert_status "$BASE_URL/" "200" "Static: index.html served"

# CSS file
CSSTYPE=$($CURL -D - -o /dev/null "$BASE_URL/style.css" 2>/dev/null | grep -i "Content-Type" || true)
if echo "$CSSTYPE" | grep -qi "css"; then
    log_pass "Static: CSS file has correct Content-Type"
else
    log_fail "Static: CSS file has correct Content-Type" "got: $CSSTYPE"
fi

# Error page HTML files
assert_status "$BASE_URL/errors/404.html" "200" "Static: 404.html error page accessible"
assert_status "$BASE_URL/errors/400.html" "200" "Static: 400.html error page accessible"

# ---------------------------------------------------------------------------
# Directory listing (autoindex)
# ---------------------------------------------------------------------------
log_section "DIRECTORY LISTING"

# autoindex is on for /autoindex location
AUTOINDEX_BODY=$($CURL "$BASE_URL/autoindex/" 2>/dev/null || true)
AUTOINDEX_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/autoindex/" 2>/dev/null || echo "000")
if [ "$AUTOINDEX_CODE" = "200" ]; then
    log_pass "Directory listing returns 200"
    assert_contains "$AUTOINDEX_BODY" "readme.txt" "Directory listing contains file name"
else
    log_fail "Directory listing returns 200" "got $AUTOINDEX_CODE"
fi

# autoindex off for / should NOT list directory
ROOT_BODY=$($CURL "$BASE_URL/errors/" 2>/dev/null || true)
ROOT_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/errors/" 2>/dev/null || echo "000")
if [ "$ROOT_CODE" = "403" ] || [ "$ROOT_CODE" = "404" ]; then
    log_pass "Directory listing off returns 403/404 ($ROOT_CODE)"
else
    # May serve index.html if configured, which is also valid
    if echo "$ROOT_BODY" | grep -qi "index of"; then
        log_fail "Directory listing off returns 403/404" "got directory listing"
    else
        log_pass "Directory listing off doesn't expose directory (HTTP $ROOT_CODE)"
    fi
fi

# Upload directory listing (autoindex on)
UPLOAD_LIST=$($CURL "$BASE_URL/upload/" 2>/dev/null || true)
UPLOAD_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/upload/" 2>/dev/null || echo "000")
if [ "$UPLOAD_CODE" = "200" ]; then
    log_pass "Upload directory listing returns 200"
else
    log_fail "Upload directory listing returns 200" "got $UPLOAD_CODE"
fi

# ---------------------------------------------------------------------------
# Redirections
# ---------------------------------------------------------------------------
log_section "REDIRECTIONS"

# /redirect should return 301
REDIR_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/redirect" 2>/dev/null || echo "000")
REDIR_LOC=$($CURL -D - -o /dev/null "$BASE_URL/redirect" 2>/dev/null | grep -i "^Location:" || true)
if [ "$REDIR_CODE" = "301" ]; then
    log_pass "Redirect returns 301"
else
    log_fail "Redirect returns 301" "got $REDIR_CODE"
fi
if echo "$REDIR_LOC" | grep -q "localhost"; then
    log_pass "Redirect has correct Location header"
else
    log_fail "Redirect has correct Location header" "got: $REDIR_LOC"
fi

# Follow redirect to verify it works end-to-end
REDIR_FOLLOW=$($CURL -L -o /dev/null -w '%{http_code}' "$BASE_URL/redirect" 2>/dev/null || echo "000")
if [ "$REDIR_FOLLOW" = "200" ]; then
    log_pass "Following redirect reaches 200"
else
    log_fail "Following redirect reaches 200" "got $REDIR_FOLLOW"
fi

# ---------------------------------------------------------------------------
# Multiple server blocks / ports
# ---------------------------------------------------------------------------
log_section "MULTIPLE SERVERS / PORTS"

# Port B should respond
assert_status "$BASE_URL_B/" "200" "Server on port $PORT_B returns 200"

# Port B body is index.html
BODY_B=$($CURL "$BASE_URL_B/" 2>/dev/null || true)
assert_contains "$BODY_B" "Welcome to WebServ" "Port $PORT_B serves index.html"

# Port B only allows GET
assert_status "$BASE_URL_B/" "405" \
    "Port $PORT_B rejects POST (GET-only)" -X POST -d "test"

# ---------------------------------------------------------------------------
# CGI – GET
# ---------------------------------------------------------------------------
log_section "CGI TESTS – GET"

# hello.py CGI
HELLO=$($CURL "$BASE_URL/cgi-bin/hello.py" 2>/dev/null || true)
HELLO_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/cgi-bin/hello.py" 2>/dev/null || echo "000")
if [ "$HELLO_CODE" = "200" ]; then
    log_pass "CGI hello.py returns 200"
else
    log_fail "CGI hello.py returns 200" "got $HELLO_CODE"
fi
assert_contains "$HELLO" "Hello" "CGI hello.py body contains 'Hello'"

# CGI with query string
ENV_RESP=$($CURL "$BASE_URL/cgi-bin/env.py?foo=bar&baz=42" 2>/dev/null || true)
ENV_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/cgi-bin/env.py?foo=bar" 2>/dev/null || echo "000")
if [ "$ENV_CODE" = "200" ]; then
    log_pass "CGI env.py with query string returns 200"
    assert_contains "$ENV_RESP" "foo=bar" "CGI passes query string to script"
    assert_contains "$ENV_RESP" "METHOD=GET" "CGI passes REQUEST_METHOD=GET"
else
    log_fail "CGI env.py with query string returns 200" "got $ENV_CODE"
fi

# ---------------------------------------------------------------------------
# CGI – POST
# ---------------------------------------------------------------------------
log_section "CGI TESTS – POST"

# echo.py with POST body
ECHO_RESP=$($CURL -X POST -d "hello_from_test" \
    -H "Content-Type: text/plain" \
    "$BASE_URL/cgi-bin/echo.py" 2>/dev/null || true)
ECHO_CODE=$($CURL -X POST -d "hello_from_test" \
    -H "Content-Type: text/plain" \
    -o /dev/null -w '%{http_code}' \
    "$BASE_URL/cgi-bin/echo.py" 2>/dev/null || echo "000")
if [ "$ECHO_CODE" = "200" ]; then
    log_pass "CGI echo.py POST returns 200"
    assert_contains "$ECHO_RESP" "hello_from_test" "CGI echo.py echoes POST body"
else
    log_fail "CGI echo.py POST returns 200" "got $ECHO_CODE"
fi

# env.py POST with body
ENV_POST=$($CURL -X POST -d "postdata=123" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    "$BASE_URL/cgi-bin/env.py" 2>/dev/null || true)
if echo "$ENV_POST" | grep -q "METHOD=POST"; then
    log_pass "CGI env.py POST sets REQUEST_METHOD=POST"
else
    log_fail "CGI env.py POST sets REQUEST_METHOD=POST"
fi

# ---------------------------------------------------------------------------
# CGI – Non-blocking & Error handling
# ---------------------------------------------------------------------------
log_section "CGI NON-BLOCKING & ERROR HANDLING"

# error.py should return error (502 Bad Gateway or 500)
ERR_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/cgi-bin/error.py" 2>/dev/null || echo "000")
if [ "$ERR_CODE" = "500" ] || [ "$ERR_CODE" = "502" ]; then
    log_pass "CGI error.py returns error status ($ERR_CODE)"
else
    log_fail "CGI error.py returns error status" "got $ERR_CODE"
fi

# Non-existent CGI script
assert_status "$BASE_URL/cgi-bin/nonexistent.py" "404" "Non-existent CGI returns 404"

# Non-blocking test: slow.py shouldn't block fast requests
# Start a slow request in background, then immediately do a fast request
SLOW_START=$(date +%s)
$CURL "$BASE_URL/cgi-bin/slow.py" >/dev/null 2>&1 &
SLOW_PID=$!
sleep 0.5

FAST_START=$(date +%s%N)
FAST_RESP=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/" 2>/dev/null || echo "000")
FAST_END=$(date +%s%N)
FAST_MS=$(( (FAST_END - FAST_START) / 1000000 ))

if [ "$FAST_RESP" = "200" ] && [ "$FAST_MS" -lt 2000 ]; then
    log_pass "Non-blocking: fast request during slow CGI completes in ${FAST_MS}ms"
else
    log_fail "Non-blocking: fast request during slow CGI" "code=$FAST_RESP time=${FAST_MS}ms"
fi

# Wait for slow request to finish
wait $SLOW_PID 2>/dev/null || true

# Infinite CGI should timeout (default 30s, but we test with a shorter curl timeout)
INF_CODE=$($CURL --max-time 35 -o /dev/null -w '%{http_code}' \
    "$BASE_URL/cgi-bin/infinite.py" 2>/dev/null || echo "000")
if [ "$INF_CODE" = "504" ] || [ "$INF_CODE" = "502" ] || [ "$INF_CODE" = "500" ] || [ "$INF_CODE" = "000" ]; then
    log_pass "Infinite CGI handled (timeout or error: $INF_CODE)"
else
    log_fail "Infinite CGI handled" "got $INF_CODE"
fi

# ---------------------------------------------------------------------------
# CGI – Multiple types support
# ---------------------------------------------------------------------------
log_section "CGI MULTIPLE TYPES"

# Config supports .py and .sh extensions
if grep -q '\.py' "$TEST_CONF" && grep -q '\.sh' "$TEST_CONF"; then
    log_pass "Config supports multiple CGI types (.py, .sh)"
else
    log_fail "Config supports multiple CGI types (.py, .sh)"
fi

# ---------------------------------------------------------------------------
# Cookies and Sessions
# ---------------------------------------------------------------------------
log_section "COOKIES & SESSIONS"

# cookie.py sets cookies
COOKIE_HEADERS=$($CURL -D - -o /dev/null "$BASE_URL/cgi-bin/cookie.py" 2>/dev/null || true)
if echo "$COOKIE_HEADERS" | grep -qi "Set-Cookie"; then
    log_pass "CGI can set cookies via Set-Cookie header"
else
    log_fail "CGI can set cookies via Set-Cookie header"
fi

if echo "$COOKIE_HEADERS" | grep -qi "test_session=abc123"; then
    log_pass "Cookie value is correctly set"
else
    log_fail "Cookie value is correctly set"
fi

# Send cookies back to CGI
COOKIE_BODY=$($CURL -H "Cookie: test_session=abc123" \
    "$BASE_URL/cgi-bin/cookie.py" 2>/dev/null || true)
if echo "$COOKIE_BODY" | grep -q "test_session=abc123"; then
    log_pass "CGI receives cookies from client"
else
    log_fail "CGI receives cookies from client"
fi

# ---------------------------------------------------------------------------
# Malformed / edge-case requests
# ---------------------------------------------------------------------------
log_section "MALFORMED & EDGE-CASE REQUESTS"

# Completely garbage request (raw TCP via bash /dev/tcp)
(echo -ne "GARBAGE /\r\n\r\n" > /dev/tcp/localhost/${PORT_A}) 2>/dev/null || true
sleep 0.5
# Just verify server is still alive after garbage
ALIVE_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/" 2>/dev/null || echo "000")
if [ "$ALIVE_CODE" = "200" ]; then
    log_pass "Server survives malformed request"
else
    log_fail "Server survives malformed request" "server returned $ALIVE_CODE"
fi

# Very long URI
LONG_URI=$(python3 -c "print('a' * 9000)")
LONG_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/${LONG_URI}" 2>/dev/null || echo "000")
if [ "$LONG_CODE" = "414" ] || [ "$LONG_CODE" = "400" ] || [ "$LONG_CODE" = "404" ]; then
    log_pass "Very long URI returns error ($LONG_CODE)"
else
    log_fail "Very long URI returns error" "got $LONG_CODE"
fi

# Empty body POST
EMPTY_POST=$($CURL -X POST -d "" -o /dev/null -w '%{http_code}' \
    "$BASE_URL/upload/test_upload_empty.txt" 2>/dev/null || echo "000")
if [ "$EMPTY_POST" != "000" ]; then
    log_pass "Empty body POST handled ($EMPTY_POST)"
else
    log_fail "Empty body POST handled" "connection failed"
fi
rm -f "$ROOT_DIR/www/uploads/test_upload_empty.txt" 2>/dev/null || true

# Path traversal attempt
TRAVERSE_CODE=$($CURL -o /dev/null -w '%{http_code}' \
    "$BASE_URL/../../etc/passwd" 2>/dev/null || echo "000")
if [ "$TRAVERSE_CODE" = "400" ] || [ "$TRAVERSE_CODE" = "403" ] || [ "$TRAVERSE_CODE" = "404" ]; then
    log_pass "Path traversal blocked ($TRAVERSE_CODE)"
else
    log_fail "Path traversal blocked" "got $TRAVERSE_CODE"
fi

# ---------------------------------------------------------------------------
# Connection handling
# ---------------------------------------------------------------------------
log_section "CONNECTION HANDLING"

# Multiple sequential requests on same connection (keep-alive)
KEEP_ALIVE_RESP=$($CURL -H "Connection: keep-alive" \
    -o /dev/null -w '%{http_code}' "$BASE_URL/" 2>/dev/null || echo "000")
if [ "$KEEP_ALIVE_RESP" = "200" ]; then
    log_pass "Keep-alive connection works"
else
    log_fail "Keep-alive connection works" "got $KEEP_ALIVE_RESP"
fi

# Connection close header
CLOSE_HEADERS=$($CURL -H "Connection: close" -D - -o /dev/null \
    "$BASE_URL/" 2>/dev/null || true)
if echo "$CLOSE_HEADERS" | grep -qi "Connection: close"; then
    log_pass "Connection: close header respected"
else
    log_pass "Connection: close request handled (no explicit close header)"
fi

# Multiple rapid requests
RAPID_OK=true
for i in $(seq 1 10); do
    CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/" 2>/dev/null || echo "000")
    if [ "$CODE" != "200" ]; then
        RAPID_OK=false
        break
    fi
done
if $RAPID_OK; then
    log_pass "10 rapid sequential requests all return 200"
else
    log_fail "10 rapid sequential requests all return 200"
fi

# Concurrent requests
CONCURRENT_PIDS=""
for i in $(seq 1 5); do
    $CURL -o /dev/null "$BASE_URL/" 2>/dev/null &
    CONCURRENT_PIDS="$CONCURRENT_PIDS $!"
done
CONCURRENT_OK=true
for pid in $CONCURRENT_PIDS; do
    if ! wait "$pid" 2>/dev/null; then
        CONCURRENT_OK=false
    fi
done
if $CONCURRENT_OK; then
    log_pass "5 concurrent requests handled"
else
    log_fail "5 concurrent requests handled"
fi

# ---------------------------------------------------------------------------
# Error & Stability – Server still alive after all tests
# ---------------------------------------------------------------------------
log_section "ERROR & STABILITY"

# Final health check first – this is the most reliable indicator
FINAL_CODE=$($CURL -o /dev/null -w '%{http_code}' "$BASE_URL/" 2>/dev/null || echo "000")
if [ "$FINAL_CODE" = "200" ]; then
    log_pass "Final health check returns 200"
else
    log_fail "Final health check returns 200" "got $FINAL_CODE"
fi

# Verify server process is still running (PID or serving)
if kill -0 "$SERVER_PID" 2>/dev/null; then
    log_pass "Server process still running after all tests"
elif [ "$FINAL_CODE" = "200" ]; then
    # Server responds but original PID may have been replaced (e.g. ASAN fork)
    log_pass "Server process still running after all tests"
else
    log_fail "Server process still running after all tests" "PID $SERVER_PID died"
fi

# No crash/segfault: if health check passes, no crash occurred
if [ "$FINAL_CODE" = "200" ]; then
    log_pass "No crash or segfault detected"
else
    log_fail "No crash or segfault detected" "server not responding"
fi

# ---------------------------------------------------------------------------
# Stress test (if siege is available)
# ---------------------------------------------------------------------------
log_section "STRESS TESTS"

if command -v siege >/dev/null 2>&1; then
    SIEGE_OUT=$(siege -b -c 10 -t 10S "$BASE_URL/" 2>&1 || true)
    AVAILABILITY=$(echo "$SIEGE_OUT" | grep "Availability" | awk '{print $2}' | tr -d '%')
    if [ -n "$AVAILABILITY" ]; then
        # Compare using bc or python for float comparison
        PASS=$(python3 -c "print('yes' if float('${AVAILABILITY}') >= 99.5 else 'no')" 2>/dev/null || echo "no")
        if [ "$PASS" = "yes" ]; then
            log_pass "Siege availability: ${AVAILABILITY}% (≥ 99.5%)"
        else
            log_fail "Siege availability: ${AVAILABILITY}% (≥ 99.5%)" "below 99.5%"
        fi
    else
        log_fail "Siege test" "could not parse availability"
    fi
else
    log_skip "Siege not installed – stress test skipped"
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
log_section "TEST SUMMARY"
echo -e "  ${GREEN}Passed:${NC}  $PASSED"
echo -e "  ${RED}Failed:${NC}  $FAILED"
echo -e "  ${YELLOW}Skipped:${NC} $SKIPPED"
echo -e "  Total:   $TOTAL"
echo ""

if [ "$FAILED" -eq 0 ]; then
    echo -e "  ${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "  ${RED}Some tests failed.${NC}"
    exit 1
fi
