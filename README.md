# doseLab

[![CI](https://github.com/DSA-NO/doseLab/actions/workflows/ci.yml/badge.svg)](https://github.com/DSA-NO/doseLab/actions/workflows/ci.yml)

A Geant4-based dose calculation example with configurable scenarios/macros and optional ROOT post-processing.

The example supports reference scenario presets for both Farmer and Roos ionization chambers, including variants with and without cavity wall material.
Scenario macros and metadata controls are designed to keep geometry and output labeling aligned across these configurations.

## Prerequisites

- CMake >= 3.16
- A C++17 compiler
- Geant4 with UI and visualization components
- Optional: ROOT (for `doseLabRootSummary`)

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

## CI

GitHub Actions runs on every push and pull request using the workflow in `.github/workflows/ci.yml`.

- **Build (Geant4)**
   - Installs Geant4 and build tools, configures with `-DDOSELAB_BUILD_ROOT_SUMMARY=OFF`, builds, and runs a batch smoke test (`./doseLab -b run-simple.mac`).
- **Build (optional ROOT summary)**
   - Tries to install ROOT, detects `root-config`, and only then configures with `-DDOSELAB_REQUIRE_ROOT_SUMMARY=ON` and builds `doseLabRootSummary`.
   - If ROOT is unavailable on the runner, this job prints a clear skip notice.

This keeps the core build mandatory while still checking the optional ROOT path when available.

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
