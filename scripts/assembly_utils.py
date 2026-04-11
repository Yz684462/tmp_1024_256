#!/usr/bin/env python3
"""
Assembly utilities for RISC-V vector translation.
Contains helper functions for extracting assembly ranges and compiling to shared libraries.
"""

import os
import subprocess
import sys
from typing import List


def extract_lines_in_range(dump_lines: List[str], start_addr: int, end_addr: int) -> List[str]:
    """Extract assembly lines within the address range [start_addr, end_addr)."""
    extracted = []
    print(f"Extracting lines from {start_addr} to {end_addr}")
    
    for line in dump_lines:
        line = line.rstrip()
        if not line.strip() or ':' not in line:
            continue
        
        addr_part = line.split(':')[0].strip()
        try:
            addr = int(addr_part, 16)  # Convert hexadecimal string to integer
        except ValueError:
            continue
        
        if start_addr <= addr < end_addr:
            extracted.append(line)
    
    return extracted


def compile_to_shared_library(asm_content: str, output_file:str) -> str:
    """Compile translated assembly to shared library."""
    # Write combined assembly to file
    translated_file = "translate.s"
    with open(translated_file, 'w', encoding='utf-8') as f:
        f.write(asm_content)
    
    print(f"Combined translated assembly written to {translated_file}")
    
    # Compile to shared library
    compile_cmd = [
        "gcc",
        "-march=rv64gcv_zba",
        translated_file,
        "-shared",
        "-fPIC",
        "-o", output_file
    ]
    
    try:
        subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
        print(f"Successfully compiled {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Compilation failed:\n{e.stderr}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print("Error: 'gcc' not found. Please install GCC.", file=sys.stderr)
        sys.exit(1)
    finally:
        # Clean up temporary file
        if os.path.exists(translated_file):
            os.remove(translated_file)
    return output_file
