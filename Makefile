CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 $(shell pkg-config --cflags r_core)
INCLUDES = -Iinclude

# Configuration group (default: 1)
CONFIG_GROUP ?= 1
CXXFLAGS += -DCONFIG_GROUP=$(CONFIG_GROUP)

# Override CONFIG_GROUP for test1 target
ifdef CONFIG_GROUP_OVERRIDE
CONFIG_GROUP = $(CONFIG_GROUP_OVERRIDE)
endif
SRCDIR = src
INCDIR = include
TESTDIR = test

# Source files
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/patch.cpp $(SRCDIR)/addr.cpp $(SRCDIR)/handle.cpp $(SRCDIR)/vector_context.cpp $(SRCDIR)/cpu.cpp $(SRCDIR)/config.cpp $(SRCDIR)/globals.cpp $(SRCDIR)/addr_analysis.cpp $(SRCDIR)/binary.cpp $(SRCDIR)/thread.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable
TARGET = framework

# Test targets
TEST_TARGET = test_addr
TEST_SOURCES = $(TESTDIR)/test_addr.cpp $(SRCDIR)/addr.cpp $(SRCDIR)/addr_analysis.cpp $(SRCDIR)/config.cpp $(SRCDIR)/globals.cpp $(SRCDIR)/binary.cpp
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

# Alternative test target with CONFIG_GROUP=2
TEST1_TARGET = test_addr1
TEST1_SOURCES = $(TESTDIR)/test_addr.cpp $(SRCDIR)/addr.cpp $(SRCDIR)/addr_analysis.cpp $(SRCDIR)/config.cpp $(SRCDIR)/globals.cpp $(SRCDIR)/binary.cpp
TEST1_OBJECTS = $(TEST1_SOURCES:.cpp=.o)

# Link flags
LDFLAGS = -ldl -lpthread

.PHONY: all clean test test1 test-run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Build test executable
test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(TEST_OBJECTS) -o $(TEST_TARGET) $(LDFLAGS)

# Build test executable with CONFIG_GROUP=2
test1: CONFIG_GROUP_OVERRIDE=2 $(TEST1_TARGET)

$(TEST1_TARGET): $(TEST1_OBJECTS)
	$(CXX) $(CXXFLAGS) $(TEST1_OBJECTS) -o $(TEST1_TARGET) $(LDFLAGS)

# Run test
test-run: $(TEST_TARGET)
	./$(TEST_TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_OBJECTS) $(TEST_TARGET) $(TEST1_OBJECTS) $(TEST1_TARGET)

clean-test:
	rm -f $(TEST_OBJECTS) $(TEST_TARGET) $(TEST1_OBJECTS) $(TEST1_TARGET)

install:
	@echo "Installing framework..."
	cp $(TARGET) /usr/local/bin/

help:
	@echo "Available targets:"
	@echo "  all       - Build the framework"
	@echo "  test      - Build test executable (CONFIG_GROUP=1)"
	@echo "  test1     - Build test executable with CONFIG_GROUP=2"
	@echo "  test-run  - Build and run test executable"
	@echo "  clean     - Remove built files"
	@echo "  clean-test- Remove test files only"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Configuration groups:"
	@echo "  CONFIG_GROUP=1  - Default configuration (func_offset=0x2)"
	@echo "  CONFIG_GROUP=2  - Alternative configuration 1 (func_offset=0x4)"
	@echo "  CONFIG_GROUP=3  - Alternative configuration 2 (func_offset=0x8)"
	@echo ""
	@echo "Usage examples:"
	@echo "  make test CONFIG_GROUP=1    # Build with group 1 configuration"
	@echo "  make test CONFIG_GROUP=2    # Build with group 2 configuration"
	@echo "  make test CONFIG_GROUP=3    # Build with group 3 configuration"
