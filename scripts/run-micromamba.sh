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

mode="${1:-run}"
shift || true

case "$mode" in
  build)
    ensure_env
    "$MICROMAMBA_BIN" run -n "$ENV_NAME" bash -lc "cd '$APP_ROOT' && rm -rf build && cmake -S . -B build -G Ninja && cmake --build build --parallel"
    ;;

  run)
    ensure_env
    if [[ $# -gt 0 ]]; then
      run_args=()
      for arg in "$@"; do
        run_args+=("$(printf '%q' "$arg")")
      done
      run_cmd="./build/${APP_NAME} ${run_args[*]}"
    else
      run_cmd="./build/${APP_NAME} -b run-simple.mac"
    fi
    "$MICROMAMBA_BIN" run -n "$ENV_NAME" bash -lc "cd '$APP_ROOT' && rm -rf build && cmake -S . -B build -G Ninja && cmake --build build --parallel && ${run_cmd}"
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
    echo "  $0 run -b run-simple.mac"
    echo "  $0 shell"
    exit 2
    ;;
esac
