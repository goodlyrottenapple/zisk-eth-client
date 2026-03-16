// Minimal C++ library stubs for RISC-V bare metal environment
// This file only contains C++ standard library stubs that are not in libstdc++
#if defined(__riscv)

#include <cstddef>
#include <system_error>

extern "C" {
    void abort();  // Provided by syscalls_stubs.cpp

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
}

// __cow_string stubs (used by error_category::_M_message)
namespace std {
    __cow_string::__cow_string() { _M_p = ""; }
    __cow_string::__cow_string(const std::string&) { _M_p = ""; }
    __cow_string::__cow_string(const char*, size_t) { _M_p = ""; }
    __cow_string::__cow_string(const __cow_string&) noexcept { _M_p = ""; }
    __cow_string& __cow_string::operator=(const __cow_string&) noexcept { return *this; }
    __cow_string::~__cow_string() {}
    __cow_string::__cow_string(__cow_string&&) noexcept { _M_p = ""; }
    __cow_string& __cow_string::operator=(__cow_string&&) noexcept { return *this; }
}

// std::error_category base class virtual function stubs
namespace std {
inline namespace _V2 {
    error_category::~error_category() {}

    __cow_string error_category::_M_message(int) const {
        return __cow_string();
    }

    error_condition error_category::default_error_condition(int ev) const noexcept {
        return error_condition(ev, *this);
    }

    bool error_category::equivalent(int code, const error_condition& condition) const noexcept {
        return default_error_condition(code) == condition;
    }

    bool error_category::equivalent(const error_code& code, int condition) const noexcept {
        return *this == code.category() && code.value() == condition;
    }
} // inline namespace _V2
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

#endif
