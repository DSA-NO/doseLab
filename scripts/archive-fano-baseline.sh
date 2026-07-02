#!/usr/bin/env bash
set -euo pipefail

SRC_DIR="${DOSELAB_FANO_SOURCE_DIR:-analysis/fano/latest}"
BASE_DIR="${DOSELAB_FANO_BASELINE_DIR:-analysis/fano/baselines}"
STAMP="${1:-$(date -u +%Y%m%d-%H%M%S)}"
TARGET_DIR="${BASE_DIR}/${STAMP}"
INCLUDE_ROOT="${DOSELAB_FANO_INCLUDE_ROOT:-0}"

if [[ ! -f "${SRC_DIR}/summary.json" || ! -f "${SRC_DIR}/summary.txt" ]]; then
  echo "Missing summary files in ${SRC_DIR}" >&2
  echo "Expected: ${SRC_DIR}/summary.json and ${SRC_DIR}/summary.txt" >&2
  exit 1
fi

mkdir -p "${TARGET_DIR}"
cp -f "${SRC_DIR}/summary.json" "${TARGET_DIR}/summary.json"
cp -f "${SRC_DIR}/summary.txt" "${TARGET_DIR}/summary.txt"

if [[ "${INCLUDE_ROOT}" == "1" ]]; then
  shopt -s nullglob
  root_files=("${SRC_DIR}"/doseLab-run-fano-v1-*.root)
  if (( ${#root_files[@]} > 0 )); then
    cp -f "${root_files[@]}" "${TARGET_DIR}/"
  fi
fi

commit_sha="unknown"
if command -v git >/dev/null 2>&1; then
  commit_sha="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
fi

cat > "${TARGET_DIR}/metadata.json" <<EOF
{
  "created_utc": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "source_dir": "${SRC_DIR}",
  "git_commit": "${commit_sha}",
  "included_root_files": $( [[ "${INCLUDE_ROOT}" == "1" ]] && echo true || echo false )
}
EOF

ln -sfn "${STAMP}" "${BASE_DIR}/latest"

echo "Archived Fano baseline to ${TARGET_DIR}"
echo "Updated symlink: ${BASE_DIR}/latest -> ${STAMP}"
