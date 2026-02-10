// Stub implementations for libff profiling functions
// These are not needed for the actual cryptographic operations
// but are referenced by the libff code

#include <string>

namespace libff {

// Global profiling control variables
bool inhibit_profiling_info = true;
bool inhibit_profiling_counters = true;

// Stub profiling functions
void enter_block(const std::string&, bool) {
    // No-op stub
}

void leave_block(const std::string&, bool) {
    // No-op stub
}

} // namespace libff
