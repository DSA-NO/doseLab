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
cp "$script_dir/../scripts/run-micromamba.sh" "$script_dir/../scripts/run-isolated.sh" "$tmp/project/scripts/"
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
  run)
    [[ "$2" == "-n" && "$3" == "geant4-fixture" ]]
    shift 3
    exec "$@"
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

if bash "$runner" build > "$tmp/output" 2>&1; then
  echo "Expected a missing Geant4 installation to be rejected." >&2
  exit 1
fi
grep -q 'Geant4 is missing from the active environment' "$tmp/output"

if "$script_dir/../scripts/run-isolated.sh" "$tmp/missing-manager" geant4-fixture true > "$tmp/output" 2>&1; then
  echo "Expected a missing environment manager to be rejected." >&2
  exit 1
fi
grep -q 'environment manager not found' "$tmp/output"

CPATH=/foreign/include LIBRARY_PATH=/foreign/lib CMAKE_PREFIX_PATH=/foreign \
Geant4_DIR=/foreign Geant4_ROOT=/foreign ZLIB_ROOT=/foreign ROOTSYS=/foreign PYTHONPATH=/foreign G4LEDATA=/foreign \
G4RUN_MANAGER_TYPE=Serial LAB_OUTPUT_DIR=/requested/output \
  "$script_dir/../scripts/run-isolated.sh" "$tmp/micromamba" geant4-fixture bash -c '
    for name in CPATH LIBRARY_PATH CMAKE_PREFIX_PATH Geant4_DIR Geant4_ROOT ZLIB_ROOT ROOTSYS PYTHONPATH G4LEDATA; do
      if [[ -v "$name" ]]; then
        echo "Inherited $name was not cleared" >&2
        exit 1
      fi
    done
    [[ "$G4RUN_MANAGER_TYPE" == Serial && "$LAB_OUTPUT_DIR" == /requested/output ]]
    [[ "$PYTHONNOUSERSITE" == 1 ]]
  '

echo "Micromamba environment checks passed."
