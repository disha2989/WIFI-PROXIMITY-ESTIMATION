# Project Directories
SRC_DIR := src
BIN_DIR := bin
SDK_DIR := $(shell find . -maxdepth 1 -type d -name "openwrt-sdk*" | head -n 1)
STAGING_DIR := $(abspath $(SDK_DIR)/staging_dir)
TOOLCHAIN_BIN := $(shell find $(STAGING_DIR)/toolchain-* -type d -name "bin" | head -n 1)

# Compiler
CROSS_COMPILE := $(TOOLCHAIN_BIN)/aarch64-openwrt-linux-musl-gcc

# Compiler flags
CFLAGS := -static -lm

# Source files
SOURCES := rssi_distance.c snr_distance.c rtt_distance.c
TARGETS := $(patsubst %.c,$(BIN_DIR)/%,$(SOURCES))

# Default target
all: export STAGING_DIR := $(STAGING_DIR)
all: export PATH := $(TOOLCHAIN_BIN):$(PATH)
all: $(TARGETS)

# Build rules
$(BIN_DIR)/%: $(SRC_DIR)/%.c | $(BIN_DIR)
	@echo "[+] Compiling $< ..."
	$(CROSS_COMPILE) $(CFLAGS) $< -o $@

# Create bin directory if not present
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# SCP target (Change router IP as needed)
ROUTER_IP := 192.168.1.1
ROUTER_USER := root
REMOTE_PATH := /root/

deploy: all
	@echo "[+] Copying binaries to router..."
	scp $(TARGETS) $(ROUTER_USER)@$(ROUTER_IP):$(REMOTE_PATH)

# Clean target
clean:
	@echo "[+] Cleaning binaries..."
	rm -rf $(BIN_DIR)/*

.PHONY: all clean deploy
