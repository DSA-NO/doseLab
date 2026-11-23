#!/usr/bin/env bash
set -euo pipefail

if (( $# < 3 )); then
  echo "Usage: $0 <environment-manager> <environment-name> <command> [args...]" >&2
  exit 2
fi

if ! manager="$(command -v "$1")"; then
  echo "ERROR: environment manager not found: $1" >&2
  exit 1
fi
env_name="$2"
shift 2

unset_args=()
while IFS= read -r name; do
  case "$name" in
    LAB_*|BASELAB_*|DOSELAB_*|SOURCELAB_*) ;;
    CONDA_*|_CE_*|G4*DATA|G4DATASETSDIR|G4INSTALL|G4SYSTEM|Geant4_DIR|ROOTSYS|ROOT_*|*_ROOT|*_ROOT_DIR|CMAKE_*|CPATH|C_INCLUDE_PATH|\
    CPLUS_INCLUDE_PATH|OBJC_INCLUDE_PATH|LIBRARY_PATH|LD_*|DYLD_*|PKG_CONFIG_*|\
    PYTHON*|QT_*|QTDIR|XERCESCROOT|CC|CXX|FC|F77|F90|AR|AS|LD|NM|RANLIB|STRIP|\
    CFLAGS|CXXFLAGS|CPPFLAGS|FFLAGS|FCFLAGS|LDFLAGS|BASH_ENV|ENV)
      unset_args+=(-u "$name")
      ;;
  esac
done < <(compgen -e)

exec env "${unset_args[@]}" PATH="/usr/bin:/bin:/usr/sbin:/sbin" PYTHONNOUSERSITE=1 \
  "$manager" run -n "$env_name" "$@"
