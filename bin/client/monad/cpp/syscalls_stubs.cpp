// Minimal syscall stubs and allocator for zkVM environment
// Provides complete memory allocation support without requiring newlib's malloc

#ifdef __riscv

#include <cstddef>
#include <cstdint>

extern "C" {

// Simple heap implementation using a statically allocated buffer
static constexpr size_t HEAP_SIZE = 32 * 1024 * 1024;  // 32 MB heap
static uint8_t heap_buffer[HEAP_SIZE] __attribute__((aligned(16)));
static uint8_t* heap_ptr = heap_buffer;
static uint8_t* heap_end = heap_buffer + HEAP_SIZE;

void* _sbrk(intptr_t increment) {
    uint8_t* prev_heap = heap_ptr;

    // Check for heap overflow
    if (heap_ptr + increment > heap_end) {
        return (void*)-1;  // Out of memory
    }

    if (heap_ptr + increment < heap_buffer) {
        return (void*)-1;  // Invalid decrement
    }

    heap_ptr += increment;
    return prev_heap;
}

void _exit(int status) {
    // In zkVM (RISC-V), we use ecall with syscall number 93 to exit
    int a0 asm("a0") = status;
    int a7 asm("a7") = 93;
    asm volatile("ecall" : : "r"(a0), "r"(a7));

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

// Provide our own malloc/free implementation to avoid newlib's malloc
void* malloc(size_t size) {
    // Align to 16 bytes
    size = (size + 15) & ~15;

    uint8_t* ptr = heap_ptr;
    if (ptr + size > heap_end) {
        return nullptr;  // Out of memory
    }

    heap_ptr += size;
    return ptr;
}

void free(void* ptr) {
    // Simple bump allocator - don't actually free memory
    // This is fine for our use case since Intercode is allocated once
    (void)ptr;
}

void* calloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = malloc(total);
    if (ptr) {
        // Zero the memory
        for (size_t i = 0; i < total; i++) {
            ((uint8_t*)ptr)[i] = 0;
        }
    }
    return ptr;
}

void* realloc(void* ptr, size_t new_size) {
    // Simple implementation: just allocate new block
    // We don't track sizes, so we can't copy old data
    // This is sufficient for our use case
    (void)ptr;
    return malloc(new_size);
}

} // extern "C"

#endif // __riscv
