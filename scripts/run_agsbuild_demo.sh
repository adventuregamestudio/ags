#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

TEMPLATES_DIR="${TEMPLATES_DIR:-/tmp/ags-templates}"
TEMPLATES_REPO="${TEMPLATES_REPO:-https://github.com/adventuregamestudio/ags-templates.git}"

TEMPLATE_NAME="${1:-Sierra-style}"
DEMO_DIR="${DEMO_DIR:-/tmp/ags-demo-${TEMPLATE_NAME// /_}}"

BUILD_AGSBUILD_DIR="${BUILD_AGSBUILD_DIR:-$ROOT/build-agsbuild}"
BUILD_ENGINE_DIR="${BUILD_ENGINE_DIR:-$ROOT/build-engine}"

AGSBUILD_BIN="${AGSBUILD_BIN:-$BUILD_AGSBUILD_DIR/Tools/agsbuild/agsbuild}"
AGSPAK_BIN="${AGSPAK_BIN:-$BUILD_AGSBUILD_DIR/Tools/agspak}"
AGS_BIN="${AGS_BIN:-$BUILD_ENGINE_DIR/ags}"

TIMEOUT_SEC="${TIMEOUT_SEC:-12}"

echo "[demo] Repo root: $ROOT"
echo "[demo] Template: $TEMPLATE_NAME"
echo "[demo] Demo dir:  $DEMO_DIR"

if [[ ! -d "$TEMPLATES_DIR/.git" ]]; then
  echo "[demo] Cloning templates repo to $TEMPLATES_DIR"
  rm -rf "$TEMPLATES_DIR"
  git clone --depth 1 "$TEMPLATES_REPO" "$TEMPLATES_DIR"
fi

TEMPLATE_FILE="$TEMPLATES_DIR/Templates/${TEMPLATE_NAME}.agt"
if [[ ! -f "$TEMPLATE_FILE" ]]; then
  echo "[demo] ERROR: template not found: $TEMPLATE_FILE" >&2
  echo "[demo] Available templates:" >&2
  ls -1 "$TEMPLATES_DIR/Templates" >&2 || true
  exit 1
fi

echo "[demo] Configuring & building agsbuild toolchain (agsbuild, agspak)"
cmake -S "$ROOT" -B "$BUILD_AGSBUILD_DIR" \
  -DAGS_BUILD_TOOLS=ON \
  -DAGS_BUILD_COMPILER=ON \
  -DAGS_BUILD_ENGINE=OFF \
  ${AGS_TESTS:+-DAGS_TESTS=ON} \
  ${AGS_USE_LOCAL_SDL2:+-DAGS_USE_LOCAL_SDL2=ON}
cmake --build "$BUILD_AGSBUILD_DIR" --target agsbuild agspak -j"$(nproc)"

echo "[demo] Configuring & building engine (ags)"
cmake -S "$ROOT" -B "$BUILD_ENGINE_DIR" \
  -DAGS_BUILD_ENGINE=ON \
  -DAGS_BUILD_TOOLS=OFF \
  ${AGS_USE_LOCAL_SDL2:+-DAGS_USE_LOCAL_SDL2=ON} \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_ENGINE_DIR" --target ags -j"$(nproc)"

echo "[demo] Extracting template to $DEMO_DIR"
rm -rf "$DEMO_DIR"
mkdir -p "$DEMO_DIR"
"$AGSPAK_BIN" --export "$TEMPLATE_FILE" "$DEMO_DIR"

echo "[demo] Compiling demo with agsbuild"
"$AGSBUILD_BIN" --open "$DEMO_DIR/Game.agf" --verbose || true

echo "[demo] Engine tell-data"
(cd "$DEMO_DIR" && "$AGS_BIN" --tell-data "Compiled/Data/${TEMPLATE_NAME}.ags")

echo "[demo] Running game for ${TIMEOUT_SEC}s (close window earlier to stop)"
set +e
(cd "$DEMO_DIR" && timeout "$TIMEOUT_SEC" "$AGS_BIN" --windowed --gfxdriver software "Compiled/Data/${TEMPLATE_NAME}.ags")
code=$?
set -e

if [[ "$code" == "124" ]]; then
  echo "[demo] Run timed out after ${TIMEOUT_SEC}s (expected)."
  exit 0
fi

echo "[demo] Engine exited with code $code"
exit "$code"

