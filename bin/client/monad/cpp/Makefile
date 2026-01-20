# Compilers
CXX_X86 = g++
CXX_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-g++
AR_RISCV = /home/sam/.zisk/riscv/bin/riscv64-unknown-elf-ar

# Compiler flags
CXXFLAGS_COMMON = -std=c++23 -O3
CXXFLAGS_X86 = $(CXXFLAGS_COMMON) -fPIC
CXXFLAGS_RISCV = $(CXXFLAGS_COMMON) -march=rv64ima -mabi=lp64 -nostartfiles -mcmodel=medany -fno-exceptions -fno-rtti

# Source files - automatically find all .cpp files in current directory
SOURCES = $(wildcard *.cpp)
OBJECTS_X86 = $(patsubst %.cpp,build/x86/%.o,$(SOURCES))
OBJECTS_RISCV = $(patsubst %.cpp,build/riscv/%.o,$(SOURCES))

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

# RISC-V target using riscv64-unknown-elf-g++
riscv: $(LIB_RISCV)

$(LIB_RISCV): $(OBJECTS_RISCV)
	@mkdir -p build/riscv
	$(AR_RISCV) rcs $(LIB_RISCV) $(OBJECTS_RISCV)

build/riscv/%.o: %.cpp
	@mkdir -p build/riscv
	$(CXX_RISCV) $(CXXFLAGS_RISCV) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf build

.PHONY: all x86 riscv clean
