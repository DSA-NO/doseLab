#!/usr/bin/env bash
set -euo pipefail

INSTALL_BASE="${MICROMAMBA_INSTALL_BASE:-$HOME/.local}"
BIN_DIR="$INSTALL_BASE/bin"
mkdir -p "$BIN_DIR"

OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
  Linux)
    case "$ARCH" in
      x86_64|amd64)
        PLATFORM="linux-64"
        ;;
      aarch64|arm64)
        PLATFORM="linux-aarch64"
        ;;
      *)
        echo "Unsupported Linux architecture: $ARCH" >&2
        exit 1
        ;;
    esac
    ;;
  Darwin)
    case "$ARCH" in
      x86_64|amd64)
        PLATFORM="osx-64"
        ;;
      arm64|aarch64)
        PLATFORM="osx-arm64"
        ;;
      *)
        echo "Unsupported macOS architecture: $ARCH" >&2
        exit 1
        ;;
    esac
    ;;
  *)
    echo "Unsupported OS: $OS" >&2
    exit 1
    ;;
esac

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

URL="https://micro.mamba.pm/api/micromamba/${PLATFORM}/latest"

echo "Installing micromamba for ${OS}/${ARCH} into ${BIN_DIR}"
echo "Source: ${URL}"
curl -LsSf "$URL" -o "$TMP_DIR/micromamba.tar.bz2"
tar -xvj -C "$TMP_DIR" -f "$TMP_DIR/micromamba.tar.bz2" "bin/micromamba"
install -m 755 "$TMP_DIR/bin/micromamba" "$BIN_DIR/micromamba"

cat <<EOF

micromamba installed at: $BIN_DIR/micromamba

Optional shell setup:
  export PATH="$BIN_DIR:\\$PATH"

To verify:
  $BIN_DIR/micromamba --version
EOF

"$BIN_DIR/micromamba" --version
