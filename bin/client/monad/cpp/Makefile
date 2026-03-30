# Compilers
CXX_X86 = g++-15
CC_X86 = gcc-15

# RISC-V toolchain: CC_RISCV64, CXX_RISCV64, AR_RISCV64 can be passed from
# build.rs (sourced from .cargo/config.toml). Falls back to nix-built GCC 15.
RISCV_TOOLCHAIN_DIR ?= $(shell nix-build $(MONAD_DIR)/riscv64-unknown-elf.nix --no-out-link)
CXX_RISCV64 ?= $(RISCV_TOOLCHAIN_DIR)/bin/riscv64-unknown-elf-g++
CC_RISCV64 ?= $(RISCV_TOOLCHAIN_DIR)/bin/riscv64-unknown-elf-gcc
AR_RISCV64 ?= $(RISCV_TOOLCHAIN_DIR)/bin/riscv64-unknown-elf-ar

# Monad submodule paths
MONAD_DIR = third_party/monad
MONAD_VM_DIR = $(MONAD_DIR)/category/vm
MONAD_ZKVM_SRC = $(MONAD_DIR)/category/zkvm
MONAD_TOOLCHAIN = $(MONAD_DIR)/category/core/toolchains/riscv64-elf-toolchain.cmake

# Monad include paths (for project-specific code that uses monad headers)
MONAD_INCLUDES = -I$(MONAD_VM_DIR) -I$(MONAD_DIR) \
                 -I$(MONAD_DIR)/third_party/intx/include \
                 -I$(MONAD_DIR)/third_party/evmc/include \
                 -I$(MONAD_DIR)/third_party/ethash/include
# zkVM includes: the zkvm/include must come BEFORE monad root to shadow uint256.hpp
MONAD_ZKVM_INCLUDES = -I$(MONAD_DIR)/category/zkvm/include $(MONAD_INCLUDES)

# Compiler flags
CXXFLAGS_COMMON = -std=c++23 -O3 -Ithird_party -Ithird_party/evmc/include -Ithird_party/intx/include -Ithird_party/ethash/include -Ithird_party/unordered_dense/include
CXXFLAGS_X86 = $(MONAD_INCLUDES) $(CXXFLAGS_COMMON) -I$(MONAD_DIR)/third_party/silkpre/lib -fPIC -mavx2 -mbmi2
CXXFLAGS_ZISK = $(MONAD_ZKVM_INCLUDES) $(CXXFLAGS_COMMON) -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG -DMONAD_ZKVM -DMONAD_ZKVM_ZISK
CXXFLAGS_SP1 = $(MONAD_ZKVM_INCLUDES) $(CXXFLAGS_COMMON) -march=rv64im -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG -DMONAD_ZKVM -DMONAD_ZKVM_SP1 -DSP1_TARGET

# C compiler flags (for .c files)
CFLAGS_COMMON = -O3 -Ithird_party/ethash/include
CFLAGS_X86 = $(CFLAGS_COMMON) -fPIC
CFLAGS_ZISK = $(CFLAGS_COMMON) -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -ffunction-sections -fdata-sections -DNDEBUG
CFLAGS_SP1 = $(CFLAGS_COMMON) -march=rv64im -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -ffunction-sections -fdata-sections -DNDEBUG -DSP1_TARGET

# ============================================================================
# Source file lists
# ============================================================================

# Project-specific sources (NOT from monad interpreter)
# Precompile code comes from the cmake-built libmonad-zkvm.a (via monad_objs/).
SOURCES_PROJECT = $(wildcard *.cpp) $(wildcard evmone_host/*.cpp) \
                  $(wildcard evmone_test/*.cpp)

# JSON-dependent sources (only needed for x86 converter, not for RISC-V runtime)
JSON_SOURCES = evmone_test/blockchaintest_loader.cpp evmone_test/statetest_loader.cpp evmone_test/utils.cpp

# x86: project sources (monad interpreter comes from cmake-built libmonad-zkvm.a)
SOURCES_X86 = $(SOURCES_PROJECT)

# RISC-V: project sources only (monad + precompiles come from cmake-built libmonad-zkvm.a)
SOURCES_ZISK = $(filter-out $(JSON_SOURCES),$(SOURCES_PROJECT))
SOURCES_SP1 = $(filter-out $(JSON_SOURCES),$(SOURCES_PROJECT))

# ============================================================================
# Third-party library sources
# ============================================================================

# Ethash library sources
ETHASH_CPP_SOURCES = third_party/ethash/lib/ethash/ethash.cpp
ETHASH_C_SOURCES = third_party/ethash/lib/keccak/keccak.c third_party/ethash/lib/ethash/primes.c
# For RISC-V: keccak comes from libmonad-zkvm.a (we override it with our patched version)
ETHASH_RISCV_CPP_SOURCES = third_party/ethash/lib/ethash/ethash.cpp
ETHASH_RISCV_C_SOURCES = third_party/ethash/lib/keccak/keccak.c third_party/ethash/lib/ethash/primes.c


# ============================================================================
# Object file lists
# ============================================================================

# x86 objects — ethash is compiled by Makefile; all other third-party libs
# (silkpre, libff, secp256k1, blst, c-kzg-4844) come from cmake via precompile_deps_objs.
ETHASH_OBJECTS_X86 = $(patsubst third_party/ethash/lib/%.cpp,build/x86/ethash_%.o,$(ETHASH_CPP_SOURCES)) \
                     $(patsubst third_party/ethash/lib/%.c,build/x86/ethash_%.o,$(ETHASH_C_SOURCES))

OBJECTS_X86 = $(patsubst %.cpp,build/x86/%.o,$(SOURCES_PROJECT)) \
              $(ETHASH_OBJECTS_X86)

# RISC-V (ZisK) objects — project-specific only; monad comes from cmake
ETHASH_OBJECTS_ZISK = $(patsubst third_party/ethash/lib/%.cpp,build/zisk/ethash_%.o,$(ETHASH_RISCV_CPP_SOURCES)) \
                      $(patsubst third_party/ethash/lib/%.c,build/zisk/ethash_%.o,$(ETHASH_RISCV_C_SOURCES))
OBJECTS_ZISK = $(patsubst %.cpp,build/zisk/%.o,$(SOURCES_ZISK)) \
                $(ETHASH_OBJECTS_ZISK)

# RISC-V (SP1) objects
ETHASH_OBJECTS_SP1 = $(patsubst third_party/ethash/lib/%.cpp,build/sp1/ethash_%.o,$(ETHASH_RISCV_CPP_SOURCES)) \
                     $(patsubst third_party/ethash/lib/%.c,build/sp1/ethash_%.o,$(ETHASH_RISCV_C_SOURCES))
OBJECTS_SP1 = $(patsubst %.cpp,build/sp1/%.o,$(SOURCES_SP1)) \
              $(ETHASH_OBJECTS_SP1)

# Library outputs
LIB_X86 = build/x86/libmonad_wrapper.a
LIB_ZISK = build/zisk/libmonad_wrapper.a
LIB_SP1 = build/sp1/libmonad_wrapper.a

# cmake build directories for monad interpreter
MONAD_BUILD_X86 = build/monad-x86
MONAD_BUILD_ZISK = build/monad-zisk
MONAD_BUILD_SP1 = build/monad-sp1

# Patch marker file
PATCH_MARKER = third_party/ethash/.patches_applied

# Default target
all: x86

# Apply patches to third-party libraries
apply-patches: $(PATCH_MARKER)

$(PATCH_MARKER):
	@echo "Applying patches to ethash..."
	@cd third_party/ethash && patch -p0 < ../../patches/ethash-zisk-keccak.patch
	@touch $(PATCH_MARKER)

# ============================================================================
# cmake builds for monad interpreter
# ============================================================================

$(MONAD_BUILD_X86)/libmonad-zkvm.a:
	cmake -B $(MONAD_BUILD_X86) -S $(MONAD_ZKVM_SRC) \
		-DMONAD_ZKVM_BACKEND=native \
		-DCMAKE_TOOLCHAIN_FILE=$(abspath $(MONAD_DIR)/category/core/toolchains/gcc-avx2.cmake) \
		-DCMAKE_C_COMPILER=$(CC_X86) \
		-DCMAKE_CXX_COMPILER=$(CXX_X86) \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(MONAD_BUILD_X86)

$(MONAD_BUILD_ZISK)/libmonad-zkvm.a:
	cmake -B $(MONAD_BUILD_ZISK) -S $(MONAD_ZKVM_SRC) \
		-DCMAKE_TOOLCHAIN_FILE=$(abspath $(MONAD_TOOLCHAIN)) \
		-DRISCV_TOOLCHAIN_DIR=$(RISCV_TOOLCHAIN_DIR) \
		-DMONAD_ZKVM_BACKEND=zisk \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(MONAD_BUILD_ZISK)

$(MONAD_BUILD_SP1)/libmonad-zkvm.a:
	cmake -B $(MONAD_BUILD_SP1) -S $(MONAD_ZKVM_SRC) \
		-DCMAKE_TOOLCHAIN_FILE=$(abspath $(MONAD_TOOLCHAIN)) \
		-DRISCV_TOOLCHAIN_DIR=$(RISCV_TOOLCHAIN_DIR) \
		-DMONAD_ZKVM_BACKEND=sp1 \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS_INIT="-march=rv64im -mabi=lp64 -mcmodel=medany -nostartfiles -ffunction-sections -fdata-sections" \
		-DCMAKE_CXX_FLAGS_INIT="-march=rv64im -mabi=lp64 -mcmodel=medany -nostartfiles -nostdlib++ -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections" \
		-DCMAKE_ASM_FLAGS_INIT="-march=rv64im -mabi=lp64"
	cmake --build $(MONAD_BUILD_SP1)

# Extract monad objects from cmake archive (removing keccak — we use our patched version)
build/x86/monad_objs: $(MONAD_BUILD_X86)/libmonad-zkvm.a
	@rm -rf $@ && mkdir -p $@
	cd $@ && ar x $(abspath $<)
	rm -f $@/keccak*

# Extract precompile dependency objects from cmake-built libraries
build/x86/precompile_deps_objs: $(MONAD_BUILD_X86)/libmonad-zkvm.a
	@rm -rf $@ && mkdir -p $@
	cd $@ && ar x $(abspath $(MONAD_BUILD_X86)/libblst.a)
	cd $@ && ar x $(abspath $(MONAD_BUILD_X86)/_silkpre/lib/libsilkpre.a)
	cd $@ && ar x $(abspath $(MONAD_BUILD_X86)/_silkpre/third_party/libff/libff/libff.a)
	cd $@ && ar x $(abspath $(MONAD_BUILD_X86)/_silkpre/libsecp256k1.a)
	cd $@ && ar x $(abspath $(MONAD_BUILD_X86)/_c-kzg-4844/libc-kzg-4844.a)

build/zisk/monad_objs: $(MONAD_BUILD_ZISK)/libmonad-zkvm.a
	@rm -rf $@ && mkdir -p $@
	cd $@ && $(AR_RISCV64) x $(abspath $<)
	rm -f $@/keccak*

build/sp1/monad_objs: $(MONAD_BUILD_SP1)/libmonad-zkvm.a
	@rm -rf $@ && mkdir -p $@
	cd $@ && $(AR_RISCV64) x $(abspath $<)
	rm -f $@/keccak*

# ============================================================================
# x86 target
# ============================================================================

x86: apply-patches $(LIB_X86)

$(LIB_X86): $(OBJECTS_X86) build/x86/monad_objs build/x86/precompile_deps_objs
	@mkdir -p build/x86
	ar rcs $(LIB_X86) $(OBJECTS_X86) build/x86/monad_objs/*.o build/x86/precompile_deps_objs/*.o

# Project-specific compilation rules for x86
build/x86/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/evmone_host/%.o: evmone_host/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/evmone_test/%.o: evmone_test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

# Third-party library compilation for x86 (ethash only; silkpre, libff,
# secp256k1, blst, c-kzg-4844 come from cmake via precompile_deps_objs)
build/x86/ethash_%.o: third_party/ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/ethash_%.o: third_party/ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -c $< -o $@

# ============================================================================
# ZisK RISC-V target (monad interpreter built via cmake)
# ============================================================================

zisk: apply-patches $(LIB_ZISK)

$(LIB_ZISK): $(OBJECTS_ZISK) build/zisk/monad_objs
	@mkdir -p build/zisk
	$(AR_RISCV64) rcs $(LIB_ZISK) $(OBJECTS_ZISK) build/zisk/monad_objs/*.o build/zisk/monad_objs/*.obj

build/zisk/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/evmone_host/%.o: evmone_host/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/evmone_test/%.o: evmone_test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/ethash_%.o: third_party/ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/ethash_%.o: third_party/ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_ZISK) -c $< -o $@

# ============================================================================
# SP1 RISC-V target (monad interpreter built via cmake)
# ============================================================================

sp1: apply-patches $(LIB_SP1)

$(LIB_SP1): $(OBJECTS_SP1) build/sp1/monad_objs
	@mkdir -p build/sp1
	$(AR_RISCV64) rcs $(LIB_SP1) $(OBJECTS_SP1) build/sp1/monad_objs/*.o build/sp1/monad_objs/*.obj

build/sp1/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/evmone_host/%.o: evmone_host/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/evmone_test/%.o: evmone_test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/ethash_%.o: third_party/ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/ethash_%.o: third_party/ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_SP1) -c $< -o $@

# ============================================================================
# Converter binary (x86 only)
# ============================================================================

CONVERTER_BIN = build/x86/ethtest_converter

converter: apply-patches $(CONVERTER_BIN)

$(CONVERTER_BIN): build/x86/converter/converter_main.o $(OBJECTS_X86) build/x86/monad_objs build/x86/precompile_deps_objs
	@mkdir -p build/x86
	$(CXX_X86) -o $@ build/x86/converter/converter_main.o $(OBJECTS_X86) build/x86/monad_objs/*.o build/x86/precompile_deps_objs/*.o -lstdc++ -lgmp -lgmpxx -lcrypto++

build/x86/converter/converter_main.o: converter/converter_main.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -Iconverter -c $< -o $@

# Clean build artifacts
clean:
	rm -rf build
	rm -f third_party/ethash/.patches_applied
	@cd third_party/ethash && git checkout -f lib/keccak/keccak.c 2>/dev/null || true

.PHONY: all x86 zisk sp1 converter clean apply-patches
