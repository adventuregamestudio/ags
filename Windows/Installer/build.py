
import win32api
from win32api import GetFileVersionInfo, LOWORD, HIWORD
import contextlib
import errno
import os
import os.path
import subprocess
import shutil

ISCC="C:/Program Files (x86)/Inno Setup 5/iscc.exe"

SCRIPT_LOC=os.path.dirname(os.path.realpath(__file__))
WORKSPACE_DIR=os.path.join(SCRIPT_LOC, "../..")

def main():
    docs_dir = workspace_rel("Windows/Installer/Source/Docs")
    if not os.path.exists(docs_dir):
        os.makedirs(docs_dir)
    shutil.copy2(workspace_rel("Changes.txt"), docs_dir)

    ver = get_version_number(workspace_rel("Windows/Installer/Source/AGSEditor.exe"))
    ver_str = ".".join(str(x) for x in ver[0:3])

    compile_installer("ags.iss", {"AgsVersion": ver_str})

def workspace_rel(path):
    return os.path.join(workspace_dir(), path)

def workspace_dir():
    return WORKSPACE_DIR

def get_version_number (filename):
    try:
        info = GetFileVersionInfo (filename, "\\")
        ms = info['FileVersionMS']
        ls = info['FileVersionLS']
        return HIWORD (ms), LOWORD (ms), HIWORD (ls), LOWORD (ls)
    except Exception, e:
        raise Exception("{0}: could not get file version info: {1}".format(filename, e.strerror))

def compile_installer(script, params):
    with dir_context(workspace_rel("Windows/Installer")):
        args = ["/D%s=%s"%(k, params[k]) for k in params]
        subprocess.check_call([ISCC, script] + args)

@contextlib.contextmanager
def dir_context(path):
    curdir = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(curdir)


if __name__ == "__main__":
    main()
