//! Implementations of the `zkvm_accelerators.h` C interface.
//!
//! Each function is `#[no_mangle] pub extern "C"` so the linker resolves them
//! against the C++ static library (`libmonad_wrapper.a`) which calls these
//! from `precompiles_impl.cpp`.

const ZKVM_EOK: i32 = 0;
const ZKVM_EFAIL: i32 = -1;

// ---------------------------------------------------------------------------
// Hashing
// ---------------------------------------------------------------------------

#[no_mangle]
pub unsafe extern "C" fn zkvm_keccak256(
    data: *const u8,
    len: usize,
    output: *mut [u8; 32],
) -> i32 {
    use tiny_keccak::{Hasher, Keccak};

    let input = core::slice::from_raw_parts(data, len);
    let mut hasher = Keccak::v256();
    hasher.update(input);
    hasher.finalize(&mut *output);
    ZKVM_EOK
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_sha256(
    data: *const u8,
    len: usize,
    output: *mut [u8; 32],
) -> i32 {
    use sha2::{Digest, Sha256};

    let input = core::slice::from_raw_parts(data, len);
    let hash = Sha256::digest(input);
    (*output).copy_from_slice(&hash);
    ZKVM_EOK
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_ripemd160(
    data: *const u8,
    len: usize,
    output: *mut [u8; 32],
) -> i32 {
    use ripemd::{Digest, Ripemd160};

    let input = core::slice::from_raw_parts(data, len);
    let hash = Ripemd160::digest(input);
    let out = &mut *output;
    out[..12].fill(0);
    out[12..32].copy_from_slice(&hash);
    ZKVM_EOK
}

// ---------------------------------------------------------------------------
// secp256k1
// ---------------------------------------------------------------------------

#[no_mangle]
pub unsafe extern "C" fn zkvm_secp256k1_ecrecover(
    msg: *const [u8; 32],
    sig: *const [u8; 64],
    recid: u8,
    output: *mut [u8; 64],
) -> i32 {
    use k256::ecdsa::{RecoveryId, Signature, VerifyingKey};

    let signature = match Signature::from_slice(&*sig) {
        Ok(s) => s,
        Err(_) => return ZKVM_EFAIL,
    };

    let recovery_id = RecoveryId::new(recid != 0, false);

    let recovered_key =
        match VerifyingKey::recover_from_prehash(&*msg as &[u8], &signature, recovery_id) {
            Ok(k) => k,
            Err(_) => return ZKVM_EFAIL,
        };

    // Uncompressed SEC1: 0x04 || x(32) || y(32).  We want x || y.
    let encoded = recovered_key.to_encoded_point(false);
    let bytes = encoded.as_bytes();
    (*output).copy_from_slice(&bytes[1..65]);
    ZKVM_EOK
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_secp256k1_verify(
    msg: *const [u8; 32],
    sig: *const [u8; 64],
    pubkey: *const [u8; 64],
    verified: *mut bool,
) -> i32 {
    use k256::ecdsa::{signature::hazmat::PrehashVerifier, Signature, VerifyingKey};
    use k256::EncodedPoint;

    let signature = match Signature::from_slice(&*sig) {
        Ok(s) => s,
        Err(_) => {
            *verified = false;
            return ZKVM_EOK;
        }
    };

    let mut uncompressed = [0u8; 65];
    uncompressed[0] = 0x04;
    uncompressed[1..65].copy_from_slice(&*pubkey);

    let point = match EncodedPoint::from_bytes(&uncompressed) {
        Ok(p) => p,
        Err(_) => {
            *verified = false;
            return ZKVM_EOK;
        }
    };

    let vk = match VerifyingKey::from_encoded_point(&point) {
        Ok(k) => k,
        Err(_) => {
            *verified = false;
            return ZKVM_EOK;
        }
    };

    *verified = vk.verify_prehash(&*msg as &[u8], &signature).is_ok();
    ZKVM_EOK
}

// ---------------------------------------------------------------------------
// Modular exponentiation
// ---------------------------------------------------------------------------

#[cfg(all(target_os = "zkvm", target_vendor = "zisk"))]
#[no_mangle]
pub unsafe extern "C" fn zkvm_modexp(
    base: *const u8,
    base_len: usize,
    exp: *const u8,
    exp_len: usize,
    modulus: *const u8,
    mod_len: usize,
    output: *mut u8,
) -> i32 {
    let base_slice = core::slice::from_raw_parts(base, base_len);
    let exp_slice = core::slice::from_raw_parts(exp, exp_len);
    let mod_slice = core::slice::from_raw_parts(modulus, mod_len);

    let result = aurora_engine_modexp::modexp(base_slice, exp_slice, mod_slice);

    let out_slice = core::slice::from_raw_parts_mut(output, mod_len);
    if result.len() >= mod_len {
        out_slice.copy_from_slice(&result[result.len() - mod_len..]);
    } else {
        out_slice[..mod_len - result.len()].fill(0);
        out_slice[mod_len - result.len()..].copy_from_slice(&result);
    }
    ZKVM_EOK
}

#[cfg(not(all(target_os = "zkvm", target_vendor = "zisk")))]
#[no_mangle]
pub unsafe extern "C" fn zkvm_modexp(
    _base: *const u8,
    _base_len: usize,
    _exp: *const u8,
    _exp_len: usize,
    _modulus: *const u8,
    _mod_len: usize,
    _output: *mut u8,
) -> i32 {
    ZKVM_EFAIL
}

// ---------------------------------------------------------------------------
// BN254
// ---------------------------------------------------------------------------

#[cfg(all(target_os = "zkvm", target_vendor = "zisk"))]
mod bn254 {
    use super::{ZKVM_EOK, ZKVM_EFAIL};

    fn read_g1(bytes: &[u8; 64]) -> Result<substrate_bn::G1, ()> {
        use substrate_bn::{AffineG1, Fq, Group, G1};

        let x = Fq::from_slice(&bytes[..32]).map_err(|_| ())?;
        let y = Fq::from_slice(&bytes[32..64]).map_err(|_| ())?;

        if x.is_zero() && y.is_zero() {
            Ok(G1::zero())
        } else {
            AffineG1::new(x, y).map(Into::into).map_err(|_| ())
        }
    }

    fn write_g1(p: &substrate_bn::G1, out: &mut [u8; 64]) {
        use substrate_bn::{AffineG1, Group};

        if p.is_zero() {
            out.fill(0);
        } else {
            let affine = AffineG1::from_jacobian(*p).unwrap();
            affine.x().to_big_endian(&mut out[..32]).unwrap();
            affine.y().to_big_endian(&mut out[32..64]).unwrap();
        }
    }

    fn read_g2(bytes: &[u8]) -> Result<substrate_bn::G2, ()> {
        use substrate_bn::{AffineG2, Fq, Fq2, Group, G2};

        let x_imag = Fq::from_slice(&bytes[0..32]).map_err(|_| ())?;
        let x_real = Fq::from_slice(&bytes[32..64]).map_err(|_| ())?;
        let y_imag = Fq::from_slice(&bytes[64..96]).map_err(|_| ())?;
        let y_real = Fq::from_slice(&bytes[96..128]).map_err(|_| ())?;

        let x = Fq2::new(x_real, x_imag);
        let y = Fq2::new(y_real, y_imag);

        if x.is_zero() && y.is_zero() {
            Ok(G2::zero())
        } else {
            AffineG2::new(x, y).map(Into::into).map_err(|_| ())
        }
    }

    #[no_mangle]
    pub unsafe extern "C" fn zkvm_bn254_g1_add(
        p1: *const [u8; 64],
        p2: *const [u8; 64],
        result: *mut [u8; 64],
    ) -> i32 {
        let a = match read_g1(&*p1) {
            Ok(p) => p,
            Err(_) => return ZKVM_EFAIL,
        };
        let b = match read_g1(&*p2) {
            Ok(p) => p,
            Err(_) => return ZKVM_EFAIL,
        };

        let sum = a + b;
        write_g1(&sum, &mut *result);
        ZKVM_EOK
    }

    #[no_mangle]
    pub unsafe extern "C" fn zkvm_bn254_g1_mul(
        point: *const [u8; 64],
        scalar: *const [u8; 32],
        result: *mut [u8; 64],
    ) -> i32 {
        use substrate_bn::Fr;

        let p = match read_g1(&*point) {
            Ok(p) => p,
            Err(_) => return ZKVM_EFAIL,
        };

        let s = Fr::from_slice(&*scalar).unwrap_or(Fr::zero());
        let product = p * s;
        write_g1(&product, &mut *result);
        ZKVM_EOK
    }

    #[no_mangle]
    pub unsafe extern "C" fn zkvm_bn254_pairing(
        pairs: *const u8,
        num_pairs: usize,
        verified: *mut bool,
    ) -> i32 {
        let mut bn_pairs = Vec::with_capacity(num_pairs);

        for i in 0..num_pairs {
            let base = pairs.add(i * 192);

            let g1_bytes: &[u8; 64] = &*(base as *const [u8; 64]);
            let g1 = match read_g1(g1_bytes) {
                Ok(p) => p,
                Err(_) => return ZKVM_EFAIL,
            };

            let g2_bytes = core::slice::from_raw_parts(base.add(64), 128);
            let g2 = match read_g2(g2_bytes) {
                Ok(p) => p,
                Err(_) => return ZKVM_EFAIL,
            };

            bn_pairs.push((g1, g2));
        }

        *verified = substrate_bn::pairing_batch(&bn_pairs) == substrate_bn::Gt::one();
        ZKVM_EOK
    }
}

#[cfg(not(all(target_os = "zkvm", target_vendor = "zisk")))]
mod bn254 {
    use super::ZKVM_EFAIL;

    #[no_mangle]
    pub unsafe extern "C" fn zkvm_bn254_g1_add(
        _p1: *const [u8; 64],
        _p2: *const [u8; 64],
        _result: *mut [u8; 64],
    ) -> i32 {
        ZKVM_EFAIL
    }

    #[no_mangle]
    pub unsafe extern "C" fn zkvm_bn254_g1_mul(
        _point: *const [u8; 64],
        _scalar: *const [u8; 32],
        _result: *mut [u8; 64],
    ) -> i32 {
        ZKVM_EFAIL
    }

    #[no_mangle]
    pub unsafe extern "C" fn zkvm_bn254_pairing(
        _pairs: *const u8,
        _num_pairs: usize,
        _verified: *mut bool,
    ) -> i32 {
        ZKVM_EFAIL
    }
}

// ---------------------------------------------------------------------------
// BLAKE2f — TODO
// ---------------------------------------------------------------------------

#[no_mangle]
pub unsafe extern "C" fn zkvm_blake2f(
    _rounds: u32,
    _h: *mut [u8; 64],
    _m: *const [u8; 128],
    _t: *const [u8; 16],
    _f: u8,
) -> i32 {
    ZKVM_EFAIL // TODO: implement with blake2f.rs
}

// ---------------------------------------------------------------------------
// KZG point evaluation — TODO
// ---------------------------------------------------------------------------

#[no_mangle]
pub unsafe extern "C" fn zkvm_kzg_point_eval(
    _commitment: *const [u8; 48],
    _z: *const [u8; 32],
    _y: *const [u8; 32],
    _proof: *const [u8; 48],
    _verified: *mut bool,
) -> i32 {
    ZKVM_EFAIL // TODO: implement with kzg-rs
}

// ---------------------------------------------------------------------------
// BLS12-381 — TODO
// ---------------------------------------------------------------------------

#[no_mangle]
pub unsafe extern "C" fn zkvm_bls12_g1_add(
    _p1: *const [u8; 96],
    _p2: *const [u8; 96],
    _result: *mut [u8; 96],
) -> i32 {
    ZKVM_EFAIL
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_bls12_g1_msm(
    _pairs: *const u8,
    _num_pairs: usize,
    _result: *mut [u8; 96],
) -> i32 {
    ZKVM_EFAIL
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_bls12_g2_add(
    _p1: *const [u8; 192],
    _p2: *const [u8; 192],
    _result: *mut [u8; 192],
) -> i32 {
    ZKVM_EFAIL
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_bls12_g2_msm(
    _pairs: *const u8,
    _num_pairs: usize,
    _result: *mut [u8; 192],
) -> i32 {
    ZKVM_EFAIL
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_bls12_pairing(
    _pairs: *const u8,
    _num_pairs: usize,
    _verified: *mut bool,
) -> i32 {
    ZKVM_EFAIL
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_bls12_map_fp_to_g1(
    _fp: *const [u8; 48],
    _result: *mut [u8; 96],
) -> i32 {
    ZKVM_EFAIL
}

#[no_mangle]
pub unsafe extern "C" fn zkvm_bls12_map_fp2_to_g2(
    _fp2: *const [u8; 96],
    _result: *mut [u8; 192],
) -> i32 {
    ZKVM_EFAIL
}

// ---------------------------------------------------------------------------
// P-256 (secp256r1) — TODO
// ---------------------------------------------------------------------------

#[no_mangle]
pub unsafe extern "C" fn zkvm_secp256r1_verify(
    _msg: *const [u8; 32],
    _sig: *const [u8; 64],
    _pubkey: *const [u8; 64],
    _verified: *mut bool,
) -> i32 {
    ZKVM_EFAIL
}
