CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Isrc -D_GNU_SOURCE -fPIC

# --- DIRECTORY STRUCTURE ---
SRC_DIR     = src
EXAMPLES_DIR = examples
BUILD_DIR   = build

# --- SOURCE FILES ---
SRC_FILES   = $(SRC_DIR)/encoder.c $(SRC_DIR)/utils.c $(SRC_DIR)/json.c
SRC_HEADERS = $(SRC_DIR)/encoder.h $(SRC_DIR)/utils.h $(SRC_DIR)/json.h $(SRC_DIR)/pic.h

# --- OBJECT FILES ---
OBJ_DIR     = $(BUILD_DIR)/obj
HOST_OBJ    = $(OBJ_DIR)/encoder.o $(OBJ_DIR)/utils.o $(OBJ_DIR)/json.o

# --- OUTPUT BINARIES ---
BIN_DIR     = $(BUILD_DIR)/bin
OUT_BUILDER = $(BIN_DIR)/builder
OUT_LOADER  = $(BIN_DIR)/loader
OUT_TESTS   = $(BIN_DIR)/tests
OUT_IMPLANT = $(BIN_DIR)/implant.bin
OUT_PAYLOAD = $(BIN_DIR)/payload.bin

.PHONY: all clean test demo demo-json help dirs

all: dirs tools shellcode


# ---[ Create Build Directories

dirs:
	@mkdir -p $(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR)

====
# ---[ BUILD TOOLS (Host-Side)

tools: $(OUT_BUILDER) $(OUT_LOADER) $(OUT_TESTS)

$(OUT_BUILDER): $(EXAMPLES_DIR)/builder.c $(HOST_OBJ) | dirs
	@echo "[BUILD] C2 Builder..."
	$(CC) $(CFLAGS) -o $@ $(EXAMPLES_DIR)/builder.c $(HOST_OBJ)

$(OUT_LOADER): $(EXAMPLES_DIR)/loader.c $(OBJ_DIR)/json.o | dirs
	@echo "[BUILD] Shellcode Loader..."
	$(CC) $(CFLAGS) -o $@ $(EXAMPLES_DIR)/loader.c $(OBJ_DIR)/json.o

$(OUT_TESTS): $(SRC_DIR)/tests.c $(HOST_OBJ) | dirs
	@echo "[BUILD] Test Suite..."
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/tests.c $(HOST_OBJ)

# ---[ BUILD SHELLCODE (The Malware)

shellcode: $(OUT_IMPLANT) $(OUT_PAYLOAD)

$(OUT_IMPLANT): $(EXAMPLES_DIR)/implant.c $(SRC_DIR)/pic.h $(BUILD_DIR)/flat.ld | dirs
	@echo "[BUILD] Implant Shellcode..."
	$(CC) -O2 -fPIC -fno-jump-tables -fno-stack-protector -nostdlib -Isrc \
		-c $(EXAMPLES_DIR)/implant.c -o $(OBJ_DIR)/implant.o
	ld -T $(BUILD_DIR)/flat.ld -o $(BUILD_DIR)/implant.elf --entry=start $(OBJ_DIR)/implant.o
	objcopy -O binary $(BUILD_DIR)/implant.elf $@
	@echo "[INFO] Implant size: $$(stat -c%s $@ 2>/dev/null || stat -f%z $@) bytes"

$(BUILD_DIR)/flat.ld: | dirs
	@echo "[GEN] Creating linker script..."
	@echo "SECTIONS" > $@
	@echo "{" >> $@
	@echo "  . = 0;" >> $@
	@echo "  .text : { *(.text .text.*) }" >> $@
	@echo "  .rodata : { *(.rodata .rodata.*) }" >> $@
	@echo "  .data : { *(.data .data.*) }" >> $@
	@echo "  .bss : { *(.bss .bss.*) *(COMMON) }" >> $@
	@echo "  /DISCARD/ : { *(.note.*) *(.eh_frame) *(.comment) }" >> $@
	@echo "}" >> $@

$(OUT_PAYLOAD): $(EXAMPLES_DIR)/payload.c | dirs
	@echo "[BUILD] Raw Payload..."
	$(CC) -nostdlib -fPIC -c $(EXAMPLES_DIR)/payload.c -o $(OBJ_DIR)/payload.o
	objcopy -O binary -j .text $(OBJ_DIR)/payload.o $@
	@echo "[INFO] Payload size: $$(stat -c%s $@ 2>/dev/null || stat -f%z $@) bytes"


# ---[ VERIFICATION

test: $(OUT_TESTS)
	@echo ""
	@echo "=== RUNNING LOGIC VERIFICATION ==="
	@cd $(BIN_DIR) && ./tests


# ---[ KILL CHAIN - Binary

demo: tools shellcode
	@echo ""
	@echo "=== RUNNING KILL CHAIN DEMO (BINARY) ==="
	@echo "[1] C2: Generating Traffic..."
	@cd $(BIN_DIR) && ./builder traffic.dat @payload.bin
	@echo "[2] LOADER: Injecting Implant..."
	@cd $(BIN_DIR) && ./loader implant.bin traffic.dat


# ---[ KILL CHAIN - JSON

demo-json: tools shellcode
	@echo ""
	@echo "=== RUNNING KILL CHAIN DEMO (JSON) ==="
	@echo "[1] C2: Generating JSON Traffic..."
	@cd $(BIN_DIR) && ./builder traffic.json @payload.bin --json
	@echo "[2] LOADER: Injecting Implant (JSON)..."
	@cd $(BIN_DIR) && ./loader implant.bin traffic.json


# ---[ Object File Compilation Rules

$(OBJ_DIR)/encoder.o: $(SRC_DIR)/encoder.c $(SRC_DIR)/encoder.h $(SRC_DIR)/utils.h | dirs
	@echo "[COMPILE] encoder.c"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/json.o: $(SRC_DIR)/json.c $(SRC_DIR)/json.h $(SRC_DIR)/encoder.h | dirs
	@echo "[COMPILE] json.c"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/utils.o: $(SRC_DIR)/utils.c $(SRC_DIR)/utils.h | dirs
	@echo "[COMPILE] utils.c"
	@$(CC) $(CFLAGS) -c $< -o $@


# ---[ Clean Target

clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "[CLEAN] Done!"


# ---[ Help Target

help:
	@echo "Whisper - Covert Channel Protocol Library"
	@echo ""
	@echo "Directory Structure:"
	@echo "  src/         - Core library source files"
	@echo "  examples/    - Example tools and demonstrations"
	@echo "  build/       - Build output (created automatically)"
	@echo ""
	@echo "Available targets:"
	@echo "  make all         - Build all components (tools + shellcode)"
	@echo "  make tools       - Build host-side tools only"
	@echo "  make shellcode   - Build implant and payload"
	@echo "  make test        - Run unit and integration tests"
	@echo "  make demo        - Run full demo with binary format"
	@echo "  make demo-json   - Run full demo with JSON format"
	@echo "  make clean       - Remove all build artifacts"
	@echo "  make help        - Show this help message"
