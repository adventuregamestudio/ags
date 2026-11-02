#!/usr/bin/env python3
"""
Build-parity harness: builds one game project with the AGS Editor and with the C++ tools
(agsbuild), then compares the results file by file.

    python Script/compare_builds.py <project dir> --editor <path to AGSEditor.exe> --build-dir <dir> [options]

Both builds start from fresh copies of the project (Compiled/ and _Debug/ are left out). The game
packages (*.ags) are unpacked with agspak, so the table shows each packed file: game28.dta, rooms,
sprite file and index, audio, fonts, custom data. Exit code 0 means every file is identical.

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
import subprocess
import sys
import tempfile

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
API_HEADER = os.path.join(REPO, 'Editor', 'AGS.Editor', 'Resources', 'agsdefns.sh')
EXE = '.exe' if os.name == 'nt' else ''
TOOLS = ['agsbuild', 'agf2dta', 'agf2dlgasc', 'agfexport', 'agspak', 'crmbuild', 'crmpak', 'spriteimport', 'spritepak',
         'trac']


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
    shutil.copytree(src, dst, ignore=shutil.ignore_patterns('Compiled', '_Debug', '*.lock'))


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

    names = sorted(set(ed_files) | set(tool_files))
    counts = {'identical': 0, 'different': 0, 'only editor': 0, 'only agsbuild': 0}
    print(f'\n| file | result | editor bytes | agsbuild bytes |\n|---|---|---:|---:|')
    for n in names:
        e, t = ed_files.get(n), tool_files.get(n)
        if e and t:
            result = 'identical' if e[1] == t[1] and hash_file(e[0]) == hash_file(t[0]) else 'different'
        else:
            result = 'only editor' if e else 'only agsbuild'
        counts[result] += 1
        print(f'| {n} | {result} | {e[1] if e else ""} | {t[1] if t else ""} |')
    print('\n' + ', '.join(f'{v} {k}' for k, v in counts.items()))

    if args.engine:
        print('auto-test on agsbuild output:', run_autotest(args.engine, tool_out, work, args.timeout))

    ok = names and counts['identical'] == len(names)
    print('RESULT:', 'PARITY' if ok else 'NO PARITY')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
