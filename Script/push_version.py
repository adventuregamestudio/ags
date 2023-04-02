#!/usr/bin/env python

import codecs
import json
import re
from collections import namedtuple

AgsVersion = namedtuple('AgsVersion', ['version', 'version_friendly', 'version_year', 'version_month', 'version_is_beta', 'app_id'])

def load_version(path):
    with open(path, "r") as f:
        j = json.load(f)

    version = j['version'].split('.')
    version_friendly = j['versionFriendly'].split('.')
    version_year = j['versionYear']
    version_month = j['versionMonth']
    version_is_beta = j["versionIsBeta"]
    app_id = j['appID']
    return AgsVersion(version, version_friendly, version_year, version_month, version_is_beta, app_id)

def read_file(path, encoding):
    print(path, encoding)

    with codecs.open(path, "r", encoding=encoding) as f:
        return f.read()

def write_file(path, encoding, data):
    with codecs.open(path, "w", encoding=encoding) as f:
        f.write(data)

def replace_group(match, group, data, replacement):
    return data[:match.start(group)] + replacement + data[match.end(group):]


def main():

    # load key/value data, we will use it as a replacement reference
    version = load_version("../version.json")

    # -----------------------------------------------------------------------------
    # Engine's version declaration

    path = "../Common/core/def_version.h"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'\#define ACI_VERSION_STR\s+"(.*)"', data)
    data = replace_group(m, 1, data, ".".join(version.version))

    m = re.search(r'\#define ACI_VERSION_MSRC_DEF\s+(.*)', data)
    data = replace_group(m, 1, data, ",".join(version.version))

    m = re.search(r'\#define ACI_COPYRIGHT_YEARS\s+"\d+-(\d*)"', data)
    data = replace_group(m, 1, data, version.version_year)

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------
    # Script compiler's copyright string

    path = "../Compiler/script/cs_parser.cpp"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'" \(c\) 2000-2007 Chris Jones and \d+-(\d*) others"', data)
    data = replace_group(m, 1, data, version.version_year)

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------
    # Editor's version declaration

    path = "../Editor/AGS.Types/Properties/AssemblyInfo.cs"
    encoding = "utf-8-sig"
    data = read_file(path, encoding)

    m = re.search(r'AGS_EDITOR_FRIENDLY_VERSION\s*=\s*"([^"]+)', data)
    data = replace_group(m, 1, data, ".".join(version.version_friendly))

    m = re.search(r'AGS_EDITOR_VERSION\s*=\s*"([^"]+)', data)
    data = replace_group(m, 1, data, ".".join(version.version))

    m = re.search(r'AGS_EDITOR_DATE\s*=\s*"([\w\s]*)"', data)
    data = replace_group(m, 1, data, version.version_month + " " + version.version_year)

    m = re.search(r'AGS_EDITOR_COPYRIGHT\s*=\s*"Copyright . 2006-2011 Chris Jones and \d+-(\d*) others."', data)
    data = replace_group(m, 1, data, version.version_year)

    m = re.search(r'IS_BETA_VERSION\s*=\s*(\w*);', data)
    data = replace_group(m, 1, data, version.version_is_beta)

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------
    # Editor's app manifest

    path = "../Editor/AGS.Editor/app.manifest"
    encoding = "utf-8-sig"
    data = read_file(path, encoding)

    m = re.search(r'assemblyIdentity version\s*=\s*"([^"]+)', data)
    data = replace_group(m, 1, data, ".".join(version.version))

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------
    # Android native lib manifest

    path = "../Android/library/runtime/src/main/AndroidManifest.xml"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'android\:versionName\s*=\s*"(.*)"', data)
    data = replace_group(m, 1, data, ".".join(version.version_friendly))

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------
    # CMakeLists

    path = "../CMakeLists.txt"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'project\s*\(\s*AGS\s+VERSION\s+([\d\.]+)', data, re.MULTILINE)
    data = replace_group(m, 1, data, ".".join(version.version))

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------
    # Copyright.txt: a list of credits

    path = "../Copyright.txt"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'Copyright \(c\) 1999-2011 Chris Jones and \d+-(\d*) various contributors.', data)
    data = replace_group(m, 1, data, version.version_year)

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------
    # Installer's License.txt

    path = "../Windows/Installer/License.txt"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'Copyright \(c\) 1999-2011 Chris Jones and \d+-(\d*) various contributors.', data)
    data = replace_group(m, 1, data, version.version_year)

    write_file(path, encoding, data)


if __name__ == "__main__":
    main()
