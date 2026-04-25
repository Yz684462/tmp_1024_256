# Simple Makefile for patch project
# Easy to read and understand

# CC = riscv64-linux-gnu-g++
# k3上是g++
CC = g++
CFLAGS = -O0 -D_GNU_SOURCE -fPIC
LDFLAGS = -shared
LIBS = -lpthread -lbpf -lelf -lz -ldl

# Object files
OBJS = patch.o inject_lib.o

# Default target
all: inject_lib.so

# Build shared library (combines patch and main_patch functionality)
inject_lib.so: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile object files
patch.o: patch.c patch.h common.h
	gcc $(CFLAGS) -c $< -o $@

inject_lib.o: inject_lib.cpp patch.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f *.o *.so 

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Build everything (default)"
	@echo "  clean     - Remove build files"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build everything"
	@echo "  make clean              # Clean build files"
	@echo "  LD_PRELOAD=./libpatch.so ./main_patch"
