
import win32api
from win32api import GetFileVersionInfo, LOWORD, HIWORD
import contextlib
import errno
import os
import os.path
import subprocess
import shutil
import json
from collections import namedtuple

ISCC="C:/Program Files (x86)/Inno Setup 5/iscc.exe"

SCRIPT_LOC=os.path.dirname(os.path.realpath(__file__))
WORKSPACE_DIR=os.path.join(SCRIPT_LOC, "../..")

AgsVersion = namedtuple('AgsVersion', ['version', 'version_friendly', 'app_id'])

def main():
    docs_dir = workspace_rel("Windows/Installer/Source/Docs")
    if not os.path.exists(docs_dir):
        os.makedirs(docs_dir)
    shutil.copy2(workspace_rel("Changes.txt"), docs_dir)

    editor_ver = get_editor_version_number(workspace_rel("Windows/Installer/Source/AGSEditor.exe"))
    project_ver = load_project_version(workspace_rel("version.json"))

    editor_ver_check = ".".join(str(x) for x in editor_ver)
    project_ver_check = ".".join(project_ver.version)

    if project_ver_check != editor_ver_check:
        raise Exception("Versions differ - editor:{0} version.json:{1}".format(editor_ver_check, project_ver_check))

    project_ver_str = ".".join(project_ver.version_friendly)
    project_app_id = ".".join(project_ver.app_id)

    compile_installer("ags.iss", {"AgsVersion": project_ver_str, "AgsAppId": project_app_id})

def workspace_rel(path):
    return os.path.join(workspace_dir(), path)

def workspace_dir():
    return WORKSPACE_DIR

def get_editor_version_number (filename):
    try:
        info = GetFileVersionInfo (filename, "\\")
        ms = info['FileVersionMS']
        ls = info['FileVersionLS']
        return HIWORD (ms), LOWORD (ms), HIWORD (ls), LOWORD (ls)
    except Exception, e:
        raise Exception("{0}: could not get file version info: {1}".format(filename, e.strerror))

def load_project_version(path):
    with open(path, "r") as f:
        j = json.load(f)

    version = j['version'].split('.')
    version_friendly = j['versionFriendly'].split('.')
    app_id = j['appID']
    return AgsVersion(version, version_friendly, app_id)

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
