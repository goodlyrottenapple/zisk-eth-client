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
- `build.rs` detects target arch, runs `make riscv` or `make x86` in `cpp/`, then links `libmonad_wrapper.a`
- `cpp/Makefile` is the primary C++ build config. Compiles with C++23, `-O3`. RISC-V uses bare-metal flags (`-nostdlib++`, `-fno-exceptions`, `-fno-rtti`)
- x86 uses `g++-15`, RISC-V uses the ZisK cross-compiler
- BLST (BLS12-381) is excluded from RISC-V builds (x86/ARM assembly only)
- RISC-V uses mini-gmp instead of system GMP
- Patches are applied to ethash via `make apply-patches` (runs automatically)

### Third-Party Libraries (cpp/third_party/)
- `intx` - 256-bit integers
- `ethash` - Keccak hashing (patched for Zisk)
- `silkpre` - Ethereum precompile implementations (includes libff for BN254, secp256k1)
- `blst` - BLS12-381 (x86 only)
- `evmc` - Ethereum VM interface standard
- `nlohmann/json.hpp` - JSON parsing
- `unordered_dense` - Fast hash maps
- `gmp/mini-gmp` - Minimal GMP for RISC-V bare-metal

### Dual Compilation Model
All C++ code compiles for both x86 (native testing) and RISC-V (ZisK proving). RISC-V builds use `cpp/libc_riscv/` for bare-metal C library stubs. When adding new C++ files, update both build paths in the Makefile.

## Current Status

The client executes Ethereum blockchain tests (JSON format from ethereum/tests). The flow is: JSON test data -> parse -> initialize pre-state -> execute blocks with Monad interpreter -> verify state roots, receipts roots, gas used -> verify final state hash. See `plan.md` for the development roadmap.

## Sibling Clients

This repo also contains `bin/client/rsp/` (production, based on Succinct's RSP) and `bin/client/zeth/` (WIP, based on Boundless's Zeth). The Monad client follows the same pattern: read input via ziskos, execute EVM, output result.
