# Compilers
CXX_X86 = g++-15
CC_X86 = gcc-15

# RISC-V toolchain (nix-built GCC 15 cross-compiler with minimal newlib)
RISCV_TOOLCHAIN_DIR ?= $(shell nix-build $(MONAD_DIR)/riscv64-none-elf.nix --no-out-link)
CXX_RISCV64 = $(RISCV_TOOLCHAIN_DIR)/bin/riscv64-none-elf-g++
CC_RISCV64 = $(RISCV_TOOLCHAIN_DIR)/bin/riscv64-none-elf-gcc
AR_RISCV64 = $(RISCV_TOOLCHAIN_DIR)/bin/riscv64-none-elf-ar

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
CXXFLAGS_COMMON = -std=c++23 -O3 -Ithird_party -Ithird_party/evmc/include -Ithird_party/intx/include -Ithird_party/ethash/include -Ithird_party/unordered_dense/include -Ithird_party/silkpre/lib -Ithird_party/silkpre/third_party/libff -Ithird_party/silkpre/third_party/secp256k1/include
CXXFLAGS_X86 = $(MONAD_INCLUDES) $(CXXFLAGS_COMMON) -fPIC -mavx2 -mbmi2
CXXFLAGS_ZISK = $(MONAD_ZKVM_INCLUDES) $(CXXFLAGS_COMMON) -Ithird_party/gmp/mini-gmp -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG -DMONAD_ZKVM -DMONAD_ZKVM_ZISK -DGMP_NUMB_BITS=64
CXXFLAGS_SP1 = $(MONAD_ZKVM_INCLUDES) $(CXXFLAGS_COMMON) -Ithird_party/gmp/mini-gmp -march=rv64im -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG -DMONAD_ZKVM -DMONAD_ZKVM_SP1 -DSP1_TARGET -DGMP_NUMB_BITS=64

# C compiler flags (for .c files)
CFLAGS_COMMON = -O3 -Ithird_party/ethash/include -Ithird_party/silkpre/lib -Ithird_party/silkpre/third_party/secp256k1/include
CFLAGS_X86 = $(CFLAGS_COMMON) -fPIC
CFLAGS_ZISK = $(CFLAGS_COMMON) -Ithird_party/gmp/mini-gmp -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -ffunction-sections -fdata-sections -DUSE_NUM_NONE -DUSE_FIELD_INV_BUILTIN -DUSE_SCALAR_INV_BUILTIN -DUSE_FIELD_5X52 -DUSE_SCALAR_4X64 -DHAVE___INT128 -DECMULT_WINDOW_SIZE=15 -DECMULT_GEN_PREC_BITS=4 -DUSE_ENDOMORPHISM -DENABLE_MODULE_ECDH -DENABLE_MODULE_RECOVERY -DGMP_NUMB_BITS=64
CFLAGS_SP1 = $(CFLAGS_COMMON) -Ithird_party/gmp/mini-gmp -march=rv64im -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -fno-exceptions -ffunction-sections -fdata-sections -DNDEBUG -DSP1_TARGET -DUSE_NUM_NONE -DUSE_FIELD_INV_BUILTIN -DUSE_SCALAR_INV_BUILTIN -DUSE_FIELD_5X52 -DUSE_SCALAR_4X64 -DHAVE___INT128 -DECMULT_WINDOW_SIZE=15 -DECMULT_GEN_PREC_BITS=4 -DUSE_ENDOMORPHISM -DENABLE_MODULE_ECDH -DENABLE_MODULE_RECOVERY -DGMP_NUMB_BITS=64

# ============================================================================
# Source file lists
# ============================================================================

# Project-specific sources (NOT from monad interpreter)
# Precompile code comes from the cmake-built libmonad-zkvm.a (via monad_objs/).
SOURCES_PROJECT = $(wildcard *.cpp) $(wildcard core/*.cpp) $(wildcard evmone_host/*.cpp) \
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

# Silkpre library sources
SILKPRE_CPP_SOURCES = third_party/silkpre/lib/silkpre/precompile.cpp third_party/silkpre/lib/silkpre/secp256k1n.cpp
SILKPRE_C_SOURCES = third_party/silkpre/lib/silkpre/blake2b.c third_party/silkpre/lib/silkpre/ecdsa.c third_party/silkpre/lib/silkpre/rmd160.c third_party/silkpre/lib/silkpre/sha256.c

# Libff library sources (for BN254/alt_bn128 operations)
LIBFF_CPP_SOURCES = third_party/silkpre/third_party/libff/libff/algebra/curves/alt_bn128/alt_bn128_init.cpp \
                    third_party/silkpre/third_party/libff/libff/algebra/curves/alt_bn128/alt_bn128_g1.cpp \
                    third_party/silkpre/third_party/libff/libff/algebra/curves/alt_bn128/alt_bn128_g2.cpp \
                    third_party/silkpre/third_party/libff/libff/algebra/curves/alt_bn128/alt_bn128_pairing.cpp \
                    third_party/silkpre/third_party/libff/libff/algebra/curves/alt_bn128/alt_bn128_pp.cpp

# Secp256k1 library source (unity build)
SECP256K1_C_SOURCES = third_party/silkpre/third_party/secp256k1/src/secp256k1.c

# Mini-GMP library source (for RISC-V bare-metal)
MINI_GMP_C_SOURCES = third_party/gmp/mini-gmp/mini-gmp.c
GMP_COMPAT_C_SOURCES = gmp_mini_compat.c

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
SILKPRE_OBJECTS_ZISK = $(patsubst third_party/silkpre/lib/%.cpp,build/zisk/silkpre_%.o,$(SILKPRE_CPP_SOURCES)) \
                        $(patsubst third_party/silkpre/lib/%.c,build/zisk/silkpre_%.o,$(SILKPRE_C_SOURCES))
LIBFF_OBJECTS_ZISK = $(patsubst third_party/silkpre/third_party/libff/libff/%.cpp,build/zisk/libff_%.o,$(LIBFF_CPP_SOURCES))
SECP256K1_OBJECTS_ZISK = $(patsubst third_party/silkpre/third_party/secp256k1/src/%.c,build/zisk/secp256k1_%.o,$(SECP256K1_C_SOURCES))
MINI_GMP_OBJECTS_ZISK = $(patsubst third_party/gmp/mini-gmp/%.c,build/zisk/mini_gmp_%.o,$(MINI_GMP_C_SOURCES))
GMP_COMPAT_OBJECTS_ZISK = $(patsubst %.c,build/zisk/%.o,$(GMP_COMPAT_C_SOURCES))
OBJECTS_ZISK = $(patsubst %.cpp,build/zisk/%.o,$(SOURCES_ZISK)) \
                $(ETHASH_OBJECTS_ZISK) \
                $(SILKPRE_OBJECTS_ZISK) \
                $(LIBFF_OBJECTS_ZISK) \
                $(SECP256K1_OBJECTS_ZISK) \
                $(MINI_GMP_OBJECTS_ZISK) \
                $(GMP_COMPAT_OBJECTS_ZISK)

# RISC-V (SP1) objects
ETHASH_OBJECTS_SP1 = $(patsubst third_party/ethash/lib/%.cpp,build/sp1/ethash_%.o,$(ETHASH_RISCV_CPP_SOURCES)) \
                     $(patsubst third_party/ethash/lib/%.c,build/sp1/ethash_%.o,$(ETHASH_RISCV_C_SOURCES))
SILKPRE_OBJECTS_SP1 = $(patsubst third_party/silkpre/lib/%.cpp,build/sp1/silkpre_%.o,$(SILKPRE_CPP_SOURCES)) \
                      $(patsubst third_party/silkpre/lib/%.c,build/sp1/silkpre_%.o,$(SILKPRE_C_SOURCES))
LIBFF_OBJECTS_SP1 = $(patsubst third_party/silkpre/third_party/libff/libff/%.cpp,build/sp1/libff_%.o,$(LIBFF_CPP_SOURCES))
SECP256K1_OBJECTS_SP1 = $(patsubst third_party/silkpre/third_party/secp256k1/src/%.c,build/sp1/secp256k1_%.o,$(SECP256K1_C_SOURCES))
MINI_GMP_OBJECTS_SP1 = $(patsubst third_party/gmp/mini-gmp/%.c,build/sp1/mini_gmp_%.o,$(MINI_GMP_C_SOURCES))
GMP_COMPAT_OBJECTS_SP1 = $(patsubst %.c,build/sp1/%.o,$(GMP_COMPAT_C_SOURCES))
OBJECTS_SP1 = $(patsubst %.cpp,build/sp1/%.o,$(SOURCES_SP1)) \
              $(ETHASH_OBJECTS_SP1) \
              $(SILKPRE_OBJECTS_SP1) \
              $(LIBFF_OBJECTS_SP1) \
              $(SECP256K1_OBJECTS_SP1) \
              $(MINI_GMP_OBJECTS_SP1) \
              $(GMP_COMPAT_OBJECTS_SP1)

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

build/x86/core/%.o: core/%.cpp
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

build/zisk/core/%.o: core/%.cpp
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

build/zisk/silkpre_%.o: third_party/silkpre/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/silkpre_%.o: third_party/silkpre/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_ZISK) -c $< -o $@

build/zisk/libff_%.o: third_party/silkpre/third_party/libff/libff/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -DNO_PROCPS=1 -DCURVE_BN128 -DBN_SUPPORT_SNARK=1 -c $< -o $@

build/zisk/secp256k1_%.o: third_party/silkpre/third_party/secp256k1/src/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_ZISK) -DECMULT_GEN_PREC_BITS=4 -DECMULT_WINDOW_SIZE=15 -c $< -o $@

build/zisk/mini_gmp_%.o: third_party/gmp/mini-gmp/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_ZISK) -c $< -o $@

build/zisk/gmp_mini_compat.o: gmp_mini_compat.c
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

build/sp1/core/%.o: core/%.cpp
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

build/sp1/silkpre_%.o: third_party/silkpre/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/silkpre_%.o: third_party/silkpre/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_SP1) -c $< -o $@

build/sp1/libff_%.o: third_party/silkpre/third_party/libff/libff/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -DNO_PROCPS=1 -DCURVE_BN128 -DBN_SUPPORT_SNARK=1 -c $< -o $@

build/sp1/secp256k1_%.o: third_party/silkpre/third_party/secp256k1/src/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_SP1) -DECMULT_GEN_PREC_BITS=4 -DECMULT_WINDOW_SIZE=15 -c $< -o $@

build/sp1/mini_gmp_%.o: third_party/gmp/mini-gmp/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_SP1) -c $< -o $@

build/sp1/gmp_mini_compat.o: gmp_mini_compat.c
	@mkdir -p $(dir $@)
	$(CC_RISCV64) $(CFLAGS_SP1) -c $< -o $@

# ============================================================================
# Converter binary (x86 only)
# ============================================================================

CONVERTER_BIN = build/x86/ethtest_converter

converter: apply-patches $(CONVERTER_BIN)

$(CONVERTER_BIN): build/x86/converter/converter_main.o $(OBJECTS_X86)
	@mkdir -p build/x86
	$(CXX_X86) -o $@ $^ -lstdc++ -lgmp -lgmpxx

build/x86/converter/converter_main.o: converter/converter_main.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -Iconverter -c $< -o $@

# Clean build artifacts
clean:
	rm -rf build
	rm -f third_party/ethash/.patches_applied
	@cd third_party/ethash && git checkout -f lib/keccak/keccak.c 2>/dev/null || true

.PHONY: all x86 zisk sp1 converter clean apply-patches
