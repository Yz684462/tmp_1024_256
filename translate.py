import sys, os, argparse, subprocess, re

current_script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(current_script_path)
sys.path.append(script_dir+"/rvv_sbt_tool")

from rvv_sbt_tool.rvv_sbt import translate_function
from rvv_sbt_tool.core.frontend.asm_parser import AsmParser


def parse_address(addr_str):
    """将十六进制字符串（如 'afa'）转换为整数"""
    return int(addr_str, 16)

def extract_lines_in_range(dump_lines, pc_b, pc_c):
    """从反汇编行中提取地址在 [pc_b, pc_c) 范围内的行"""
    extracted = []
    print("pc_b is ", pc_b)
    print("pc_c is ", pc_c)
    for line in dump_lines:
        line = line.rstrip()
        if not line.strip():
            continue
        # 提取地址部分（冒号前）
        if ':' not in line:
            continue
        addr_part = line.split(':')[0].strip()
        try:
            addr = parse_address(addr_part)
        except ValueError:
            continue  # 跳过无法解析的行

        if pc_b <= addr < pc_c:
            extracted.append(line)
    return extracted

def translate(asm_content: str, func_name="translated_function") -> str:
    parser = AsmParser()

    insn_list = parser.parse_file("extracted.s")

    output = translate_function(
        insn_list,
        func_name=func_name,
        vlenb=128,
        source_file="extracted.s",
        text_only=False,
    )
    output = modify(output, func_name == "translated_function_2")
    return output

def modify(translated:str, no_global=False) -> str:
    if '.globl simulated_cpu_state' in translated:
        return translated

    # 查找 .comm simulated_cpu_state, ... 的行
    pattern = r'^(\s*\.comm\s+simulated_cpu_state\b.*)$'
    match = re.search(pattern, translated, flags=re.MULTILINE)

    if match:
        # 构造带 .globl 的新内容
        comm_line = match.group(1)
        if no_global:
            # new_lines = comm_line.replace("4160" , "4192")
            translated_lines = translated.splitlines()
            new_lines = '\n\t.extern simulated_cpu_state\n'
            translated = "\n".join(translated_lines[:4]) + new_lines + "\n".join(translated_lines[4:-1])
        else:
            new_lines = '\t.globl simulated_cpu_state\n' + comm_line.replace("4160" , "4192")
            translated = translated[:match.start()] + new_lines + translated[match.end():]

    return translated


def main():
    parser = argparse.ArgumentParser(description="Extract a range of assembly instructions and compile translated version.")
    parser.add_argument("pc_b", help="Start PC address (hex, e.g., afa)")
    parser .add_argument("pc_c", help="End PC address (hex, e.g., afe)")
    parser.add_argument("dump_file", help="Path to the disassembly dump file")
    parser.add_argument("func_name", help="translated function name", default="translated_function")
    parser.add_argument("output_file", help="translated file .so", default="translate.so")
    
    args = parser.parse_args()

    # 解析输入地址
    try:
        pc_b = parse_address(args.pc_b)
        pc_c = parse_address(args.pc_c)
    except ValueError as e:
        print(f"Error: Invalid hex address - {e}", file=sys.stderr)
        sys.exit(1)

    if pc_b > pc_c:
        print("Error: pc_b must be <= pc_c", file=sys.stderr)
        sys.exit(1)

    # 读取 dump 文件
    if not os.path.isfile(args.dump_file):
        print(f"Error: File not found - {args.dump_file}", file=sys.stderr)
        sys.exit(1)

    with open(args.dump_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # 提取范围内的行
    extracted_lines = extract_lines_in_range(lines, pc_b, pc_c)
    if not extracted_lines:
        print("Warning: No lines found in the specified address range.", file=sys.stderr)
        sys.exit(1)

    # 写入临时 .s 文件（原始片段）
    temp_asm = "extracted.s"
    with open(temp_asm, 'w', encoding='utf-8') as f:
        for line in extracted_lines:
            f.write(line + '\n')

    print(f"Extracted {len(extracted_lines)} lines to {temp_asm}")

    # 读取并翻译
    with open(temp_asm, 'r', encoding='utf-8') as f:
        original_asm = f.read()

    translated_asm = translate(original_asm, args.func_name)

    # 写入翻译后的文件
    translated_file = "translate.s"
    with open(translated_file, 'w', encoding='utf-8') as f:
        f.write(translated_asm)

    print(f"Translated assembly written to {translated_file}")

    # 调用 gcc 编译为共享库
    compile_cmd = [
        "gcc",
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
        print("Error: 'gcc' not found. Please install GCC.", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()