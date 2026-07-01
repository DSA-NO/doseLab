#!/usr/bin/env bash
set -euo pipefail

ENV_NAME="${DOSELAB_ENV_NAME:-doselab-production}"
BUILD_DIR="${DOSELAB_BUILD_DIR:-build-production}"
OUT_DIR="${DOSELAB_OUTPUT_DIR:-analysis/fano/latest}"
SOURCE_MACROS_DIR="${DOSELAB_SOURCE_MACROS_DIR:-macros}"

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
  echo "If micromamba works interactively but not in scripts, set:" >&2
  echo "  DOSELAB_ENV_CMD=\"$HOME/.local/bin/micromamba\"" >&2
  exit 1
fi

echo "[doseLab] env command: $ENV_CMD"
echo "[doseLab] env name: $ENV_NAME"
echo "[doseLab] build dir: $BUILD_DIR"

run_env() {
  "$ENV_CMD" run -n "$ENV_NAME" "$@"
}

sync_required_macros() {
  local required=(
    cavity-farmer.mac
    cavity-roos.mac
    cavity-farmer-walled.mac
    cavity-roos-walled.mac
    depth-ref-5cm-center.mac
    fano-base.mac
    source-co60-teletherapy.mac
    field-ref-10x10-ssd100.mac
  )

  for macro in "${required[@]}"; do
    if [[ ! -f "$SOURCE_MACROS_DIR/$macro" ]]; then
      echo "Missing required source macro: $SOURCE_MACROS_DIR/$macro" >&2
      exit 1
    fi
    cp -f "$SOURCE_MACROS_DIR/$macro" "$BUILD_DIR/$macro"
  done
}

mkdir -p "$BUILD_DIR"
mkdir -p "$OUT_DIR"

if [[ ! -x "$BUILD_DIR/doseLab" ]]; then
  echo "Missing executable: $BUILD_DIR/doseLab" >&2
  exit 1
fi

sync_required_macros

CHAMBERS=(
  farmer
  roos
  farmer-walled
  roos-walled
)

MODELS=(
  option4
  livermore
  penelope
)

for chamber in "${CHAMBERS[@]}"; do
  cavity_macro="cavity-${chamber}.mac"
  chamber_tag="${chamber//-/_}"

  for model in "${MODELS[@]}"; do
    tag="run-fano-v1-${chamber}-${model}"
    case_macro="$BUILD_DIR/${tag}.mac"

    cat > "$case_macro" <<EOF
/doseLab/output/tag ${tag}
/control/execute ${cavity_macro}
/control/execute fano-base.mac
EOF

    echo "[doseLab] running chamber=${chamber_tag} model=${model}"
    run_env "$BUILD_DIR/doseLab" -p "$model" -b "$case_macro"

    out_file="$BUILD_DIR/doseLab-${tag}.root"
    if [[ ! -f "$out_file" ]]; then
      echo "Missing expected output: $out_file" >&2
      exit 1
    fi
    cp -f "$out_file" "$OUT_DIR/"
  done
done

echo "[doseLab] fano matrix run completed"
echo "[doseLab] outputs copied to: $OUT_DIR"