#!/bin/bash
# AGS Editor (ImGui) — Simple install script for Linux
# Usage: ./install.sh [prefix]
# Default prefix: /usr/local

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PREFIX="${1:-/usr/local}"
BUILD_DIR="${SCRIPT_DIR}/../../build"

echo "=== AGS Editor (ImGui) Installer ==="
echo "Prefix: ${PREFIX}"
echo ""

# Check if binary exists
BINARY="${BUILD_DIR}/EditorImGui/ags_editor_imgui"
if [ ! -f "$BINARY" ]; then
    echo "Error: Binary not found at ${BINARY}"
    echo "Please build the project first:"
    echo "  cd ags/build && cmake .. && cmake --build . --target ags_editor_imgui"
    exit 1
fi

# Install binary
echo "Installing binary..."
install -Dm755 "$BINARY" "${PREFIX}/bin/ags_editor_imgui"

# Install desktop file
DESKTOP_FILE="${SCRIPT_DIR}/ags-editor.desktop"
if [ -f "$DESKTOP_FILE" ]; then
    echo "Installing desktop entry..."
    install -Dm644 "$DESKTOP_FILE" "${PREFIX}/share/applications/ags-editor.desktop"
fi

# Install font files (needed at runtime)
FONT_DIR="${SCRIPT_DIR}/../imgui/misc/fonts"
if [ -d "$FONT_DIR" ]; then
    echo "Installing fonts..."
    install -d "${PREFIX}/share/ags-editor/fonts"
    for f in "$FONT_DIR"/*.ttf; do
        [ -f "$f" ] && install -m644 "$f" "${PREFIX}/share/ags-editor/fonts/"
    done
fi

echo ""
echo "=== Installation complete ==="
echo "Run: ags_editor_imgui"
