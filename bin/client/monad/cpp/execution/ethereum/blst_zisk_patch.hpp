// Copyright (C) 2025 Category Labs, Inc.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <blst.h>
#include <cstdint>
#include <cstring>

// This header provides zisk-patched implementations of BLST functions for RISC-V
// Based on: https://github.com/0xPolygonHermez/zisk-patch-blst/tree/zisk-patch/v0.3.15

#if defined(__riscv)

// Zisk BLS12-381 library C function declarations
extern "C" {
    // G1 operations (12 u64s = 96 bytes)
    bool add_bls12_381_c(uint64_t* p1, const uint64_t* p2);
    void scalar_mul_bls12_381_c(uint64_t* ret, const uint64_t* p, const uint64_t* k);
    bool is_on_curve_bls12_381_c(const uint64_t* p);
    bool is_on_subgroup_bls12_381_c(const uint64_t* p);
    uint8_t decompress_bls12_381_c(uint64_t* ret, const uint8_t* input);

    // G2 operations (24 u64s = 192 bytes)
    bool add_twist_bls12_381_c(uint64_t* p1, const uint64_t* p2);
    void scalar_mul_twist_bls12_381_c(uint64_t* ret, const uint64_t* p, const uint64_t* k);
    bool is_on_curve_twist_bls12_381_c(const uint64_t* p);
    bool is_on_subgroup_twist_bls12_381_c(const uint64_t* p);
    uint8_t decompress_twist_bls12_381_c(uint64_t* ret, const uint8_t* input);

    // Pairing operations (72 u64s = 576 bytes for Fp12)
    void miller_loop_bls12_381_c(uint64_t* ret, const uint64_t* q, const uint64_t* p);
    void final_exp_bls12_381_c(uint64_t* f);
    void mul_fp12_bls12_381_c(uint64_t* ret, const uint64_t* a, const uint64_t* b);
}

namespace blst_zisk {

// Helper: Check if point is at infinity (all zeros)
inline bool is_inf(const uint64_t* p, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (p[i] != 0) return false;
    }
    return true;
}

// Helper: Convert blst_fp (Montgomery form) to standard form
inline void fp_from_montgomery(uint64_t* std_out, const blst_fp* mont_in) {
    blst_fp_from((blst_fp*)std_out, mont_in);
}

// Helper: Convert standard form to blst_fp (Montgomery form)
inline void fp_to_montgomery(blst_fp* mont_out, const uint64_t* std_in) {
    blst_fp_to(mont_out, (const blst_fp*)std_in);
}

} // namespace blst_zisk

// Patched G1 addition (replaces blst_p1_add_or_double_affine)
inline void blst_p1_add_or_double_affine_zisk(
    blst_p1* out,
    const blst_p1* a,
    const blst_p1_affine* b)
{
    using namespace blst_zisk;

    // Handle infinity cases
    if (blst_p1_is_inf(a)) {
        blst_p1_from_affine(out, b);
        return;
    }
    if (blst_p1_affine_is_inf(b)) {
        *out = *a;
        return;
    }

    // Convert a from projective to affine
    blst_p1_affine a_affine;
    blst_p1_to_affine(&a_affine, a);

    // Convert both points from Montgomery to standard form (2 Fp elements each = 12 u64s)
    uint64_t std_a[12] = {0};
    uint64_t std_b[12] = {0};

    fp_from_montgomery(&std_a[0], &a_affine.x);
    fp_from_montgomery(&std_a[6], &a_affine.y);
    fp_from_montgomery(&std_b[0], &b->x);
    fp_from_montgomery(&std_b[6], &b->y);

    // Call zisk implementation (modifies std_a in place)
    bool is_infinity = add_bls12_381_c(std_a, std_b);

    if (is_infinity) {
        std::memset(out, 0, sizeof(blst_p1));
        return;
    }

    // Convert result back to Montgomery form
    blst_p1_affine ret_affine;
    fp_to_montgomery(&ret_affine.x, &std_a[0]);
    fp_to_montgomery(&ret_affine.y, &std_a[6]);

    // Convert affine back to projective
    blst_p1_from_affine(out, &ret_affine);
}

// Patched G1 scalar multiplication (replaces blst_p1_mult)
inline void blst_p1_mult_zisk(
    blst_p1* out,
    const blst_p1* p,
    const uint8_t* scalar,
    size_t nbits)
{
    using namespace blst_zisk;

    // Handle infinity
    if (blst_p1_is_inf(p)) {
        std::memset(out, 0, sizeof(blst_p1));
        return;
    }

    // Check for zero/one scalar
    size_t nbytes = (nbits + 7) / 8;
    bool scalar_is_zero = true;
    bool scalar_is_one = (scalar[0] == 1);

    for (size_t i = 0; i < nbytes && i < 48; i++) {
        if (scalar[i] != 0) scalar_is_zero = false;
        if (i == 0) {
            if (scalar[i] != 1) scalar_is_one = false;
        } else if (scalar[i] != 0) {
            scalar_is_one = false;
        }
    }

    if (scalar_is_zero) {
        std::memset(out, 0, sizeof(blst_p1));
        return;
    }

    if (scalar_is_one) {
        *out = *p;
        return;
    }

    // Convert projective to affine
    blst_p1_affine p_affine;
    blst_p1_to_affine(&p_affine, p);

    // Convert from Montgomery to standard form
    uint64_t std_p[12] = {0};
    fp_from_montgomery(&std_p[0], &p_affine.x);
    fp_from_montgomery(&std_p[6], &p_affine.y);

    // Convert scalar to [u64; 6] (little-endian)
    uint64_t k[6] = {0};
    std::memcpy(k, scalar, (nbytes < 48) ? nbytes : 48);

    // Call zisk implementation
    uint64_t std_ret[12];
    scalar_mul_bls12_381_c(std_ret, std_p, k);

    // Convert result back to Montgomery form
    blst_p1_affine ret_affine;
    fp_to_montgomery(&ret_affine.x, &std_ret[0]);
    fp_to_montgomery(&ret_affine.y, &std_ret[6]);

    // Convert affine back to projective
    blst_p1_from_affine(out, &ret_affine);
}

// Patched G2 addition (replaces blst_p2_add_or_double_affine)
inline void blst_p2_add_or_double_affine_zisk(
    blst_p2* out,
    const blst_p2* a,
    const blst_p2_affine* b)
{
    using namespace blst_zisk;

    // Handle infinity cases
    if (blst_p2_is_inf(a)) {
        blst_p2_from_affine(out, b);
        return;
    }
    if (blst_p2_affine_is_inf(b)) {
        *out = *a;
        return;
    }

    // Convert a from projective to affine
    blst_p2_affine a_affine;
    blst_p2_to_affine(&a_affine, a);

    // Convert both points from Montgomery to standard form (4 Fp elements each = 24 u64s)
    uint64_t std_a[24] = {0};
    uint64_t std_b[24] = {0};

    // Each Fp2 is 2 Fp elements, so x is 2 Fp and y is 2 Fp
    fp_from_montgomery(&std_a[0], &a_affine.x.fp[0]);
    fp_from_montgomery(&std_a[6], &a_affine.x.fp[1]);
    fp_from_montgomery(&std_a[12], &a_affine.y.fp[0]);
    fp_from_montgomery(&std_a[18], &a_affine.y.fp[1]);

    fp_from_montgomery(&std_b[0], &b->x.fp[0]);
    fp_from_montgomery(&std_b[6], &b->x.fp[1]);
    fp_from_montgomery(&std_b[12], &b->y.fp[0]);
    fp_from_montgomery(&std_b[18], &b->y.fp[1]);

    // Call zisk implementation (modifies std_a in place)
    bool is_infinity = add_twist_bls12_381_c(std_a, std_b);

    if (is_infinity) {
        std::memset(out, 0, sizeof(blst_p2));
        return;
    }

    // Convert result back to Montgomery form
    blst_p2_affine ret_affine;
    fp_to_montgomery(&ret_affine.x.fp[0], &std_a[0]);
    fp_to_montgomery(&ret_affine.x.fp[1], &std_a[6]);
    fp_to_montgomery(&ret_affine.y.fp[0], &std_a[12]);
    fp_to_montgomery(&ret_affine.y.fp[1], &std_a[18]);

    // Convert affine back to projective
    blst_p2_from_affine(out, &ret_affine);
}

// Patched G2 scalar multiplication (replaces blst_p2_mult)
inline void blst_p2_mult_zisk(
    blst_p2* out,
    const blst_p2* p,
    const uint8_t* scalar,
    size_t nbits)
{
    using namespace blst_zisk;

    // Handle infinity
    if (blst_p2_is_inf(p)) {
        std::memset(out, 0, sizeof(blst_p2));
        return;
    }

    // Check for zero/one scalar
    size_t nbytes = (nbits + 7) / 8;
    bool scalar_is_zero = true;
    bool scalar_is_one = (scalar[0] == 1);

    for (size_t i = 0; i < nbytes && i < 48; i++) {
        if (scalar[i] != 0) scalar_is_zero = false;
        if (i == 0) {
            if (scalar[i] != 1) scalar_is_one = false;
        } else if (scalar[i] != 0) {
            scalar_is_one = false;
        }
    }

    if (scalar_is_zero) {
        std::memset(out, 0, sizeof(blst_p2));
        return;
    }

    if (scalar_is_one) {
        *out = *p;
        return;
    }

    // Convert projective to affine
    blst_p2_affine p_affine;
    blst_p2_to_affine(&p_affine, p);

    // Convert from Montgomery to standard form (4 Fp elements = 24 u64s)
    uint64_t std_p[24] = {0};
    fp_from_montgomery(&std_p[0], &p_affine.x.fp[0]);
    fp_from_montgomery(&std_p[6], &p_affine.x.fp[1]);
    fp_from_montgomery(&std_p[12], &p_affine.y.fp[0]);
    fp_from_montgomery(&std_p[18], &p_affine.y.fp[1]);

    // Convert scalar to [u64; 6] (little-endian)
    uint64_t k[6] = {0};
    std::memcpy(k, scalar, (nbytes < 48) ? nbytes : 48);

    // Call zisk implementation
    uint64_t std_ret[24];
    scalar_mul_twist_bls12_381_c(std_ret, std_p, k);

    // Convert result back to Montgomery form
    blst_p2_affine ret_affine;
    fp_to_montgomery(&ret_affine.x.fp[0], &std_ret[0]);
    fp_to_montgomery(&ret_affine.x.fp[1], &std_ret[6]);
    fp_to_montgomery(&ret_affine.y.fp[0], &std_ret[12]);
    fp_to_montgomery(&ret_affine.y.fp[1], &std_ret[18]);

    // Convert affine back to projective
    blst_p2_from_affine(out, &ret_affine);
}

#endif // __riscv
