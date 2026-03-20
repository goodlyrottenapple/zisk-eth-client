#!/bin/bash
# Run Cancun blockchain tests on x86 and RISC-V, then compare results.
# Both targets run every (non-excluded) test. The final report shows
# whether x86 and RISC-V agree on each test (both pass or both fail).
#
# Usage: ./scripts/run_tests.sh [--x86-only] [--riscv-only] [--filter PATTERN]

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_DIR="$PROJECT_DIR/build/tests"

RUN_X86=true
RUN_RISCV=true
FILTER=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --riscv-only) RUN_X86=false; shift ;;
        --x86-only)   RUN_RISCV=false; shift ;;
        --filter)     FILTER="$2"; shift 2 ;;
        *)            echo "Unknown arg: $1"; exit 1 ;;
    esac
done

if [ ! -d "$TEST_DIR" ]; then
    echo "ERROR: Test directory not found: $TEST_DIR"
    echo "Run ./scripts/prepare_tests.sh first."
    exit 1
fi

# --- Exclusion patterns (from Monad's cancun.cmake + hardcoded slow tests) ---

EXCLUDE_PATTERNS=(
    # Hardcoded slow tests
    "GeneralStateTests/stTimeConsuming/"
    "GeneralStateTests/VMTests/vmPerformance/"
    "GeneralStateTests/stQuadraticComplexityTest/Call50000_sha256"
    "ValidBlocks/bcForkStressTest/ForkStressTest"

    # Blobs (EIP-4844)
    "Pyspecs/cancun/eip4844_blobs/"
    "cancun/eip4844_blobs/"

    # withdrawals root
    "InvalidBlocks/bc4895_withdrawals/incorrectWithdrawalsRoot"

    # EIP-1559 invalid blocks
    "InvalidBlocks/bcEIP1559/"

    # Stricter validation
    "InvalidBlocks/bc4895_withdrawals/accountInteractions"
    "InvalidBlocks/bcEIP3675/timestampPerBlock"
    "InvalidBlocks/bcInvalidHeaderTest/badTimestamp"
    "InvalidBlocks/bcInvalidHeaderTest/timeDiff0"
    "InvalidBlocks/bcInvalidHeaderTest/wrongCoinbase"
    "InvalidBlocks/bcInvalidHeaderTest/wrongGasLimit"
    "InvalidBlocks/bcInvalidHeaderTest/wrongParentHash"
    "InvalidBlocks/bcInvalidHeaderTest/wrongParentHash2"
    "InvalidBlocks/bcInvalidHeaderTest/wrongReceiptTrie"
    "InvalidBlocks/bcInvalidHeaderTest/wrongStateRoot"
    "InvalidBlocks/bcInvalidHeaderTest/wrongTimestamp"
    "InvalidBlocks/bcInvalidHeaderTest/wrongTransactionsTrie"
    "InvalidBlocks/bcMultiChainTest/UncleFromSideChain"
    "InvalidBlocks/bcStateTests/CreateTransactionReverted"
    "InvalidBlocks/bcStateTests/RefundOverflow"
    "InvalidBlocks/bcStateTests/RefundOverflow2"
    "InvalidBlocks/bcStateTests/callcodeOutput2"
    "InvalidBlocks/bcStateTests/createNameRegistratorPerTxsNotEnoughGasAt"
    "InvalidBlocks/bcStateTests/dataTx"

    # Slow + stack-heavy tests
    "GeneralStateTests/stBadOpcode/"
    "GeneralStateTests/stBugs/"
    "GeneralStateTests/stCallCodes/"
    "GeneralStateTests/stCallCreateCallCodeTest/"
    "GeneralStateTests/stCallDelegateCodesCallCodeHomestead/"
    "GeneralStateTests/stCallDelegateCodesHomestead/"
    "GeneralStateTests/stCreate2/"
    "GeneralStateTests/stCreateTest/"

    # Stricter base fee validation
    "london/validation/test_invalid_header"

    # EIP-7610 create collision
    "stCreate2/RevertInCreateInInitCreate2Paris"
    "stCreate2/create2collisionStorageParis"
    "stExtCodeHash/dynamicAccountOverwriteEmpty_Paris"
    "stRevertTest/RevertInCreateInInit_Paris"
    "stSStoreTest/InitCollisionParis"
    "eip7610_create_collision/"
    "state_tests/stCreate2/RevertInCreateInInitCreate2Paris"
    "state_tests/stCreate2/create2collisionStorageParis"
    "state_tests/stExtCodeHash/dynamicAccountOverwriteEmpty_Paris"
)

is_excluded() {
    local path="$1"
    for pattern in "${EXCLUDE_PATTERNS[@]}"; do
        if [[ "$path" == *"$pattern"* ]]; then
            return 0
        fi
    done
    return 1
}

# --- Collect test files ---

mapfile -t test_files < <(find "$TEST_DIR" -name '*.bin' -type f | sort)
total=${#test_files[@]}

if [ "$total" -eq 0 ]; then
    echo "No test files found in $TEST_DIR"
    exit 1
fi

# Filter and exclude
active_files=()
skipped=0
for bin_file in "${test_files[@]}"; do
    rel="$(realpath --relative-to="$TEST_DIR" "$bin_file")"
    if [ -n "$FILTER" ] && [[ "$rel" != *"$FILTER"* ]]; then
        skipped=$((skipped + 1))
        continue
    fi
    if is_excluded "$rel"; then
        skipped=$((skipped + 1))
        continue
    fi
    active_files+=("$bin_file")
done

active=${#active_files[@]}
echo "Found $total test files ($active active, $skipped skipped)"

# --- Run a single test, return "pass" or "fail" ---

run_one_x86() {
    local output
    output="$(cargo run --release -q -- -i "$1" 2>&1)" || true
    if [[ "$output" == *"Test passed"* ]]; then
        echo "pass"
    else
        echo "fail"
    fi
}

run_one_riscv() {
    local output
    output="$(cargo-zisk run --release -i "$1" 2>&1)" || true
    if [[ "$output" == *"Test passed"* ]]; then
        echo "pass"
    else
        echo "fail"
    fi
}

# --- Main loop ---

cd "$PROJECT_DIR"

# Build
if $RUN_X86; then
    echo "Building x86 release..."
    cargo build --release -q 2>/dev/null
fi
if $RUN_RISCV; then
    echo "Building RISC-V release..."
    cargo-zisk build --release 2>/dev/null
fi

# --- Run tests ---

echo ""
echo "========================================="
echo "  Running $active tests"
echo "========================================="

x86_passed=0; x86_failed=0
rv_passed=0; rv_failed=0
agree=0; disagree=0
disagree_list=()
i=0

for bin_file in "${active_files[@]}"; do
    i=$((i + 1))
    rel="$(realpath --relative-to="$TEST_DIR" "$bin_file")"

    x86_r="skip"; rv_r="skip"

    if $RUN_X86; then
        x86_r="$(run_one_x86 "$bin_file")"
        if [ "$x86_r" = "pass" ]; then
            x86_passed=$((x86_passed + 1))
        else
            x86_failed=$((x86_failed + 1))
        fi
    fi

    if $RUN_RISCV; then
        rv_r="$(run_one_riscv "$bin_file")"
        if [ "$rv_r" = "pass" ]; then
            rv_passed=$((rv_passed + 1))
        else
            rv_failed=$((rv_failed + 1))
        fi
    fi

    # Compare if both ran
    if $RUN_X86 && $RUN_RISCV; then
        if [ "$x86_r" = "$rv_r" ]; then
            agree=$((agree + 1))
        else
            disagree=$((disagree + 1))
            disagree_list+=("$rel  x86=$x86_r  riscv=$rv_r")
        fi
    fi

    # Status line
    status="[$i/$active]"
    if $RUN_X86 && $RUN_RISCV; then
        if [ "$x86_r" = "pass" ] && [ "$rv_r" = "pass" ]; then
            tag="PASS"
        elif [ "$x86_r" = "$rv_r" ]; then
            tag="FAIL (both)"
        else
            tag="DISAGREE x86=$x86_r riscv=$rv_r"
        fi
        echo "  $status $tag  $rel"
    elif $RUN_X86; then
        echo "  $status x86=$x86_r  $rel"
    else
        echo "  $status riscv=$rv_r  $rel"
    fi
done

# --- Summary ---

echo ""
echo "========================================="
echo "  Summary"
echo "========================================="
if $RUN_X86; then
    echo "  x86:    $x86_passed passed, $x86_failed failed (out of $active)"
fi
if $RUN_RISCV; then
    echo "  RISC-V: $rv_passed passed, $rv_failed failed (out of $active)"
fi

if $RUN_X86 && $RUN_RISCV; then
    echo "  Agree:    $agree / $active"
    echo "  Disagree: $disagree / $active"
    if [ ${#disagree_list[@]} -gt 0 ]; then
        echo ""
        echo "  Disagreements:"
        for d in "${disagree_list[@]}"; do
            echo "    $d"
        done
    fi

    echo ""
    if [ "$disagree" -eq 0 ] && [ "$x86_failed" -eq 0 ]; then
        echo "ALL $active TESTS PASSED on both x86 and RISC-V"
    elif [ "$disagree" -eq 0 ]; then
        echo "x86 and RISC-V AGREE on all $active tests ($x86_failed failures on both)"
    else
        echo "WARNING: $disagree tests DISAGREE between x86 and RISC-V"
        exit 1
    fi
fi
