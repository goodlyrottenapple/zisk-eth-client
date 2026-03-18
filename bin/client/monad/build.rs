use std::env;
use std::fs;
use std::path::Path;
use std::process::Command;

fn main() {
    let target = env::var("TARGET").unwrap();
    let cpp_dir = Path::new("cpp");

    // Determine which Make target to build
    let make_target = if target.contains("zkvm") || target.contains("riscv") {
        "zisk"
    } else {
        "x86"
    };

    println!("cargo:warning=Building for target: {} using make target: {}", target, make_target);

    // For RISC-V targets, pass CC/CXX/AR from cargo env to the Makefile so both
    // Rust and C++ use the same toolchain (configured in .cargo/config.toml).
    let mut make_args: Vec<String> = vec![make_target.to_string()];
    if make_target != "x86" {
        let target_env = target.replace('-', "_");

        if let Some(cc) = get_target_env("CC", &target_env) {
            println!("cargo:warning=Using CC from env: {}", cc);
            make_args.push(format!("CC_RISCV64={}", cc));

            // Derive CXX from CC if CXX is not explicitly set.
            // Handles both "gcc" -> "g++" and "cc" -> "c++" conventions.
            let cxx = get_target_env("CXX", &target_env).unwrap_or_else(|| {
                derive_cxx_from_cc(&cc)
            });
            println!("cargo:warning=Using CXX: {}", cxx);
            make_args.push(format!("CXX_RISCV64={}", cxx));

            // Derive RISCV_TOOLCHAIN_DIR from CC path for cmake builds.
            // CC is typically at <toolchain_dir>/bin/<triple>-gcc
            if let Some(toolchain_dir) = derive_toolchain_dir(&cc) {
                println!("cargo:warning=Using RISCV_TOOLCHAIN_DIR: {}", toolchain_dir);
                make_args.push(format!("RISCV_TOOLCHAIN_DIR={}", toolchain_dir));
            }
        }

        if let Some(ar) = get_target_env("AR", &target_env) {
            println!("cargo:warning=Using AR from env: {}", ar);
            make_args.push(format!("AR_RISCV64={}", ar));
        }
    }

    // Run make with the appropriate target and toolchain overrides
    let make_str_args: Vec<&str> = make_args.iter().map(|s| s.as_str()).collect();
    run_make(&make_str_args, cpp_dir);

    // Determine the library path based on the target
    let lib_path = cpp_dir.join("build").join(make_target);

    // Link the static library
    println!("cargo:rustc-link-search=native={}", lib_path.display());
    println!("cargo:rustc-link-lib=static=monad_wrapper");

    // For RISC-V, don't link libgcc — Rust's compiler_builtins provides all
    // needed runtime helpers (__clzdi2, __bswapdi2, __bswapsi2, etc.)
    if make_target == "zisk" {
        // Force 8-byte alignment for .srodata.cst4 sections (ZisK VM requires it)
        let align_ld = env::current_dir().unwrap().join("align.ld");
        println!("cargo:rustc-link-arg=-T{}", align_ld.display());
        // Strip unused sections (C++ templates instantiate all EVM revisions,
        // but we only use EVMC_CANCUN)
        println!("cargo:rustc-link-arg=--gc-sections");
    } else {
        // For x86, link C++ standard library and precompile dependencies
        println!("cargo:rustc-link-lib=stdc++");
        println!("cargo:rustc-link-lib=crypto++");
        println!("cargo:rustc-link-lib=gmp");
    }

    // Trigger rebuild if source files or toolchain config change
    println!("cargo:rerun-if-changed=cpp/Makefile");
    let target_env = target.replace('-', "_");
    println!("cargo:rerun-if-env-changed=CC_{}", target_env);
    println!("cargo:rerun-if-env-changed=CXX_{}", target_env);
    println!("cargo:rerun-if-env-changed=AR_{}", target_env);

    // Track all project source files for rebuilds (skip third_party/)
    track_source_files(cpp_dir, true);

    // Track monad submodule sources specifically
    track_source_files(&cpp_dir.join("third_party/monad/category/vm"), false);
    track_source_files(&cpp_dir.join("third_party/monad/category/zkvm"), false);
}

/// Look up a target-specific env var. Checks `<VAR>_<target>` first, then `<VAR>`.
fn get_target_env(var: &str, target_env: &str) -> Option<String> {
    // Try target-specific form first: e.g. CC_riscv64ima_zisk_zkvm_elf
    env::var(format!("{}_{}", var, target_env))
        .ok()
        .or_else(|| env::var(var).ok())
        .filter(|v| !v.is_empty())
}

/// Derive C++ compiler path from C compiler path.
/// "riscv64-unknown-elf-gcc" -> "riscv64-unknown-elf-g++"
/// "/path/to/bin/riscv64-none-elf-gcc" -> "/path/to/bin/riscv64-none-elf-g++"
/// Also handles "cc" -> "c++" as a fallback.
fn derive_cxx_from_cc(cc: &str) -> String {
    if cc.ends_with("-gcc") || cc == "gcc" {
        // Replace trailing "gcc" with "g++"
        format!("{}g++", &cc[..cc.len() - 3])
    } else if cc.ends_with("-cc") || cc == "cc" {
        format!("{}c++", &cc[..cc.len() - 2])
    } else {
        // Unknown pattern — append "++" and hope for the best
        format!("{}++", cc)
    }
}

/// Derive the toolchain root directory from a compiler path.
/// "/nix/store/.../bin/riscv64-none-elf-gcc" -> "/nix/store/..."
fn derive_toolchain_dir(cc: &str) -> Option<String> {
    let path = Path::new(cc);
    // Only works for absolute paths with a bin/ parent
    path.parent()
        .filter(|p| p.file_name().map_or(false, |n| n == "bin"))
        .and_then(|p| p.parent())
        .map(|p| p.to_string_lossy().into_owned())
}

fn track_source_files(dir: &Path, skip_third_party: bool) {
    if let Ok(entries) = fs::read_dir(dir) {
        for entry in entries.flatten() {
            let path = entry.path();

            if path.is_dir() {
                // Skip third_party/ when recursing from project root
                if skip_third_party && path.file_name().map_or(false, |n| n == "third_party") {
                    continue;
                }
                track_source_files(&path, false);
            } else if let Some(ext) = path.extension() {
                // Track files with relevant extensions
                let ext_str = ext.to_string_lossy();
                if ext_str == "h" || ext_str == "hpp" || ext_str == "c" || ext_str == "cpp" || ext_str == "S" {
                    println!("cargo:rerun-if-changed={}", path.display());
                }
            }
        }
    }
}

fn run_make(args: &[&str], dir: &Path) {
    let status = Command::new("make")
        .args(args)
        .current_dir(dir)
        .status()
        .unwrap_or_else(|e| panic!("Failed to execute make: {}", e));

    if !status.success() {
        panic!("make {:?} failed with exit code {:?}", args, status.code());
    }
}
