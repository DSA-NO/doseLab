# doseLab

[![CI (main)](https://github.com/DSA-NO/doseLab/actions/workflows/ci.yml/badge.svg?branch=main&event=push)](https://github.com/DSA-NO/doseLab/actions/workflows/ci.yml?query=branch%3Amain+event%3Apush)

## Introduction

doseLab is a Geant4-based simulation project for ionization chamber dosimetry in a water phantom.
It includes reference scenarios for Farmer-type and Roos-type chambers, with variants both with and without wall material.
It can run both Fano checks and beam quality calibration-style runs; a more detailed description will be added to the wiki.

The project is intended as a practical simulation and regression-validation framework where users can configure:

- physics lists (including EM model selections)
- chamber geometry and cavity presets
- materials and phantom/chamber setup
- production cuts, step controls, and run settings through Geant4 macros
- output metadata and scenario tagging for traceable ROOT output

## How To Run

You can run doseLab in two supported ways:

- **A) Reproducible micromamba workflow** (recommended for consistency with CI)
- **B) Your own local Geant4 environment** (recommended if you already maintain a validated Geant4 setup)

## Build Folder Policy

To avoid mixing host and environment-specific builds, use dedicated build directories:

- `build-production`
  - Canonical reproducible build used for baseline checks and CI parity.
- `build-dev-mamba`
  - Optional local sandbox for iterative development inside the micromamba environment.
- `build-check`
  - Temporary sanity-check directory; safe to delete after validation.
- `build`
  - Reserved for local host-Geant4 builds (non-micromamba path), if you use that workflow.

Recommended rule: do not reuse the same build directory across micromamba and host-Geant4 workflows.

## Parallel Setup (Local Geant4 + Micromamba)

If you use both workflows on the same machine, keep your shell neutral by default and opt in per session.

- Do not auto-source `geant4.sh` in shell startup files.
- Do not auto-activate a conda/mamba environment in shell startup files.
- Activate only one stack at a time in a given shell.

Quick checks before configuring/building:

```bash
env | grep -Ei '(^|_)(g4|geant4|rootsys|root_|cmake_prefix_path|ld_library_path)=' || true
echo "CONDA_DEFAULT_ENV=${CONDA_DEFAULT_ENV:-none}"
```

Typical session switches:

```bash
# Local Geant4 session
source "$HOME/geant4/install/bin/geant4.sh"

# Micromamba session
micromamba activate doselab-production

# Return to neutral (example)
micromamba deactivate || true
unset Geant4_DIR G4INSTALL G4SYSTEM G4DATASETSDIR ROOTSYS CMAKE_PREFIX_PATH LD_LIBRARY_PATH
```

Optional local shell helpers:

If you switch often, you may prefer small shell functions in your own shell startup file. These are not provided by doseLab itself, but they can make session switching faster:

```bash
use_doselab_mamba() {
  micromamba activate doselab-production
}

use_doselab_geant4() {
  micromamba deactivate || true
  source "$HOME/geant4/install/bin/geant4.sh"
}
```

This avoids mixed include/library paths and reduces hard-to-debug CMake/runtime conflicts.

### A) Run with micromamba (recommended)

This path mirrors CI and gives the most reproducible dependency stack.

#### 1) Install micromamba

Official instructions:

- https://mamba.readthedocs.io/en/latest/installation/micromamba-installation.html

Quick install helper (Linux/macOS):

```bash
"${SHELL}" <(curl -L micro.mamba.pm/install.sh)
```

Verify:

```bash
micromamba --version
```

#### 2) Clone and run

```bash
git clone git@github.com:DSA-NO/doseLab.git
cd doseLab

micromamba env create -f envs/doselab-production.yml -y
DOSELAB_ENV_CMD=micromamba ./scripts/build-production.sh
DOSELAB_ENV_CMD=micromamba ./scripts/run-production-reference.sh
micromamba run -n doselab-production ./scripts/check-baseline.py --build-dir build-production
```

#### 3) Optional: initialize/refresh baseline intentionally

Use only after intentional physics/model changes and validation:

```bash
micromamba run -n doselab-production ./scripts/check-baseline.py --build-dir build-production --write-baseline
```

#### 4) How to run after setup

Macro families (quick reference):

- `cavity-*`: chamber geometry presets only (Farmer/Roos, walled and non-walled).
- `depth-*`: chamber placement depth + aligned output depth metadata.
- `field-*`: field shape/size/position + field metadata.
- `source-*`: source particle/energy spectrum + source metadata.
- `run-ref-*`: composed batch reference runs.
- `vis-ref-*`: composed visual reference runs.
- `run-fano-*`: composed Fano-like chamber checks (use `-p` to sweep EM model).

Run a quick single macro test:

```bash
micromamba run -n doselab-production ./build-production/doseLab -b ./build-production/run-simple.mac
```

Run a visual macro from the micromamba environment:

```bash
micromamba run -n doselab-production ./build-production/doseLab -v ./build-production/vis-ref-10x10-d5cm-6mv-farmer.mac
```

Note: `-n` belongs to `micromamba run` and must be followed by the environment name, not the executable path. If the environment is already activated in your shell, you can also run the binary directly:

```bash
./build-production/doseLab -v ./build-production/vis-ref-10x10-d5cm-6mv-farmer.mac
```

Run the four production reference scenarios:

```bash
DOSELAB_ENV_CMD=micromamba ./scripts/run-production-reference.sh
```

Run Fano matrix in full-statistics mode (default, 20M primaries per case):

```bash
DOSELAB_ENV_CMD=micromamba ./scripts/run-fano-matrix.sh
```

Run Fano matrix in fast mode (2M primaries per case):

```bash
DOSELAB_ENV_CMD=micromamba DOSELAB_FANO_MODE=fast ./scripts/run-fano-matrix.sh
```

Override primaries explicitly (takes precedence over mode):

```bash
DOSELAB_ENV_CMD=micromamba DOSELAB_FANO_PRIMARIES=5000000 ./scripts/run-fano-matrix.sh
```

Summarize and gate Fano results (strict):

```bash
micromamba run -n doselab-production ./scripts/summarize-fano-matrix.py --strict
```

CI-style gate (fails only if precision passes and physics agreement fails):

```bash
micromamba run -n doselab-production ./scripts/summarize-fano-matrix.py --strict --ci-physics-gate
```

Archive the current Fano summary to a dated baseline folder:

```bash
./scripts/archive-fano-baseline.sh
```

Run full baseline workflow in one command (matrix run, summarize, archive):

```bash
DOSELAB_ENV_CMD=micromamba ./scripts/run-fano-baseline.sh
```

Run the same workflow in fast mode:

```bash
DOSELAB_ENV_CMD=micromamba DOSELAB_FANO_MODE=fast ./scripts/run-fano-baseline.sh
```

Include ROOT files in the archive snapshot:

```bash
DOSELAB_FANO_INCLUDE_ROOT=1 ./scripts/archive-fano-baseline.sh
```

Run calibration matrix (Dw/Dc and kQ workflow) in fast mode (default):

```bash
DOSELAB_ENV_CMD=micromamba ./scripts/run-calibration-matrix.sh
```

Run calibration matrix in full-statistics mode:

```bash
DOSELAB_ENV_CMD=micromamba DOSELAB_CALIB_MODE=full ./scripts/run-calibration-matrix.sh
```

Summarize calibration matrix results (strict):

```bash
micromamba run -n doselab-production ./scripts/summarize-calibration-matrix.py --strict
```

Archive the current calibration summary to a dated baseline folder:

```bash
./scripts/archive-calibration-baseline.sh
```

Run full calibration baseline workflow in one command (matrix run, summarize, archive):

```bash
DOSELAB_ENV_CMD=micromamba ./scripts/run-calibration-baseline.sh
```

Include calibration ROOT files in the archive snapshot:

```bash
DOSELAB_CALIB_INCLUDE_ROOT=1 ./scripts/archive-calibration-baseline.sh
```

Check results against baseline:

```bash
micromamba run -n doselab-production ./scripts/check-baseline.py --build-dir build-production
```

Summarize a ROOT output file:

```bash
micromamba run -n doselab-production ./build-production/doseLabRootSummary ./build-production/doseLab-run-ref-10x10-d5cm-6mv-farmer-walled.root
```

#### Optional fresh-user simulation

If you want to simulate a "new user" shell with no inherited Geant4/ROOT setup:

```bash
env -i HOME="$HOME" USER="$USER" TERM="${TERM:-xterm-256color}" LANG="${LANG:-C.UTF-8}" PATH="$HOME/.local/bin:$HOME/micromamba/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin" bash --noprofile --norc
env | grep -Ei '(^|_)(g4|geant4|rootsys|root_|cmake_prefix_path|ld_library_path)=' || true
command -v micromamba
micromamba --version
```

Then clone and run from that shell.

### B) Run with your own local Geant4 environment

Use this path if Geant4 (and optionally ROOT) are already installed and configured on your system.

#### 1) Prerequisites

- CMake >= 3.16
- C++17 compiler
- Geant4 with UI and visualization components
- Optional ROOT for `doseLabRootSummary`

#### 2) Configure and build

```bash
git clone git@github.com:DSA-NO/doseLab.git
cd doseLab

cmake -S . -B build
cmake --build build -j
```

#### 3) Run examples

```bash
cd build
./doseLab -b run-simple.mac
./doseLab -b run-ref-10x10-d5cm-6mv-farmer.mac
```

#### 4) Optional ROOT helper build control

```bash
cmake -S . -B build -DDOSELAB_BUILD_ROOT_SUMMARY=ON -DDOSELAB_REQUIRE_ROOT_SUMMARY=ON
cmake --build build -j
```

## CI

GitHub Actions runs on push and pull requests using `.github/workflows/ci.yml`.

- **Build (Geant4)**
  - Creates a micromamba environment, configures with `-DDOSELAB_BUILD_ROOT_SUMMARY=OFF`, builds, and runs a batch smoke test.
- **Build (optional ROOT summary)**
  - Creates a micromamba environment with ROOT and verifies `doseLabRootSummary` is produced.
- **Production validation**
  - Uses `envs/doselab-production.yml`, runs four reference scenarios (Farmer/Roos, with/without walls), and compares metrics against `analysis/baseline/reference_metrics.json`.
  - Uploads `analysis/production/latest/metrics.json` as a CI artifact.

## ROOT Summary Helper

When ROOT is available, CMake builds an additional executable:

- `doseLabRootSummary`

This helper reads and summarizes doseLab ROOT outputs.

ROOT detection order in CMake:

1. `DOSELAB_ROOT_CONFIG` (explicit path)
2. `PATH`
3. `$ROOTSYS/bin`
4. `$CONDA_PREFIX/bin`
5. Common local env locations under `$HOME`

## Useful CMake Options

- `-DDOSELAB_BUILD_ROOT_SUMMARY=ON|OFF`
- `-DDOSELAB_REQUIRE_ROOT_SUMMARY=ON|OFF`
- `-DDOSELAB_ROOT_CONFIG=/absolute/path/to/root-config`

## Install

```bash
cmake -S . -B build
cmake --build build -j
cmake --install build --prefix /desired/prefix
```

Installed content:

- `doseLab` in `${CMAKE_INSTALL_BINDIR}`
- `doseLabRootSummary` in `${CMAKE_INSTALL_BINDIR}` (if enabled/found)
- `macros/` in `${CMAKE_INSTALL_DATADIR}/doseLab`

## Troubleshooting

If CMake reports ROOT missing:

```bash
root-config --version
cmake -S . -B build -DDOSELAB_ROOT_CONFIG=$(command -v root-config)
```

If baseline checks fail unexpectedly:

- verify you are using the intended environment (`doselab-production`)
- re-run the reference scenarios before checking baseline
- compare current report `analysis/production/latest/metrics.json` to baseline

If `DOSELAB_ENV_CMD=micromamba ./scripts/build-production.sh` fails with `command not found`:

- your interactive shell may expose `micromamba` via shell init/function, while scripts require an executable path
- use an absolute executable path instead:

```bash
DOSELAB_ENV_CMD="$HOME/.local/bin/micromamba" ./scripts/build-production.sh
```

## Acknowledgements

- This repository is released under the MIT License. See LICENSE.
- This project uses Geant4 and follows the Geant4 license terms: http://cern.ch/geant4/license
- Parts of this code and documentation workflow were developed with assistance from GitHub Copilot in VS Code.
- Copilot may route requests across different language models depending on context and availability.
