#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_NAME="$(basename "$APP_ROOT")"

ENV_NAME="$(awk -F': ' '/^name: / {print $2; exit}' "$APP_ROOT/environment.yml" 2>/dev/null || true)"
if [[ -z "$ENV_NAME" ]]; then
  ENV_NAME="geant4-${APP_NAME}"
fi

resolve_micromamba() {
  if command -v micromamba >/dev/null 2>&1; then
    command -v micromamba
    return 0
  fi

  for candidate in "$HOME/.local/bin/micromamba" "$HOME/micromamba/bin/micromamba"; do
    if [[ -x "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  echo "ERROR: micromamba could not be found on PATH. Install it or set MICROMAMBA_BIN." >&2
  return 1
}

MICROMAMBA_BIN="${MICROMAMBA_BIN:-$(resolve_micromamba)}"

ensure_env() {
  if ! "$MICROMAMBA_BIN" env list 2>/dev/null | awk '{print $1}' | grep -Fxq "$ENV_NAME"; then
    echo "Creating micromamba environment: $ENV_NAME"
    "$MICROMAMBA_BIN" env create -n "$ENV_NAME" -f "$APP_ROOT/environment.yml" --yes
  fi
}

resolve_macro_path() {
  local macro_path="$1"

  if [[ "$macro_path" == */* || "$macro_path" == .* ]]; then
    printf '%s\n' "$macro_path"
    return 0
  fi

  if [[ -f "$APP_ROOT/macros/$macro_path" ]]; then
    printf '%s\n' "macros/$macro_path"
    return 0
  fi

  if [[ -f "$APP_ROOT/build/$macro_path" ]]; then
    printf '%s\n' "build/$macro_path"
    return 0
  fi

  printf '%s\n' "$macro_path"
}

prepare_run_cmd() {
  local -a raw_args=("$@")
  local -a rewritten_args=()
  local expecting_macro_path=0
  local arg

  for arg in "${raw_args[@]}"; do
    if [[ "$expecting_macro_path" -eq 1 ]]; then
      rewritten_args+=("$(resolve_macro_path "$arg")")
      expecting_macro_path=0
      continue
    fi

    case "$arg" in
      -b|-v)
        rewritten_args+=("$arg")
        expecting_macro_path=1
        ;;
      *.mac)
        rewritten_args+=("$(resolve_macro_path "$arg")")
        ;;
      *)
        rewritten_args+=("$arg")
        ;;
    esac
  done

  if [[ "$expecting_macro_path" -eq 1 ]]; then
    echo "ERROR: missing macro path after -b or -v" >&2
    exit 2
  fi

  local -a quoted_args=()
  for arg in "${rewritten_args[@]}"; do
    quoted_args+=("$(printf '%q' "$arg")")
  done

  printf './build/%s %s' "$APP_NAME" "${quoted_args[*]}"
}

run_hardened_configure_build() {
  local post_cmd="${1:-}"
  local build_cmd
  build_cmd="cd '$APP_ROOT' && rm -rf build && export CPATH=\"\$CONDA_PREFIX/include/freetype2:\$CONDA_PREFIX/include\${CPATH:+:\$CPATH}\" CPLUS_INCLUDE_PATH=\"\$CONDA_PREFIX/include/freetype2:\$CONDA_PREFIX/include\${CPLUS_INCLUDE_PATH:+:\$CPLUS_INCLUDE_PATH}\" LIBRARY_PATH=\"\$CONDA_PREFIX/lib\${LIBRARY_PATH:+:\$LIBRARY_PATH}\" && cmake -S . -B build -G Ninja -DCMAKE_INCLUDE_PATH=\"\$CONDA_PREFIX/include/freetype2;\$CONDA_PREFIX/include\" -DCMAKE_LIBRARY_PATH=\"\$CONDA_PREFIX/lib\" -DEXPAT_INCLUDE_DIR=\"\$CONDA_PREFIX/include\" -DEXPAT_LIBRARY=\"\$CONDA_PREFIX/lib/libexpat.so\" -DZLIB_INCLUDE_DIR=\"\$CONDA_PREFIX/include\" -DZLIB_LIBRARY=\"\$CONDA_PREFIX/lib/libz.so\" -DXercesC_INCLUDE_DIR=\"\$CONDA_PREFIX/include\" -DXercesC_LIBRARY=\"\$CONDA_PREFIX/lib/libxerces-c.so\" -DFREETYPE_INCLUDE_DIR_freetype2=\"\$CONDA_PREFIX/include/freetype2\" -DFREETYPE_INCLUDE_DIR_ft2build=\"\$CONDA_PREFIX/include/freetype2\" -DFREETYPE_LIBRARY_RELEASE=\"\$CONDA_PREFIX/lib/libfreetype.so\" && cmake --build build --parallel"

  if [[ -n "$post_cmd" ]]; then
    "$MICROMAMBA_BIN" run -n "$ENV_NAME" bash -c "$build_cmd && $post_cmd"
  else
    "$MICROMAMBA_BIN" run -n "$ENV_NAME" bash -c "$build_cmd"
  fi
}

mode="${1:-run}"
shift || true

case "$mode" in
  build)
    ensure_env
    run_hardened_configure_build
    ;;

  run)
    ensure_env
    if [[ $# -gt 0 ]]; then
      run_cmd="$(prepare_run_cmd "$@")"
    else
      run_cmd="$(prepare_run_cmd -b macros/run-simple.mac)"
    fi
    run_hardened_configure_build "${run_cmd}"
    ;;

  shell)
    ensure_env
    exec "$MICROMAMBA_BIN" run -n "$ENV_NAME" bash -il
    ;;

  *)
    echo "Usage: $0 [build|run|shell] [args...]"
    echo "Examples:"
    echo "  $0 build"
    echo "  $0 run"
    echo "  $0 run -b macros/run-simple.mac"
    echo "  $0 shell"
    exit 2
    ;;
esac
