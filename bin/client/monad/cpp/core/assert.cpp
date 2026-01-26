// Copyright (C) 2025 - Assertion implementation for Monad VM
// Minimal implementation for the assertion mechanism

#include "assert.h"

extern "C" {

void __attribute__((noreturn)) monad_vm_assertion_failed(
    char const *expr, char const *function, char const *file, long line)
{
    // In a zkVM environment, we can't use abort or exit
    // Just trigger an infinite loop to halt execution
    while (1) {
        // Infinite loop to halt
    }
    __builtin_unreachable();
}

} // extern "C"
