use std::env;
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

    // Run make clean first
    run_make(&["clean"], cpp_dir);

    // Run make with the appropriate target
    run_make(&[make_target], cpp_dir);

    // Determine the library path based on the target
    let lib_subdir = if make_target == "riscv" { "riscv" } else { "x86" };
    let lib_path = cpp_dir.join("build").join(lib_subdir);

    // Link the static library
    println!("cargo:rustc-link-search=native={}", lib_path.display());
    println!("cargo:rustc-link-lib=static=monad_wrapper");

    // For RISC-V, link only libgcc (no libstdc++ or libc to avoid relocation issues)
    // We provide our own malloc/syscalls and use newlib sources from cpp/libc_riscv/
    if make_target == "riscv" {
        println!("cargo:rustc-link-search=native=/home/sam/.zisk/riscv/riscv64-unknown-elf/lib");
        println!("cargo:rustc-link-search=native=/home/sam/.zisk/riscv/lib/gcc/riscv64-unknown-elf/15.2.0");
        println!("cargo:rustc-link-lib=gcc");
    } else {
        // For x86, link C++ standard library
        println!("cargo:rustc-link-lib=stdc++");
    }

    // Trigger rebuild if source files change
    println!("cargo:rerun-if-changed=cpp/wrapper.cpp");
    println!("cargo:rerun-if-changed=cpp/Makefile");
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
