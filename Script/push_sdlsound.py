#!/usr/bin/env python

import codecs
import json
import os
import re
import sys
from collections import namedtuple

SDLSoundVersion = namedtuple('SDLSoundVersion', ['revision', 'url_hash'])

def load_version(path):
    with open(path, "r") as f:
        j = json.load(f)

    sdlsound_data = j['sdlsound'];
    revision = sdlsound_data['revision']
    url_hash = sdlsound_data['urlHash']
    return SDLSoundVersion(revision, url_hash)

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

    m = re.search(r'https://github.com/icculus/SDL_sound/archive/(\w+).tar.gz', data)
    if m is not None:
        data = replace_group(m, 1, data, version.revision)

    m = re.search(r'https://github.com/icculus/SDL_sound/archive/(\w+).zip', data)
    if m is not None:
        data = replace_group(m, 1, data, version.revision)

    m = re.search(r'SDL2_SOUND_VERSION=(\w+)', data)
    if m is not None:
        data = replace_group(m, 1, data, version.revision)

    # must lookup for a double line section here, avoid changing any random MD5 string
    m = re.search(r'URL https://github.com/icculus/SDL_sound/\S*.tar.gz\s+URL_HASH MD5=(\w*)', data)
    if m is not None:
        data = replace_group(m, 1, data, version.url_hash)

    write_file(file_path, encoding, data)
    print('\t\t- PROCESSED')

def main():

    # load key/value data, we will use it as a replacement reference
    version = load_version("../version.json")
    
    # -----------------------------------------------------------------------------
    # Do the replacement in all the code dirs, where applicable
    replace_in_file("../CMake/FetchSDL_Sound.cmake", "utf-8", version)
    replace_in_file("../debian/README.md", "utf-8", version)
    replace_in_file("../OSX/README.md", "utf-8", version)
    replace_in_file("../Windows/README.md", "utf-8", version)


if __name__ == "__main__":
    main()
