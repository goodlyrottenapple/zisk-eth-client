// Compatibility layer for mini-gmp to provide missing mpn functions needed by libff
// These functions are required by libff but not provided by mini-gmp

#include <gmp.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

// Helper function to convert mpn limb array to mpz
static void mpn_to_mpz(mpz_t result, mp_srcptr limbs, mp_size_t n) {
    mpz_set_ui(result, 0);
    for (mp_size_t i = n; i > 0; i--) {
        mpz_mul_2exp(result, result, GMP_NUMB_BITS);
        mpz_add_ui(result, result, limbs[i-1]);
    }
}

// Helper function to convert mpz to mpn limb array
// Returns the number of limbs written
static mp_size_t mpz_to_mpn(mp_ptr limbs, mp_size_t max_limbs, mpz_srcptr value) {
    mpz_t tmp;
    mpz_init_set(tmp, value);

    mp_size_t n = 0;
    while (mpz_sgn(tmp) > 0 && n < max_limbs) {
        limbs[n] = mpz_get_ui(tmp);
        mpz_fdiv_q_2exp(tmp, tmp, GMP_NUMB_BITS);
        n++;
    }

    mpz_clear(tmp);
    return n;
}

// Implementation of mpn_gcdext using mpz-level operations
// Computes gcd(a,b) and coefficient s such that gcd = s*a + t*b
// Returns the size of the gcd result
mp_size_t mpn_gcdext(mp_ptr gp, mp_ptr sp, mp_size_t *sn,
                     mp_ptr ap, mp_size_t an, mp_ptr bp, mp_size_t bn) {
    mpz_t a, b, g, s, t;
    mpz_init(a);
    mpz_init(b);
    mpz_init(g);
    mpz_init(s);
    mpz_init(t);

    // Convert input limb arrays to mpz
    mpn_to_mpz(a, ap, an);
    mpn_to_mpz(b, bp, bn);

    // Compute extended GCD
    mpz_gcdext(g, s, t, a, b);

    // Convert gcd result back to limb array
    mp_size_t gn = mpz_to_mpn(gp, an + bn, g);

    // Convert s coefficient back to limb array if requested
    if (sp && sn) {
        mp_size_t abs_sn = mpz_to_mpn(sp, an + bn, s);
        *sn = mpz_sgn(s) < 0 ? -abs_sn : abs_sn;
    }

    mpz_clear(a);
    mpz_clear(b);
    mpz_clear(g);
    mpz_clear(s);
    mpz_clear(t);

    return gn;
}

// Implementation of mpn_tdiv_qr using mpz-level operations
// Computes quotient and remainder of n/d
void mpn_tdiv_qr(mp_ptr qp, mp_ptr rp, mp_size_t qxn,
                 mp_srcptr np, mp_size_t nn, mp_srcptr dp, mp_size_t dn) {
    (void)qxn; // Unused parameter

    mpz_t n, d, q, r;
    mpz_init(n);
    mpz_init(d);
    mpz_init(q);
    mpz_init(r);

    // Convert numerator and denominator
    mpn_to_mpz(n, np, nn);
    mpn_to_mpz(d, dp, dn);

    // Compute truncated division (rounds toward zero)
    mpz_tdiv_qr(q, r, n, d);

    // Convert results back to limb arrays
    mpz_to_mpn(qp, nn, q);
    if (rp) {
        mpz_to_mpn(rp, dn, r);
    }

    mpz_clear(n);
    mpz_clear(d);
    mpz_clear(q);
    mpz_clear(r);
}

// Stub implementation of gmp_printf
// In bare-metal environment, we don't have printf, so this is a no-op
// This function is only used for debug output in libff
int gmp_printf(const char *fmt, ...) {
    (void)fmt;
    // No-op in bare-metal environment
    return 0;
}
