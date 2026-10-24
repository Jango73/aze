#!/bin/bash
# Builds a .deb package installable via "sudo apt install ./aze_<version>_<arch>.deb".
# Two modes:
#   --debhelper  Standard Debian build (needs debhelper installed).
#   --staging    Staging build with dpkg-deb only (no extra dependency).
# With no option, debhelper is used when available, staging otherwise.
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"

EXIT_SUCCESS=0
EXIT_FAILURE=1

MODE="auto"

usage() {
  cat <<'USAGE'
Usage: ./package-deb.sh [--debhelper|--staging]
  --debhelper  Build with dpkg-buildpackage + debhelper (needs: sudo apt install debhelper)
  --staging    Build with dpkg-deb only (works with the current dependencies)
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --debhelper)
      MODE="debhelper"
      shift
      ;;
    --staging)
      MODE="staging"
      shift
      ;;
    -h|--help)
      usage
      exit "${EXIT_SUCCESS}"
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit "${EXIT_FAILURE}"
      ;;
  esac
done

VERSION="$(cat "${PROJECT_ROOT_FOLDER}/VERSION")"
ARCHITECTURE="$(dpkg --print-architecture)"

build_with_debhelper() {
  if ! command -v dh >/dev/null 2>&1; then
    echo "[package-deb] debhelper not found, install it first:" >&2
    echo "[package-deb]   sudo apt install debhelper devscripts" >&2
    exit "${EXIT_FAILURE}"
  fi

  cd "${PROJECT_ROOT_FOLDER}"
  dpkg-buildpackage -us -uc -b
  echo "[package-deb] Built package, install it with:"
  echo "[package-deb]   sudo apt install ./aze_${VERSION}_${ARCHITECTURE}.deb"
}

build_with_staging() {
  STAGE_FOLDER="${PROJECT_ROOT_FOLDER}/tmp/deb-stage"
  OUTPUT_FILE="${PROJECT_ROOT_FOLDER}/tmp/aze_${VERSION}_${ARCHITECTURE}.deb"

  # Clean build: incremental builds do not recompile objects when only
  # CMake settings or VERSION change, which would embed stale data.
  echo "[package-deb] Cleaning previous build..."
  "${SCRIPT_FOLDER}/clean.sh" --release

  echo "[package-deb] Building (Release)..."
  "${SCRIPT_FOLDER}/build.sh" --release

  echo "[package-deb] Staging files..."
  rm -rf "${STAGE_FOLDER}"
  mkdir -p "${STAGE_FOLDER}/DEBIAN" "${PROJECT_ROOT_FOLDER}/tmp"
  DESTDIR="${STAGE_FOLDER}" cmake --install "${PROJECT_ROOT_FOLDER}/build-release" --prefix /usr

  INSTALLED_SIZE="$(du -sk "${STAGE_FOLDER}/usr" | cut -f1)"

  cat > "${STAGE_FOLDER}/DEBIAN/control" <<CONTROL_EOF
Package: aze
Version: ${VERSION}
Section: vcs
Priority: optional
Architecture: ${ARCHITECTURE}
Maintainer: Jango73 <guillaumed00@gmail.com>
Depends: libqt6core6, libqt6network6, libqt6test6, libqt6xml6
Installed-Size: ${INSTALLED_SIZE}
Description: distributed versioning system (client and server)
 Aze is a versioning tool shipped as a single binary providing both
 the client commands and the "aze serve" server.
 .
 This package installs /usr/bin/aze and the libaze runtime library.
CONTROL_EOF

  dpkg-deb --build "${STAGE_FOLDER}" "${OUTPUT_FILE}"

  echo "[package-deb] Built ${OUTPUT_FILE}, install it with:"
  echo "[package-deb]   sudo apt install ${OUTPUT_FILE}"
}

if [[ "${MODE}" == "debhelper" ]]; then
  build_with_debhelper
  exit "${EXIT_SUCCESS}"
fi

if [[ "${MODE}" == "staging" ]]; then
  build_with_staging
  exit "${EXIT_SUCCESS}"
fi

if command -v dh >/dev/null 2>&1; then
  build_with_debhelper
else
  build_with_staging
fi
