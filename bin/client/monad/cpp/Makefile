# Compilers
CXX_X86 = g++-15
CC_X86 = gcc-15
CXX_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-g++
CC_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-gcc
AR_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-ar

# Compiler flags
CXXFLAGS_COMMON = -std=c++23 -O3 -Ievmc/include -Iintx/include -Iethash/include -Iunordered_dense/include
CXXFLAGS_X86 = $(CXXFLAGS_COMMON) -fPIC
CXXFLAGS_RISCV = $(CXXFLAGS_COMMON) -Ilibc_riscv -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections -DNDEBUG

# C compiler flags (for .c files)
CFLAGS_COMMON = -O3 -Iethash/include
CFLAGS_X86 = $(CFLAGS_COMMON) -fPIC
CFLAGS_RISCV = $(CFLAGS_COMMON) -Ilibc_riscv -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib -mcmodel=medany -ffunction-sections -fdata-sections

# Assembly flags (for .S files in RISC-V build)
ASFLAGS_RISCV = -march=rv64ima -mabi=lp64 -Ilibc_riscv

# Common source files for both targets
SOURCES_COMMON = $(wildcard *.cpp) $(wildcard runtime/*.cpp) $(wildcard interpreter/*.cpp) $(wildcard core/*.cpp) $(wildcard evm/*.cpp) $(wildcard evmone_host/*.cpp)

# Both x86 and RISC-V use the same sources
SOURCES_X86 = $(SOURCES_COMMON)
SOURCES_RISCV = $(SOURCES_COMMON)

ASM_SOURCES_X86 = runtime/math.S interpreter/entry.S runtime/exit.S

# Ethash library sources
ETHASH_CPP_SOURCES = ethash/lib/ethash/ethash.cpp
ETHASH_C_SOURCES = ethash/lib/keccak/keccak.c ethash/lib/ethash/primes.c

# For assembly files, use _asm suffix to avoid conflict with .cpp files
ETHASH_OBJECTS_X86 = $(patsubst ethash/lib/%.cpp,build/x86/ethash_%.o,$(ETHASH_CPP_SOURCES)) \
                     $(patsubst ethash/lib/%.c,build/x86/ethash_%.o,$(ETHASH_C_SOURCES))
OBJECTS_X86 = $(patsubst %.cpp,build/x86/%.o,$(SOURCES_X86)) \
              $(patsubst %.S,build/x86/%_asm.o,$(ASM_SOURCES_X86)) \
              $(ETHASH_OBJECTS_X86)

# For RISC-V: cpp files + newlib C library sources
LIBC_ASM_SOURCES = $(wildcard libc_riscv/*.S)
LIBC_OBJECTS_RISCV = $(patsubst libc_riscv/%.S,build/riscv/libc_riscv_%.o,$(LIBC_ASM_SOURCES))
ETHASH_OBJECTS_RISCV = $(patsubst ethash/lib/%.cpp,build/riscv/ethash_%.o,$(ETHASH_CPP_SOURCES)) \
                       $(patsubst ethash/lib/%.c,build/riscv/ethash_%.o,$(ETHASH_C_SOURCES))
OBJECTS_RISCV = $(patsubst %.cpp,build/riscv/%.o,$(SOURCES_RISCV)) \
                $(LIBC_OBJECTS_RISCV) \
                $(ETHASH_OBJECTS_RISCV)

# Library outputs
LIB_X86 = build/x86/libmonad_wrapper.a
LIB_RISCV = build/riscv/libmonad_wrapper.a

# Default target
all: x86

# x86 target using standard g++
x86: $(LIB_X86)

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
build/x86/ethash_%.o: ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/ethash_%.o: ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_X86) $(CFLAGS_X86) -c $< -o $@

# RISC-V target using riscv64-unknown-elf-g++
riscv: $(LIB_RISCV)

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

build/riscv/libc_riscv_%.o: libc_riscv/%.S
	@mkdir -p build/riscv
	$(CXX_RISCV) $(ASFLAGS_RISCV) -c $< -o $@

# Ethash library compilation for RISC-V
build/riscv/ethash_%.o: ethash/lib/%.cpp
	@mkdir -p $(dir $@)
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/ethash_%.o: ethash/lib/%.c
	@mkdir -p $(dir $@)
	$(CC_RISCV) $(CFLAGS_RISCV) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf build

.PHONY: all x86 riscv clean
