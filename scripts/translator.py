#!/usr/bin/env python3
"""
RISC-V Vector Assembly Translator

This script translates RISC-V vector assembly code segments into C-compatible functions,
handling multiple address ranges and thread-specific offsets for simulated CPU state.
"""

import sys
import os
import tempfile
import subprocess
from typing import List, Tuple

# Constants
pool_name = 'global_simulated_vector_contexts_pool'
cc = "gcc"


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
    
    def _modify_translated_code(self, translated: str) -> str:
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
    
    def split_dump_fragments(self, dump_file: str) -> List[str]:
        """Split dump file into individual function fragments."""
        with open(dump_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Split by function markers
        fragments = content.split('\n\n')
        self.fragments =  [fragment.strip() for fragment in fragments if fragment.strip()]

    def process_fragments(self, func_names):
        fragments_processed = []
        for i, fragment in enumerate(self.fragments):
            first_instr = fragment.split('\n')[0].strip()
            pc = first_instr.split()[0].replace(':', '')
            fragments_processed.append(self.translate_assembly(fragment, func_names[i] + pc))
        self.fragments = fragments_processed

    def compile_to_shared_library(self, output_file):
        """Compile translated assembly to shared library using utility function."""
        asm_content = "\n\n".join(self.fragments)
        with tempfile.NamedTemporaryFile(mode='w', suffix='.s', delete=False) as temp_file:
            translated_file = temp_file.name
            temp_file.write(asm_content)
            print(f"Combined translated assembly written to {translated_file}")
    
        # Compile to shared library
        compile_cmd = [
            cc,
            "-march=rv64gcv",
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


def parse_arguments() -> Tuple[str, str, list[str], str]:
    """Parse command line arguments."""
    if len(sys.argv) != 5:
        print("Usage: python translator.py <translation_id> <dump_file> <func_names> <output_file>")
        print("Example: python translator.py 1 dump.s 'func1,func2' output.so")
        print("Parameters:")
        print("  translation_id: Translation ID number")
        print("  dump_file: Path to dump fragments file")
        print("  func_names: Comma-separated function names")
        print("  output_file: Output shared library path")
        sys.exit(1)
    
    try:
        translation_id = sys.argv[1]
        dump_file = sys.argv[2]
        func_names = sys.argv[3].split(',')
        output_file = sys.argv[4]
        
        return translation_id, dump_file, func_names, output_file
    
    except (ValueError, IndexError) as e:
        print(f"Error parsing arguments: {e}")
        sys.exit(1)


def main():
    """Main translation function."""
    # Parse arguments
    translation_id, dump_file, func_names, output_file = parse_arguments()
    
    print(f"Translation ID: {translation_id}")
    print(f"Dump file: {dump_file}")
    func_names_str = ", ".join(func_names)
    print(f"Function names: {func_names_str}")
    print(f"Output file: {output_file}")
    
    # Read dump file
    if not os.path.isfile(dump_file):
        print(f"Error: File not found - {dump_file}", file=sys.stderr)
        sys.exit(1)

    translator = AssemblyTranslator()
    translator.split_dump_fragments(dump_file)
    translator.process_fragments(func_names)
    translator.compile_to_shared_library(output_file)
    print(f"\nTranslation completed successfully: {compiled_output}")


if __name__ == "__main__":
    main()
