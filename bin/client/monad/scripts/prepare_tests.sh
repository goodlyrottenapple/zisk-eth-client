#!/bin/bash
# Convert Ethereum blockchain test JSON files to binary format for Cancun revision.
# Downloads test fixtures if not present, builds the converter, then converts all tests.
#
# Usage: ./scripts/prepare_tests.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CPP_DIR="$PROJECT_DIR/cpp"
CONVERTER="$CPP_DIR/build/x86/ethtest_converter"
OUTPUT_DIR="$PROJECT_DIR/build/tests"

# Test sources — downloaded into tmp/
TMP_DIR="$PROJECT_DIR/tmp"
ETHEREUM_TESTS_REPO="https://github.com/ethereum/tests"
ETHEREUM_TESTS_COMMIT="e46e1db503"
ETHEREUM_TESTS="$TMP_DIR/ethereum-tests/BlockchainTests"

EXEC_SPEC_URL="https://github.com/ethereum/execution-spec-tests/releases/download/v5.4.0/fixtures_develop.tar.gz"
EXEC_SPEC_SHA256="3e2b02d49fe903eda4fd8caca5cbf0d139c470e97e1de9a85299b1b034f97099"
EXEC_SPEC_TESTS="$TMP_DIR/execution-spec-tests/fixtures/blockchain_tests"

# --- Download test fixtures if missing ---

mkdir -p "$TMP_DIR"

if [ ! -d "$ETHEREUM_TESTS" ]; then
    echo "Cloning ethereum/tests at $ETHEREUM_TESTS_COMMIT..."
    git clone "$ETHEREUM_TESTS_REPO" "$TMP_DIR/ethereum-tests"
    git -C "$TMP_DIR/ethereum-tests" checkout "$ETHEREUM_TESTS_COMMIT"
    echo "Cloned ethereum-tests."
fi

if [ ! -d "$EXEC_SPEC_TESTS" ]; then
    echo "Downloading execution-spec-tests fixtures..."
    TARBALL="$TMP_DIR/fixtures_develop.tar.gz"
    curl -fSL "$EXEC_SPEC_URL" -o "$TARBALL"
    echo "$EXEC_SPEC_SHA256  $TARBALL" | sha256sum -c -
    mkdir -p "$TMP_DIR/execution-spec-tests"
    tar xzf "$TARBALL" -C "$TMP_DIR/execution-spec-tests"
    rm "$TARBALL"
    echo "Downloaded execution-spec-tests fixtures."
fi

# --- Build converter ---

if [ ! -f "$CONVERTER" ]; then
    echo "Building ethtest_converter..."
    make -C "$CPP_DIR" converter
fi

# --- Convert tests ---

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

converted=0
skipped=0

convert_test() {
    local json_file="$1"
    local bin_file="$2"

    mkdir -p "$(dirname "$bin_file")"

    # Converter exits non-zero if no tests match the revision filter
    local output
    output=$("$CONVERTER" "$json_file" "$bin_file" --rev Cancun 2>&1)
    local exit_code=$?
    if [ $exit_code -eq 0 ]; then
        if [ -f "$bin_file" ] && [ -s "$bin_file" ]; then
            return 0
        fi
    elif [ $exit_code -ge 128 ]; then
        echo "CRASH (exit $exit_code): $json_file" >&2
        echo "$output" >&2
    fi
    rm -f "$bin_file"
    return 1
}

echo "=== Converting ethereum-tests (--rev Cancun) ==="
while IFS= read -r json; do
    rel="$(realpath --relative-to="$ETHEREUM_TESTS" "$json")"
    bin_file="$OUTPUT_DIR/ethereum-tests/${rel%.json}.bin"
    if convert_test "$json" "$bin_file"; then
        converted=$((converted + 1))
    else
        skipped=$((skipped + 1))
    fi
done < <(find "$ETHEREUM_TESTS" -name '*.json' -type f | sort)

echo "  Converted: $converted  Skipped (no Cancun tests): $skipped"

et_converted=$converted
converted=0
skipped=0

echo "=== Converting execution-spec-tests (--rev Cancun) ==="
while IFS= read -r json; do
    rel="$(realpath --relative-to="$EXEC_SPEC_TESTS" "$json")"
    bin_file="$OUTPUT_DIR/exec-spec/${rel%.json}.bin"
    if convert_test "$json" "$bin_file"; then
        converted=$((converted + 1))
    else
        skipped=$((skipped + 1))
    fi
done < <(find "$EXEC_SPEC_TESTS" -name '*.json' -type f | sort)

echo "  Converted: $converted  Skipped (no Cancun tests): $skipped"

total=$(find "$OUTPUT_DIR" -name '*.bin' -type f | wc -l)
echo ""
echo "=== Done ==="
echo "Total binary test files: $total"
echo "Output directory: $OUTPUT_DIR"
