# Compilers
CXX_X86 = g++-15
CC_X86 = gcc-15
CXX_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-g++
CC_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-gcc
AR_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-ar
# SP1 v6 uses rv64im — ZisK rv64ima toolchain works with -march=rv64im subset
CXX_SP1 = $(CXX_RISCV)
CC_SP1 = $(CC_RISCV)
AR_SP1 = $(AR_RISCV)

# Compiler flags
CXXFLAGS_COMMON = -std=c++23 -O3 -Ithird_party -Ithird_party/evmc/include -Ithird_party/intx/include -Ithird_party/ethash/include -Ithird_party/unordered_dense/include -Ithird_party/silkpre/lib -Ithird_party/silkpre/third_party/libff -Ithird_party/silkpre/third_party/secp256k1/include -Ithird_party/blst/bindings
CXXFLAGS_X86 = $(CXXFLAGS_COMMON) -fPIC
CXXFLAGS_RISCV = $(CXXFLAGS_COMMON) -Ilibc_riscv -Ithird_party/gmp/mini-gmp -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG -DGMP_NUMB_BITS=64

# C compiler flags (for .c files)
CFLAGS_COMMON = -O3 -Ithird_party/ethash/include -Ithird_party/silkpre/lib -Ithird_party/silkpre/third_party/secp256k1/include -Ithird_party/blst/bindings
CFLAGS_X86 = $(CFLAGS_COMMON) -fPIC
# RISC-V needs secp256k1 configuration flags
CFLAGS_RISCV = $(CFLAGS_COMMON) -Ilibc_riscv -Ithird_party/gmp/mini-gmp -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -ffunction-sections -fdata-sections -DUSE_NUM_NONE -DUSE_FIELD_INV_BUILTIN -DUSE_SCALAR_INV_BUILTIN -DUSE_FIELD_5X52 -DUSE_SCALAR_4X64 -DHAVE___INT128 -DECMULT_WINDOW_SIZE=15 -DECMULT_GEN_PREC_BITS=4 -DUSE_ENDOMORPHISM -DENABLE_MODULE_ECDH -DENABLE_MODULE_RECOVERY -DGMP_NUMB_BITS=64
CXXFLAGS_SP1 = $(CXXFLAGS_COMMON) -Ilibc_riscv -Ithird_party/gmp/mini-gmp -march=rv64im -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG -DSP1_TARGET -DGMP_NUMB_BITS=64
CFLAGS_SP1 = $(CFLAGS_COMMON) -Ilibc_riscv -Ithird_party/gmp/mini-gmp -march=rv64im -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -fno-exceptions -ffunction-sections -fdata-sections -DNDEBUG -DSP1_TARGET -DUSE_NUM_NONE -DUSE_FIELD_INV_BUILTIN -DUSE_SCALAR_INV_BUILTIN -DUSE_FIELD_5X52 -DUSE_SCALAR_4X64 -DHAVE___INT128 -DECMULT_WINDOW_SIZE=15 -DECMULT_GEN_PREC_BITS=4 -DUSE_ENDOMORPHISM -DENABLE_MODULE_ECDH -DENABLE_MODULE_RECOVERY -DGMP_NUMB_BITS=64

# Assembly flags (for .S files in RISC-V build)
ASFLAGS_RISCV = -march=rv64ima -mabi=lp64 -Ilibc_riscv
ASFLAGS_SP1 = -march=rv64im -mabi=lp64 -Ilibc_riscv

# Common source files for both targets
# Note: libff_stubs.cpp provides stub implementations for libff profiling functions
SOURCES_COMMON = $(wildcard *.cpp) $(wildcard runtime/*.cpp) $(wildcard interpreter/*.cpp) $(wildcard core/*.cpp) $(wildcard evm/*.cpp) $(wildcard evmone_host/*.cpp) $(wildcard execution/ethereum/*.cpp) $(wildcard evmone_test/*.cpp)

# JSON-dependent sources (only needed for x86 converter, not for RISC-V runtime)
JSON_SOURCES = evmone_test/blockchaintest_loader.cpp evmone_test/statetest_loader.cpp evmone_test/utils.cpp

# x86: all common sources (converter/ is in a separate directory, not matched by wildcards)
SOURCES_X86 = $(SOURCES_COMMON)

# RISC-V: exclude JSON loaders and BLST
SOURCES_RISCV = $(filter-out $(JSON_SOURCES) execution/ethereum/precompiles_bls12.cpp,$(SOURCES_COMMON))

ASM_SOURCES_X86 = runtime/math.S interpreter/entry.S runtime/exit.S

# Ethash library sources
ETHASH_CPP_SOURCES = third_party/ethash/lib/ethash/ethash.cpp
ETHASH_C_SOURCES = third_party/ethash/lib/keccak/keccak.c third_party/ethash/lib/ethash/primes.c

# Silkpre library sources
SILKPRE_CPP_SOURCES = third_party/silkpre/lib/silkpre/precompile.cpp third_party/silkpre/lib/silkpre/secp256k1n.cpp
SILKPRE_C_SOURCES = third_party/silkpre/lib/silkpre/blake2b.c third_party/silkpre/lib/silkpre/ecdsa.c third_party/silkpre/lib/silkpre/rmd160.c third_party/silkpre/lib/silkpre/sha256.c

# Libff library sources (for BN254/alt_bn128 operations)
# Note: profiling.cpp and utils.cpp are omitted as they pull in unnecessary dependencies
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

# For assembly files, use _asm suffix to avoid conflict with .cpp files
ETHASH_OBJECTS_X86 = $(patsubst third_party/ethash/lib/%.cpp,build/x86/ethash_%.o,$(ETHASH_CPP_SOURCES)) \
                     $(patsubst third_party/ethash/lib/%.c,build/x86/ethash_%.o,$(ETHASH_C_SOURCES))
SILKPRE_OBJECTS_X86 = $(patsubst third_party/silkpre/lib/%.cpp,build/x86/silkpre_%.o,$(SILKPRE_CPP_SOURCES)) \
                      $(patsubst third_party/silkpre/lib/%.c,build/x86/silkpre_%.o,$(SILKPRE_C_SOURCES))
LIBFF_OBJECTS_X86 = $(patsubst third_party/silkpre/third_party/libff/libff/%.cpp,build/x86/libff_%.o,$(LIBFF_CPP_SOURCES))
SECP256K1_OBJECTS_X86 = $(patsubst third_party/silkpre/third_party/secp256k1/src/%.c,build/x86/secp256k1_%.o,$(SECP256K1_C_SOURCES))
BLST_OBJECTS_X86 = $(patsubst third_party/blst/src/%.c,build/x86/blst_%.o,$(BLST_C_SOURCES)) \
                   $(patsubst third_party/blst/build/%.S,build/x86/blst_%.o,$(BLST_ASM_SOURCES))
OBJECTS_X86 = $(patsubst %.cpp,build/x86/%.o,$(SOURCES_X86)) \
              $(patsubst %.S,build/x86/%_asm.o,$(ASM_SOURCES_X86)) \
              $(ETHASH_OBJECTS_X86) \
              $(SILKPRE_OBJECTS_X86) \
              $(LIBFF_OBJECTS_X86) \
              $(SECP256K1_OBJECTS_X86) \
              $(BLST_OBJECTS_X86)

# For RISC-V: cpp files + newlib C library sources
LIBC_ASM_SOURCES = $(wildcard libc_riscv/*.S)
LIBC_OBJECTS_RISCV = $(patsubst libc_riscv/%.S,build/riscv/libc_riscv_%.o,$(LIBC_ASM_SOURCES))
ETHASH_OBJECTS_RISCV = $(patsubst third_party/ethash/lib/%.cpp,build/riscv/ethash_%.o,$(ETHASH_CPP_SOURCES)) \
                       $(patsubst third_party/ethash/lib/%.c,build/riscv/ethash_%.o,$(ETHASH_C_SOURCES))
SILKPRE_OBJECTS_RISCV = $(patsubst third_party/silkpre/lib/%.cpp,build/riscv/silkpre_%.o,$(SILKPRE_CPP_SOURCES)) \
                        $(patsubst third_party/silkpre/lib/%.c,build/riscv/silkpre_%.o,$(SILKPRE_C_SOURCES))
LIBFF_OBJECTS_RISCV = $(patsubst third_party/silkpre/third_party/libff/libff/%.cpp,build/riscv/libff_%.o,$(LIBFF_CPP_SOURCES))
SECP256K1_OBJECTS_RISCV = $(patsubst third_party/silkpre/third_party/secp256k1/src/%.c,build/riscv/secp256k1_%.o,$(SECP256K1_C_SOURCES))
# BLST not included for RISC-V (heavily relies on x86/ARM assembly optimizations)
# BLST_OBJECTS_RISCV = $(patsubst third_party/blst/src/%.c,build/riscv/blst_%.o,$(BLST_C_SOURCES))
MINI_GMP_OBJECTS_RISCV = $(patsubst third_party/gmp/mini-gmp/%.c,build/riscv/mini_gmp_%.o,$(MINI_GMP_C_SOURCES))
GMP_COMPAT_OBJECTS_RISCV = $(patsubst %.c,build/riscv/%.o,$(GMP_COMPAT_C_SOURCES))
OBJECTS_RISCV = $(patsubst %.cpp,build/riscv/%.o,$(SOURCES_RISCV)) \
                $(LIBC_OBJECTS_RISCV) \
                $(ETHASH_OBJECTS_RISCV) \
                $(SILKPRE_OBJECTS_RISCV) \
                $(LIBFF_OBJECTS_RISCV) \
                $(SECP256K1_OBJECTS_RISCV) \
                $(MINI_GMP_OBJECTS_RISCV) \
                $(GMP_COMPAT_OBJECTS_RISCV)

# SP1 (rv32im): same exclusions as RISC-V (no JSON, no BLST)
SOURCES_SP1 = $(filter-out $(JSON_SOURCES) execution/ethereum/precompiles_bls12.cpp,$(SOURCES_COMMON))
LIBC_ASM_SOURCES_SP1 = $(wildcard libc_riscv/*.S)
LIBC_OBJECTS_SP1 = $(patsubst libc_riscv/%.S,build/sp1/libc_riscv_%.o,$(LIBC_ASM_SOURCES_SP1))
ETHASH_OBJECTS_SP1 = $(patsubst third_party/ethash/lib/%.cpp,build/sp1/ethash_%.o,$(ETHASH_CPP_SOURCES)) \
                     $(patsubst third_party/ethash/lib/%.c,build/sp1/ethash_%.o,$(ETHASH_C_SOURCES))
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
LIB_RISCV = build/riscv/libmonad_wrapper.a
LIB_SP1 = build/sp1/libmonad_wrapper.a

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

# x86 target using standard g++
x86: apply-patches $(LIB_X86)

$(LIB_X86): $(OBJECTS_X86)
	@mkdir -p build/x86
	ar rcs $(LIB_X86) $(OBJECTS_X86)

build/x86/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/runtime/%.o: runtime/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/interpreter/%.o: interpreter/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/core/%.o: core/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/evm/%.o: evm/%.cpp
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

build/x86/%_asm.o: %.S
	@mkdir -p $(dir $@)
	$(CXX_X86) -c $< -o $@

build/x86/runtime/%_asm.o: runtime/%.S
	@mkdir -p $(dir $@)
	$(CXX_X86) -c $< -o $@

build/x86/interpreter/%_asm.o: interpreter/%.S
	@mkdir -p $(dir $@)
	$(CXX_X86) -c $< -o $@

# Ethash library compilation for x86
build/x86/ethash_%.o: third_party/ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/ethash_%.o: third_party/ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -c $< -o $@

# Silkpre library compilation for x86
build/x86/silkpre_%.o: third_party/silkpre/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/silkpre_%.o: third_party/silkpre/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -c $< -o $@

# Libff library compilation for x86
build/x86/libff_%.o: third_party/silkpre/third_party/libff/libff/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -DNO_PROCPS=1 -DCURVE_BN128 -DBN_SUPPORT_SNARK=1 -c $< -o $@

# Secp256k1 library compilation for x86
build/x86/secp256k1_%.o: third_party/silkpre/third_party/secp256k1/src/%.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -DECMULT_GEN_PREC_BITS=4 -DECMULT_WINDOW_SIZE=15 -DENABLE_MODULE_ECDH -DENABLE_MODULE_RECOVERY -c $< -o $@

# BLST library compilation for x86
build/x86/blst_server.o: third_party/blst/src/server.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -D__ADX__ -c $< -o $@

build/x86/blst_assembly.o: third_party/blst/build/assembly.S
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -D__ADX__ -c $< -o $@

# RISC-V target using riscv64-unknown-elf-g++
riscv: apply-patches $(LIB_RISCV)

$(LIB_RISCV): $(OBJECTS_RISCV)
	@mkdir -p build/riscv
	$(AR_RISCV) rcs $(LIB_RISCV) $(OBJECTS_RISCV)

build/riscv/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/runtime/%.o: runtime/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/interpreter/%.o: interpreter/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/core/%.o: core/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/evm/%.o: evm/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/evmone_host/%.o: evmone_host/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/execution/ethereum/%.o: execution/ethereum/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/evmone_test/%.o: evmone_test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/libc_riscv_%.o: libc_riscv/%.S
	@mkdir -p build/riscv
	$(CXX_RISCV) $(ASFLAGS_RISCV) -c $< -o $@

# Ethash library compilation for RISC-V
build/riscv/ethash_%.o: third_party/ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/ethash_%.o: third_party/ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV) $(CFLAGS_RISCV) -c $< -o $@

# Silkpre library compilation for RISC-V
build/riscv/silkpre_%.o: third_party/silkpre/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/silkpre_%.o: third_party/silkpre/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV) $(CFLAGS_RISCV) -c $< -o $@

# Libff library compilation for RISC-V
build/riscv/libff_%.o: third_party/silkpre/third_party/libff/libff/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -DNO_PROCPS=1 -DCURVE_BN128 -DBN_SUPPORT_SNARK=1 -c $< -o $@

# Secp256k1 library compilation for RISC-V
build/riscv/secp256k1_%.o: third_party/silkpre/third_party/secp256k1/src/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV) $(CFLAGS_RISCV) -DECMULT_GEN_PREC_BITS=4 -DECMULT_WINDOW_SIZE=15 -c $< -o $@

# Mini-GMP library compilation for RISC-V
build/riscv/mini_gmp_%.o: third_party/gmp/mini-gmp/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV) $(CFLAGS_RISCV) -c $< -o $@

# GMP compatibility layer compilation for RISC-V
build/riscv/gmp_mini_compat.o: gmp_mini_compat.c
	@mkdir -p $(dir $@)
	$(CC_RISCV) $(CFLAGS_RISCV) -c $< -o $@

# SP1 target using riscv32-unknown-elf-g++ (32-bit RISC-V for Succinct SP1 zkVM)
sp1: apply-patches $(LIB_SP1)

$(LIB_SP1): $(OBJECTS_SP1)
	@mkdir -p build/sp1
	$(AR_SP1) rcs $(LIB_SP1) $(OBJECTS_SP1)

build/sp1/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/runtime/%.o: runtime/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/interpreter/%.o: interpreter/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/core/%.o: core/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/evm/%.o: evm/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/evmone_host/%.o: evmone_host/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/execution/ethereum/%.o: execution/ethereum/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/evmone_test/%.o: evmone_test/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/libc_riscv_%.o: libc_riscv/%.S
	@mkdir -p build/sp1
	$(CC_SP1) $(ASFLAGS_SP1) -c $< -o $@

# Ethash library compilation for SP1
build/sp1/ethash_%.o: third_party/ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/ethash_%.o: third_party/ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_SP1) $(CFLAGS_SP1) -c $< -o $@

# Silkpre library compilation for SP1
build/sp1/silkpre_%.o: third_party/silkpre/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -c $< -o $@

build/sp1/silkpre_%.o: third_party/silkpre/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_SP1) $(CFLAGS_SP1) -c $< -o $@

# Libff library compilation for SP1
build/sp1/libff_%.o: third_party/silkpre/third_party/libff/libff/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_SP1) $(CXXFLAGS_SP1) -DNO_PROCPS=1 -DCURVE_BN128 -DBN_SUPPORT_SNARK=1 -c $< -o $@

# Secp256k1 library compilation for SP1
build/sp1/secp256k1_%.o: third_party/silkpre/third_party/secp256k1/src/%.c
	@mkdir -p $(dir $@)
	$(CC_SP1) $(CFLAGS_SP1) -DECMULT_GEN_PREC_BITS=4 -DECMULT_WINDOW_SIZE=15 -c $< -o $@

# Mini-GMP library compilation for SP1
build/sp1/mini_gmp_%.o: third_party/gmp/mini-gmp/%.c
	@mkdir -p $(dir $@)
	$(CC_SP1) $(CFLAGS_SP1) -c $< -o $@

# GMP compatibility layer compilation for SP1
build/sp1/gmp_mini_compat.o: gmp_mini_compat.c
	@mkdir -p $(dir $@)
	$(CC_SP1) $(CFLAGS_SP1) -c $< -o $@

# Converter binary (x86 only) - converts JSON blockchain tests to binary format
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

.PHONY: all x86 riscv sp1 converter clean apply-patches
