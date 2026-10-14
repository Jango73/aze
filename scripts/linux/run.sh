#!/bin/bash
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"

EXIT_SUCCESS=0
EXIT_FAILURE=1

BUILD_TYPE="Release"

usage() {
  cat <<'USAGE'
Usage: ./run.sh [--debug|--release] [-d|-r] [-c Debug|Release] [-- <app args>]
  --debug,   -d  Build type Debug
  --release, -r  Build type Release (default)
  -c             Build type (Debug or Release)
  Extra arguments after -- are forwarded to aze (eg. -- run-tests).
USAGE
}

FORWARD_ARGS=()
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
    --)
      shift
      FORWARD_ARGS=("$@")
      break
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

BUILD_FOLDER="${PROJECT_ROOT_FOLDER}/build-${BUILD_TYPE,,}"
LOG_FILE="${PROJECT_ROOT_FOLDER}/tmp/run.log"

if [[ "${BUILD_TYPE}" == "Debug" ]]; then
  BINARY="${BUILD_FOLDER}/Aze/bin/aze-dbg"
else
  BINARY="${BUILD_FOLDER}/Aze/bin/aze"
fi

NEEDS_BUILD=0

if [ ! -f "${BINARY}" ]; then
  echo "[run] Binary not found, running build first..."
  NEEDS_BUILD=1
elif [ -n "$(find "${PROJECT_ROOT_FOLDER}/Aze" "${PROJECT_ROOT_FOLDER}/AzeLib" -path "${PROJECT_ROOT_FOLDER}/build-*" -prune -o \( -name '*.pro' -o -name '*.pri' -o -name '*.cpp' -o -name '*.h' \) -newer "${BINARY}" -print -quit 2>/dev/null)" ]; then
  echo "[run] Sources changed, running build first..."
  NEEDS_BUILD=1
fi

if [[ "${NEEDS_BUILD}" -eq 1 ]]; then
  "${SCRIPT_FOLDER}/build.sh" -c "${BUILD_TYPE}"
fi

if [ ! -f "${BINARY}" ]; then
  echo "[run] Binary not found at ${BINARY}" >&2
  exit "${EXIT_FAILURE}"
fi

mkdir -p "${PROJECT_ROOT_FOLDER}/tmp"
echo "[run] Logging to ${LOG_FILE}"
LD_LIBRARY_PATH="${BUILD_FOLDER}/AzeLib/bin" \
  exec "${BINARY}" "${FORWARD_ARGS[@]}" 2>&1 | tee "${LOG_FILE}"
