#!/usr/bin/env python3
"""
RISC-V Vector Assembly Translator

This script translates RISC-V vector assembly code segments into C-compatible functions,
handling multiple address ranges and thread-specific offsets for simulated CPU state.
"""

import sys
import os
import re
from typing import List, Tuple

# Import assembly utilities
from assembly_utils import extract_lines_in_range
from assembly_utils import compile_to_shared_library as compile_lib

# Constants
VECTOR_CONTEXT_SIZE = 4192
DUMP_FILE = "dump.s"
EXTRACTED_FILE = "extracted.s"

# Setup path for rvv_sbt_tool
current_script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(current_script_path)
sys.path.append(os.path.join(script_dir, "..", "rvv_sbt_tool"))

from rvv_sbt_tool.rvv_sbt import translate_function
from rvv_sbt_tool.core.frontend.asm_parser import AsmParser


class AssemblyTranslator:
    """Handles translation of RISC-V assembly to C-compatible functions."""
    
    def __init__(self, thread_index: int):
        self.thread_index = thread_index
        self.translated_functions = []
    
    def translate_assembly(self, asm_content: str, func_name: str) -> str:
        """Translate assembly content to C-compatible function."""
        parser = AsmParser()
        
        # Write assembly content to temporary file for parsing
        with open(EXTRACTED_FILE, 'w', encoding='utf-8') as f:
            f.write(asm_content)
        
        insn_list = parser.parse_file(EXTRACTED_FILE)
        
        output = translate_function(
            insn_list,
            func_name=func_name,
            vlenb=128,
            source_file=EXTRACTED_FILE,
            text_only=False,
        )
        
        # Clean up temporary file
        os.remove(EXTRACTED_FILE)
        
        return self._modify_translated_code(output, func_name)
    
    def _modify_translated_code(self, translated: str, func_name: str) -> str:
        """Modify translated code to handle simulated_cpu_state with thread-specific offsets."""
        lines = translated.splitlines()
        modified_lines = []
        
        # Remove global simulated_cpu_state declarations
        for line in lines:
            if '.globl simulated_cpu_state' in line:
                continue
            if '.comm   simulated_cpu_state' in line:
                continue
            modified_lines.append(line)
            if '.type' in line:
                modified_lines.append('\t.extern global_simulated_vector_contexts_pool')
        
        translated = '\n'.join(modified_lines)
        
        # Add thread-specific offset after each "la t6, simulated_cpu_state"
        offset = self.thread_index * VECTOR_CONTEXT_SIZE
        
        # First replace simulated_cpu_state with global_simulated_vector_contexts_pool
        translated = translated.replace('simulated_cpu_state', 'global_simulated_vector_contexts_pool')
        
        # Then add offset after each "la t6, global_simulated_vector_contexts_pool"
        pattern = r'(\s+la\s+t6,\s+global_simulated_vector_contexts_pool)'
        
        # Use multiple instructions if offset is too large for addi
        if offset <= 2047:
            # Single addi instruction is sufficient
            replacement = f'\\1\n\taddi\tt6, t6, {offset}'
        else:
            # Need to use li + add sequence for large offsets
            replacement = f'\\1\n\tli\tt0, {offset}\n\tadd\tt6, t6, t0'
        
        translated = re.sub(pattern, replacement, translated)
        
        return translated
    
    def process_address_range(self, dump_lines: List[str], start_offset: int, end_offset: int, range_num: int) -> str:
        """Process a single address range and return translated function."""
        print(f"\nProcessing range {range_num}: {start_offset} to {end_offset}")
        
        # Extract lines in range using utility function
        extracted_lines = extract_lines_in_range(dump_lines, start_offset, end_offset)
        if not extracted_lines:
            print(f"Warning: No lines found in range {start_offset} to {end_offset}", file=sys.stderr)
            return ""
        
        print(f"Extracted {len(extracted_lines)} lines")
        
        # Join lines into assembly content
        asm_content = '\n'.join(extracted_lines)
        
        # Translate to function
        func_name = f"translated_function_{start_offset}"
        translated_asm = self.translate_assembly(asm_content, func_name)
        
        return translated_asm
    
    def translate_ranges(self, ranges: List[Tuple[int, int]]) -> str:
        """Translate multiple address ranges and combine results."""
        # Read dump file
        if not os.path.isfile(DUMP_FILE):
            print(f"Error: File not found - {DUMP_FILE}", file=sys.stderr)
            sys.exit(1)
        
        with open(DUMP_FILE, 'r', encoding='utf-8') as f:
            dump_lines = f.readlines()
        
        # Process each range
        all_functions = []
        for i, (start_offset, end_offset) in enumerate(ranges, 1):
            translated_func = self.process_address_range(dump_lines, start_offset, end_offset, i)
            if translated_func:
                all_functions.append(translated_func)
        
        if not all_functions:
            print("Error: No functions were translated")
            sys.exit(1)
        
        # Combine all translated functions
        return '\n\n'.join(all_functions)
    
    def compile_to_shared_library(self, asm_content: str) -> str:
        """Compile translated assembly to shared library using utility function."""
        return compile_lib(asm_content, f"translate_part{self.thread_index}.so")


def parse_arguments() -> Tuple[int, List[Tuple[int, int]]]:
    """Parse command line arguments."""
    if len(sys.argv) < 2:
        print("Usage: python translator.py <thread_index> [start_offset1 end_offset1 start_offset2 end_offset2 ...]")
        print("Offsets can be decimal or hexadecimal (e.g., 4096 or 0x1000)")
        sys.exit(1)
    
    thread_index = int(sys.argv[1])
    
    # Parse address range pairs
    ranges = []
    i = 2
    while i < len(sys.argv):
        if i + 1 >= len(sys.argv):
            print("Error: Incomplete address range pair")
            sys.exit(1)
        
        try:
            # Support both decimal and hexadecimal formats
            start_offset = int(sys.argv[i], 0)  # auto-detect base (0=auto)
            end_offset = int(sys.argv[i + 1], 0)  # auto-detect base (0=auto)
        except ValueError:
            print(f"Error: Invalid address values: {sys.argv[i]}, {sys.argv[i + 1]}")
            print("Use decimal (4096) or hexadecimal (0x1000) format")
            sys.exit(1)
        
        ranges.append((start_offset, end_offset))
        i += 2
    
    if not ranges:
        print("Error: No address ranges provided")
        sys.exit(1)
    
    return thread_index, ranges


def main():
    """Main translation function."""
    # Parse arguments
    thread_index, ranges = parse_arguments()
    
    print(f"Thread index: {thread_index}")
    print(f"Address ranges: {ranges}")
    
    # Create translator and process ranges
    translator = AssemblyTranslator(thread_index)
    combined_asm = translator.translate_ranges(ranges)
    
    # Compile to shared library
    output_file = translator.compile_to_shared_library(combined_asm)
    
    print(f"\nTranslation completed successfully: {output_file}")


if __name__ == "__main__":
    main()