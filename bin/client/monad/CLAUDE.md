# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is the **Monad EVM Client** (`zec-monad`), part of the Zisk Ethereum Client project. It executes Ethereum blockchain tests inside the ZisK zkVM (zero-knowledge virtual machine) using Monad's high-performance EVM interpreter. The architecture is a hybrid Rust/C++ system connected via FFI.

The Monad interpreter is GPL v3 licensed.

## Build Commands

### Build for x86 (native development/testing)
```bash
# Build C++ library only
cd cpp && make x86

# Build full Rust+C++ project
cargo build --release
```

### Build for RISC-V (ZisK target)
```bash
# Requires ZisK RISC-V toolchain at ~/.zisk/riscv/bin/
cargo-zisk build --release
```

### Run in ZisK emulator
```bash
cargo-zisk run --release -i ./inputs/<block>.bin
```

### Clean C++ build artifacts
```bash
cd cpp && make clean
```


## Architecture

### Execution Flow
```
main.rs (Zisk entrypoint)
  -> ffi.rs (Rust FFI bindings)
    -> wrapper.cpp (C FFI bridge)
      -> evmone_test/ (blockchain test loader/runner)
      -> evmone_host/ (EVMC host, state, MPT, precompiles)
      -> interpreter/ (Monad EVM bytecode interpreter)
      -> runtime/ (uint256, memory, storage, gas)
```

### Key Source Files
- `src/main.rs` - Reads input via `ziskos::read_input_slice()`, calls `execute_blockchain_test_safe()`, writes result via `ziskos::set_output()`
- `src/ffi.rs` - Rust FFI declarations and safe wrappers for C++ functions
- `cpp/wrapper.cpp` - C FFI entry points: `execute_evm_bytecode()` and `execute_blockchain_test()`. Owns the global `TestVM` instance
- `cpp/evmone_host/host.cpp` - EVMC Host interface implementation (state access, calls, storage)
- `cpp/evmone_host/state.cpp` - Account/storage state management
- `cpp/evmone_host/mpt_hash.cpp` - Merkle Patricia Trie hashing for state root verification
- `cpp/evmone_test/blockchaintest_runner.cpp` - `apply_block()` and `validate_block()` functions
- `cpp/evmone_test/blockchaintest_loader.cpp` - JSON blockchain test parser

### Build System
- `build.rs` detects target arch, runs `make zisk`, `make sp1`, or `make x86` in `cpp/`, then links `libmonad_wrapper.a`
- `cpp/Makefile` is the primary C++ build config. Compiles with C++23, `-O3`. RISC-V uses bare-metal flags (`-nostdlib++`, `-fno-exceptions`, `-fno-rtti`)
- x86 uses `g++-15`, RISC-V uses a nix-built GCC 15 cross-compiler (toolchain resolved via `nix-build` from the monad submodule's `riscv64-none-elf.nix`)
- Patches are applied to ethash via `make apply-patches` (runs automatically)

### Monad Submodule (`cpp/third_party/monad`)
The Monad EVM interpreter and precompile implementations live in the `third_party/monad` git submodule. The cmake entry point is `category/zkvm/CMakeLists.txt`, which builds a standalone `libmonad-zkvm.a` without the full monad dependency tree.

**cmake build flow** (driven by Makefile targets):
1. cmake configures from `$(MONAD_ZKVM_SRC)` (`category/zkvm/`) with a backend flag (`native`, `zisk`, or `sp1`)
2. cmake builds `libmonad-zkvm.a` plus precompile dependency libraries (libsilkpre, libff, libsecp256k1, libblst, libc-kzg-4844)
3. Makefile extracts `.o` files from the cmake archives into `build/<target>/monad_objs/` and `build/<target>/precompile_deps_objs/`
4. Extracted objects are linked into the final `libmonad_wrapper.a` alongside project-specific objects

**Per-target cmake configuration:**
- **x86**: `MONAD_ZKVM_BACKEND=native`, uses `gcc-avx2.cmake` toolchain. Precompile deps (silkpre, libff, secp256k1, blst, c-kzg-4844) extracted separately into `precompile_deps_objs/`
- **ZisK**: `MONAD_ZKVM_BACKEND=zisk`, uses `riscv64-elf-toolchain.cmake`. Precompile deps bundled inside `libmonad-zkvm.a`
- **SP1**: `MONAD_ZKVM_BACKEND=sp1`, same toolchain with `rv64im` arch (no atomics)

**Important**: keccak objects are removed from the extracted monad archive (`rm -f $@/keccak*`) because we use our own patched ethash keccak for now.

### Third-Party Libraries (cpp/third_party/)
- `monad` - Git submodule containing the Monad EVM interpreter, precompiles, and their dependencies (intx, evmc, ethash, silkpre, libff, secp256k1, blst, c-kzg-4844). Built via cmake (see above)
- `ethash` - Keccak hashing (patched for Zisk) — our local copy overrides the one inside monad
- `nlohmann/json.hpp` - JSON parsing
- `unordered_dense` - Fast hash maps
- `gmp/mini-gmp` - Minimal GMP for RISC-V bare-metal

### Dual Compilation Model
All C++ code compiles for both x86 (native testing) and RISC-V (ZisK proving). RISC-V builds use `cpp/libc_riscv/` for bare-metal C library stubs. When adding new C++ files, update both build paths in the Makefile.

## Scripts

### `scripts/prepare_tests.sh`
Downloads and converts Ethereum blockchain test fixtures to binary format for the Cancun revision. Steps:
1. Clones `ethereum/tests` repo (pinned to commit `e46e1db503`) into `tmp/`
2. Downloads `execution-spec-tests` fixtures (v5.4.0) into `tmp/`
3. Builds the `ethtest_converter` tool via `make -C cpp converter`
4. Converts all JSON tests to `.bin` files in `build/tests/` (filtered to Cancun revision only)

```bash
./scripts/prepare_tests.sh
```

### `scripts/run_tests.sh`
Runs the converted binary tests on x86, RISC-V, or both, and compares results. Includes an exclusion list for slow, unsupported, or known-failing tests (blobs, EIP-7610 collisions, time-consuming state tests, etc.).

```bash
# Run on both x86 and RISC-V (default)
./scripts/run_tests.sh

# Run on x86 only
./scripts/run_tests.sh --x86-only

# Run on RISC-V only
./scripts/run_tests.sh --riscv-only

# Filter tests by path substring
./scripts/run_tests.sh --filter "stEIP1559"
```

Tests are expected to output "Test passed" on success. When both targets run, the script reports agreement/disagreement and exits non-zero if any tests disagree.

## Current Status

The client executes Ethereum blockchain tests (JSON format from ethereum/tests). The flow is: JSON test data -> parse -> initialize pre-state -> execute blocks with Monad interpreter -> verify state roots, receipts roots, gas used -> verify final state hash. See `plan.md` for the development roadmap.

## Sibling Clients

This repo also contains `bin/client/rsp/` (production, based on Succinct's RSP) and `bin/client/zeth/` (WIP, based on Boundless's Zeth). The Monad client follows the same pattern: read input via ziskos, execute EVM, output result.
