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

    // Run make with the appropriate target
    run_make(&[make_target], cpp_dir);

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

    // Trigger rebuild if source files change
    println!("cargo:rerun-if-changed=cpp/Makefile");

    // Track all project source files for rebuilds (skip third_party/)
    track_source_files(cpp_dir, true);

    // Track monad submodule sources specifically
    track_source_files(&cpp_dir.join("third_party/monad/category/vm"), false);
    track_source_files(&cpp_dir.join("third_party/monad/category/zkvm"), false);
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
