#!/bin/bash
# Simple unit tests for if-then-else-fi support in myshell
# Run from repo root (where ./bin/myshell exists)

MYSHELL=./bin/myshell
if [ ! -x "$MYSHELL" ]; then
  echo "ERROR: $MYSHELL not found or not executable. Build first (make)."
  exit 2
fi

fail=0
pass=0

after_run() {
  local name="$1"; shift
  local out="$1"; shift
  local expect="$1"; shift
  if printf "%s" "$out" | grep -F -q -- "$expect"; then
    echo "PASS: $name"
    pass=$((pass+1))
  else
    echo "FAIL: $name"
    echo "---- expected to contain: $expect"
    echo "---- got:"; printf "%s\n" "$out"
    fail=$((fail+1))
  fi
}

run_test() {
  local name="$1"; shift
  local input="$1"; shift
  local expect="$1"; shift
  # feed the test into myshell and capture stdout+stderr
  out=$(printf "%s\nexit\n" "$input" | "$MYSHELL" 2>&1)
  after_run "$name" "$out" "$expect"
}

# Tests
run_test "if-true-then-echo" "if true
then
 echo OK
fi" "OK"

run_test "if-false-else-echo" "if false
then
 echo OK
else
 echo NOTOK
fi" "NOTOK"

run_test "grep-nonexistent" "if grep 'NO_SUCH_PATTERN_12345' /etc/passwd > /dev/null
then
 echo FOUND
else
 echo NOTFOUND
fi" "NOTFOUND"

run_test "test-file-exists" "if [ -f /etc/passwd ]
then
 echo EXISTS
fi" "EXISTS"

run_test "pipeline-then" "if echo hello | grep hello > /dev/null
then
 echo PIPE_OK
else
 echo PIPE_BAD
fi" "PIPE_OK"

# Summary
echo
echo "Passed: $pass  Failed: $fail"
if [ $fail -gt 0 ]; then
  exit 1
fi
