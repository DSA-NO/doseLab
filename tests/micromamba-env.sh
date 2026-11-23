#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != Linux || "$(uname -m)" != x86_64 ]]; then
  echo "Skipping Linux lock tests on this platform."
  exit 0
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
tmp="$(mktemp -d)"
trap 'rm -rf -- "$tmp"' EXIT
mkdir -p "$tmp/project/scripts" "$tmp/state"
cp "$script_dir/../scripts/run-micromamba.sh" "$tmp/project/scripts/"
printf 'name: geant4-fixture\n' > "$tmp/project/environment.yml"
{
  printf '# environment.yml sha256: %s\n' "$(sha256sum "$tmp/project/environment.yml" | awk '{print $1}')"
  printf '@EXPLICIT\nhttps://example.invalid/package.conda#1234\n'
} > "$tmp/project/environment-linux-64.lock"

cat > "$tmp/micromamba" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
case "$1" in
  env)
    [[ "${FAIL_ENV_LIST:-0}" == 0 ]] || exit 1
    [[ ! -f "$TEST_ROOT/state/exists" ]] || printf 'geant4-fixture %s\n' "$TEST_ROOT/state/env"
    ;;
  create)
    cp "$TEST_ROOT/project/environment-linux-64.lock" "$TEST_ROOT/state/installed"
    touch "$TEST_ROOT/state/exists"
    ;;
  list)
    [[ "${FAIL_LIST:-0}" == 0 ]] || exit 1
    cat "$TEST_ROOT/state/installed"
    ;;
  *) exit 1 ;;
esac
EOF
chmod +x "$tmp/micromamba"
export TEST_ROOT="$tmp" MICROMAMBA_BIN="$tmp/micromamba"
runner="$tmp/project/scripts/run-micromamba.sh"

bash "$runner" setup
bash "$runner" check

printf 'https://example.invalid/extra.conda#abcd\n' >> "$tmp/state/installed"
if bash "$runner" check > "$tmp/output" 2>&1; then
  echo "Expected an extra installed package to be rejected." >&2
  exit 1
fi
grep -q 'does not match environment-linux-64.lock' "$tmp/output"

cp "$tmp/project/environment-linux-64.lock" "$tmp/state/installed"
printf '# changed\n' >> "$tmp/project/environment.yml"
if bash "$runner" check > "$tmp/output" 2>&1; then
  echo "Expected a changed manifest to be rejected." >&2
  exit 1
fi
grep -q 'environment.yml does not match' "$tmp/output"
sed -i '$d' "$tmp/project/environment.yml"

if FAIL_LIST=1 bash "$runner" check > "$tmp/output" 2>&1; then
  echo "Expected a failed package listing to be rejected." >&2
  exit 1
fi
grep -q 'could not inspect' "$tmp/output"

if FAIL_ENV_LIST=1 bash "$runner" setup > "$tmp/output" 2>&1; then
  echo "Expected a failed environment listing to be rejected." >&2
  exit 1
fi
grep -q 'could not list' "$tmp/output"

echo "Micromamba environment checks passed."
