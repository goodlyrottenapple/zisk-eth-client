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

#include "../../core/byte_string.hpp"
#include "../../core/config.hpp"
#include "precompiles.hpp"

#include <intx/intx.hpp>

#include <cstdint>
#include <optional>

// Zisk syscall declarations - implementations linked from ziskos at build time
// Following the pattern from runtime/math.hpp
extern "C" {
    struct SyscallPoint384 {
        uint64_t x[6];  // 384-bit x coordinate
        uint64_t y[6];  // 384-bit y coordinate
    };

    struct SyscallComplex384 {
        uint64_t x[6];  // 384-bit first component
        uint64_t y[6];  // 384-bit second component
    };

    struct SyscallBls12_381CurveAddParams {
        SyscallPoint384* p1;
        const SyscallPoint384* p2;
    };

    // BLS12-381 syscalls - linked from ziskos
    void syscall_bls12_381_curve_add(SyscallBls12_381CurveAddParams* params);
    void syscall_bls12_381_curve_dbl(SyscallPoint384* point);
    void syscall_bls12_381_complex_add(SyscallComplex384* result, const SyscallComplex384* a, const SyscallComplex384* b);
    void syscall_bls12_381_complex_sub(SyscallComplex384* result, const SyscallComplex384* a, const SyscallComplex384* b);
    void syscall_bls12_381_complex_mul(SyscallComplex384* result, const SyscallComplex384* a, const SyscallComplex384* b);
}

MONAD_NAMESPACE_BEGIN

namespace bls12
{
    using namespace intx::literals;

    inline constexpr auto BASE_FIELD_MODULUS =
        0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab_u384;

    // Type aliases - map BLST types to ziskos syscall structures
    // This allows precompiles_bls12.cpp to work with both x86 (BLST) and RISC-V (syscalls)
    using blst_fp = SyscallComplex384;     // Field element (384-bit)
    using blst_fp2 = SyscallComplex384;    // Fp2 element (two 384-bit components)
    using blst_p1_affine = SyscallPoint384;  // G1 point in affine coordinates
    using blst_p2_affine = SyscallPoint384;  // G2 point in affine coordinates
    using blst_p1 = SyscallPoint384;       // G1 point (no distinction between affine/projective for syscalls)
    using blst_p2 = SyscallPoint384;       // G2 point

    // 256-bit scalar for multiplication
    struct blst_scalar {
        uint64_t l[4];  // 256-bit scalar as 4 × u64
    };

    // Function declarations - implemented in precompiles_bls12.cpp
    template <typename Group>
    uint16_t msm_discount(uint64_t);

    blst_scalar read_scalar(uint8_t const *);
    std::optional<blst_fp> read_fp(uint8_t const *);
    std::optional<blst_fp2> read_fp2(uint8_t const *);
    std::optional<blst_p1_affine> read_g1(uint8_t const *);
    std::optional<blst_p2_affine> read_g2(uint8_t const *);

    void write_fp(blst_fp const &, uint8_t *);
    void write_fp2(blst_fp2 const &, uint8_t *);
    void write_g1(blst_p1_affine const &, uint8_t *);
    void write_g2(blst_p2_affine const &, uint8_t *);

    template <typename Group>
    PrecompileResult add(byte_string_view);

    template <typename Group>
    PrecompileResult msm(byte_string_view);

    template <typename Group>
    PrecompileResult mul(byte_string_view);

    template <typename Group>
    PrecompileResult msm_pippenger(byte_string_view, uint64_t);

    PrecompileResult pairing_check(byte_string_view);

    template <typename Group>
    PrecompileResult map_fp_to_g(byte_string_view);

    // G1 group operations using zisk syscalls
    // These methods match the interface from precompiles_bls12.hpp exactly
    struct G1
    {
        using FieldElement = blst_fp;
        using Point = blst_p1;
        using AffinePoint = blst_p1_affine;

        static constexpr auto element_encoded_size = 64;
        static constexpr auto encoded_size = 2 * element_encoded_size;

        [[gnu::always_inline]]
        static inline std::optional<blst_p1_affine> read(uint8_t const* data) {
            return read_g1(data);
        }

        [[gnu::always_inline]]
        static inline std::optional<blst_fp> read_element(uint8_t const* data) {
            return read_fp(data);
        }

        [[gnu::always_inline]]
        static inline void write(blst_p1_affine const& point, uint8_t* data) {
            write_g1(point, data);
        }

        // Point addition: result = a + b (using syscall)
        [[gnu::always_inline]]
        static inline void add(blst_p1* result, const blst_p1* a, const blst_p1_affine* b) {
            *result = *a;  // Copy a to result
            SyscallBls12_381CurveAddParams params = { result, b };
            syscall_bls12_381_curve_add(&params);
        }

        // Stub implementations for operations not yet available as syscalls
        // These will need proper implementation or additional syscalls
        [[gnu::always_inline]]
        static inline void map_to_group(blst_p1* result, const blst_fp* element) {
            // TODO: Implement hash-to-curve for G1
            (void)element;
            for (int i = 0; i < 6; i++) {
                result->x[i] = 0;
                result->y[i] = 0;
            }
        }

        [[gnu::always_inline]]
        static inline bool point_in_group(const blst_p1* point) {
            // TODO: Implement proper subgroup check
            // For now, check if point is not at infinity
            for (int i = 0; i < 6; i++) {
                if (point->x[i] != 0 || point->y[i] != 0) {
                    return true;
                }
            }
            return false;
        }

        [[gnu::always_inline]]
        static inline bool affine_point_in_group(const blst_p1_affine* point) {
            return point_in_group(point);
        }

        [[gnu::always_inline]]
        static inline bool affine_point_is_inf(const blst_p1_affine* point) {
            for (int i = 0; i < 6; i++) {
                if (point->x[i] != 0 || point->y[i] != 0) {
                    return false;
                }
            }
            return true;
        }

        // Scalar multiplication using double-and-add
        [[gnu::always_inline]]
        static inline void mul(blst_p1* result, const blst_p1* point, const blst_scalar* scalar, int nbits) {
            // Initialize result to point at infinity
            for (int i = 0; i < 6; i++) {
                result->x[i] = 0;
                result->y[i] = 0;
            }

            // Double-and-add algorithm
            for (int bit = nbits - 1; bit >= 0; bit--) {
                // Double
                syscall_bls12_381_curve_dbl(result);

                // Check if bit is set
                int word_idx = bit / 64;
                int bit_idx = bit % 64;
                if (word_idx < 4 && (scalar->l[word_idx] & (1ULL << bit_idx))) {
                    // Add
                    SyscallBls12_381CurveAddParams params = { result, point };
                    syscall_bls12_381_curve_add(&params);
                }
            }
        }

        [[gnu::always_inline]]
        static inline size_t msm_scratch_size(size_t npoints) {
            // Scratch space for Pippenger algorithm
            return npoints * sizeof(blst_p1) * 2;
        }

        [[gnu::always_inline]]
        static inline void msm(blst_p1* result, const blst_p1_affine* points[], size_t npoints,
                               const blst_scalar* scalars, size_t nbits, void* scratch) {
            // TODO: Implement optimized Pippenger MSM
            // For now, use naive approach: sum of scalar multiplications
            (void)scratch;

            for (int i = 0; i < 6; i++) {
                result->x[i] = 0;
                result->y[i] = 0;
            }

            for (size_t i = 0; i < npoints; i++) {
                blst_p1 temp;
                mul(&temp, points[i], &scalars[i], nbits);
                SyscallBls12_381CurveAddParams params = { result, &temp };
                syscall_bls12_381_curve_add(&params);
            }
        }

        // No-ops for affine/projective conversion (syscalls work on affine points)
        [[gnu::always_inline]]
        static inline void to_affine(blst_p1_affine* result, const blst_p1* point) {
            *result = *point;
        }

        [[gnu::always_inline]]
        static inline void from_affine(blst_p1* result, const blst_p1_affine* point) {
            *result = *point;
        }
    };

    // G2 group operations - same structure as G1
    struct G2
    {
        using FieldElement = blst_fp2;
        using Point = blst_p2;
        using AffinePoint = blst_p2_affine;

        static constexpr auto element_encoded_size = 2 * G1::element_encoded_size;
        static constexpr auto encoded_size = 2 * element_encoded_size;

        [[gnu::always_inline]]
        static inline std::optional<blst_p2_affine> read(uint8_t const* data) {
            return read_g2(data);
        }

        [[gnu::always_inline]]
        static inline std::optional<blst_fp2> read_element(uint8_t const* data) {
            return read_fp2(data);
        }

        [[gnu::always_inline]]
        static inline void write(blst_p2_affine const& point, uint8_t* data) {
            write_g2(point, data);
        }

        [[gnu::always_inline]]
        static inline void add(blst_p2* result, const blst_p2* a, const blst_p2_affine* b) {
            *result = *a;
            SyscallBls12_381CurveAddParams params = { result, b };
            syscall_bls12_381_curve_add(&params);
        }

        [[gnu::always_inline]]
        static inline void map_to_group(blst_p2* result, const blst_fp2* element) {
            // TODO: Implement hash-to-curve for G2
            (void)element;
            for (int i = 0; i < 6; i++) {
                result->x[i] = 0;
                result->y[i] = 0;
            }
        }

        [[gnu::always_inline]]
        static inline bool point_in_group(const blst_p2* point) {
            // TODO: Implement proper subgroup check
            for (int i = 0; i < 6; i++) {
                if (point->x[i] != 0 || point->y[i] != 0) {
                    return true;
                }
            }
            return false;
        }

        [[gnu::always_inline]]
        static inline bool affine_point_in_group(const blst_p2_affine* point) {
            return point_in_group(point);
        }

        [[gnu::always_inline]]
        static inline bool affine_point_is_inf(const blst_p2_affine* point) {
            for (int i = 0; i < 6; i++) {
                if (point->x[i] != 0 || point->y[i] != 0) {
                    return false;
                }
            }
            return true;
        }

        [[gnu::always_inline]]
        static inline void mul(blst_p2* result, const blst_p2* point, const blst_scalar* scalar, int nbits) {
            for (int i = 0; i < 6; i++) {
                result->x[i] = 0;
                result->y[i] = 0;
            }

            for (int bit = nbits - 1; bit >= 0; bit--) {
                syscall_bls12_381_curve_dbl(result);

                int word_idx = bit / 64;
                int bit_idx = bit % 64;
                if (word_idx < 4 && (scalar->l[word_idx] & (1ULL << bit_idx))) {
                    SyscallBls12_381CurveAddParams params = { result, point };
                    syscall_bls12_381_curve_add(&params);
                }
            }
        }

        [[gnu::always_inline]]
        static inline size_t msm_scratch_size(size_t npoints) {
            return npoints * sizeof(blst_p2) * 2;
        }

        [[gnu::always_inline]]
        static inline void msm(blst_p2* result, const blst_p2_affine* points[], size_t npoints,
                               const blst_scalar* scalars, size_t nbits, void* scratch) {
            (void)scratch;

            for (int i = 0; i < 6; i++) {
                result->x[i] = 0;
                result->y[i] = 0;
            }

            for (size_t i = 0; i < npoints; i++) {
                blst_p2 temp;
                mul(&temp, points[i], &scalars[i], nbits);
                SyscallBls12_381CurveAddParams params = { result, &temp };
                syscall_bls12_381_curve_add(&params);
            }
        }

        [[gnu::always_inline]]
        static inline void to_affine(blst_p2_affine* result, const blst_p2* point) {
            *result = *point;
        }

        [[gnu::always_inline]]
        static inline void from_affine(blst_p2* result, const blst_p2_affine* point) {
            *result = *point;
        }
    };

} // namespace bls12

MONAD_NAMESPACE_END
