#!/usr/bin/env python3
"""
Build-parity harness: builds one game project with the AGS Editor and with the C++ tools
(agsbuild), then compares the results file by file.

    python Script/compare_builds.py <project dir> --editor <path to AGSEditor.exe> --build-dir <dir> [options]

Both builds start from fresh copies of the project (Compiled/ and _Debug/ are left out). The game
packages (*.ags) are unpacked with agspak, so the table shows each packed file: game28.dta, rooms,
sprite file and index, audio, fonts, custom data; a "(package)" row compares each .ags package
itself (part names, asset order, ender). Exit code 0 means every file is identical.

The one layout difference allowed: the editor embeds the compiled scripts in game28.dta, while agsbuild
packs them as separate files (GlobalScript.o, __DialogScripts.o and the modules in ScriptModules.lst),
which the engine loads since 3.6.1. The comparison takes the scripts out of the editor's game28.dta
(see unembed_scripts), so each separate file must match the script the editor embedded, and the rest
of game28.dta must match byte for byte.

Options:
  --build-dir DIR   CMake build with tools (-DAGS_BUILD_TOOLS=1); agsbuild, agscc and the other tools
                    are collected from it into one tools directory next to the script API header
  --engine PATH     also run the auto-test on the agsbuild output with this engine (Software driver)
  --work-dir DIR    where copies and outputs go (default: ags-compare-builds in the temp directory)
  --timeout SEC     per build (default: 900); a build that takes longer is killed and reported

The editor build needs Windows; it also needs acwin.exe next to AGSEditor.exe if the project
builds a Windows target.
"""
import argparse
import glob
import hashlib
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
API_HEADER = os.path.join(REPO, 'Editor', 'AGS.Editor', 'Resources', 'agsdefns.sh')
EXE = '.exe' if os.name == 'nt' else ''
TOOLS = ['agsbuild', 'agf2dta', 'agf2dlgasc', 'agfexport', 'agspak', 'crmbuild', 'crmpak', 'spriteimport',
         'spritepak', 'trac']


def run(cmd, log_path, timeout, cwd=None):
    """Runs a command with output to a log file; returns (exit code or None on timeout, seconds)."""
    import time
    start = time.time()
    with open(log_path, 'w', encoding='utf-8', errors='replace') as log:
        proc = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT, cwd=cwd)
        try:
            code = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
            code = None
    return code, time.time() - start


def collect_tools(build_dir, sys_dir):
    os.makedirs(sys_dir, exist_ok=True)
    found = {}
    for name in TOOLS + ['agscc']:
        candidates = glob.glob(os.path.join(build_dir, '**', name + EXE), recursive=True)
        candidates = [c for c in candidates if '_deps' not in c]
        if candidates:
            shutil.copy2(candidates[0], sys_dir)
            found[name] = candidates[0]
    shutil.copy2(API_HEADER, sys_dir)
    missing = [t for t in ['agsbuild', 'agscc', 'agspak'] if t not in found]
    if missing:
        sys.exit(f'Tools not found in {build_dir}: {", ".join(missing)} (build with -DAGS_BUILD_TOOLS=1)')
    return found


def copy_project(src, dst):
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst, ignore=shutil.ignore_patterns('Compiled', '_Debug', '*.lock', '.git'))
    # The editor packs a compiled room that is newer than its sources as it is; both builds must
    # compile AGS 4 rooms (Rooms/<n>/) from their sources, so the copy has none
    rooms_dir = os.path.join(dst, 'Rooms')
    for n in os.listdir(rooms_dir) if os.path.isdir(rooms_dir) else []:
        crm = os.path.join(dst, f'room{n}.crm')
        if os.path.isfile(os.path.join(rooms_dir, n, 'data.xml')) and os.path.isfile(crm):
            os.remove(crm)


def hash_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


def flatten_output(out_dir, unpack_dir, agspak):
    """Returns {name: (path, size)} for the deployed files, with *.ags packages unpacked in place."""
    files = {}
    if os.path.isdir(unpack_dir):
        shutil.rmtree(unpack_dir)  # files of an earlier run would show up as this run's output
    if not os.path.isdir(out_dir):
        return files
    for name in sorted(os.listdir(out_dir)):
        path = os.path.join(out_dir, name)
        if not os.path.isfile(path):
            continue
        if name.lower().endswith('.ags'):
            dest = os.path.join(unpack_dir, name)
            os.makedirs(dest, exist_ok=True)
            subprocess.run([agspak, '-e', path, dest], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            for root, _, packed in os.walk(dest):
                for p in packed:
                    full = os.path.join(root, p)
                    rel = os.path.relpath(full, dest).replace(os.sep, '/')
                    files[f'{name}/{rel}'] = (full, os.path.getsize(full))
        else:
            files[name] = (path, os.path.getsize(path))
    return files


def ext_block_header(name):
    """An extension block's id, as Tools/data/data_file_writer.cpp WriteExtension writes it."""
    return b'\x00' + name.encode('ascii').ljust(16, b'\x00')


def separate_scripts(files, package):
    """agsbuild's compiled scripts in the order the editor embeds them (global, dialog, then
    ScriptModules.lst), as [(packed name, bytes)], or None if the package has no global script."""
    names = ['GlobalScript.o', '__DialogScripts.o']
    lst = files.get(package + 'ScriptModules.lst')
    if lst:
        names += [line.strip() for line in open(lst[0], encoding='utf-8').read().splitlines() if line.strip()]
    scripts = []
    for n in names:
        if package + n not in files:
            return None
        scripts.append((n, open(files[package + n][0], 'rb').read()))
    return scripts


def unembed_scripts(ed_dta, scripts):
    """Rewrites the editor's game28.dta as it is with the scripts left out, as agsbuild writes it.
    Returns (data, None), or (None, reason) if the editor's file doesn't hold exactly these scripts.

    The editor writes the scripts after the text parser dictionary (Common/game/main_game_file.cpp,
    ReadGameData), so three other places depend on them: the HasCCScript flag and the offset of the
    extension list, both in the GameSetupStructBase block before the scripts, and the script names at
    the start of the v362_interevent2 extension."""
    modules = scripts[2:]
    section = scripts[0][1] + scripts[1][1] + struct.pack('<i', len(modules)) + b''.join(m[1] for m in modules)
    pos = ed_dta.find(section)
    if pos < 0 or ed_dta.find(section, pos + 1) >= 0:
        return None, 'scripts differ'
    ext_start = ed_dta.find(ext_block_header('v360_fonts'), pos + len(section))
    field = ed_dta.find(struct.pack('<i', ext_start), 0, pos)
    if ext_start < 0 or field < 0 or ed_dta.find(struct.pack('<i', ext_start), field + 1, pos) >= 0:
        return None, 'extension offset not found'
    # GameSetupStructBase: the extension offset, then the dict, globalscript, chars and HasCCScript flags
    flag = field + 16
    if struct.unpack_from('<i', ed_dta, flag)[0] != 1:
        return None, 'HasCCScript flag not found'
    data = bytearray(ed_dta[:pos] + ed_dta[pos + len(section):])
    struct.pack_into('<i', data, field, ext_start - len(section))
    struct.pack_into('<i', data, flag, 0)

    # v362_interevent2: the global and dialog script names, the module count and names
    block = data.find(ext_block_header('v362_interevent2'), ext_start - len(section))
    if block < 0:
        return None, 'v362_interevent2 not found'
    length_at = block + 17
    names_at = length_at + 8

    def read_string(at):
        size = struct.unpack_from('<i', data, at)[0]
        return data[at + 4:at + 4 + size].decode('utf-8', 'replace'), at + 4 + size

    global_name, at = read_string(names_at)
    dialog_name, at = read_string(at)
    names = [global_name, dialog_name]
    count = struct.unpack_from('<i', data, at)[0]
    at += 4
    for _ in range(count):
        name, at = read_string(at)
        names.append(name)
    expected = [os.path.splitext(n)[0] + '.asc' for n, _ in scripts]
    if names != expected:
        return None, f'script names {names} instead of {expected}'
    data[names_at:at] = struct.pack('<iii', 0, 0, 0)
    length = struct.unpack_from('<q', data, length_at)[0]
    struct.pack_into('<q', data, length_at, length - (at - names_at - 12))
    return bytes(data), None


def package_layout(path):
    """What an asset library holds besides its assets (Common/data/multifilelib.cpp, format 30): the
    file names of its parts, the asset names in order, and whether it ends with an ender."""
    with open(path, 'rb') as f:
        head = f.read(1 << 20)
        f.seek(-12, os.SEEK_END)
        has_ender = f.read(12) == b'CLIB\x01\x02\x03\x04SIGE'
    if head[:5] != b'CLIB\x1a' or head[5] < 30:
        return None
    at = 7 + 4  # signature, version, part index; reserved options

    def cstr(at):
        end = head.index(b'\0', at)
        return head[at:end].decode('utf-8', 'replace'), end + 1

    count = struct.unpack_from('<i', head, at)[0]
    at += 4
    parts = []
    for _ in range(count):
        name, at = cstr(at)
        parts.append(name)
    count = struct.unpack_from('<i', head, at)[0]
    at += 4
    assets = []
    for _ in range(count):
        name, at = cstr(at)
        assets.append(name)
        at += 1 + 8 + 8  # part, offset, size
    return parts, assets, has_ender


def compare_packages(ed_path, tool_path):
    """Compares the layout of two packages; returns None if it matches, or what differs."""
    e, t = package_layout(ed_path), package_layout(tool_path)
    if e is None or t is None:
        return 'not a package of format 30'
    if e[0] != t[0]:
        return f'part names {e[0]} and {t[0]}'
    if e[2] != t[2]:
        return 'ender only in the ' + ('editor\'s' if e[2] else 'agsbuild\'s')
    shared = set(e[1]) & set(t[1])
    if [a for a in e[1] if a in shared] != [a for a in t[1] if a in shared]:
        return 'assets in another order'
    return None


def run_autotest(engine, files_dir, work_dir, timeout):
    run_dir = os.path.join(work_dir, 'run')
    if os.path.isdir(run_dir):
        shutil.rmtree(run_dir)
    shutil.copytree(files_dir, run_dir)
    engine_dir = os.path.dirname(os.path.abspath(engine))
    for dll in glob.glob(os.path.join(engine_dir, '*.dll')):
        shutil.copy2(dll, run_dir)
    games = glob.glob(os.path.join(run_dir, '*.ags'))
    if not games:
        return 'no game package to run'
    code, _ = run([os.path.abspath(engine), '--gfxdriver', 'Software', '--no-message-box', '--user-data-dir', '.',
                   os.path.basename(games[0])], os.path.join(work_dir, 'engine.log'), timeout, cwd=run_dir)
    tap = os.path.join(run_dir, 'agstest.tap')
    if not os.path.isfile(tap):
        return f'engine exit {code}, no agstest.tap'
    text = open(tap, encoding='utf-8', errors='replace').read()
    ok = len(re.findall(r'^ok\b', text, re.M))
    not_ok = len(re.findall(r'^not ok\b', text, re.M))
    plan = re.search(r'^1\.\.(\d+)', text, re.M)
    return f'{ok} ok, {not_ok} not ok, of {plan.group(1) if plan else "?"} planned (engine exit {code})'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('project', help='game project directory (containing Game.agf)')
    ap.add_argument('--editor', required=True, help='path to AGSEditor.exe')
    ap.add_argument('--build-dir', required=True, help='CMake build directory with the tools')
    ap.add_argument('--engine', help='engine executable for the auto-test run')
    ap.add_argument('--work-dir', default=os.path.join(tempfile.gettempdir(), 'ags-compare-builds'))
    ap.add_argument('--timeout', type=int, default=900)
    args = ap.parse_args()

    project = os.path.abspath(args.project)
    if not os.path.isfile(os.path.join(project, 'Game.agf')):
        sys.exit(f'No Game.agf in {project}')
    work = os.path.abspath(args.work_dir)
    os.makedirs(work, exist_ok=True)
    sys_dir = os.path.join(work, 'tools')
    collect_tools(os.path.abspath(args.build_dir), sys_dir)
    agspak = os.path.join(sys_dir, 'agspak' + EXE)

    # Editor build
    ed_proj = os.path.join(work, 'editor-project')
    copy_project(project, ed_proj)
    ed_code, ed_time = run([os.path.abspath(args.editor), '/compile', os.path.join(ed_proj, 'Game.agf')],
                           os.path.join(work, 'editor.log'), args.timeout)
    ed_out = os.path.join(ed_proj, 'Compiled', 'Data')

    # agsbuild
    tool_proj = os.path.join(work, 'tools-project')
    copy_project(project, tool_proj)
    tool_out = os.path.join(work, 'tools-output')
    if os.path.isdir(tool_out):
        shutil.rmtree(tool_out)
    os.makedirs(tool_out)
    tool_code, tool_time = run([os.path.join(sys_dir, 'agsbuild' + EXE), os.path.join(tool_proj, 'Game.agf'),
                                tool_out, '--sys-dir', sys_dir], os.path.join(work, 'agsbuild.log'), args.timeout)

    ed_files = flatten_output(ed_out, os.path.join(work, 'unpacked-editor'), agspak)
    tool_files = flatten_output(tool_out, os.path.join(work, 'unpacked-tools'), agspak)

    def status(code):
        if code is None:
            return 'timed out (killed)'
        return f'exit {code - (1 << 32) if code >= (1 << 31) else code}'  # Windows reports -1 as 4294967295

    print(f'editor:   {status(ed_code)} in {ed_time:.0f}s, {len(ed_files)} files  (log: {work}/editor.log)')
    print(f'agsbuild: {status(tool_code)} in {tool_time:.0f}s, {len(tool_files)} files  (log: {work}/agsbuild.log)')
    if tool_code not in (0, None):  # print the distinct errors
        log = open(os.path.join(work, 'agsbuild.log'), encoding='utf-8', errors='replace').read()
        errors = sorted(set(re.findall(r'^(?:Error|Failure): .*$', log, re.M)))
        for e in errors[:12]:
            print(f'  {e[:160]}')
        if len(errors) > 12:
            print(f'  ... {len(errors) - 12} more distinct errors')

    # The scripts agsbuild packs separately are compared with those the editor embeds in game28.dta
    unembedded = {}  # game28.dta -> (the editor's file without the scripts, or None; the reason)
    embedded = set()  # separate script files found in the editor's game28.dta
    for n in tool_files:
        if not n.endswith('/game28.dta') or n not in ed_files:
            continue
        package = n[:-len('game28.dta')]
        scripts = separate_scripts(tool_files, package)
        if scripts is None:
            continue
        unembedded[n] = unembed_scripts(open(ed_files[n][0], 'rb').read(), scripts)
        if unembedded[n][0] is not None:
            embedded.update(package + s for s, _ in scripts)
            embedded.add(package + 'ScriptModules.lst')

    names = sorted(set(ed_files) | set(tool_files))
    counts = {'identical': 0, 'different': 0, 'only editor': 0, 'only agsbuild': 0}
    print(f'\n| file | result | editor bytes | agsbuild bytes |\n|---|---|---:|---:|')
    for n in names:
        e, t = ed_files.get(n), tool_files.get(n)
        note = ''
        if e and t and n in unembedded:
            data, reason = unembedded[n]
            result = 'identical' if data is not None and data == open(t[0], 'rb').read() else 'different'
            note = f' ({reason})' if reason else ' (scripts separate)'
        elif e and t:
            result = 'identical' if e[1] == t[1] and hash_file(e[0]) == hash_file(t[0]) else 'different'
        elif t and n in embedded:
            result, note = 'identical', ' (embedded in the editor\'s game28.dta)'
        else:
            result = 'only editor' if e else 'only agsbuild'
        counts[result] += 1
        print(f'| {n} | {result}{note} | {e[1] if e else ""} | {t[1] if t else ""} |')
    # The packages themselves, besides the files in them: part names, asset order, ender
    for n in sorted(os.listdir(ed_out) if os.path.isdir(ed_out) else []):
        ed_pkg, tool_pkg = os.path.join(ed_out, n), os.path.join(tool_out, n)
        if not n.lower().endswith('.ags') or not os.path.isfile(tool_pkg):
            continue
        reason = compare_packages(ed_pkg, tool_pkg)
        result = 'different' if reason else 'identical'
        names.append(n + ' (package)')
        counts[result] += 1
        print(f'| {n} (package) | {result}{f" ({reason})" if reason else ""} | | |')
    print('\n' + ', '.join(f'{v} {k}' for k, v in counts.items()))

    if args.engine:
        print('auto-test on agsbuild output:', run_autotest(args.engine, tool_out, work, args.timeout))

    ok = names and counts['identical'] == len(names)
    print('RESULT:', 'PARITY' if ok else 'NO PARITY')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
