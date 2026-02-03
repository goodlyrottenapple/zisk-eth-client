// Compatibility header for mini-gmp that adds missing mpn function declarations
#ifndef GMP_COMPAT_H
#define GMP_COMPAT_H

// Include the base mini-gmp header
#include "mini-gmp.h"

#ifdef __cplusplus
extern "C" {
#endif

// Extended GCD function declaration
mp_size_t mpn_gcdext(mp_ptr gp, mp_ptr sp, mp_size_t *sn,
                     mp_ptr ap, mp_size_t an, mp_ptr bp, mp_size_t bn);

// Truncated division with quotient and remainder
void mpn_tdiv_qr(mp_ptr qp, mp_ptr rp, mp_size_t qxn,
                 mp_srcptr np, mp_size_t nn, mp_srcptr dp, mp_size_t dn);

// Stub for gmp_printf (not needed in bare-metal, just for compatibility)
int gmp_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // GMP_COMPAT_H
