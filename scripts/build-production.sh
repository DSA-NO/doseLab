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
  "$SCRIPT_DIR/run-isolated.sh" "$ENV_CMD" "$ENV_NAME" "$@"
}

if [[ "$ENV_NAME" == "geant4-doseLab" && "$ENV_FILE" == "environment.yml" ]]; then
  if [[ "${1:-}" == "--create-env" ]]; then
    MICROMAMBA_BIN="$ENV_CMD" "$SCRIPT_DIR/run-micromamba.sh" setup
  elif [[ "$(uname -s)" == Linux && "$(uname -m)" == x86_64 ]]; then
    MICROMAMBA_BIN="$ENV_CMD" "$SCRIPT_DIR/run-micromamba.sh" check
  fi
else
  echo "WARNING: custom production environment is not checked against the project lockfile." >&2
  if [[ "${1:-}" == "--create-env" ]]; then
    "$ENV_CMD" env create -f "$ENV_FILE" -y || "$ENV_CMD" env update -f "$ENV_FILE" -y
  fi
fi

ENV_PREFIX="$($ENV_CMD env list | awk -v env="$ENV_NAME" '$1==env {print $NF}')"
if [[ -z "$ENV_PREFIX" ]]; then
  ENV_PREFIX="$HOME/micromamba/envs/$ENV_NAME"
fi

if [[ ! -f "$ENV_PREFIX/lib/cmake/Geant4/Geant4Config.cmake" ]]; then
  echo "ERROR: Geant4 is missing from environment $ENV_NAME ($ENV_PREFIX)." >&2
  exit 1
fi

BUILD_ENV=(
  "CPATH=$ENV_PREFIX/include/freetype2:$ENV_PREFIX/include"
  "CPLUS_INCLUDE_PATH=$ENV_PREFIX/include/freetype2:$ENV_PREFIX/include"
  "LIBRARY_PATH=$ENV_PREFIX/lib"
)

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

run_env env "${BUILD_ENV[@]}" cmake --fresh -S . -B "$BUILD_DIR" -G Ninja \
  -DDOSELAB_BUILD_ROOT_SUMMARY=OFF \
  -DDOSELAB_REQUIRE_ROOT_SUMMARY=OFF \
  -DGeant4_DIR="$ENV_PREFIX/lib/cmake/Geant4" \
  -DCMAKE_PREFIX_PATH="$ENV_PREFIX" \
  -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE \
  -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE \
  -DCMAKE_INCLUDE_PATH="$ENV_PREFIX/include/freetype2;$ENV_PREFIX/include" \
  -DCMAKE_LIBRARY_PATH="$ENV_PREFIX/lib" \
  "${EXPAT_ARGS[@]}" \
  "${ZLIB_ARGS[@]}" \
  "${FREETYPE_ARGS[@]}"

run_env env "${BUILD_ENV[@]}" cmake --build "$BUILD_DIR" --parallel
