CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2
INCLUDES = -Iinclude
SRCDIR = src
INCDIR = include
TESTDIR = test

# Source files
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/patch.cpp $(SRCDIR)/addr.cpp $(SRCDIR)/handle.cpp $(SRCDIR)/vector_context.cpp $(SRCDIR)/cpu.cpp $(SRCDIR)/config.cpp $(SRCDIR)/globals.cpp $(SRCDIR)/addr_analysis.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Target executable
TARGET = framework

# Test targets
TEST_TARGET = test_addr
TEST_SOURCES = $(TESTDIR)/test_addr.cpp $(SRCDIR)/addr.cpp $(SRCDIR)/addr_init.cpp $(SRCDIR)/addr_analysis.cpp
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

.PHONY: all clean test test-run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) -ldl -lpthread

# Build test executable
test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(TEST_OBJECTS) -o $(TEST_TARGET) -ldl -lpthread

# Run test
test-run: $(TEST_TARGET)
	./$(TEST_TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_OBJECTS) $(TEST_TARGET)

clean-test:
	rm -f $(TEST_OBJECTS) $(TEST_TARGET)

install:
	@echo "Installing framework..."
	cp $(TARGET) /usr/local/bin/

help:
	@echo "Available targets:"
	@echo "  all       - Build the framework"
	@echo "  test      - Build test executable"
	@echo "  test-run  - Build and run test executable"
	@echo "  clean     - Remove built files"
	@echo "  clean-test- Remove test files only"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  help      - Show this help message"
