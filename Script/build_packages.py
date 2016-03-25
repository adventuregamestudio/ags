#!/usr/bin/env python

from __future__ import print_function

from collections import namedtuple
import codecs
import hashlib
import json
import os
import os.path
import re
import shutil
import time
import zipfile

WINDOWS_IS_STUPID_RETRIES=8
WINDOWS_IS_STUPID_DELAY=2


SCRIPT_LOC=os.path.dirname(os.path.realpath(__file__))
WORKSPACE_DIR=os.path.join(SCRIPT_LOC, "..")

def workspace_dir():
    assert os.path.exists(os.path.join(WORKSPACE_DIR, "version.json"))
    assert os.path.exists(os.path.join(WORKSPACE_DIR, "Copyright.txt"))
    return WORKSPACE_DIR

def workspace_rel(path):
    return os.path.join(workspace_dir(), path)


AgsVersion = namedtuple('AgsVersion', ['version', 'version_friendly', 'app_id'])

def load_version(path):
    with open(path, "r") as f:
        j = json.load(f)

    version = (int(x) for x in j['version'].split('.'))
    version_friendly = (int(x) for x in j['versionFriendly'].split('.'))
    app_id = j['appID']
    return AgsVersion(version, version_friendly, app_id)


def retry_rmtree(path):
    retries = WINDOWS_IS_STUPID_RETRIES
    while True:
        try:
            if os.path.exists(path):
                shutil.rmtree(path)
            return
        except WindowsError, e:
            if retries <= 0:
                raise
            time.sleep(WINDOWS_IS_STUPID_DELAY)
            retries -= 1

def retry_makedirs(path):
    retries = WINDOWS_IS_STUPID_RETRIES
    while True:
        try:
            if not os.path.exists(path):
                os.makedirs(path)
            return
        except WindowsError, e:
            if retries <= 0:
                raise
            time.sleep(WINDOWS_IS_STUPID_DELAY)
            retries -= 1

def refresh_staging():
    staging_path = workspace_rel("Staging")
    if os.path.exists(staging_path):
        retry_rmtree(staging_path)
    retry_makedirs(staging_path)
    return staging_path

def extract_into(destdir, zippath):
    with zipfile.ZipFile(zippath, "r") as z:
        z.extractall(destdir)

def delete_pdbs(path):
    for dirpath, dirnames, filenames in os.walk(path):
        for filename in filenames:
            _, ext = os.path.splitext(filename)
            if ext.lower() == ".pdb":
                os.remove(os.path.join(dirpath, filename))

def only_pdbs(path):
    for dirpath, dirnames, filenames in os.walk(path):
        for filename in filenames:
            _, ext = os.path.splitext(filename)
            if ext.lower() != ".pdb":
                os.remove(os.path.join(dirpath, filename))

def zip_from_dir(zippath, srcdirpath):
    with zipfile.ZipFile(zippath, 'w', zipfile.ZIP_DEFLATED) as z:
        for dirpath, dirnames, filenames in os.walk(srcdirpath):
            for filename in filenames:
                src_file = os.path.join(dirpath, filename)
                arcname = os.path.relpath(src_file, srcdirpath)
                z.write(src_file, arcname)

def file_data(path, bufsize=4096):
    with file(path, 'rb') as f:
        while True:
            data = f.read(bufsize)
            if not data:
                break
            yield data

def create_checksums(path):
    # Microsoft FCIV only supports MD5 and SHA1
    hash_md5 = hashlib.new("md5")
    hash_sha1 = hashlib.new("sha1")
    hash_sha256 = hashlib.new("sha256")
    for data in file_data(path):
        hash_md5.update(data)
        hash_sha1.update(data)
        hash_sha256.update(data)
    with file(path+".md5", "w") as f:
        print("{0} {1}".format(hash_md5.hexdigest(), os.path.basename(path)), file=f)
    with file(path+".sha1", "w") as f:
        print("{0} {1}".format(hash_sha1.hexdigest(), os.path.basename(path)), file=f)
    with file(path+".sha256", "w") as f:
        print("{0} {1}".format(hash_sha256.hexdigest(), os.path.basename(path)), file=f)

def transfer_installer(output_path, staging_path, ver_str):
    installer_src = workspace_rel("AGS-{0}.exe".format(ver_str))
    installer_dest = os.path.join(output_path, "AGS-{0}.exe".format(ver_str))
    shutil.copy(installer_src, installer_dest)
    create_checksums(installer_dest)


def build_ags_zip(output_path, staging_path, ver_str):

    shutil.copytree(workspace_rel("Windows/Installer/Source/Docs"), os.path.join(staging_path,"Docs"))

    extract_into(staging_path, workspace_rel("editor.zip"))
    extract_into(os.path.join(staging_path,"Templates"), workspace_rel("templates.zip"))
    extract_into(staging_path, workspace_rel("engine.zip"))
    extract_into(staging_path, workspace_rel("manual.zip"))

    delete_pdbs (staging_path)

    zip_path = os.path.join(output_path, "AGS-{0}.zip".format(ver_str))
    zip_from_dir(zip_path, staging_path)

    create_checksums(zip_path)


def build_nomp3_zip(output_path, staging_path, ver_str):

    extract_into(staging_path, workspace_rel("engine-no-mp3.zip"))

    delete_pdbs (staging_path)

    zip_path = os.path.join(output_path, "AGS-{0}-noMP3.zip".format(ver_str))
    zip_from_dir(zip_path, staging_path)

    create_checksums(zip_path)


def build_pdb_zip(output_path, staging_path, ver_str):

    extract_into(staging_path, workspace_rel("editor.zip"))
    extract_into(staging_path, workspace_rel("engine.zip"))
    extract_into(os.path.join(staging_path,"No MP3"), workspace_rel("engine-no-mp3.zip"))

    only_pdbs(staging_path)

    zip_path = os.path.join(output_path, "AGS-{0}-PDB.zip".format(ver_str))
    zip_from_dir(zip_path, staging_path)

    create_checksums(zip_path)


def main():

    output_path = workspace_rel("Output")
    if os.path.exists(output_path):
        retry_rmtree(output_path)
    retry_makedirs(output_path)

    version = load_version(workspace_rel("version.json"))
    ver_str = ".".join(str(x) for x in version.version_friendly)

    staging_path = refresh_staging()
    transfer_installer(output_path, staging_path, ver_str)

    staging_path = refresh_staging()
    build_ags_zip(output_path, staging_path, ver_str)

    staging_path = refresh_staging()
    build_nomp3_zip(output_path, staging_path, ver_str)

    staging_path = refresh_staging()
    build_pdb_zip(output_path, staging_path, ver_str)



if __name__ == "__main__":
    main()
