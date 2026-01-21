// Minimal C++ library stubs for RISC-V bare metal environment
// C library functions are provided by newlib sources in libc_riscv/ directory
// This file only contains C++ standard library stubs that are not in libstdc++

#include <cstddef>

extern "C" {
    void abort();  // Provided by syscalls_stubs.cpp
    void* malloc(size_t size);  // Provided by syscalls_stubs.cpp
    void free(void* ptr);  // Provided by syscalls_stubs.cpp

    // C++ ABI function for global destructors
    // We don't support proper global destructors in bare-metal RISC-V environment
    int __cxa_atexit(void (*)(void *), void *, void *) {
        return 0;  // Success - but we won't actually call the destructor
    }
}

// C++ standard library stubs
namespace std {
    [[noreturn]] void __throw_bad_function_call() {
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
