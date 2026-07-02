#!/usr/bin/env bash
set -euo pipefail

# Shared helper used by production/fano scripts to resolve the environment
# command consistently in one place.

resolve_env_cmd() {
  local candidate="${DOSELAB_ENV_CMD:-}"

  if [[ -n "$candidate" ]]; then
    if [[ -x "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
    if command -v "$candidate" >/dev/null 2>&1; then
      command -v "$candidate"
      return 0
    fi
  fi

  for cmd in micromamba mamba conda; do
    if command -v "$cmd" >/dev/null 2>&1; then
      command -v "$cmd"
      return 0
    fi
  done

  for path in "$HOME/.local/bin/micromamba" "$HOME/micromamba/bin/micromamba"; do
    if [[ -x "$path" ]]; then
      printf '%s\n' "$path"
      return 0
    fi
  done

  return 1
}
