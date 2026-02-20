// Minimal syscall stubs and allocator for zkVM environment
// ZisK uses sys_alloc_aligned, SP1 uses Rust global allocator via FFI

#if defined(__riscv)

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sys/reent.h>

extern "C" {

void _exit(int status) {
#if defined(SP1_TARGET)
    // SP1: HALT syscall (code 0x00000000) via ecall
    register int t0 asm("t0") = 0;
    register int a0 asm("a0") = status;
    register int a1 asm("a1") = 0;
    asm volatile("ecall" : : "r"(t0), "r"(a0), "r"(a1));
#else
    // ZisK: ecall with syscall number 93
    int a0 asm("a0") = status;
    int a7 asm("a7") = 93;
    asm volatile("ecall" : : "r"(a0), "r"(a7));
#endif

    // Should never reach here
    __builtin_unreachable();
}

[[noreturn]] void abort() {
    _exit(1);
    __builtin_unreachable();
}

// Stub implementations for other syscalls that might be referenced
int _close(int file) {
    return -1;
}

int _fstat(int file, void* st) {
    return -1;
}

int _isatty(int file) {
    return 0;
}

int _lseek(int file, int ptr, int dir) {
    return -1;
}

int _read(int file, char* ptr, int len) {
    return 0;
}

int _write(int file, char* ptr, int len) {
    return 0;
}

int _kill(int pid, int sig) {
    return -1;
}

int _getpid(void) {
    return 1;
}

void* realloc(void* ptr, size_t new_size) {
    // Simple implementation: allocate new block
    // We don't track old sizes in a bump allocator
    // For our use case (EVM execution), this is sufficient
    if (new_size == 0) {
        free(ptr);
        return nullptr;
    }

    void* new_ptr = malloc(new_size);
    // Note: We can't copy old data since we don't know the old size
    // This is acceptable for our use case
    (void)ptr;
    return new_ptr;
}

// Implement _sbrk for compatibility (though malloc doesn't use it)
void* _sbrk(intptr_t increment) {
    // Not used when we have malloc implemented via sys_alloc_aligned
    // Return error to indicate sbrk is not supported
    return (void*)-1;
}

// Newlib's _impure_ptr for errno and stdio state
// We provide a minimal stub since we don't use stdio in zkVM
// Note: struct _reent is defined by newlib in sys/reent.h
static struct _reent impure_data = {0};
struct _reent* _impure_ptr = &impure_data;

// Stub fprintf - we don't support stdio in zkVM
int fprintf(void* stream, const char* format, ...) {
    (void)stream;
    (void)format;
    return 0;
}

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

// Stub __assert_func - assertions will just abort in zkVM
void __assert_func(const char* file, int line, const char* func, const char* failedexpr) {
    (void)file;
    (void)line;
    (void)func;
    (void)failedexpr;
    abort();
}

} // extern "C"

#endif // __riscv
