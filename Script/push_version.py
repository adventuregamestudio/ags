#!/usr/bin/env python

import codecs
import json
import re
from collections import namedtuple

AgsVersion = namedtuple('AgsVersion', ['version', 'version_friendly', 'app_id'])

def load_version(path):
    with open(path, "r") as f:
        j = json.load(f)

    version = j['version'].split('.')
    version_friendly = j['versionFriendly'].split('.')
    app_id = j['appID']
    return AgsVersion(version, version_friendly, app_id)

def read_file(path, encoding):
    print path, encoding

    with codecs.open(path, "r", encoding=encoding) as f:
        return f.read()

def write_file(path, encoding, data):
    with codecs.open(path, "w", encoding=encoding) as f:
        f.write(data)

def replace_group(match, group, data, replacement):
    return data[:match.start(group)] + replacement + data[match.end(group):]


def main():

    version = load_version("../version.json")

    # -----------------------------------------------------------------------------

    path = "../Common/core/def_version.h"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'\#define ACI_VERSION_STR\s+"(.*)"', data)
    data = replace_group(m, 1, data, ".".join(version.version))

    m = re.search(r'\#define ACI_VERSION_MSRC_DEF\s+(.*)', data)
    data = replace_group(m, 1, data, ",".join(version.version))

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------

    path = "../Editor/AGS.Types/Properties/AssemblyInfo.cs"
    encoding = "utf-8-sig"
    data = read_file(path, encoding)

    m = re.search(r'AGS_EDITOR_FRIENDLY_VERSION\s*=\s*"([^"]+)', data)
    data = replace_group(m, 1, data, ".".join(version.version_friendly))

    m = re.search(r'AGS_EDITOR_VERSION\s*=\s*"([^"]+)', data)
    data = replace_group(m, 1, data, ".".join(version.version))

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------

    path = "../Editor/AGS.Editor/app.manifest"
    encoding = "utf-8-sig"
    data = read_file(path, encoding)

    m = re.search(r'assemblyIdentity version\s*=\s*"([^"]+)', data)
    data = replace_group(m, 1, data, ".".join(version.version))

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------

    path = "../Manual/ags.tex"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'\\title\{Adventure Game Studio\s+(.*)\}', data)
    data = replace_group(m, 1, data, ".".join(version.version_friendly))

    write_file(path, encoding, data)

    # -----------------------------------------------------------------------------

    path = "../Android/library/AndroidManifest.xml"
    encoding = "utf-8"
    data = read_file(path, encoding)

    m = re.search(r'android\:versionName\s*=\s*"(.*)"', data)
    data = replace_group(m, 1, data, ".".join(version.version_friendly))

    write_file(path, encoding, data)


if __name__ == "__main__":
    main()
