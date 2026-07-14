#!/usr/bin/env bash
# Build Compiled/Data archives by running one standalone tool per step.
# Matches the pipeline the AGS maintainers prefer over a single monolith.
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: build-data.sh --open Game.agf [options]

Options:
  --open <path>         Path to Game.agf (required)
  --project-dir <dir>   Project root (default: directory containing Game.agf)
  --output <dir>        Output root (default: <project>/Compiled)
  --force               Pass --force to agsscripts
  -h, --help            Show this help

Requires these tools on PATH (built from Tools/):
  agsscripts, agdta, agsvox, agstrans, agsassets, agscfg

Existing single-purpose tools used elsewhere in the pipeline:
  agscc (one .asc), trac (one .TRS), spritepak (sprites), agspak (generic pack)
EOF
}

AGF=""
PROJECT_DIR=""
OUTPUT=""
FORCE=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --open) AGF="$2"; shift 2 ;;
    --project-dir) PROJECT_DIR="$2"; shift 2 ;;
    --output) OUTPUT="$2"; shift 2 ;;
    --force) FORCE="--force"; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 1 ;;
  esac
done

if [[ -z "$AGF" ]]; then
  echo "Error: --open <Game.agf> is required" >&2
  usage
  exit 1
fi

common=(--open "$AGF")
[[ -n "$PROJECT_DIR" ]] && common+=(--project-dir "$PROJECT_DIR")
[[ -n "$OUTPUT" ]] && common+=(--output "$OUTPUT")

set +e
agsscripts "${common[@]}" $FORCE
agdta "${common[@]}"
agsvox "${common[@]}"
agstrans "${common[@]}"
agsassets "${common[@]}"
agscfg "${common[@]}"
build_code=$?
set -e

if [[ $build_code -gt 1 ]]; then
  exit "$build_code"
fi

echo "Data build finished."
