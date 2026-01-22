# Compilers
CXX_X86 = g++-15
CXX_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-g++
AR_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-ar

# Compiler flags
CXXFLAGS_COMMON = -std=c++23 -O3
CXXFLAGS_X86 = $(CXXFLAGS_COMMON) -fPIC
CXXFLAGS_RISCV = $(CXXFLAGS_COMMON) -Ilibc_riscv -march=rv64ima -mabi=lp64 -nostartfiles -nostdlib++ -mcmodel=medany -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections

# Assembly flags (for .S files in RISC-V build)
ASFLAGS_RISCV = -march=rv64ima -mabi=lp64 -Ilibc_riscv

# Source files - automatically find all .cpp files in current directory
SOURCES = $(wildcard *.cpp)
ASM_SOURCES_X86 = runtime_math.S
# For assembly files, use _asm suffix to avoid conflict with .cpp files
OBJECTS_X86 = $(patsubst %.cpp,build/x86/%.o,$(SOURCES)) $(patsubst %.S,build/x86/%_asm.o,$(ASM_SOURCES_X86))

# For RISC-V: cpp files + newlib C library sources
LIBC_ASM_SOURCES = $(wildcard libc_riscv/*.S)
LIBC_OBJECTS_RISCV = $(patsubst libc_riscv/%.S,build/riscv/libc_riscv_%.o,$(LIBC_ASM_SOURCES))
OBJECTS_RISCV = $(patsubst %.cpp,build/riscv/%.o,$(SOURCES)) $(LIBC_OBJECTS_RISCV)

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
	@mkdir -p build/x86
	$(CXX_X86) $(CXXFLAGS_X86) -c $< -o $@

build/x86/%_asm.o: %.S
	@mkdir -p build/x86
	$(CXX_X86) -c $< -o $@

# RISC-V target using riscv64-unknown-elf-g++
riscv: $(LIB_RISCV)

$(LIB_RISCV): $(OBJECTS_RISCV)
	@mkdir -p build/riscv
	$(AR_RISCV) rcs $(LIB_RISCV) $(OBJECTS_RISCV)

build/riscv/%.o: %.cpp
	@mkdir -p build/riscv
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

build/riscv/libc_riscv_%.o: libc_riscv/%.S
	@mkdir -p build/riscv
	$(CXX_RISCV) $(ASFLAGS_RISCV) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf build

.PHONY: all x86 riscv clean
