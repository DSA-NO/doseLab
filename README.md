# doseLab

[![CI (main)](https://github.com/DSA-NO/doseLab/actions/workflows/ci.yml/badge.svg?branch=main&event=push)](https://github.com/DSA-NO/doseLab/actions/workflows/ci.yml?query=branch%3Amain+event%3Apush)

A Geant4-based dose calculation example with configurable scenarios/macros and optional ROOT post-processing.

The example supports reference scenario presets for both Farmer and Roos ionization chambers, including variants with and without cavity wall material.
Scenario macros and metadata controls are designed to keep geometry and output labeling aligned across these configurations.

## Prerequisites

- CMake >= 3.16
- A C++17 compiler
- Geant4 with UI and visualization components
- Optional: ROOT (for `doseLabRootSummary`)

For the reproducible production-validation workflow in this repository, **micromamba is required**.

## Install micromamba

Official installation methods are documented at:

- https://mamba.readthedocs.io/en/latest/installation/micromamba-installation.html

Quick install helper (Linux/macOS):

```bash
"${SHELL}" <(curl -L micro.mamba.pm/install.sh)
```

After installation, open a new shell and verify:

```bash
micromamba --version
```

## Quick Start

From a fresh clone:

```bash
cmake -S . -B build
cmake --build build -j
```

Run an example macro:

```bash
cd build
./doseLab -b run-simple.mac
```

## Local Setup with micromamba

### Fresh-user simulation workflow (recommended first test)

Use this flow to simulate a new user machine with no inherited Geant4/ROOT shell setup:

```bash
env -i HOME="$HOME" USER="$USER" TERM="${TERM:-xterm-256color}" LANG="${LANG:-C.UTF-8}" PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin" bash --noprofile --norc
env | grep -Ei '(^|_)(g4|geant4|rootsys|root_|cmake_prefix_path|ld_library_path)=' || true
mkdir -p "$HOME/geant4-usr"
cd "$HOME/geant4-usr"
git clone git@github.com:DSA-NO/doseLab.git doseLab-micromamba
cd doseLab-micromamba
```

Then run the production-validation workflow below.

To mirror CI locally with pinned conda-forge packages, use the committed environment file:

```bash
micromamba env create -f envs/doselab-production.yml -y
DOSELAB_ENV_CMD=micromamba ./scripts/build-production.sh
DOSELAB_ENV_CMD=micromamba ./scripts/run-production-reference.sh
micromamba run -n doselab-production ./scripts/check-baseline.py --build-dir build-production
```

To initialize or refresh the baseline file after an intentional physics/model update:

```bash
micromamba run -n doselab-production ./scripts/check-baseline.py --build-dir build-production --write-baseline
```

## CI

GitHub Actions runs on every push and pull request using the workflow in `.github/workflows/ci.yml`.

- **Build (Geant4)**
   - Creates a micromamba environment, configures with `-DDOSELAB_BUILD_ROOT_SUMMARY=OFF`, builds, and runs a batch smoke test (`./doseLab -b run-simple.mac`).
- **Build (optional ROOT summary)**
   - Creates a micromamba environment with ROOT, configures with `-DDOSELAB_REQUIRE_ROOT_SUMMARY=ON`, and verifies `doseLabRootSummary` exists.
- **Production validation**
   - Uses the pinned environment file `envs/doselab-production.yml`, runs four reference scenarios (Farmer/Roos, with/without walls), and compares metrics against `analysis/baseline/reference_metrics.json`.
   - Uploads a metrics report artifact (`production-metrics`) for traceability.

This keeps core build checks fast while enforcing a reproducible physics regression gate.

## Contributing Quickstart

Before opening a pull request, run the same baseline checks locally:

```bash
cmake -S . -B build
cmake --build build -j
cd build
./doseLab -b run-simple.mac
```

Optional local ROOT validation:

```bash
cmake -S . -B build-root -DDOSELAB_BUILD_ROOT_SUMMARY=ON -DDOSELAB_REQUIRE_ROOT_SUMMARY=ON
cmake --build build-root -j
test -x build-root/doseLabRootSummary
```

## ROOT Summary Helper

When ROOT is available, CMake builds an additional executable:

- `doseLabRootSummary`

This helper reads and summarizes output ROOT files.

### ROOT detection behavior

CMake tries to find `root-config` in this order:

1. `DOSELAB_ROOT_CONFIG` cache variable (explicit path)
2. `PATH`
3. `$ROOTSYS/bin`
4. `$CONDA_PREFIX/bin`
5. Common local env locations under `$HOME`:
   - `micromamba/envs/*/bin`
   - `miniconda3/envs/*/bin`
   - `anaconda3/envs/*/bin`

If ROOT is not found, the main `doseLab` executable still builds by default.

## Useful CMake Options

- `-DDOSELAB_BUILD_ROOT_SUMMARY=ON|OFF`
  - Enable/disable building `doseLabRootSummary` (default `ON`)
- `-DDOSELAB_REQUIRE_ROOT_SUMMARY=ON|OFF`
  - Fail configure if ROOT summary helper cannot be configured (default `OFF`)
- `-DDOSELAB_ROOT_CONFIG=/absolute/path/to/root-config`
  - Explicit override for ROOT detection

Example with explicit ROOT path:

```bash
cmake -S . -B build -DDOSELAB_ROOT_CONFIG=/path/to/root-config
cmake --build build -j
```

## Install

```bash
cmake -S . -B build
cmake --build build -j
cmake --install build --prefix /desired/prefix
```

Installed content:

- `doseLab` binary in `${CMAKE_INSTALL_BINDIR}`
- `doseLabRootSummary` binary in `${CMAKE_INSTALL_BINDIR}` (if enabled/found)
- `macros/` in `${CMAKE_INSTALL_DATADIR}/doseLab`

## Troubleshooting

If CMake reports ROOT missing:

1. Ensure `root-config` works in your shell:
   ```bash
   root-config --version
   ```
2. Reconfigure with explicit ROOT path:
   ```bash
   cmake -S . -B build -DDOSELAB_ROOT_CONFIG=$(command -v root-config)
   ```
3. To require ROOT summary at configure time:
   ```bash
   cmake -S . -B build -DDOSELAB_REQUIRE_ROOT_SUMMARY=ON
   ```
