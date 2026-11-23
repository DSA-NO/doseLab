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

run_and_copy_output() {
  local expected_name="$1" destination="$2" log output_path
  shift 2
  log="$(mktemp)"
  if ! run_env "$@" | tee "$log"; then
    rm -f -- "$log"
    return 1
  fi
  output_path="$(sed -n 's/^Output path: //p' "$log")"
  rm -f -- "$log"
  if [[ -z "$output_path" || "$output_path" == *$'\n'* ||
        "$(basename "$output_path")" != "$expected_name" || ! -f "$output_path" ]]; then
    echo "Expected one output file named $expected_name; reported: $output_path" >&2
    return 1
  fi
  cp -f -- "$output_path" "$destination/"
}
