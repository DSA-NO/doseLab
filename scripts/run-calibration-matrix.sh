#!/usr/bin/env bash
set -euo pipefail

ENV_NAME="${DOSELAB_ENV_NAME:-doselab-production}"
BUILD_DIR="${DOSELAB_BUILD_DIR:-build-production}"
OUT_DIR="${DOSELAB_OUTPUT_DIR:-analysis/calibration/latest}"
SOURCE_MACROS_DIR="${DOSELAB_SOURCE_MACROS_DIR:-macros}"
CALIB_MODE="${DOSELAB_CALIB_MODE:-fast}"
CALIB_PRIMARIES="${DOSELAB_CALIB_PRIMARIES:-}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
# shellcheck source=./env-helpers.sh
source "$SCRIPT_DIR/env-helpers.sh"

if [[ "$SOURCE_MACROS_DIR" != /* ]]; then
  SOURCE_MACROS_DIR="$REPO_ROOT/$SOURCE_MACROS_DIR"
fi
if [[ "$BUILD_DIR" != /* ]]; then
  BUILD_DIR="$REPO_ROOT/$BUILD_DIR"
fi
if [[ "$OUT_DIR" != /* ]]; then
  OUT_DIR="$REPO_ROOT/$OUT_DIR"
fi

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
echo "[doseLab] calibration mode: $CALIB_MODE"

run_env() {
  "$ENV_CMD" run -n "$ENV_NAME" "$@"
}

resolve_calib_primaries() {
  if [[ -n "$CALIB_PRIMARIES" ]]; then
    printf '%s\n' "$CALIB_PRIMARIES"
    return 0
  fi

  case "$CALIB_MODE" in
    fast)
      printf '2000000\n'
      ;;
    full)
      printf '20000000\n'
      ;;
    *)
      echo "Invalid DOSELAB_CALIB_MODE='$CALIB_MODE' (expected: fast|full)" >&2
      return 1
      ;;
  esac
}

sync_required_macros() {
  local required=(
    cavity-farmer.mac
    cavity-roos.mac
    cavity-farmer-walled.mac
    cavity-roos-walled.mac
    source-co60-teletherapy.mac
    source-linac-photons-6mv.mac
    source-linac-photons-10mv.mac
    field-ref-10x10-ssd100.mac
    depth-ref-5cm-center.mac
    calibration-base.mac
  )

  for macro in "${required[@]}"; do
    if [[ ! -f "$SOURCE_MACROS_DIR/$macro" ]]; then
      echo "Missing required source macro: $SOURCE_MACROS_DIR/$macro" >&2
      exit 1
    fi
    cp -f "$SOURCE_MACROS_DIR/$macro" "$BUILD_DIR/$macro"
  done
}

source_macro_for_beam() {
  local beam="$1"
  case "$beam" in
    co60)
      echo "source-co60-teletherapy.mac"
      ;;
    6mv)
      echo "source-linac-photons-6mv.mac"
      ;;
    10mv)
      echo "source-linac-photons-10mv.mac"
      ;;
    *)
      echo "Unknown beam key: $beam" >&2
      return 1
      ;;
  esac
}

mkdir -p "$BUILD_DIR"
mkdir -p "$OUT_DIR"

if [[ ! -x "$BUILD_DIR/doseLab" ]]; then
  echo "Missing executable: $BUILD_DIR/doseLab" >&2
  exit 1
fi

sync_required_macros
CALIB_PRIMARIES="$(resolve_calib_primaries)"
if [[ ! "$CALIB_PRIMARIES" =~ ^[0-9]+$ ]]; then
  echo "Invalid DOSELAB_CALIB_PRIMARIES='$CALIB_PRIMARIES' (must be integer)" >&2
  exit 1
fi
echo "[doseLab] primaries per case: $CALIB_PRIMARIES"

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

BEAMS=(
  co60
  6mv
  10mv
)

MEDIA=(
  cavity
  water
)

for chamber in "${CHAMBERS[@]}"; do
  cavity_macro="cavity-${chamber}.mac"

  for model in "${MODELS[@]}"; do
    for beam in "${BEAMS[@]}"; do
      source_macro="$(source_macro_for_beam "$beam")"

      for medium in "${MEDIA[@]}"; do
        tag="run-calib-v1-${beam}-${chamber}-${model}-${medium}"
        case_macro="$BUILD_DIR/${tag}.mac"

        {
          echo "/doseLab/output/tag ${tag}"
          echo "/control/execute ${cavity_macro}"
          if [[ "$medium" == "water" ]]; then
            echo "/doseLab/cavity/material G4_WATER"
            echo "/doseLab/cavity/wallMaterial G4_WATER"
          fi
          echo "/control/execute calibration-base.mac"
          echo "/control/execute ${source_macro}"
          echo "/control/execute field-ref-10x10-ssd100.mac"
          echo "/run/printProgress 10000"
          echo "/run/beamOn ${CALIB_PRIMARIES}"
        } > "$case_macro"

        echo "[doseLab] running beam=${beam} chamber=${chamber} model=${model} medium=${medium}"
        run_env "$BUILD_DIR/doseLab" -p "$model" -b "$case_macro"

        out_file="$BUILD_DIR/doseLab-${tag}.root"
        if [[ ! -f "$out_file" ]]; then
          echo "Missing expected output: $out_file" >&2
          exit 1
        fi
        cp -f "$out_file" "$OUT_DIR/"
      done
    done
  done
done

echo "[doseLab] calibration matrix run completed"
echo "[doseLab] outputs copied to: $OUT_DIR"
