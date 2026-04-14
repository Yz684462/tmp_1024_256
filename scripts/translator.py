#!/usr/bin/env python3
"""
RISC-V Vector Assembly Translator

This script translates RISC-V vector assembly code segments into C-compatible functions,
handling multiple address ranges and thread-specific offsets for simulated CPU state.
"""

import sys
import os
import tempfile
from typing import List, Tuple

# Import assembly utilities
from assembly_utils import extract_lines_in_range
from assembly_utils import compile_to_shared_library as compile_lib

# Constants
pool_name = 'global_simulated_vector_contexts_pool'

# Setup path for rvv_sbt_tool
current_script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(current_script_path)
sys.path.append(os.path.join(script_dir, "..", "rvv_sbt_tool"))

from rvv_sbt_tool.rvv_sbt import translate_function
from rvv_sbt_tool.core.frontend.asm_parser import AsmParser


class AssemblyTranslator:
    """Handles translation of RISC-V assembly to C-compatible functions."""
    
    def __init__(self):
        self.translated_functions = []
    
    def translate_assembly(self, asm_content: str, func_name: str) -> str:
        """Translate assembly content to C-compatible function."""
        parser = AsmParser()
        
        # Write assembly content to temporary file for parsing
        with tempfile.NamedTemporaryFile(mode='w', suffix='.s', delete=False) as f:
            f.write(asm_content)
            temp_file = f.name
        
        insn_list = parser.parse_file(temp_file)
        
        output = translate_function(
            insn_list,
            func_name=func_name,
            vlenb=128,
            source_file=temp_file,
            text_only=False,
        )
        
        return self._modify_translated_code(output, func_name)
    
    def _modify_translated_code(self, translated: str, func_name: str) -> str:
        """Modify translated code to handle simulated_cpu_state."""
        lines = translated.splitlines()
        modified_lines = []
        
        # Remove global simulated_cpu_state declarations
        for line in lines:
            if '.globl' in line and 'simulated_cpu_state' in line:
                continue
            if '.comm' in line and 'simulated_cpu_state' in line:
                continue
            modified_lines.append(line)
            if '.type' in line:
                modified_lines.append(f'\t.extern {pool_name}')
        
        translated = '\n'.join(modified_lines)
        
        # Replace simulated_cpu_state with pool_name
        translated = translated.replace('simulated_cpu_state', pool_name)
        
        return translated
    
    def process_address_range(self, dump_lines: List[str], start_offset: int, end_offset: int, func_name: str, should_translate: bool = True) -> str:
        """Process a single address range and return translated function."""
        print(f"\nProcessing range {start_offset} to {end_offset} with function name: {func_name}")
        print(f"Should translate: {should_translate}")
        
        # Extract lines in range using utility function
        extracted_lines = extract_lines_in_range(dump_lines, start_offset, end_offset)
        if not extracted_lines:
            print(f"Warning: No lines found in range {start_offset} to {end_offset}", file=sys.stderr)
            return ""
        
        print(f"Extracted {len(extracted_lines)} lines")
        
        # Join lines into assembly content
        asm_content = '\n'.join(extracted_lines)
        
        # Translate to function with provided name if should_translate is True
        if should_translate:
            translated_asm = self.translate_assembly(asm_content, func_name)
        else:
            # If not translating, just wrap the assembly content in a basic function
            translated_asm = self._wrap_assembly_as_function(asm_content, func_name)
        
        return translated_asm
    
    def _wrap_assembly_as_function(self, asm_content: str, func_name: str) -> str:
        """Wrap assembly content in a basic function."""
        content = f"""\
            .globl {func_name}
            .type {func_name}, @function
            .extern {pool_name}
        {func_name}:
            {asm_content}
            ret
            .size {func_name}, .-{func_name}
        """
        return content
    
    def translate_ranges(self, dump_file: str, ranges: List[Tuple[int, int]], func_names: List[str], should_translate: bool = True) -> str:
        """Translate multiple address ranges and combine results."""
        # Read dump file
        if not os.path.isfile(dump_file):
            print(f"Error: File not found - {dump_file}", file=sys.stderr)
            sys.exit(1)
        
        with open(dump_file, 'r', encoding='utf-8') as f:
            dump_lines = f.readlines()
        
        # Process each range with corresponding function name
        all_functions = []
        for (start_offset, end_offset), func_name in zip(ranges, func_names):
            translated_func = self.process_address_range(dump_lines, start_offset, end_offset, func_name, should_translate)
            if translated_func:
                all_functions.append(translated_func)
        
        if not all_functions:
            print("Error: No functions were translated")
            sys.exit(1)
        
        # Combine all translated functions
        return '\n\n'.join(all_functions)
    
    def compile_to_shared_library(self, asm_content: str, output_file: str) -> str:
        """Compile translated assembly to shared library using utility function."""
        return compile_lib(asm_content, output_file)


def parse_arguments() -> Tuple[str, List[Tuple[int, int]], List[str], str, bool]:
    """Parse command line arguments."""
    if len(sys.argv) < 5 or len(sys.argv) > 6:
        print("Usage: python translator.py <dump_file> <ranges> <func_names> <output_file> [should_translate]")
        print("Example: python translator.py dump.s '0x1000,0x2000' 'func1,func2' output.so")
        print("Example: python translator.py dump.s '0x1000,0x2000' 'func1,func2' output.so false")
        print("Ranges format: start1,end1,start2,end2 (comma-separated)")
        print("Func_names format: name1,name2 (comma-separated, must match ranges)")
        print("should_translate: optional, true/false (default: true)")
        sys.exit(1)
    
    try:
        dump_file = sys.argv[1]
        ranges_str = sys.argv[2]
        func_names_str = sys.argv[3]
        output_file = sys.argv[4]
        
        # Parse optional should_translate parameter
        should_translate = True  # default value
        if len(sys.argv) == 6:
            should_translate_str = sys.argv[5].lower()
            if should_translate_str in ['true', '1', 'yes', 'on']:
                should_translate = True
            elif should_translate_str in ['false', '0', 'no', 'off']:
                should_translate = False
            else:
                print(f"Error: Invalid should_translate value '{sys.argv[5]}'. Use true/false, 1/0, yes/no, or on/off")
                sys.exit(1)
                
    except ValueError as e:
        print(f"Error: Invalid arguments. {e}")
        sys.exit(1)
    
    # Parse ranges (comma-separated pairs)
    try:
        range_parts = ranges_str.split(',')
        if len(range_parts) % 2 != 0:
            print("Error: Ranges must contain pairs of start and end addresses")
            sys.exit(1)
        
        ranges = []
        for i in range(0, len(range_parts), 2):
            start_offset = int(range_parts[i], 0)  # auto-detect base
            end_offset = int(range_parts[i + 1], 0)  # auto-detect base
            ranges.append((start_offset, end_offset))
    except ValueError as e:
        print(f"Error: Invalid range values. {e}")
        print("Use decimal (4096) or hexadecimal (0x1000) format")
        sys.exit(1)
    
    # Parse function names (comma-separated)
    func_names = func_names_str.split(',')
    
    # Check if func_names count matches ranges count
    if len(func_names) != len(ranges):
        print(f"Error: Number of function names ({len(func_names)}) must match number of ranges ({len(ranges)})")
        sys.exit(1)
    
    if not ranges:
        print("Error: No address ranges provided")
        sys.exit(1)
    
    return dump_file, ranges, func_names, output_file, should_translate


def main():
    """Main translation function."""
    # Parse arguments
    dump_file, ranges, func_names, output_file, should_translate = parse_arguments()
    
    print(f"Dump file: {dump_file}")
    print(f"Address ranges: {ranges}")
    print(f"Function names: {func_names}")
    print(f"Output file: {output_file}")
    print(f"Should translate: {should_translate}")
    
    # Create translator and process ranges
    translator = AssemblyTranslator()
    combined_asm = translator.translate_ranges(dump_file, ranges, func_names, should_translate)
    
    # Compile to shared library
    compiled_output = translator.compile_to_shared_library(combined_asm, output_file)
    
    print(f"\nTranslation completed successfully: {compiled_output}")


if __name__ == "__main__":
    main()