#!/bin/bash
set -e
#   This is a command line helper to download the sdkmanager and command line
# tools. This will always get the latest stuff. Later you can use the sdkmanager
# to get the specific version of the ndk and build tools you need, but I think
# it should work fine with the latest version of command line tools. If it turns
# out I am wrong we will figure this later.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ANDROID_HOME="${ANDROID_HOME:-$HOME/android-sdk}"
CMDLINE_DIR_BASE="${ANDROID_HOME}/cmdline-tools"
CMDLINE_DIR="${CMDLINE_DIR_BASE}/latest"

PLATFORM=""
CMD_TOOLS_URL=""


function usage
{
   echo "Android SDK and Command Line Tools setup script."
   echo
   echo "Syntax: and-download-sdk-tools.sh [platform] [version]"
   echo "platform:"
   echo "  linux"
   echo "  macos"
   echo "  windows"
   echo
   echo "version:"
   echo "  7.0 | 8.0 | 9.0 | 10.0 | 11.0 | 12.0 | 13.0 | 16.0 | 20.0"
   echo "  default is 11.0"
   echo
   echo "Example:"
   echo "  ./and-download-sdk-tools linux"
   echo
}

function extract_zip
{
  ZIP_FILE="$1"
  TARGET_DIR="$2"

  mkdir -p "$TARGET_DIR"

  if command -v bsdtar >/dev/null 2>&1; then
    bsdtar -xf "$ZIP_FILE" -C "$TARGET_DIR"
  elif command -v unzip >/dev/null 2>&1; then
    unzip -q "$ZIP_FILE" -d "$TARGET_DIR"
  else
    echo "Error: neither bsdtar nor unzip is available"
    exit 1
  fi
}

function download_cmdline_tools
{
  set -e
  echo "Downloading Android command line tools..."

  TMP_DIR="${SCRIPT_DIR}/tmp_cmd_tools_dir"
  TMP_ZIP="${SCRIPT_DIR}/tmp_cmd_tools.zip"

  curl -L "$CMD_TOOLS_URL" -o "$TMP_ZIP"

  mkdir -p "${CMDLINE_DIR_BASE}"
  mkdir -p "${TMP_DIR}"

  echo "Extracting..."
  extract_zip "$TMP_ZIP" "${TMP_DIR}"

  rm -rf "${CMDLINE_DIR}"
  mkdir -p "${CMDLINE_DIR_BASE}"

  mv "$TMP_DIR/cmdline-tools" "$CMDLINE_DIR"

  rm -rf "${TMP_DIR}"
  rm -f "${TMP_ZIP}"

  echo "Download and extraction complete."
}

function setup_environment
{
  set -e
  echo "Setting up environment..."

  mkdir -p "${ANDROID_HOME}"
  touch "${ANDROID_HOME}/repositories.cfg"

  export ANDROID_HOME
  export ANDROID_SDK_ROOT="${ANDROID_HOME}"
  export PATH="${CMDLINE_DIR}/bin:${ANDROID_HOME}/platform-tools:$PATH"

  echo "Environment ready."
}

function test_sdkmanager
{
  set -e
  echo "Testing sdkmanager..."
  "$CMDLINE_DIR/bin/sdkmanager" --version || true
}

function resolve_version_long {
  case "$1" in
    20.0) echo "14742923" ;;
    16.0) echo "12266719" ;;
    13.0) echo "11479570" ;;
    12.0) echo "11076708" ;;
    11.0) echo "10406996" ;;
    10.0) echo "9862592" ;;
    9.0)  echo "9477386" ;;
    8.0)  echo "9123335" ;;
    7.0)  echo "8512546" ;;
    *)    echo "$1" ;;  # already a long version
  esac
}

if [[ $# -eq 0 ]]; then
    usage
    exit
fi

case "$1" in
  macos)
     PLATFORM=mac
     ;;
  windows)
     PLATFORM=win
     ;;
  linux)
     PLATFORM=linux
     ;;
  -h|--help)
     usage
     exit 0
     ;;
  *)
     usage
     exit 1
     ;;
esac

INPUT_VERSION="${2:-11.0}"
CMDLINE_VERSION="$(resolve_version_long "$INPUT_VERSION")"

if [[ -z "${PLATFORM}" ]]; then
    usage
    exit
fi

CMD_TOOLS_URL="https://dl.google.com/android/repository/commandlinetools-${PLATFORM}-${CMDLINE_VERSION}_latest.zip"

download_cmdline_tools
setup_environment
test_sdkmanager
