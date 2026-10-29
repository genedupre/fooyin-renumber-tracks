"""Generate MSVC import libraries (.lib) from installed fooyin DLLs."""
import subprocess, sys, os, re, tempfile

def exports(dll):
    result = subprocess.run(['dumpbin', '/EXPORTS', dll], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f'dumpbin failed on {dll}')
    lines, started = [], False
    for line in result.stdout.splitlines():
        if 'ordinal' in line and 'hint' in line and 'RVA' in line:
            started = True; continue
        if started:
            if not line.strip(): break
            parts = line.split()
            if len(parts) >= 4:
                name = parts[3]
                is_data = '(forwarded' not in line and len(parts) >= 5 and parts[4] == '[DATA]'
                lines.append((name, is_data))
            elif len(parts) == 3:
                lines.append((parts[2], False))
    return lines

def make_lib(dll_path, output_dir):
    dll_name = os.path.basename(dll_path)
    lib_name = os.path.splitext(dll_name)[0] + '.lib'
    syms = exports(dll_path)
    with tempfile.NamedTemporaryFile('w', suffix='.def', delete=False) as f:
        f.write(f'LIBRARY {dll_name}\nEXPORTS\n')
        for name, is_data in syms:
            f.write(f'  {name}' + (' DATA' if is_data else '') + '\n')
        def_path = f.name
    try:
        out = os.path.join(output_dir, lib_name)
        subprocess.run(['lib', f'/DEF:{def_path}', f'/OUT:{out}', '/MACHINE:X64'], check=True)
    finally:
        os.unlink(def_path)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} <fooyin-dir> <output-dir>')
        sys.exit(1)
    fooyin_dir, output_dir = sys.argv[1], sys.argv[2]
    for module in ['core', 'gui', 'utils']:
        dll = os.path.join(fooyin_dir, f'fooyin_{module}.dll')
        if not os.path.isfile(dll):
            print(f'Not found: {dll}'); sys.exit(1)
        make_lib(dll, output_dir)
        print(f'Created {module} import library')
