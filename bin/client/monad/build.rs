use std::env;
use std::fs;
use std::path::Path;
use std::process::Command;

fn main() {
    let target = env::var("TARGET").unwrap();
    let cpp_dir = Path::new("cpp");

    // Determine which Make target to build
    let make_target = if target.contains("zkvm") || target.contains("riscv") {
        "riscv"
    } else {
        "x86"
    };

    println!("cargo:warning=Building for target: {} using make target: {}", target, make_target);

    // Run make with the appropriate target
    run_make(&[make_target], cpp_dir);

    // Determine the library path based on the target
    let lib_subdir = if make_target == "riscv" { "riscv" } else { "x86" };
    let lib_path = cpp_dir.join("build").join(lib_subdir);

    // Link the static library
    println!("cargo:rustc-link-search=native={}", lib_path.display());
    println!("cargo:rustc-link-lib=static=monad_wrapper");

    // For RISC-V, don't link libgcc to avoid relocation issues with __clzdi2
    // We provide our own implementations of required libgcc functions in runtime_math.cpp
    // and use newlib sources from cpp/libc_riscv/
    if make_target == "riscv" {
        // No libgcc linking for RISC-V
    } else {
        // For x86, link C++ standard library
        println!("cargo:rustc-link-lib=stdc++");
    }

    // Trigger rebuild if source files change
    println!("cargo:rerun-if-changed=cpp/Makefile");

    // Track all source files for rebuilds
    track_source_files(cpp_dir);
}

fn track_source_files(dir: &Path) {
    if let Ok(entries) = fs::read_dir(dir) {
        for entry in entries.flatten() {
            let path = entry.path();

            if path.is_dir() {
                // Recursively track files in subdirectories
                track_source_files(&path);
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
