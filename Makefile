# COSPAS-SARSAT T.001 Beacon Transmitter Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11
INCLUDES = -Iinclude
LIBS = -liio -lm

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Target executable
TARGET = $(BIN_DIR)/sarsat_t001

# Source files
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/t001_protocol.c \
          $(SRC_DIR)/biphase_modulator.c \
          $(SRC_DIR)/bessel_filter.c \
          $(SRC_DIR)/pluto_control.c \
          $(SRC_DIR)/gpio_control.c \
          $(SRC_DIR)/gps_nmea.c \
          $(SRC_DIR)/homing_generator.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Header dependencies
HEADERS = $(INC_DIR)/t001_protocol.h \
          $(INC_DIR)/biphase_modulator.h \
          $(INC_DIR)/bessel_filter.h \
          $(INC_DIR)/pluto_control.h \
          $(INC_DIR)/gpio_control.h \
          $(INC_DIR)/gps_nmea.h \
          $(INC_DIR)/homing_generator.h

# Default target
all: directories $(TARGET)

# Create build directories
directories:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Link executable
$(TARGET): $(OBJECTS)
	@echo "Linking $@"
	@$(CC) $(OBJECTS) $(LIBS) -o $@
	@echo "Build complete: $@"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Install (copy to /usr/local/bin)
install: $(TARGET)
	@echo "Installing to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/
	@echo "Install complete"

# Uninstall
uninstall:
	@echo "Uninstalling..."
	@sudo rm -f /usr/local/bin/sarsat_t001
	@echo "Uninstall complete"

# Run (for testing)
run: $(TARGET)
	@echo "Running $(TARGET)..."
	@$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean all

# Help
help:
	@echo "COSPAS-SARSAT T.001 Beacon Transmitter Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the project (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall  - Remove from /usr/local/bin (requires sudo)"
	@echo "  run        - Build and run the program"
	@echo "  debug      - Build with debug symbols"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Dependencies:"
	@echo "  - libiio (PlutoSDR control)"
	@echo "  - libm (math library)"
	@echo ""
	@echo "Example:"
	@echo "  make clean && make"
	@echo "  sudo make install"
	@echo "  sarsat_t001 -f 431975000 -g -10 -m 0"

.PHONY: all clean install uninstall run debug help directories
