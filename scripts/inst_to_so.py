#!/usr/bin/env python3
"""
4-Byte Content to Shared Library Converter

This script extracts 4 bytes of content from dump file at specified address,
wraps it into an assembly function, and compiles it to a shared library.
The 4 bytes may contain one 4-byte instruction or two 2-byte instructions.
"""

import sys
import os
from typing import List

# Import assembly utilities
from assembly_utils import extract_lines_in_range, compile_to_shared_library

# Constants
DUMP_FILE = "dump.s"


def extract_4bytes_content(dump_lines: List[str], target_addr: int) -> str:
    """Extract 4 bytes of content at specified address."""
    print(f"Extracting 4 bytes from address {target_addr} (0x{target_addr:x})")
    
    # Extract lines in range [target_addr, target_addr + 4)
    extracted_lines = extract_lines_in_range(dump_lines, target_addr, target_addr + 4)
    if not extracted_lines:
        print(f"Error: No content found at address {target_addr:x}", file=sys.stderr)
        return ""
    
    # Combine all extracted lines into content
    content_lines = []
    for line in extracted_lines:
        # Extract instruction part (after colon)
        if ':' in line:
            instruction_part = line.split(':', 1)[1].strip()
            if instruction_part:
                content_lines.append(instruction_part)
    
    content = '\n'.join(content_lines)
    print(f"Extracted content:\n{content}")
    return content


def wrap_content_in_function(content: str, addr: int) -> str:
    """Wrap 4 bytes of content into an assembly function."""
    func_name = f"content_func_{addr:x}"
    
    # Create assembly function wrapper
    asm_content = f"""\
    .globl {func_name}
    .type {func_name}, @function
{func_name}:
    {content}
    ret
    .size {func_name}, .-{func_name}
    
    .extern global_simulated_vector_contexts_pool
"""
    
    return asm_content


def parse_arguments() -> int:
    """Parse command line arguments."""
    if len(sys.argv) != 2:
        print("Usage: python inst_to_so.py <address>")
        print("Address can be decimal or hexadecimal (e.g., 4096 or 0x1000)")
        sys.exit(1)
    
    try:
        # Support both decimal and hexadecimal formats
        addr = int(sys.argv[1], 0)  # auto-detect base (0=auto)
    except ValueError:
        print(f"Error: Invalid address value: {sys.argv[1]}")
        print("Use decimal (4096) or hexadecimal (0x1000) format")
        sys.exit(1)
    
    return addr


def main():
    """Main conversion function."""
    # Parse arguments
    target_addr = parse_arguments()
    
    print(f"Processing 4 bytes at address: {target_addr} (0x{target_addr:x})")
    
    # Read dump file
    if not os.path.isfile(DUMP_FILE):
        print(f"Error: File not found - {DUMP_FILE}", file=sys.stderr)
        sys.exit(1)
    
    with open(DUMP_FILE, 'r', encoding='utf-8') as f:
        dump_lines = f.readlines()
    
    # Extract 4 bytes of content at target address
    content = extract_4bytes_content(dump_lines, target_addr)
    if not content:
        sys.exit(1)
    
    # Wrap content in assembly function
    asm_content = wrap_content_in_function(content, target_addr)
    print(f"Generated assembly function:\n{asm_content}")
    
    # Compile to shared library using utility function
    output_file = f"content_{target_addr:x}"
    compiled_lib = compile_to_shared_library(asm_content, output_file)
    
    print(f"\nSuccessfully compiled: {compiled_lib}")
    print(f"Function name: content_func_{target_addr:x}")


if __name__ == "__main__":
    main()
