// Minimal stubs for symbols referenced by C++ code but not provided by the zkVM runtime.
// The zkVM environment has no stdio or process control — these are no-ops.

#if defined(__riscv)

#include <cstddef>
#include <cstdint>
#include <sys/reent.h>
#include <category/zkvm/zkvm_exit.h>

extern "C" {

[[noreturn]] void abort() {
    zkvm_exit(1);
}

// Newlib's _impure_ptr for errno and stdio state
// We provide a minimal stub since we don't use stdio in zkVM
// Note: struct _reent is defined by newlib in sys/reent.h
static struct _reent impure_data = {0};
struct _reent* _impure_ptr = &impure_data;

// Stub printf - we don't support stdio in zkVM
int printf(const char* format, ...) {
    (void)format;
    return 0;
}

// Stub puts - we don't support stdio in zkVM
int puts(const char* s) {
    (void)s;
    return 0;
}

// Stub fwrite - referenced by intx::to_string via snprintf internals
size_t fwrite(const void* ptr, size_t size, size_t count, void* stream) {
    (void)ptr;
    (void)size;
    (void)stream;
    return count;
}

} // extern "C"

#endif // __riscv
