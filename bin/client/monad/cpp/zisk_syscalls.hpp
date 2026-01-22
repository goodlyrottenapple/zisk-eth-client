// Zisk syscall integration for Monad EVM interpreter
// This header provides FFI bindings to call Zisk syscalls implemented in Rust
#pragma once

#include <cstdint>

#if defined(__riscv) && __riscv_xlen == 64

// Forward declare the Rust syscall functions
// These are defined in ziskos/entrypoint/src/syscalls/
extern "C" {

/// Syscall parameters structure matching Rust's SyscallAdd256Params
/// Defined in zisk/ziskos/entrypoint/src/syscalls/add256.rs
struct SyscallAdd256Params {
    const uint64_t* a;  // &[u64; 4]
    const uint64_t* b;  // &[u64; 4]
    uint64_t cin;       // u64
    uint64_t* c;        // &mut [u64; 4]
};

/// Execute Add256 syscall: c = a + b + cin (mod 2^256)
/// Returns carry out (0 or 1)
///
/// Implemented in: zisk/ziskos/entrypoint/src/syscalls/add256.rs
/// Uses CSR 0x811 to trigger Zisk Add256 precompile
uint64_t syscall_add256(SyscallAdd256Params* params);

// TODO: Add other syscalls as needed (arith256, etc.)
// extern uint64_t syscall_arith256(...);

} // extern "C"

namespace zisk::syscalls {

/// C++ wrapper for syscall_add256 with convenient interface
///
/// @param a Pointer to 4x u64 array (256-bit operand A)
/// @param b Pointer to 4x u64 array (256-bit operand B)
/// @param cin Carry in (0 or 1)
/// @param c Pointer to 4x u64 array for result
/// @return Carry out (0 or 1)
[[gnu::always_inline]]
inline uint64_t add256(
    const uint64_t* a,
    const uint64_t* b,
    uint64_t cin,
    uint64_t* c) noexcept
{
    SyscallAdd256Params params{a, b, cin, c};
    return syscall_add256(&params);
}

} // namespace zisk::syscalls

#endif // __riscv && __riscv_xlen == 64
