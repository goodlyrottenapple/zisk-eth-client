// Minimal C++ library stubs for RISC-V bare metal environment
// C library functions are provided by newlib sources in libc_riscv/ directory
// This file only contains C++ standard library stubs that are not in libstdc++
#if defined(__riscv)

#include <cstddef>

extern "C" {
    void abort();  // Provided by syscalls_stubs.cpp
    void* malloc(size_t size);  // Provided by syscalls_stubs.cpp
    void free(void* ptr);  // Provided by syscalls_stubs.cpp

    // DSO handle for dynamic shared objects
    // In bare-metal RISC-V, we provide a dummy handle
    void* __dso_handle = nullptr;

    // C++ ABI function for global destructors
    // We don't support proper global destructors in bare-metal RISC-V environment
    int __cxa_atexit(void (*)(void *), void *, void *) {
        return 0;  // Success - but we won't actually call the destructor
    }

    // C++ guard functions for static local variable initialization
    // In bare-metal RISC-V, we assume single-threaded execution
    int __cxa_guard_acquire(long long* guard) {
        // Check if already initialized (bit 0 is set)
        if (*guard & 1) {
            return 0;  // Already initialized
        }
        return 1;  // Need to initialize
    }

    void __cxa_guard_release(long long* guard) {
        // Mark as initialized
        *guard = 1;
    }

    void __cxa_guard_abort(long long* guard) {
        // Initialization failed, reset guard
        *guard = 0;
    }
}

// C++ standard library stubs
namespace std {
    [[noreturn]] void __throw_bad_function_call() {
        abort();
    }

    [[noreturn]] void __throw_length_error(const char*) {
        abort();
    }

    [[noreturn]] void __throw_logic_error(const char*) {
        abort();
    }
}

// C++ new/delete operators
void* operator new(size_t size) {
    return malloc(size);
}

void* operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void* ptr) noexcept {
    free(ptr);
}

void operator delete[](void* ptr) noexcept {
    free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    free(ptr);
}

#endif