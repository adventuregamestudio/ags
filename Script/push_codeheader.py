#!/usr/bin/env python

import codecs
import json
import os
import re
import sys
from collections import namedtuple

AgsVersion = namedtuple('AgsVersion', ['version_year', 'license_link'])

def load_version(path):
    with open(path, "r") as f:
        j = json.load(f)

    version_year = j['versionYear']
    license_link = j['licenseLink']
    return AgsVersion(version_year, license_link)

def read_file(path, encoding):
    print(path, encoding)

    with codecs.open(path, "r", encoding=encoding) as f:
        try:
            return f.read()
        except UnicodeDecodeError:
            return None
        except:
            raise

def write_file(path, encoding, data):
    with codecs.open(path, "w", encoding=encoding) as f:
        f.write(data)

def replace_group(match, group, data, replacement):
    return data[:match.start(group)] + replacement + data[match.end(group):]
    
def replace_in_file(file_path, encoding, version):
    data = read_file(file_path, encoding)
    if data is None:
        print('\t\t- READ FAILED')
        return

    m = re.search(r'// Copyright \(C\) 1999-2011 Chris Jones and \d+-(\w*) ([\S ]*)\s*//', data)
    if m is None:
        print('\t\t- NO PATTERN')
        return # first pattern not found, no sense in searching for the rest
    data = replace_group(m, 1, data, version.version_year)
    data = replace_group(m, 2, data, "various contributors")
    
    m = re.search(r'// A copy of this license can be found in the file License.txt and at\s*// (\S+://\S+.\S+)\s*//', data)
    if m is not None:
        data = replace_group(m, 1, data, version.license_link)

    write_file(file_path, encoding, data)
    print('\t\t- PROCESSED')

# -----------------------------------------------------------------------------
# For each C or C++ code file in the given dir, do the replacement
def replace_in_dir_rec(dir_path, encoding, version):
    for dir, subdirs, files in os.walk(dir_path):
        print('--\ndirectory = ' + dir)
        for subdir in subdirs:
            print('\t- subdirectory ' + subdir)
        for filename in files:
            _, file_ext = os.path.splitext(filename)
            file_path = os.path.join(dir, filename)
            # print('\t- file %s (ext: %s) (full path: %s)' % (filename, file_ext, file_path))
            if file_ext != '.h' and file_ext != '.c' and file_ext != '.cpp':
                continue
            replace_in_file(file_path, encoding, version)

def main():

    # load key/value data, we will use it as a replacement reference
    version = load_version("../version.json")
    
    # -----------------------------------------------------------------------------
    # Do the replacement in all the code dirs, where applicable
    replace_in_dir_rec("../Common", "utf-8", version)
    replace_in_dir_rec("../Compiler", "utf-8", version)
    replace_in_dir_rec("../Editor/AGS.Native", "utf-8", version)
    replace_in_dir_rec("../Engine", "utf-8", version)
    replace_in_dir_rec("../Plugins", "utf-8", version)
    replace_in_dir_rec("../Tools", "utf-8", version)


if __name__ == "__main__":
    main()
