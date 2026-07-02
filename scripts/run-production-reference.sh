#!/usr/bin/env bash
set -euo pipefail

ENV_NAME="${DOSELAB_ENV_NAME:-doselab-production}"
BUILD_DIR="${DOSELAB_BUILD_DIR:-build-production}"
OUT_DIR="${DOSELAB_OUTPUT_DIR:-analysis/production/latest}"

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

MACROS=(
  run-ref-10x10-d5cm-6mv-farmer.mac
  run-ref-10x10-d5cm-6mv-roos.mac
  run-ref-10x10-d5cm-6mv-farmer-walled.mac
  run-ref-10x10-d5cm-6mv-roos-walled.mac
)

mkdir -p "$OUT_DIR"

pushd "$BUILD_DIR" >/dev/null
for macro in "${MACROS[@]}"; do
  run_env ./doseLab -b "$macro"
done
popd >/dev/null

cp -f "$BUILD_DIR"/doseLab-run-ref-10x10-d5cm-6mv-*.root "$OUT_DIR"/
