# Project name and directories
PROJECT_NAME  = OS2025
SRC_DIR       = src
BUILD_DIR     = bin
INCLUDE_DIR   = src/header

# Tools
CC      = gcc
ASM     = nasm
LINKER  = ld
GENISO  = genisoimage

# Source files - automatically find all .c and .s files
C_SOURCES   := $(shell find $(SRC_DIR) -name "*.c")
ASM_SOURCES := $(shell find $(SRC_DIR) -name "*.s")

# Object files
C_OBJECTS   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.s,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
ALL_OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

# Flags
WARNING_CFLAGS = -Wall -Wextra -Werror
DEBUG_CFLAGS   = -fshort-wchar -g
STRIP_CFLAGS   = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS         = $(DEBUG_CFLAGS) $(WARNING_CFLAGS) $(STRIP_CFLAGS) -m32 -c -I$(INCLUDE_DIR)
ASMFLAGS       = -f elf32 -g -F dwarf
LDFLAGS        = -T $(SRC_DIR)/linker.ld -melf_i386
ISOFLAGS       = -R -b boot/grub/grub1 -no-emul-boot -boot-load-size 4 -A os -input-charset utf8 -quiet -boot-info-table

# Targets
.PHONY: all clean build iso run

all: build

# Create directories for object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) $< -o $@

# Link everything together
$(BUILD_DIR)/kernel: $(ALL_OBJECTS)
	@echo "Linking object files to generate elf32..."
	$(LINKER) $(LDFLAGS) $(ALL_OBJECTS) -o $@

# Build kernel
kernel: $(BUILD_DIR)/kernel

# Create ISO image
iso: kernel
	@mkdir -p $(BUILD_DIR)/iso/boot/grub
	@cp $(BUILD_DIR)/kernel $(BUILD_DIR)/iso/boot/
	@cp other/grub1 $(BUILD_DIR)/iso/boot/grub/
	@cp $(SRC_DIR)/menu.lst $(BUILD_DIR)/iso/boot/grub/
	@cd $(BUILD_DIR) && $(GENISO) $(ISOFLAGS) -o $(PROJECT_NAME).iso iso
	@echo "ISO created: $(BUILD_DIR)/$(PROJECT_NAME).iso"
	@rm -r $(BUILD_DIR)/iso/

# Build everything
build: iso

# Run in QEMU
run: build
	@echo "Starting QEMU..."
	@qemu-system-i386 -s -cdrom $(BUILD_DIR)/$(PROJECT_NAME).iso

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)/*
