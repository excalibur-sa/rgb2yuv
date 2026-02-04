# Makefile for RGB to YUV Conversion Library
# Cross-compilation enabled build system

# Toolchain configuration - can be overridden via environment variables
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip

# Project directory structure
PROJECT_ROOT := .
SRC_DIR := $(PROJECT_ROOT)/src
INC_DIR := $(PROJECT_ROOT)/include
SAMPLE_DIR := $(PROJECT_ROOT)/sample
BUILD_DIR := $(PROJECT_ROOT)/build
OUT_DIR := $(PROJECT_ROOT)/out
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin

# Source files and target files
LIB_SOURCES := $(SRC_DIR)/demosaic.c $(SRC_DIR)/yuv_convert.c
LIB_OBJECTS := $(LIB_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

SAMPLE_SOURCES := $(wildcard $(SAMPLE_DIR)/*.c)
SAMPLE_OBJECTS := $(SAMPLE_SOURCES:$(SAMPLE_DIR)/%.c=$(OBJ_DIR)/%.o)

# Library files and executables
STATIC_LIB := $(LIB_DIR)/libimageprocessor.a
SHARED_LIB := $(LIB_DIR)/libimageprocessor.so
# Generate executable for each sample source file
SAMPLE_BINS := $(SAMPLE_SOURCES:$(SAMPLE_DIR)/%.c=$(BIN_DIR)/%)

# Compilation flags
CFLAGS := -Wall -Wextra -std=c99 -O2 -fPIC
CPPFLAGS := -I$(INC_DIR)
LDFLAGS := -L$(LIB_DIR)
LIBS := -limageprocessor -lm

# Debug mode flags
ifdef DEBUG
    CFLAGS += -g -DDEBUG -O0
    STRIP := @echo "Debug mode - skipping strip for"
else
    # Disable strip on macOS to avoid issues
    ifeq ($(shell uname -s),Darwin)
        STRIP := @echo "macOS - skipping strip for"
    endif
endif

# Architecture-specific optimization flags
ifdef TARGET_ARCH
    ifeq ($(TARGET_ARCH),arm)
        CFLAGS += -mcpu=cortex-a9 -mfloat-abi=hard -mfpu=neon
    else ifeq ($(TARGET_ARCH),aarch64)
        CFLAGS += -mcpu=cortex-a53
    else ifeq ($(TARGET_ARCH),x86_64)
        CFLAGS += -march=native -mtune=native
    endif
endif

# Default target
.PHONY: all clean install uninstall help test debug release compile_db out_directories
.DEFAULT_GOAL := all

all: directories out_directories $(STATIC_LIB) $(SHARED_LIB) $(SAMPLE_BINS)

# Create directory structure
directories:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

# Create output directory structure
out_directories:
	@mkdir -p $(OUT_DIR)
	@for sample in $(notdir $(SAMPLE_SOURCES:.c=)); do \
		echo "Creating output directory: $(OUT_DIR)/$$sample"; \
		mkdir -p $(OUT_DIR)/$$sample; \
	done

# Compilation rules
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling library file: $<"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SAMPLE_DIR)/%.c
	@echo "Compiling sample program: $<"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Static library
$(STATIC_LIB): $(LIB_OBJECTS)
	@echo "Creating static library: $@"
	$(AR) rcs $@ $^

# Dynamic library
$(SHARED_LIB): $(LIB_OBJECTS)
	@echo "Creating dynamic library: $@"
	$(CC) -shared -o $@ $^ -lm
	$(STRIP) $@

# Sample programs (link with static library)
$(BIN_DIR)/%: $(SAMPLE_DIR)/%.c $(STATIC_LIB)
	@echo "Linking sample program: $@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(STATIC_LIB) -lm
	$(STRIP) $@

# Build libraries only
lib: directories $(STATIC_LIB) $(SHARED_LIB)

# Build samples only
sample: directories $(STATIC_LIB) $(SAMPLE_BINS)

# Test target
test: $(SAMPLE_BINS)
	@echo "Running all test programs..."
	@for bin in $(SAMPLE_BINS); do \
		echo "Running test: $$bin"; \
		if [ -f "$$bin" ]; then \
			cd $(dir $$bin) && ./$(notdir $$bin) || exit 1; \
		else \
			echo "Warning: $$bin does not exist"; \
		fi; \
	done

# Debug build
debug:
	$(MAKE) DEBUG=1 all

# Release build
release:
	$(MAKE) all

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf $(BUILD_DIR)
	@echo "Cleaning output files..."
	rm -rf $(OUT_DIR)

# Deep clean (including generated test files)
distclean: clean
	@echo "Deep cleaning..."
	rm -f *.yuv *.raw test_output_*.yuv output_*.yuv

# Installation (requires root privileges)
INSTALL_PREFIX ?= /usr/local
INSTALL_INCDIR := $(INSTALL_PREFIX)/include
INSTALL_LIBDIR := $(INSTALL_PREFIX)/lib
INSTALL_BINDIR := $(INSTALL_PREFIX)/bin

install: all
	@echo "Installing to $(INSTALL_PREFIX)..."
	install -d $(INSTALL_INCDIR) $(INSTALL_LIBDIR) $(INSTALL_BINDIR)
	install -m 644 $(INC_DIR)/image_processor.h $(INSTALL_INCDIR)/
	install -m 644 $(STATIC_LIB) $(INSTALL_LIBDIR)/
	install -m 755 $(SHARED_LIB) $(INSTALL_LIBDIR)/
	@for bin in $(SAMPLE_BINS); do \
		install -m 755 $$bin $(INSTALL_BINDIR)/; \
	done
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling from $(INSTALL_PREFIX)..."
	rm -f $(INSTALL_INCDIR)/image_processor.h
	rm -f $(INSTALL_LIBDIR)/libimageprocessor.a
	rm -f $(INSTALL_LIBDIR)/libimageprocessor.so
	@for bin in $(notdir $(SAMPLE_BINS)); do \
		rm -f $(INSTALL_BINDIR)/$$bin; \
	done
	@echo "Uninstallation complete"

# Display help information
help:
	@echo "RGB to YUV Conversion Library Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build all targets (default)"
	@echo "  lib        - Build library files only"
	@echo "  sample     - Build sample programs only"
	@echo "  test       - Run all test programs"
	@echo "  debug      - Debug mode build"
	@echo "  release    - Release mode build"
	@echo "  compile_db - Generate compile_commands.json"
	@echo "  clean      - Clean build files and output files"
	@echo "  distclean  - Deep clean all generated files"
	@echo "  install    - Install to system directories"
	@echo "  uninstall  - Uninstall from system directories"
	@echo "  help       - Display this help information"
	@echo ""
	@echo "Output file organization:"
	@echo "  Test-generated YUV files will be saved in out/ directory"
	@echo "  Each test program has its own subdirectory:"
	@echo "    out/test_image_processor/ - Main test program output"
	@echo "    out/full_test/           - Full test program output"
	@echo "    out/simple_test/         - Simple test program output"
	@echo ""
	@echo "Cross-compilation examples:"
	@echo "  make CROSS_COMPILE=arm-linux-gnueabihf- TARGET_ARCH=arm"
	@echo "  make CROSS_COMPILE=aarch64-linux-gnu- TARGET_ARCH=aarch64"
	@echo ""
	@echo "Environment variables:"
	@echo "  CROSS_COMPILE  - Cross-compilation toolchain prefix"
	@echo "  TARGET_ARCH    - Target architecture (arm, aarch64, x86_64)"
	@echo "  DEBUG          - Enable Debug mode (DEBUG=1)"
	@echo "  INSTALL_PREFIX - Installation prefix path (default: /usr/local)"

# Generate compile_commands.json for IDE intelligence
compile_db:
	@echo "Generating compile_commands.json..."
	@echo '[' > compile_commands.json
	@echo '  {' >> compile_commands.json
	@echo '    "directory": "$(shell pwd)",' >> compile_commands.json
	@echo '    "command": "$(CC) $(CFLAGS) $(CPPFLAGS) -c src/demosaic.c",' >> compile_commands.json
	@echo '    "file": "src/demosaic.c"' >> compile_commands.json
	@echo '  },' >> compile_commands.json
	@echo '  {' >> compile_commands.json
	@echo '    "directory": "$(shell pwd)",' >> compile_commands.json
	@echo '    "command": "$(CC) $(CFLAGS) $(CPPFLAGS) -c src/yuv_convert.c",' >> compile_commands.json
	@echo '    "file": "src/yuv_convert.c"' >> compile_commands.json
	@echo '  },' >> compile_commands.json
	@echo '  {' >> compile_commands.json
	@echo '    "directory": "$(shell pwd)",' >> compile_commands.json
	@echo '    "command": "$(CC) $(CFLAGS) $(CPPFLAGS) -c sample/test_image_processor.c",' >> compile_commands.json
	@echo '    "file": "sample/test_image_processor.c"' >> compile_commands.json
	@echo '  }' >> compile_commands.json
	@echo ']' >> compile_commands.json
	@echo "compile_commands.json generation complete"

# Display build information
info:
	@echo "Build configuration information:"
	@echo "  Cross-compile prefix: $(CROSS_COMPILE)"
	@echo "  Compiler: $(CC)"
	@echo "  Archiver: $(AR)"
	@echo "  Target architecture: $(TARGET_ARCH)"
	@echo "  Compilation flags: $(CFLAGS)"
	@echo "  Preprocessor flags: $(CPPFLAGS)"
	@echo "  Linker flags: $(LDFLAGS)"
	@echo "  Libraries: $(LIBS)"
	@echo "  Installation prefix: $(INSTALL_PREFIX)"

# Dependencies
$(LIB_OBJECTS): $(INC_DIR)/image_processor.h
$(SAMPLE_OBJECTS): $(INC_DIR)/image_processor.h

# Auto-generate dependencies (optional)
-include $(LIB_OBJECTS:.o=.d)
-include $(SAMPLE_OBJECTS:.o=.d)

%.d: %.c
	$(CC) -M $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$