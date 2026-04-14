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


def wrap_content_in_function(content: str, func_name: str) -> str:
    """Wrap 4 bytes of content into an assembly function."""
    
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


def parse_arguments() -> tuple[str, int, str, str]:
    """Parse command line arguments."""
    if len(sys.argv) != 5:
        print("Usage: python inst_to_so.py <dump_file> <address> <func_name> <output_file>")
        print("Address can be decimal or hexadecimal (e.g., 4096 or 0x1000)")
        print("Example: python inst_to_so.py dump.s 0x1000 migration_func migration_code")
        sys.exit(1)
    
    try:
        dump_file = sys.argv[1]
        address = int(sys.argv[2], 0)  # Supports both decimal and hex
        func_name = sys.argv[3]
        output_file = sys.argv[4]
    except ValueError as e:
        print(f"Error: Invalid address '{sys.argv[2]}'. Must be a valid integer.")
        sys.exit(1)
    
    return dump_file, address, func_name, output_file


def main():
    """Main conversion function."""
    # Parse arguments
    dump_file, target_addr, func_name, output_file = parse_arguments()
    
    print(f"Processing 4 bytes at address: {target_addr} (0x{target_addr:x})")
    print(f"Function name: {func_name}")
    print(f"Output file: {output_file}")
    
    # Read dump file
    if not os.path.isfile(dump_file):
        print(f"Error: File not found - {dump_file}", file=sys.stderr)
        sys.exit(1)
    
    with open(dump_file, 'r', encoding='utf-8') as f:
        dump_lines = f.readlines()
    
    # Extract 4 bytes of content at target address
    content = extract_4bytes_content(dump_lines, target_addr)
    if not content:
        sys.exit(1)
    
    # Wrap content in assembly function
    asm_content = wrap_content_in_function(content, func_name)
    print(f"Generated assembly function:\n{asm_content}")
    
    # Compile to shared library using utility function
    compiled_lib = compile_to_shared_library(asm_content, output_file)
    
    print(f"\nSuccessfully compiled: {compiled_lib}")
    print(f"Function name: {func_name}")


if __name__ == "__main__":
    main()
