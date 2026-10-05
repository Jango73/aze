#!/bin/bash
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"

EXIT_SUCCESS=0
EXIT_FAILURE=1

BUILD_TYPE="Release"

usage() {
  cat <<'USAGE'
Usage: ./build.sh [--debug|--release] [-d|-r] [-c Debug|Release]
  --debug,   -d  Build type Debug
  --release, -r  Build type Release (default)
  -c             Build type (Debug or Release)
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --debug|-d)
      BUILD_TYPE="Debug"
      shift
      ;;
    --release|-r)
      BUILD_TYPE="Release"
      shift
      ;;
    -c)
      if [[ -z "${2:-}" ]]; then
        echo "Option -c requires an argument." >&2
        usage >&2
        exit "${EXIT_FAILURE}"
      fi
      BUILD_TYPE="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit "${EXIT_SUCCESS}"
      ;;
    *)
      echo "Unknown option: ${1}" >&2
      usage >&2
      exit "${EXIT_FAILURE}"
      ;;
  esac
done

if [[ "${BUILD_TYPE}" != "Debug" && "${BUILD_TYPE}" != "Release" ]]; then
  echo "Invalid build type: ${BUILD_TYPE} (use Debug or Release)" >&2
  exit "${EXIT_FAILURE}"
fi

BUILD_DIR="${PROJECT_ROOT_FOLDER}/build-${BUILD_TYPE,,}"
QMAKE_CONFIG="release"
if [[ "${BUILD_TYPE}" == "Debug" ]]; then
  QMAKE_CONFIG="debug"
fi

mkdir -p "${BUILD_DIR}"
qmake6 -o "${BUILD_DIR}/Makefile" "${PROJECT_ROOT_FOLDER}/Aze.pro" "CONFIG+=${QMAKE_CONFIG}"
make -C "${BUILD_DIR}" -j"$(nproc)"

printf '\nBuilt target in %s\n' "${BUILD_DIR}"
