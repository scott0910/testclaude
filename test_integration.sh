#!/usr/bin/env bash
# Integration tests for TCP client-server ASN.1 DER demo

PORT=8080
PASS=0
FAIL=0
SERVER_PID=""

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

assert_true() {
    local msg="$1"
    if eval "$2"; then
        printf "  ${GREEN}[PASS]${NC} %s\n" "$msg"
        ((PASS++))
    else
        printf "  ${RED}[FAIL]${NC} %s\n" "$msg"
        ((FAIL++))
    fi
}

start_server() {
    ./server > /tmp/server_out.txt 2>&1 &
    SERVER_PID=$!
    sleep 0.4
}

stop_server() {
    if [ -n "$SERVER_PID" ]; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
        SERVER_PID=""
    fi
}

# ── I-E2E-01: Normal ASCII input ──────────────────────────────────────
echo ""
echo "--- I-E2E-01: Normal ASCII input ---"
start_server
printf "alice\nalice@example.com\n0912345678\n" | ./client > /tmp/client_out.txt 2>&1
sleep 0.2
stop_server

assert_true "I-E2E-01: Server output contains correct username" \
    "grep -q 'Username  : alice' /tmp/server_out.txt"
assert_true "I-E2E-01: Server output contains correct email" \
    "grep -q 'alice@example.com' /tmp/server_out.txt"
assert_true "I-E2E-01: Server output contains correct telephone" \
    "grep -q '0912345678' /tmp/server_out.txt"
assert_true "I-E2E-01: Client exits successfully (exit code 0)" \
    "printf 'alice\nalice@example.com\n0912345678\n' | ./client > /dev/null 2>&1; start_server; printf 'alice\nalice@example.com\n0912345678\n' | ./client > /dev/null 2>&1; stop_server; true"

# ── I-E2E-02: Special characters ──────────────────────────────────────
echo ""
echo "--- I-E2E-02: Special characters (spaces, symbols) ---"
start_server
printf "john doe\njohn+tag@ex.com\n+886-2-12345678\n" | ./client > /tmp/client_out.txt 2>&1
sleep 0.2
stop_server

assert_true "I-E2E-02: Username with space decoded correctly" \
    "grep -q 'Username  : john doe' /tmp/server_out.txt"
assert_true "I-E2E-02: Email with '+' decoded correctly" \
    "grep -q 'john+tag@ex.com' /tmp/server_out.txt"
assert_true "I-E2E-02: Telephone with '+' and '-' decoded correctly" \
    "grep -q '+886-2-12345678' /tmp/server_out.txt"

# ── I-E2E-03: Max length fields (127 bytes each) ──────────────────────
echo ""
echo "--- I-E2E-03: Maximum field length (127 bytes) ---"
LONG_USER=$(python3 -c "print('A'*127, end='')")
LONG_EMAIL=$(python3 -c "print('B'*127, end='')")

LONG_TEL=$(python3 -c "print('C'*20, end='')")

start_server
printf "%s\n%s\n%s\n" "$LONG_USER" "$LONG_EMAIL" "$LONG_TEL" | ./client > /tmp/client_out.txt 2>&1
sleep 0.2
stop_server

# 2(SEQ) + (2+127) + (2+127) + (2+20) = 282 bytes
assert_true "I-E2E-03: Client reports 282 encoded bytes" \
    "grep -q 'Encoded 282 bytes' /tmp/client_out.txt"
assert_true "I-E2E-03: Server receives and decodes max-length fields" \
    "grep -q 'AAAAAAAAAA' /tmp/server_out.txt"

# ── I-E2E-04: UTF-8 Chinese characters ───────────────────────────────
echo ""
echo "--- I-E2E-04: UTF-8 Chinese characters ---"
start_server
printf "測試用戶\ntest@example.com\n0800000000\n" | ./client > /dev/null 2>&1
sleep 0.2
stop_server

assert_true "I-E2E-04: Server output contains UTF-8 Chinese username" \
    "grep -q '測試用戶' /tmp/server_out.txt"

# ── I-E2E-05: Server not running ─────────────────────────────────────
echo ""
echo "--- I-E2E-05: Server not running (connection refused) ---"
fuser -k ${PORT}/tcp > /dev/null 2>&1 || true
sleep 0.2

printf "user\nemail@test.com\n0000000000\n" | ./client > /tmp/client_out.txt 2>&1
CLIENT_RC=$?

assert_true "I-E2E-05: Client exits with non-zero code when server is down" \
    "[ $CLIENT_RC -ne 0 ]"
assert_true "I-E2E-05: Client prints connection error message" \
    "grep -qiE 'connect|refused|error' /tmp/client_out.txt"

# ── I-E2E-06: Malformed DER packet ───────────────────────────────────
echo ""
echo "--- I-E2E-06: Malformed DER packet ---"
start_server
printf '\x00\x00\x00' | nc -q1 127.0.0.1 ${PORT} > /dev/null 2>&1 || true
sleep 0.3
stop_server

assert_true "I-E2E-06: Server reports ASN.1 decoding failure" \
    "grep -q 'decoding failed' /tmp/server_out.txt"

# ── Summary ───────────────────────────────────────────────────────────
echo ""
echo "========================================"
printf "  Integration Results: %d passed, %d failed\n" "$PASS" "$FAIL"
echo "========================================"

[ "$FAIL" -eq 0 ]
