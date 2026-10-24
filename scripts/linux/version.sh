#!/bin/bash
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"

EXIT_SUCCESS=0
EXIT_FAILURE=1

VERSION_FILE="${PROJECT_ROOT_FOLDER}/VERSION"

usage() {
  cat <<'USAGE'
Usage: ./version.sh [--get] [<new-version>]
  --get          Print current version (default with no arguments)
  <new-version>  Set version (format: major.minor.patch, e.g. 0.1.0)
  -h, --help     Show this help
Version is read from the VERSION file at the repository root.
CMakeLists.txt reads that file at configure time, so no other
file needs editing when the version changes.
USAGE
}

print_current_version() {
  if [[ ! -f "${VERSION_FILE}" ]]; then
    echo "[version] VERSION file not found: ${VERSION_FILE}" >&2
    exit "${EXIT_FAILURE}"
  fi
  cat "${VERSION_FILE}"
}

if [[ $# -eq 0 ]]; then
  print_current_version
  exit "${EXIT_SUCCESS}"
fi

case "$1" in
  -h|--help)
    usage
    exit "${EXIT_SUCCESS}"
    ;;
  --get)
    print_current_version
    exit "${EXIT_SUCCESS}"
    ;;
  -*)
    echo "Unknown option: $1" >&2
    usage >&2
    exit "${EXIT_FAILURE}"
    ;;
esac

if [[ $# -gt 1 ]]; then
  echo "Too many arguments." >&2
  usage >&2
  exit "${EXIT_FAILURE}"
fi

NEW_VERSION="$1"

if [[ ! "${NEW_VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+([._-][0-9A-Za-z]+)*$ ]]; then
  echo "[version] Invalid version: ${NEW_VERSION}" >&2
  echo "[version] Expected format like major.minor.patch (e.g. 0.1.0)" >&2
  exit "${EXIT_FAILURE}"
fi

OLD_VERSION=""
if [[ -f "${VERSION_FILE}" ]]; then
  OLD_VERSION="$(cat "${VERSION_FILE}")"
fi

printf '%s\n' "${NEW_VERSION}" > "${VERSION_FILE}"
printf 'Version changed: %s -> %s\n' "${OLD_VERSION:-none}" "${NEW_VERSION}"
