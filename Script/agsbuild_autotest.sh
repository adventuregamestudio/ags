#!/bin/sh
# Builds a game project with the C++ tools (agsbuild, in strict mode) and runs the auto-test
# on the result with the engine from the same CMake build.
#
# Usage: Script/agsbuild_autotest.sh <cmake-build-dir> <project-dir> <work-dir> [--build-only]
#
# The build dir needs the tools and the engine (cmake -DAGS_BUILD_TOOLS=1). The project dir is the
# one that holds Game.agf. The tap.py module must be installed (pip install tap.py).
# --build-only stops after the build: the game is in <work-dir>/out, and the engine is not needed.
set -eu

build=$(cd "$1" && pwd)
project=$(cd "$2" && pwd)
mkdir -p "$3"
work=$(cd "$3" && pwd)
repo=$(cd "$(dirname "$0")/.." && pwd)
sys="$work/tools"
out="$work/out"
rm -rf "$sys" "$out"
mkdir -p "$sys" "$out"

for tool in agsbuild agscc agf2dta agf2dlgasc agfexport agspak crmbuild crmpak spriteimport spritepak trac; do
    path=$(find "$build" -type f \( -name "$tool" -o -name "$tool.exe" \) -perm -u+x -not -path '*/_deps/*' | head -n 1)
    if [ -z "$path" ]; then
        echo "Tool not found in $build: $tool" >&2
        exit 1
    fi
    cp "$path" "$sys/"
done
cp "$repo/Editor/AGS.Editor/Resources/agsdefns.sh" "$sys/"

echo "::group::agsbuild"
# --strict: a project element the tools do not know is an error, not a warning
"$sys/agsbuild" "$project/Game.agf" "$out" --sys-dir "$sys" --strict
echo "::endgroup::"

if [ "${4:-}" = "--build-only" ]; then
    exit 0
fi

engine=$(find "$build" -type f \( -name ags -o -name ags.exe \) -perm -u+x -not -path '*/_deps/*' | head -n 1)
if [ -z "$engine" ]; then
    echo "Engine not found in $build" >&2
    exit 1
fi

game=$(ls "$out"/*.ags | head -n 1)
cd "$out"
"$engine" --gfxdriver Software --no-message-box --log-stdout=script:info,main:debug,game:warn \
    --user-data-dir . "$(basename "$game")"
tappy agstest.tap
