#!/usr/bin/env bash
set -euo pipefail

ENV_NAME="${DOSELAB_ENV_NAME:-doselab-production}"
OUT_DIR="${DOSELAB_OUTPUT_DIR:-analysis/fano/latest}"
SUMMARY_JSON="${DOSELAB_FANO_SUMMARY_JSON:-${OUT_DIR}/summary.json}"
SUMMARY_TEXT="${DOSELAB_FANO_SUMMARY_TEXT:-${OUT_DIR}/summary.txt}"
ARCHIVE_STAMP="${1:-}"

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

ENV_CMD="$(resolve_env_cmd || true)"
if [[ -z "$ENV_CMD" ]]; then
  echo "No environment manager found (micromamba/mamba/conda)." >&2
  exit 1
fi

run_env() {
  "$ENV_CMD" run -n "$ENV_NAME" "$@"
}

echo "[doseLab] step 1/3 run Fano matrix"
./scripts/run-fano-matrix.sh

echo "[doseLab] step 2/3 summarize Fano matrix"
run_env ./scripts/summarize-fano-matrix.py \
  --input-dir "$OUT_DIR" \
  --output-json "$SUMMARY_JSON" \
  --output-text "$SUMMARY_TEXT" \
  --strict

echo "[doseLab] step 3/3 archive baseline"
if [[ -n "$ARCHIVE_STAMP" ]]; then
  DOSELAB_FANO_SOURCE_DIR="$OUT_DIR" ./scripts/archive-fano-baseline.sh "$ARCHIVE_STAMP"
else
  DOSELAB_FANO_SOURCE_DIR="$OUT_DIR" ./scripts/archive-fano-baseline.sh
fi

echo "[doseLab] baseline run complete"
echo "[doseLab] summary json: $SUMMARY_JSON"
echo "[doseLab] summary text: $SUMMARY_TEXT"
