#!/usr/bin/env python

import codecs
import json
import os
import re
import sys
from collections import namedtuple

SDLSoundVersion = namedtuple('SDLSoundVersion', ['revision', 'url_hash'])
SDLVersion = namedtuple('SDLVersion', ['revision', 'release_type', 'url_hash'])


def load_version(path):
    with open(path, "r") as f:
        j = json.load(f)

    sdl_data = j['dependencies']['sdl']
    sdl_revision = sdl_data['revision']
    sdl_release_type = sdl_data['release_type']
    sdl_url_hash = sdl_data['urlHash']

    sdl_version = SDLVersion(sdl_revision, sdl_release_type, sdl_url_hash)

    sdlsound_data = j['dependencies']['sdlsound']
    sdlsound_revision = sdlsound_data['revision']
    sdlsound_url_hash = sdlsound_data['urlHash']
    sdlsound_version = SDLSoundVersion(sdlsound_revision, sdlsound_url_hash)

    d = dict()
    d['sdl_version'] = sdl_version
    d['sdlsound_version'] = sdlsound_version
    return d


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


def replace_shasums_in_file(file_path, encoding, version, artifact):
    data = read_file(file_path, encoding)
    if data is None:
        print('\t\t- READ FAILED')
        return

    m = re.search(r'(\w*) {2}' + artifact, data)
    if m is not None:
        data = replace_group(m, 1, data, version.url_hash)

    write_file(file_path, encoding, data)
    print('\t\t- PROCESSED')


def sdl_replace_shasums_in_file(file_path, encoding, version):
    replace_shasums_in_file(file_path, encoding, version, 'SDL.tar.gz')


def sdlsound_replace_shasums_in_file(file_path, encoding, version):
    replace_shasums_in_file(file_path, encoding, version, 'SDL_sound.tar.gz')


def sdl_replace_in_file(file_path, encoding, version):
    data = read_file(file_path, encoding)
    if data is None:
        print('\t\t- READ FAILED')
        return

    release_name = version.release_type + "-" + version.revision

    m = re.search(r'URL https://github.com/libsdl-org/SDL/archive/refs/tags/(\w+-[0-9\.]+).tar.gz', data)
    if m is not None:
        data = replace_group(m, 1, data, release_name)

    m = re.search(r'SDL_VERSION_NUMBER=([0-9\.]+)', data)
    if m is not None:
        data = replace_group(m, 1, data, version.revision)

    m = re.search(r'SDL_VERSION=(\w+-[0-9\.]+)', data)
    if m is not None:
        data = replace_group(m, 1, data, release_name)

    # must lookup for a double line section here, avoid changing any random SHA1 string
    m = re.search(r'URL https://github.com/libsdl-org/SDL/archive/refs/tags/\S*.tar.gz\s+URL_HASH SHA1=(\w*)', data)
    if m is not None:
        data = replace_group(m, 1, data, version.url_hash)

    write_file(file_path, encoding, data)
    print('\t\t- PROCESSED')


def sdlsound_replace_in_file(file_path, encoding, version):
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

    # must lookup for a double line section here, avoid changing any random SHA1 string
    m = re.search(r'URL https://github.com/icculus/SDL_sound/\S*.tar.gz\s+URL_HASH SHA1=(\w*)', data)
    if m is not None:
        data = replace_group(m, 1, data, version.url_hash)

    write_file(file_path, encoding, data)
    print('\t\t- PROCESSED')


def main():
    # load key/value data, we will use it as a replacement reference
    version = load_version("../version.json")
    sdl_version = version['sdl_version']
    sdlsound_version = version['sdlsound_version']

    # -----------------------------------------------------------------------------
    # For SDL_Sound, do the replacement in all the code dirs, where applicable
    sdlsound_replace_in_file("../CMake/FetchSDL_Sound.cmake", "utf-8", sdlsound_version)
    sdlsound_replace_in_file("../debian/README.md", "utf-8", sdlsound_version)
    sdlsound_replace_in_file("../OSX/README.md", "utf-8", sdlsound_version)
    sdlsound_replace_in_file("../Windows/README.md", "utf-8", sdlsound_version)

    sdlsound_replace_shasums_in_file("../libsrc/sha1sums", "utf-8", sdlsound_version)

    # -----------------------------------------------------------------------------
    # For SDL, do the replacement in all the code dirs, where applicable
    sdl_replace_in_file("../CMake/FetchSDL2.cmake", "utf-8", sdl_version)
    sdl_replace_in_file("../ci/windows/Dockerfile", "utf-8", sdl_version)
    sdl_replace_in_file("../ci/linux/Dockerfile", "utf-8", sdl_version)
    sdl_replace_in_file("../libsrc/download.sh", "utf-8", sdl_version)

    sdl_replace_shasums_in_file("../libsrc/sha1sums", "utf-8", sdl_version)


if __name__ == "__main__":
    main()
