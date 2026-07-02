#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${DOSELAB_BACKUP_REPO_URL:-git@github.com:lindbohansen/doseLab-backup.git}"
WORK_BASE="${DOSELAB_BACKUP_WORK_BASE:-$HOME/geant4-dev/backups}"
MIRROR_DIR="${DOSELAB_BACKUP_MIRROR_DIR:-$WORK_BASE/doseLab-backup-mirror.git}"
ARCHIVE_DIR="${DOSELAB_BACKUP_ARCHIVE_DIR:-$WORK_BASE/archives}"
STAMP="$(date +%Y-%m-%d)"
ARCHIVE_PATH="$ARCHIVE_DIR/doseLab-backup-mirror-$STAMP.tar.gz"

mkdir -p "$WORK_BASE" "$ARCHIVE_DIR"

if [[ -d "$MIRROR_DIR" ]]; then
  git -C "$MIRROR_DIR" remote set-url origin "$REPO_URL"
  git -C "$MIRROR_DIR" fetch --prune origin
else
  git clone --mirror "$REPO_URL" "$MIRROR_DIR"
fi

tar -czf "$ARCHIVE_PATH" -C "$WORK_BASE" "$(basename "$MIRROR_DIR")"

echo "Created weekly mirror archive: $ARCHIVE_PATH"