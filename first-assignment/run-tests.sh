#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LLVM_BIN="${LLVM_BIN:-/opt/homebrew/opt/llvm@19/bin}"
CLANG="${CLANG:-clang}"
OPT="${OPT:-$LLVM_BIN/opt}"
LLVM_CONFIG="${LLVM_CONFIG:-$LLVM_BIN/llvm-config}"

if [[ ! -x "$OPT" ]]; then
  echo "error: opt not found at $OPT"
  echo "Set LLVM_BIN to your LLVM 19 bin directory."
  exit 1
fi

build_pass() {
  local name="$1"
  local source="$2"
  echo "==> Building $name"
  # shellcheck disable=SC2046
  clang++ -std=c++17 -fPIC -shared -o "$ROOT/$name.dylib" "$ROOT/$source" \
    $($LLVM_CONFIG --cxxflags --ldflags --system-libs --libs core passes)
}

emit_llvm() {
  local source="$1"
  local output="$2"
  "$CLANG" -O0 -Xclang -disable-O0-optnone -S -emit-llvm "$source" -o "$output"
}

run_opt() {
  local plugin="$1"
  local passes="$2"
  local input="$3"
  local output="$4"
  "$OPT" -S -load-pass-plugin="$ROOT/$plugin.dylib" -passes="$passes" \
    "$input" -o "$output"
}

extract_function() {
  local file="$1"
  local fn="$2"
  awk "/^define .+ @$fn\\(/,/^}/" "$file"
}

assert_fn_not_contains() {
  local file="$1"
  local fn="$2"
  local pattern="$3"
  local label="$4"
  if extract_function "$file" "$fn" | grep -qE "$pattern"; then
    echo "FAIL: $label — @$fn still contains '$pattern'"
    extract_function "$file" "$fn" | grep -nE "$pattern" || true
    exit 1
  fi
}

assert_fn_contains() {
  local file="$1"
  local fn="$2"
  local pattern="$3"
  local label="$4"
  if ! extract_function "$file" "$fn" | grep -qE "$pattern"; then
    echo "FAIL: $label — @$fn missing '$pattern'"
    exit 1
  fi
}

assert_fn_ret_arg() {
  local file="$1"
  local fn="$2"
  local arg_idx="$3"
  local label="$4"
  # es. ret i32 %0  oppure ret i32 %1
  if ! extract_function "$file" "$fn" | grep -qE "ret i32 %${arg_idx}\b"; then
    echo "FAIL: $label — @$fn should return argument %${arg_idx}"
    extract_function "$file" "$fn"
    exit 1
  fi
}

echo "=== First assignment test suite ==="

build_pass "algebraic-identity" "algebraic-identity.cpp"
build_pass "strength-reduction" "strength-reduction.cpp"
build_pass "multi-inst-opt" "multi-inst-opt.cpp"

# --- Algebraic Identity ---
ALG_DIR="$ROOT/tests/algebraic-identity"
ALG_OUT="$ALG_DIR/alg-id.test.optimized.dce.ll"
emit_llvm "$ALG_DIR/test.c" "$ALG_DIR/test.ll"
run_opt "algebraic-identity" "alg-id,dce" "$ALG_DIR/test.ll" "$ALG_OUT"

assert_fn_not_contains "$ALG_OUT" "add" 'add nsw i32 .* 0' "algebraic identity @add"
assert_fn_not_contains "$ALG_OUT" "add" 'add nsw i32 0,' "algebraic identity @add"
assert_fn_contains "$ALG_OUT" "add" 'add nsw i32' "algebraic identity @add (a+b preserved)"

assert_fn_not_contains "$ALG_OUT" "sub" 'sub nsw i32 .* 0' "algebraic identity @sub"
assert_fn_contains "$ALG_OUT" "sub" 'sub nsw i32 0,' "algebraic identity @sub (0-a preserved)"
assert_fn_contains "$ALG_OUT" "sub" 'sub nsw i32' "algebraic identity @sub (a-b preserved)"

assert_fn_not_contains "$ALG_OUT" "mul" 'mul nsw i32 .* 1' "algebraic identity @mul"
assert_fn_not_contains "$ALG_OUT" "mul" 'mul nsw i32 1,' "algebraic identity @mul"
assert_fn_contains "$ALG_OUT" "mul" 'mul nsw i32 .* 0' "algebraic identity @mul (a*0 not optimized)"
assert_fn_contains "$ALG_OUT" "mul" 'mul nsw i32' "algebraic identity @mul (a*b preserved)"

assert_fn_not_contains "$ALG_OUT" "div_test" 'sdiv i32 .* 1$' "algebraic identity @div_test"
assert_fn_contains "$ALG_OUT" "div_test" 'sdiv i32 0,' "algebraic identity @div_test (0/a not optimized)"
assert_fn_contains "$ALG_OUT" "div_test" 'sdiv i32 1,' "algebraic identity @div_test (1/a preserved)"
assert_fn_contains "$ALG_OUT" "div_test" 'sdiv i32' "algebraic identity @div_test (a/b preserved)"

assert_fn_not_contains "$ALG_OUT" "nested" 'add nsw i32 .* 0' "algebraic identity @nested"
echo "PASS: algebraic identity"

# --- Strength Reduction ---
SR_DIR="$ROOT/tests/strength-reduction"
SR_OUT="$SR_DIR/strength-reduction.test.optimized.dce.ll"
emit_llvm "$SR_DIR/test.c" "$SR_DIR/test.ll"
run_opt "strength-reduction" "strength-reduction,dce" "$SR_DIR/test.ll" "$SR_OUT"

assert_fn_contains "$SR_OUT" "test_mul" 'shl i32' "strength reduction @test_mul"
assert_fn_contains "$SR_OUT" "test_mul" 'add i32' "strength reduction @test_mul (9 = shl+add)"
assert_fn_contains "$SR_OUT" "test_mul" 'sub i32' "strength reduction @test_mul (7/15 = shl-sub)"
assert_fn_contains "$SR_OUT" "test_mul" 'mul nsw i32 .* 6' "strength reduction @test_mul (a*6 preserved)"

assert_fn_contains "$SR_OUT" "test_udiv" 'lshr i32' "strength reduction @test_udiv"
assert_fn_contains "$SR_OUT" "test_udiv" 'udiv i32 .* 6' "strength reduction @test_udiv (a/6 preserved)"

assert_fn_contains "$SR_OUT" "test_sdiv" 'ashr i32' "strength reduction @test_sdiv"
assert_fn_contains "$SR_OUT" "test_sdiv" 'sdiv i32 .* 3' "strength reduction @test_sdiv (a/3 preserved)"

assert_fn_contains "$SR_OUT" "test_no_opt" 'mul nsw i32 .* -8' "strength reduction @test_no_opt (negative mul preserved)"
assert_fn_contains "$SR_OUT" "test_no_opt" 'sdiv i32 .* -4' "strength reduction @test_no_opt (negative div preserved)"
echo "PASS: strength reduction"

# --- Multi Instruction Optimization ---
MIO_DIR="$ROOT/tests/multi-inst-opt"
MIO_OUT="$MIO_DIR/multi-inst-opt.test.optimized.dce.ll"
emit_llvm "$MIO_DIR/test.c" "$MIO_DIR/test.ll"
run_opt "multi-inst-opt" "mem2reg,multi-inst-opt,dce" "$MIO_DIR/test.ll" "$MIO_OUT"

assert_fn_ret_arg "$MIO_OUT" "test_add_sub" "0" "multi-inst-opt @test_add_sub"
assert_fn_not_contains "$MIO_OUT" "test_add_sub" 'sub nsw i32' "multi-inst-opt @test_add_sub"

assert_fn_ret_arg "$MIO_OUT" "test_add_sub_comm" "0" "multi-inst-opt @test_add_sub_comm"
assert_fn_not_contains "$MIO_OUT" "test_add_sub_comm" 'sub nsw i32' "multi-inst-opt @test_add_sub_comm"

assert_fn_ret_arg "$MIO_OUT" "test_sub_add" "0" "multi-inst-opt @test_sub_add"
assert_fn_not_contains "$MIO_OUT" "test_sub_add" 'add nsw i32' "multi-inst-opt @test_sub_add"

assert_fn_ret_arg "$MIO_OUT" "test_assignment" "0" "multi-inst-opt @test_assignment"
assert_fn_not_contains "$MIO_OUT" "test_assignment" 'sub nsw i32' "multi-inst-opt @test_assignment"

assert_fn_ret_arg "$MIO_OUT" "test_mul_sdiv" "0" "multi-inst-opt @test_mul_sdiv"
assert_fn_not_contains "$MIO_OUT" "test_mul_sdiv" 'sdiv i32' "multi-inst-opt @test_mul_sdiv"

assert_fn_ret_arg "$MIO_OUT" "test_mul_sdiv_comm" "0" "multi-inst-opt @test_mul_sdiv_comm"
assert_fn_not_contains "$MIO_OUT" "test_mul_sdiv_comm" 'sdiv i32' "multi-inst-opt @test_mul_sdiv_comm"

assert_fn_ret_arg "$MIO_OUT" "test_mul_udiv" "0" "multi-inst-opt @test_mul_udiv"
assert_fn_not_contains "$MIO_OUT" "test_mul_udiv" 'udiv i32' "multi-inst-opt @test_mul_udiv"

assert_fn_contains "$MIO_OUT" "test_wrong_constants" 'sub nsw i32' "multi-inst-opt @test_wrong_constants (no opt)"
assert_fn_contains "$MIO_OUT" "test_sub_wrong_order" 'add nsw i32' "multi-inst-opt @test_sub_wrong_order (no opt)"
assert_fn_contains "$MIO_OUT" "test_div_mul_no_opt" 'mul nsw i32' "multi-inst-opt @test_div_mul_no_opt (div then mul unsafe)"
assert_fn_contains "$MIO_OUT" "test_div_mul_power2_no_opt" 'mul nsw i32' "multi-inst-opt @test_div_mul_power2_no_opt (div then mul unsafe)"
echo "PASS: multi instruction optimization"

echo
echo "All first-assignment tests passed."
