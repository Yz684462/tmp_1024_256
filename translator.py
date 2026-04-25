import sys, os, argparse, subprocess, re

current_script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(current_script_path)
sys.path.append(script_dir+"/rvv_sbt_tool")

from rvv_sbt_tool.rvv_sbt import translate_function
from rvv_sbt_tool.core.frontend.asm_parser import AsmParser

def extract_lines_in_range(dump_lines, pc_b, pc_c):
    """从反汇编行中提取地址在 [pc_b, pc_c) 范围内的行"""
    extracted = []
    for line in dump_lines:
        line = line.rstrip()
        if not line.strip():
            continue
        # 提取地址部分（冒号前）
        if ':' not in line:
            continue
        addr_part = line.split(':')[0].strip()
        try:
            addr = int(addr_part, 16)
        except ValueError:
            continue  # 跳过无法解析的行

        if pc_b <= addr < pc_c:
            extracted.append(line)
    return extracted

def translate(func_name:str) -> str:
    parser = AsmParser()

    insn_list = parser.parse_file("extracted.s")

    output = translate_function(
        insn_list,
        func_name=func_name,
        vlenb=128,
        source_file="extracted.s",
        text_only=False,
    )
    output = modify(output)
    return output

def modify(translated:str) -> str:
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
            modified_lines.append(f'\t.extern simulated_cpu_state')
    
    translated = '\n'.join(modified_lines)

    return translated

def parse_ranges():
    # 解析环境变量vector_snippet_ranges=("0x94c,0x950" "0x934,0x938" "0x942,0x946")
    ranges = os.getenv("vector_snippet_ranges", "")
    if not ranges:
        return []
    # 解析格式
    ranges = ranges.split()
    ranges = [range.split(',') for range in ranges]
    ranges = [(int(range[0], 16), int(range[1], 16)) for range in ranges]
    return ranges

def main():
    parser = argparse.ArgumentParser(description="Extract a range of assembly instructions and compile translated version.")
    parser.add_argument("dump_file", help="Path to the disassembly dump file")
    parser.add_argument("output_file", help="translated file .so", default="translate.so")
    
    args = parser.parse_args()

    ranges = parse_ranges()
    # 读取 dump 文件
    if not os.path.isfile(args.dump_file):
        print(f"Error: File not found - {args.dump_file}", file=sys.stderr)
        sys.exit(1)

    with open(args.dump_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    translated_contents = ""
    for start_addr, end_addr in ranges:
        # 提取范围内的行
        extracted_lines = extract_lines_in_range(lines, start_addr, end_addr)
        if not extracted_lines:
            print("Warning: No lines found in the specified address range.", file=sys.stderr)
            sys.exit(1)

        # 写入临时 .s 文件（原始片段）
        temp_asm = "extracted.s"
        with open(temp_asm, 'w', encoding='utf-8') as f:
            for line in extracted_lines:
                f.write(line + '\n')

        # 翻译
        translated_asm = translate(f'translated_function_{start_addr}')
        translated_contents += translated_asm + "\n"

    # 写入翻译后的文件
    translated_file = "translate.s"
    with open(translated_file, 'w', encoding='utf-8') as f:
        f.write(translated_contents)

    # 调用 g++ 编译为共享库
    compile_cmd = [
        "g++",
        "-march=rv64gcv_zba",
        translated_file,
        "-shared",
        "-fPIC",
        "-o", args.output_file
    ]

    try:
        result = subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
        print("Successfully compiled translate.so")
    except subprocess.CalledProcessError as e:
        print(f"Compilation failed:\n{e.stderr}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print("Error: 'g++' not found. Please install g++.", file=sys.stderr)
        sys.exit(1)
    
if __name__ == "__main__":
    main()