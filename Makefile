# Makefile for Binary Translation Project
# Author: Generated for 1024_256_patch project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -march=rv64gcv -Wall -Wextra -O2 -fPIC
LDFLAGS = -shared -ldl -lpthread

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
TEST_DIR = test

# Source files
CORE_SOURCES = $(wildcard $(SRC_DIR)/core/*.cpp)
UTILS_SOURCES = $(wildcard $(SRC_DIR)/utils/*.cpp)
VECTOR_SOURCES = $(wildcard $(SRC_DIR)/vector_translation/*.cpp)
MAIN_SOURCE = $(SRC_DIR)/main.cpp

# Object files
CORE_OBJECTS = $(CORE_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
UTILS_OBJECTS = $(UTILS_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
VECTOR_OBJECTS = $(VECTOR_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
MAIN_OBJECT = $(MAIN_SOURCE:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

ALL_OBJECTS = $(CORE_OBJECTS) $(UTILS_OBJECTS) $(VECTOR_OBJECTS) $(MAIN_OBJECT)

# Target
TARGET = inject_lib.so

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/core $(BUILD_DIR)/utils $(BUILD_DIR)/vector_translation

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Link target
$(TARGET): $(ALL_OBJECTS)
	$(CXX) $(LDFLAGS) $(ALL_OBJECTS) -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Install target (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/lib/

# Uninstall target (optional)
uninstall:
	rm -f /usr/local/lib/$(TARGET)

# Test target
test: $(TARGET)
	@echo "Running tests..."
	@cd $(TEST_DIR) && make test

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build
release: CXXFLAGS += -DNDEBUG -O3
release: clean $(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all      - Build the library (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  debug    - Build with debug symbols"
	@echo "  release  - Build optimized release version"
	@echo "  test     - Run tests"
	@echo "  install  - Install library to system"
	@echo "  uninstall- Remove library from system"
	@echo "  help     - Show this help message"

# Phony targets
.PHONY: all clean install uninstall test debug release help

# Dependencies
$(CORE_OBJECTS): $(INCLUDE_DIR)/core.h $(INCLUDE_DIR)/utils.h
$(UTILS_OBJECTS): $(INCLUDE_DIR)/utils.h
$(VECTOR_OBJECTS): $(INCLUDE_DIR)/vector_translation.h $(INCLUDE_DIR)/utils.h
$(MAIN_OBJECT): $(INCLUDE_DIR)/core.h $(INCLUDE_DIR)/utils.h
