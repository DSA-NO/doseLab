#!/usr/bin/env bash
set -euo pipefail

project="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prefix="${CONDA_PREFIX:?Run this test in the Geant4 environment}"
tmp="$(mktemp -d)"
trap 'rm -rf -- "$tmp"' EXIT
mkdir -p "$tmp/foreign/bin" "$tmp/active/bin"
export ROOT_TEST_MARKER="$tmp/root-config-ran"
cat > "$tmp/foreign/bin/root-config" <<'EOF'
#!/usr/bin/env bash
touch "$ROOT_TEST_MARKER"
EOF
chmod +x "$tmp/foreign/bin/root-config"

configure() {
  cmake -S "$project" -G Ninja \
    -DGeant4_DIR="$prefix/lib/cmake/Geant4" \
    -DCMAKE_PREFIX_PATH="$prefix" "$@"
}

export CONDA_PREFIX="$tmp/active" ROOTSYS="$tmp/foreign"
export PATH="$tmp/foreign/bin:$PATH"
configure -B "$tmp/default" -DROOT_CONFIG_EXECUTABLE="$tmp/foreign/bin/root-config" > "$tmp/default.log"
if [[ -e "$ROOT_TEST_MARKER" ]] || ! grep -q 'ROOT not found: skipping' "$tmp/default.log"; then
  echo "An unrelated ROOT installation was discovered." >&2
  exit 1
fi

if configure -B "$tmp/explicit-foreign" \
  -DDOSELAB_ROOT_CONFIG="$tmp/foreign/bin/root-config" > "$tmp/foreign.log" 2>&1; then
  echo "An explicit ROOT installation outside the active environment was accepted." >&2
  exit 1
fi
grep -q 'root-config must be inside' "$tmp/foreign.log"

ln -s "$tmp/foreign/bin/root-config" "$tmp/active/bin/root-config"
if configure -B "$tmp/symlink" \
  -DDOSELAB_ROOT_CONFIG="$tmp/active/bin/root-config" > "$tmp/symlink.log" 2>&1; then
  echo "A ROOT symlink outside the active environment was accepted." >&2
  exit 1
fi
grep -q 'root-config must be inside' "$tmp/symlink.log"
if configure -B "$tmp/symlink-auto" > "$tmp/symlink-auto.log" 2>&1; then
  echo "Automatic ROOT discovery followed a symlink outside the active environment." >&2
  exit 1
fi
grep -q 'root-config must be inside' "$tmp/symlink-auto.log"

rm "$tmp/active/bin/root-config"
cp "$tmp/foreign/bin/root-config" "$tmp/active/bin/root-config"
configure -B "$tmp/inside" -DDOSELAB_ROOT_CONFIG="$tmp/active/bin/root-config" > "$tmp/inside.log"
[[ -e "$ROOT_TEST_MARKER" ]] && grep -q 'enabled target doseLabRootSummary' "$tmp/inside.log"

rm "$ROOT_TEST_MARKER"
unset CONDA_PREFIX
configure -B "$tmp/host" -DDOSELAB_ROOT_CONFIG="$tmp/foreign/bin/root-config" > "$tmp/host.log"
[[ -e "$ROOT_TEST_MARKER" ]] && grep -q 'enabled target doseLabRootSummary' "$tmp/host.log"

echo "ROOT discovery isolation checks passed."
