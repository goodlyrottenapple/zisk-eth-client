# Compilers
CXX_X86 = g++-15
CC_X86 = gcc-15
CXX_RISCV64 = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-g++
CC_RISCV64 = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-gcc
AR_RISCV64 = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-ar

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
CXXFLAGS_COMMON = -std=c++23 -O3 -Ithird_party -Ithird_party/evmc/include -Ithird_party/intx/include -Ithird_party/ethash/include -Ithird_party/unordered_dense/include -Ithird_party/silkpre/lib -Ithird_party/silkpre/third_party/libff -Ithird_party/silkpre/third_party/secp256k1/include -Ithird_party/blst/bindings
CXXFLAGS_X86 = $(MONAD_INCLUDES) $(CXXFLAGS_COMMON) -fPIC -mavx2 -mbmi2
CXXFLAGS_ZISK = $(MONAD_ZKVM_INCLUDES) $(CXXFLAGS_COMMON) -Ilibc_riscv -Ithird_party/gmp/mini-gmp -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG -DMONAD_ZKVM -DMONAD_ZKVM_ZISK -DGMP_NUMB_BITS=64
CXXFLAGS_SP1 = $(MONAD_ZKVM_INCLUDES) $(CXXFLAGS_COMMON) -Ilibc_riscv -Ithird_party/gmp/mini-gmp -march=rv64im -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG -DMONAD_ZKVM -DMONAD_ZKVM_SP1 -DSP1_TARGET -DGMP_NUMB_BITS=64

# C compiler flags (for .c files)
CFLAGS_COMMON = -O3 -Ithird_party/ethash/include -Ithird_party/silkpre/lib -Ithird_party/silkpre/third_party/secp256k1/include -Ithird_party/blst/bindings
CFLAGS_X86 = $(CFLAGS_COMMON) -fPIC
CFLAGS_ZISK = $(CFLAGS_COMMON) -Ilibc_riscv -Ithird_party/gmp/mini-gmp -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -ffunction-sections -fdata-sections -DUSE_NUM_NONE -DUSE_FIELD_INV_BUILTIN -DUSE_SCALAR_INV_BUILTIN -DUSE_FIELD_5X52 -DUSE_SCALAR_4X64 -DHAVE___INT128 -DECMULT_WINDOW_SIZE=15 -DECMULT_GEN_PREC_BITS=4 -DUSE_ENDOMORPHISM -DENABLE_MODULE_ECDH -DENABLE_MODULE_RECOVERY -DGMP_NUMB_BITS=64
CFLAGS_SP1 = $(CFLAGS_COMMON) -Ilibc_riscv -Ithird_party/gmp/mini-gmp -march=rv64im -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -fno-exceptions -ffunction-sections -fdata-sections -DNDEBUG -DSP1_TARGET -DUSE_NUM_NONE -DUSE_FIELD_INV_BUILTIN -DUSE_SCALAR_INV_BUILTIN -DUSE_FIELD_5X52 -DUSE_SCALAR_4X64 -DHAVE___INT128 -DECMULT_WINDOW_SIZE=15 -DECMULT_GEN_PREC_BITS=4 -DUSE_ENDOMORPHISM -DENABLE_MODULE_ECDH -DENABLE_MODULE_RECOVERY -DGMP_NUMB_BITS=64

# Assembly flags (for .S files in RISC-V build)
ASFLAGS_ZISK = -march=rv64ima -mabi=lp64 -Ilibc_riscv
ASFLAGS_SP1 = -march=rv64im -mabi=lp64 -Ilibc_riscv

# ============================================================================
# Source file lists
# ============================================================================

# Project-specific sources (NOT from monad interpreter)
SOURCES_PROJECT = $(wildcard *.cpp) $(wildcard core/*.cpp) $(wildcard evmone_host/*.cpp) \
                  $(wildcard execution/ethereum/*.cpp) $(wildcard evmone_test/*.cpp)

# JSON-dependent sources (only needed for x86 converter, not for RISC-V runtime)
JSON_SOURCES = evmone_test/blockchaintest_loader.cpp evmone_test/statetest_loader.cpp evmone_test/utils.cpp

# Monad interpreter sources from submodule (for x86 direct compilation)
MONAD_CPP_SOURCES = $(wildcard $(MONAD_VM_DIR)/interpreter/*.cpp) \
                    $(wildcard $(MONAD_VM_DIR)/runtime/*.cpp) \
                    $(wildcard $(MONAD_VM_DIR)/core/*.cpp) \
                    $(wildcard $(MONAD_VM_DIR)/evm/*.cpp)
MONAD_ASM_SOURCES = $(MONAD_VM_DIR)/runtime/math.S \
                    $(MONAD_VM_DIR)/runtime/exit.S \
                    $(MONAD_VM_DIR)/runtime/context.S \
                    $(MONAD_VM_DIR)/runtime/transmute.S \
                    $(MONAD_VM_DIR)/interpreter/entry.S

# x86: project sources + monad sources from submodule
SOURCES_X86 = $(SOURCES_PROJECT) $(MONAD_CPP_SOURCES)
ASM_SOURCES_X86 = $(MONAD_ASM_SOURCES)

# RISC-V: project sources only (monad comes from cmake-built libmonad-zkvm.a)
SOURCES_ZISK = $(filter-out $(JSON_SOURCES) execution/ethereum/precompiles_bls12.cpp,$(SOURCES_PROJECT))
SOURCES_SP1 = $(filter-out $(JSON_SOURCES) execution/ethereum/precompiles_bls12.cpp,$(SOURCES_PROJECT))

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

# BLST library sources
BLST_C_SOURCES = third_party/blst/src/server.c
BLST_ASM_SOURCES = third_party/blst/build/assembly.S

# Mini-GMP library source (for RISC-V bare-metal)
MINI_GMP_C_SOURCES = third_party/gmp/mini-gmp/mini-gmp.c
GMP_COMPAT_C_SOURCES = gmp_mini_compat.c

# ============================================================================
# Object file lists
# ============================================================================

# x86 objects
MONAD_OBJECTS_X86 = $(patsubst $(MONAD_VM_DIR)/%.cpp,build/x86/monad_%.o,$(MONAD_CPP_SOURCES))
MONAD_ASM_OBJECTS_X86 = $(patsubst $(MONAD_VM_DIR)/%.S,build/x86/monad_%_asm.o,$(MONAD_ASM_SOURCES))
ETHASH_OBJECTS_X86 = $(patsubst third_party/ethash/lib/%.cpp,build/x86/ethash_%.o,$(ETHASH_CPP_SOURCES)) \
                     $(patsubst third_party/ethash/lib/%.c,build/x86/ethash_%.o,$(ETHASH_C_SOURCES))
SILKPRE_OBJECTS_X86 = $(patsubst third_party/silkpre/lib/%.cpp,build/x86/silkpre_%.o,$(SILKPRE_CPP_SOURCES)) \
                      $(patsubst third_party/silkpre/lib/%.c,build/x86/silkpre_%.o,$(SILKPRE_C_SOURCES))
LIBFF_OBJECTS_X86 = $(patsubst third_party/silkpre/third_party/libff/libff/%.cpp,build/x86/libff_%.o,$(LIBFF_CPP_SOURCES))
SECP256K1_OBJECTS_X86 = $(patsubst third_party/silkpre/third_party/secp256k1/src/%.c,build/x86/secp256k1_%.o,$(SECP256K1_C_SOURCES))
BLST_OBJECTS_X86 = $(patsubst third_party/blst/src/%.c,build/x86/blst_%.o,$(BLST_C_SOURCES)) \
                   $(patsubst third_party/blst/build/%.S,build/x86/blst_%.o,$(BLST_ASM_SOURCES))
OBJECTS_X86 = $(patsubst %.cpp,build/x86/%.o,$(SOURCES_PROJECT)) \
              $(MONAD_OBJECTS_X86) \
              $(MONAD_ASM_OBJECTS_X86) \
              $(ETHASH_OBJECTS_X86) \
              $(SILKPRE_OBJECTS_X86) \
              $(LIBFF_OBJECTS_X86) \
              $(SECP256K1_OBJECTS_X86) \
              $(BLST_OBJECTS_X86)

# RISC-V (ZisK) objects — project-specific only; monad comes from cmake
LIBC_ASM_SOURCES = $(wildcard libc_riscv/*.S)
LIBC_OBJECTS_ZISK = $(patsubst libc_riscv/%.S,build/zisk/libc_riscv_%.o,$(LIBC_ASM_SOURCES))
ETHASH_OBJECTS_ZISK = $(patsubst third_party/ethash/lib/%.cpp,build/zisk/ethash_%.o,$(ETHASH_RISCV_CPP_SOURCES)) \
                      $(patsubst third_party/ethash/lib/%.c,build/zisk/ethash_%.o,$(ETHASH_RISCV_C_SOURCES))
SILKPRE_OBJECTS_ZISK = $(patsubst third_party/silkpre/lib/%.cpp,build/zisk/silkpre_%.o,$(SILKPRE_CPP_SOURCES)) \
                        $(patsubst third_party/silkpre/lib/%.c,build/zisk/silkpre_%.o,$(SILKPRE_C_SOURCES))
LIBFF_OBJECTS_ZISK = $(patsubst third_party/silkpre/third_party/libff/libff/%.cpp,build/zisk/libff_%.o,$(LIBFF_CPP_SOURCES))
SECP256K1_OBJECTS_ZISK = $(patsubst third_party/silkpre/third_party/secp256k1/src/%.c,build/zisk/secp256k1_%.o,$(SECP256K1_C_SOURCES))
MINI_GMP_OBJECTS_ZISK = $(patsubst third_party/gmp/mini-gmp/%.c,build/zisk/mini_gmp_%.o,$(MINI_GMP_C_SOURCES))
GMP_COMPAT_OBJECTS_ZISK = $(patsubst %.c,build/zisk/%.o,$(GMP_COMPAT_C_SOURCES))
OBJECTS_ZISK = $(patsubst %.cpp,build/zisk/%.o,$(SOURCES_ZISK)) \
                $(LIBC_OBJECTS_ZISK) \
                $(ETHASH_OBJECTS_ZISK) \
                $(SILKPRE_OBJECTS_ZISK) \
                $(LIBFF_OBJECTS_ZISK) \
                $(SECP256K1_OBJECTS_ZISK) \
                $(MINI_GMP_OBJECTS_ZISK) \
                $(GMP_COMPAT_OBJECTS_ZISK)

# RISC-V (SP1) objects
LIBC_OBJECTS_SP1 = $(patsubst libc_riscv/%.S,build/sp1/libc_riscv_%.o,$(LIBC_ASM_SOURCES))
ETHASH_OBJECTS_SP1 = $(patsubst third_party/ethash/lib/%.cpp,build/sp1/ethash_%.o,$(ETHASH_RISCV_CPP_SOURCES)) \
                     $(patsubst third_party/ethash/lib/%.c,build/sp1/ethash_%.o,$(ETHASH_RISCV_C_SOURCES))
SILKPRE_OBJECTS_SP1 = $(patsubst third_party/silkpre/lib/%.cpp,build/sp1/silkpre_%.o,$(SILKPRE_CPP_SOURCES)) \
                      $(patsubst third_party/silkpre/lib/%.c,build/sp1/silkpre_%.o,$(SILKPRE_C_SOURCES))
LIBFF_OBJECTS_SP1 = $(patsubst third_party/silkpre/third_party/libff/libff/%.cpp,build/sp1/libff_%.o,$(LIBFF_CPP_SOURCES))
SECP256K1_OBJECTS_SP1 = $(patsubst third_party/silkpre/third_party/secp256k1/src/%.c,build/sp1/secp256k1_%.o,$(SECP256K1_C_SOURCES))
MINI_GMP_OBJECTS_SP1 = $(patsubst third_party/gmp/mini-gmp/%.c,build/sp1/mini_gmp_%.o,$(MINI_GMP_C_SOURCES))
GMP_COMPAT_OBJECTS_SP1 = $(patsubst %.c,build/sp1/%.o,$(GMP_COMPAT_C_SOURCES))
OBJECTS_SP1 = $(patsubst %.cpp,build/sp1/%.o,$(SOURCES_SP1)) \
              $(LIBC_OBJECTS_SP1) \
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
# cmake builds for monad interpreter (RISC-V only)
# ============================================================================

$(MONAD_BUILD_ZISK)/libmonad-zkvm.a:
	cmake -B $(MONAD_BUILD_ZISK) -S $(MONAD_ZKVM_SRC) \
		-DCMAKE_TOOLCHAIN_FILE=$(abspath $(MONAD_TOOLCHAIN)) \
		-DRISCV_TOOLCHAIN_DIR=$(HOME)/.zisk/riscv \
		-DMONAD_ZKVM_BACKEND=zisk \
		-DCMAKE_BUILD_TYPE=Release
	cmake --build $(MONAD_BUILD_ZISK)

$(MONAD_BUILD_SP1)/libmonad-zkvm.a:
	cmake -B $(MONAD_BUILD_SP1) -S $(MONAD_ZKVM_SRC) \
		-DCMAKE_TOOLCHAIN_FILE=$(abspath $(MONAD_TOOLCHAIN)) \
		-DRISCV_TOOLCHAIN_DIR=$(HOME)/.zisk/riscv \
		-DMONAD_ZKVM_BACKEND=sp1 \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_FLAGS_INIT="-march=rv64im -mabi=lp64 -mcmodel=medany -nostartfiles -ffunction-sections -fdata-sections" \
		-DCMAKE_CXX_FLAGS_INIT="-march=rv64im -mabi=lp64 -mcmodel=medany -nostartfiles -nostdlib++ -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections" \
		-DCMAKE_ASM_FLAGS_INIT="-march=rv64im -mabi=lp64"
	cmake --build $(MONAD_BUILD_SP1)

# Extract monad objects from cmake archive (removing keccak — we use our patched version)
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

$(LIB_X86): $(OBJECTS_X86)
	@mkdir -p build/x86
	ar rcs $(LIB_X86) $(OBJECTS_X86)

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

build/x86/execution/ethereum/%.o: execution/ethereum/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/evmone_test/%.o: evmone_test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

# Monad interpreter compilation rules for x86 (from submodule)
build/x86/monad_interpreter/%.o: $(MONAD_VM_DIR)/interpreter/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/monad_runtime/%.o: $(MONAD_VM_DIR)/runtime/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/monad_core/%.o: $(MONAD_VM_DIR)/core/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/monad_evm/%.o: $(MONAD_VM_DIR)/evm/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/monad_runtime/%_asm.o: $(MONAD_VM_DIR)/runtime/%.S
	@mkdir -p $(dir $@)
	$(CXX_X86) -c $< -o $@

build/x86/monad_interpreter/%_asm.o: $(MONAD_VM_DIR)/interpreter/%.S
	@mkdir -p $(dir $@)
	$(CXX_X86) -c $< -o $@

# Third-party library compilation for x86
build/x86/ethash_%.o: third_party/ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/ethash_%.o: third_party/ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -c $< -o $@

build/x86/silkpre_%.o: third_party/silkpre/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/silkpre_%.o: third_party/silkpre/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -c $< -o $@

build/x86/libff_%.o: third_party/silkpre/third_party/libff/libff/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -DNO_PROCPS=1 -DCURVE_BN128 -DBN_SUPPORT_SNARK=1 -c $< -o $@

build/x86/secp256k1_%.o: third_party/silkpre/third_party/secp256k1/src/%.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -DECMULT_GEN_PREC_BITS=4 -DECMULT_WINDOW_SIZE=15 -DENABLE_MODULE_ECDH -DENABLE_MODULE_RECOVERY -c $< -o $@

build/x86/blst_server.o: third_party/blst/src/server.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -D__ADX__ -c $< -o $@

build/x86/blst_assembly.o: third_party/blst/build/assembly.S
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -D__ADX__ -c $< -o $@

# ============================================================================
# ZisK RISC-V target (monad interpreter built via cmake)
# ============================================================================

zisk: apply-patches $(LIB_ZISK)

$(LIB_ZISK): $(OBJECTS_ZISK) build/zisk/monad_objs
	@mkdir -p build/zisk
	$(AR_RISCV64) rcs $(LIB_ZISK) $(OBJECTS_ZISK) build/zisk/monad_objs/*.*obj

build/zisk/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/core/%.o: core/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/evmone_host/%.o: evmone_host/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/execution/ethereum/%.o: execution/ethereum/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/evmone_test/%.o: evmone_test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_ZISK) -c $< -o $@

build/zisk/libc_riscv_%.o: libc_riscv/%.S
	@mkdir -p build/zisk
	$(CXX_RISCV64) $(ASFLAGS_ZISK) -c $< -o $@

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
	$(AR_RISCV64) rcs $(LIB_SP1) $(OBJECTS_SP1) build/sp1/monad_objs/*.*obj

build/sp1/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/core/%.o: core/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/evmone_host/%.o: evmone_host/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/execution/ethereum/%.o: execution/ethereum/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/evmone_test/%.o: evmone_test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV64) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/libc_riscv_%.o: libc_riscv/%.S
	@mkdir -p build/sp1
	$(CC_RISCV64) $(ASFLAGS_SP1) -c $< -o $@

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
