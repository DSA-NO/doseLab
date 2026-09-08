#!/usr/bin/env bash
set -euo pipefail

ENV_NAME="${DOSELAB_ENV_NAME:-geant4-doseLab}"
ENV_FILE="${DOSELAB_ENV_FILE:-environment.yml}"
BUILD_DIR="${DOSELAB_BUILD_DIR:-build-production}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./env-helpers.sh
source "$SCRIPT_DIR/env-helpers.sh"

ENV_CMD="$(resolve_env_cmd || true)"

if [[ -z "$ENV_CMD" ]]; then
  echo "No environment manager found (micromamba/mamba/conda)." >&2
  echo "If micromamba works interactively but not in scripts, set:" >&2
  echo "  DOSELAB_ENV_CMD=\"$HOME/.local/bin/micromamba\"" >&2
  exit 1
fi

echo "[doseLab] env command: $ENV_CMD"
echo "[doseLab] env name: $ENV_NAME"

run_env() {
  "$ENV_CMD" run -n "$ENV_NAME" "$@"
}

ENV_PREFIX="$($ENV_CMD env list | awk -v env="$ENV_NAME" '$1==env {print $NF}')"
if [[ -z "$ENV_PREFIX" ]]; then
  ENV_PREFIX="$HOME/micromamba/envs/$ENV_NAME"
fi

# Match CI configure environment hints for Geant4 dependency discovery.
export CPATH="$ENV_PREFIX/include/freetype2:$ENV_PREFIX/include${CPATH:+:$CPATH}"
export CPLUS_INCLUDE_PATH="$ENV_PREFIX/include/freetype2:$ENV_PREFIX/include${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"
export LIBRARY_PATH="$ENV_PREFIX/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"

EXPAT_ARGS=()
if [[ -f "$ENV_PREFIX/include/expat.h" && -f "$ENV_PREFIX/lib/libexpat.so" ]]; then
  EXPAT_ARGS=(
    -DEXPAT_INCLUDE_DIR="$ENV_PREFIX/include"
    -DEXPAT_LIBRARY="$ENV_PREFIX/lib/libexpat.so"
  )
fi

ZLIB_ARGS=()
if [[ -f "$ENV_PREFIX/include/zlib.h" && -f "$ENV_PREFIX/lib/libz.so" ]]; then
  ZLIB_ARGS=(
    -DZLIB_INCLUDE_DIR="$ENV_PREFIX/include"
    -DZLIB_LIBRARY="$ENV_PREFIX/lib/libz.so"
  )
fi

FREETYPE_ARGS=()
if [[ -d "$ENV_PREFIX/include/freetype2" && -f "$ENV_PREFIX/lib/libfreetype.so" ]]; then
  FREETYPE_ARGS=(
    -DFREETYPE_INCLUDE_DIR_freetype2="$ENV_PREFIX/include/freetype2"
    -DFREETYPE_INCLUDE_DIR_ft2build="$ENV_PREFIX/include/freetype2"
    -DFREETYPE_LIBRARY_RELEASE="$ENV_PREFIX/lib/libfreetype.so"
  )
fi

if [[ "${1:-}" == "--create-env" ]]; then
  "$ENV_CMD" env create -f "$ENV_FILE" -y || "$ENV_CMD" env update -f "$ENV_FILE" -y
fi

run_env cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DDOSELAB_BUILD_ROOT_SUMMARY=OFF \
  -DDOSELAB_REQUIRE_ROOT_SUMMARY=OFF \
  -DCMAKE_INCLUDE_PATH="$ENV_PREFIX/include/freetype2;$ENV_PREFIX/include" \
  -DCMAKE_LIBRARY_PATH="$ENV_PREFIX/lib" \
  "${EXPAT_ARGS[@]}" \
  "${ZLIB_ARGS[@]}" \
  "${FREETYPE_ARGS[@]}"

run_env cmake --build "$BUILD_DIR" --parallel
