use std::env;
use std::fs;
use std::path::Path;
use std::process::Command;

fn main() {
    let target = env::var("TARGET").unwrap();
    // cpp symlink is at workspace root (monad-sp1/cpp -> ../monad/cpp)
    // build.rs runs from the program/ directory
    let cpp_dir = Path::new("../cpp");

    // Determine which Make target to build
    // SP1 v6 uses riscv64im-succinct-zkvm-elf
    let make_target = if target.contains("riscv") || target.contains("succinct") {
        "sp1"
    } else {
        "x86"
    };

    println!("cargo:warning=Building for target: {} using make target: {}", target, make_target);

    // Run make with the appropriate target
    run_make(&[make_target], cpp_dir);

    // Link the static library (use absolute path for linker)
    let lib_path = fs::canonicalize(cpp_dir.join("build").join(make_target))
        .unwrap_or_else(|e| panic!("Cannot resolve library path: {}", e));
    println!("cargo:rustc-link-search=native={}", lib_path.display());
    println!("cargo:rustc-link-lib=static=monad_wrapper");

    if make_target == "x86" {
        println!("cargo:rustc-link-lib=stdc++");
    }

    // Trigger rebuild if source files change
    println!("cargo:rerun-if-changed=../cpp/Makefile");
    track_source_files(cpp_dir);
}

fn track_source_files(dir: &Path) {
    if let Ok(entries) = fs::read_dir(dir) {
        for entry in entries.flatten() {
            let path = entry.path();

            if path.is_dir() {
                track_source_files(&path);
            } else if let Some(ext) = path.extension() {
                let ext_str = ext.to_string_lossy();
                if ext_str == "h" || ext_str == "hpp" || ext_str == "c" || ext_str == "cpp" || ext_str == "S" {
                    println!("cargo:rerun-if-changed={}", path.display());
                }
            }
        }
    }
}

fn run_make(args: &[&str], dir: &Path) {
    // SP1's build environment may have a minimal PATH — use absolute path
    let make = if Path::new("/usr/bin/make").exists() {
        "/usr/bin/make"
    } else {
        "make"
    };

    let status = Command::new(make)
        .args(args)
        .current_dir(dir)
        .status()
        .unwrap_or_else(|e| panic!("Failed to execute make: {}", e));

    if !status.success() {
        panic!("make {:?} failed with exit code {:?}", args, status.code());
    }
}
