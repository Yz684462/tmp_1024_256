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

def translate(func_name:str, asm_file:str, func_addr:str, vtype:int) -> str:
    parser = AsmParser()

    insn_list = parser.parse_file(asm_file)

    output = translate_function(
        insn_list,
        func_name=func_name,
        vlenb=128,
        source_file=asm_file,
        text_only=False,
        init_vtype=str(vtype),
    )
    output = modify(output, func_addr)
    return output

def modify(translated:str, func_addr:str) -> str:
    lines = translated.splitlines()
    modified_lines = []
    
    # Remove global simulated_cpu_state declarations
    for line in lines:
        if '.globl' in line and 'simulated_cpu_state' in line:
            continue
        if '.comm' in line and 'simulated_cpu_state' in line:
            continue
        modified_lines.append(line)
        if '.type' in line and f'translated_function_{func_addr}' in line:
            modified_lines.append(f'\t.extern simulated_cpu_state')
    
    return '\n'.join(modified_lines)

def parse_ranges():
    # 解析环境变量vector_snippet_ranges="0x94c,0x950 0x934,0x938 0x942,0x946"
    ranges = os.getenv("vector_snippet_ranges", "")
    if not ranges:
        return []
    # 解析格式
    ranges = ranges.split()
    ranges = [range.split(',') for range in ranges]
    ranges = [(int(range[0], 16), int(range[1], 16)) for range in ranges]
    return ranges

def parse_vtypes():
    # 解析环境变量vector_vtypes="197 205 213"
    vtypes = os.getenv("vector_vtypes", "")
    if not vtypes:
        return []
    # 解析格式
    vtypes = vtypes.split()
    vtypes = [int(vtype) for vtype in vtypes]
    return vtypes

def main():
    parser = argparse.ArgumentParser(description="Extract a range of assembly instructions and compile translated version.")
    parser.add_argument("dump_file", help="Path to the disassembly dump file")
    parser.add_argument("output_file", help="translated file .so", default="translate.so")
    
    args = parser.parse_args()

    ranges = parse_ranges()
    vtypes = parse_vtypes()
    # 读取 dump 文件
    if not os.path.isfile(args.dump_file):
        print(f"Error: File not found - {args.dump_file}", file=sys.stderr)
        sys.exit(1)

    with open(args.dump_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # 创建临时目录存放汇编文件
    temp_dir = "temp_asm"
    os.makedirs(temp_dir, exist_ok=True)
    
    asm_files = []
    translated_contents = ""
    
    for idx, (start_addr, end_addr) in enumerate(ranges):
        # 提取范围内的行
        extracted_lines = extract_lines_in_range(lines, start_addr, end_addr)
        if not extracted_lines:
            print(f"Warning: No lines found in address range 0x{start_addr:x}-0x{end_addr:x}", file=sys.stderr)
            continue

        # 创建临时 .s 文件（原始片段）
        temp_asm = os.path.join(temp_dir, f"extracted_{start_addr:x}.s")
        with open(temp_asm, 'w', encoding='utf-8') as f:
            for line in extracted_lines:
                f.write(line + '\n')

        # 翻译函数
        func_name = f'translated_function_{start_addr}'
        translated_asm = translate(func_name, temp_asm, f"{start_addr}", vtypes[idx])
        
        # 保存翻译后的汇编到单独文件
        translated_file = os.path.join(temp_dir, f"translated_{start_addr:x}.s")
        with open(translated_file, 'w', encoding='utf-8') as f:
            f.write(translated_asm)
        
        asm_files.append(translated_file)
        translated_contents += translated_asm + "\n"

    if not asm_files:
        print("Error: No assembly files generated", file=sys.stderr)
        sys.exit(1)

    # 可选：保存合并后的文件用于调试
    merged_file = "translate.s"
    with open(merged_file, 'w', encoding='utf-8') as f:
        f.write(translated_contents)

    # 编译所有单独的汇编文件为一个共享库
    compile_cmd = [
        "g++",
        "-march=rv64gcv_zba",
    ] + asm_files + [
        "-shared",
        "-fPIC",
        "-o", args.output_file
    ]

    print(f"Compiling {len(asm_files)} assembly files...")
    try:
        result = subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
        print(f"Successfully compiled {args.output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Compilation failed:\n{e.stderr}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print("Error: 'g++' not found. Please install g++.", file=sys.stderr)
        sys.exit(1)
    
    # 清理临时文件（可选）
    if not os.getenv("KEEP_TEMP", False):
        import shutil
        shutil.rmtree(temp_dir)
        # if os.path.exists(merged_file):
        #     os.remove(merged_file)
    
if __name__ == "__main__":
    main()