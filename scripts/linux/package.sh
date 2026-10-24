#!/bin/bash
# Builds Release packages from an isolated build folder, then copies the
# versioned artifacts into dist/.
# Produces a .deb (dpkg-deb staging of "cmake --install") and a .tar.gz of
# the staged install tree.
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"
BUILD_TYPE="Release"
BUILD_FOLDER="${PROJECT_ROOT_FOLDER}/build-package-${BUILD_TYPE,,}"
OUTPUT_FOLDER="${PROJECT_ROOT_FOLDER}/dist"
VERSION="$(tr -d '[:space:]' < "${PROJECT_ROOT_FOLDER}/VERSION")"

EXIT_SUCCESS=0
EXIT_FAILURE=1

usage() {
  cat <<'USAGE'
Usage: ./package.sh [-h|--help]
  Build Release packages (.deb + .tar.gz) into dist/.
USAGE
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit "${EXIT_SUCCESS}"
fi

if [[ $# -ne 0 ]]; then
  usage >&2
  exit "${EXIT_FAILURE}"
fi

if [ ! -f "${PROJECT_ROOT_FOLDER}/qt-plus/source/cpp/CXMLNode.h" ] \
  || [ ! -f "${PROJECT_ROOT_FOLDER}/AzeLib/sources/dtl/dtl/dtl.hpp" ]; then
  echo "[package] Submodules missing, run ./scripts/linux/deps.sh first." >&2
  exit "${EXIT_FAILURE}"
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "[package] cmake not found, run ./scripts/linux/deps.sh first." >&2
  exit "${EXIT_FAILURE}"
fi

if ! command -v ninja >/dev/null 2>&1; then
  echo "[package] ninja not found, run ./scripts/linux/deps.sh first." >&2
  exit "${EXIT_FAILURE}"
fi

if ! command -v dpkg-deb >/dev/null 2>&1; then
  echo "[package] dpkg-deb not found, install it first." >&2
  exit "${EXIT_FAILURE}"
fi

ARCHITECTURE="$(dpkg --print-architecture)"
STAGE_FOLDER="${BUILD_FOLDER}/stage"
DEB_FILE="aze_${VERSION}_${ARCHITECTURE}.deb"
TARBALL_FILE="aze-${VERSION}-linux-${ARCHITECTURE}.tar.gz"

mkdir -p "${BUILD_FOLDER}" "${OUTPUT_FOLDER}"
rm -f "${BUILD_FOLDER}/"*.deb "${BUILD_FOLDER}/"*.tar.gz 2>/dev/null || true

cmake -S "${PROJECT_ROOT_FOLDER}" -B "${BUILD_FOLDER}" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_FOLDER}"

rm -rf "${STAGE_FOLDER}"
mkdir -p "${STAGE_FOLDER}/DEBIAN"
DESTDIR="${STAGE_FOLDER}" cmake --install "${BUILD_FOLDER}" --prefix /usr

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

dpkg-deb --build "${STAGE_FOLDER}" "${BUILD_FOLDER}/${DEB_FILE}"
tar -czf "${BUILD_FOLDER}/${TARBALL_FILE}" -C "${STAGE_FOLDER}" usr

rm -f "${OUTPUT_FOLDER}/aze_"*.deb "${OUTPUT_FOLDER}/aze-"*.tar.gz 2>/dev/null || true
cp -f "${BUILD_FOLDER}/${DEB_FILE}" "${BUILD_FOLDER}/${TARBALL_FILE}" "${OUTPUT_FOLDER}/"

echo
printf 'Packages generated in %s\n' "${OUTPUT_FOLDER}"
find "${OUTPUT_FOLDER}" -maxdepth 1 -type f \( -name "*.deb" -o -name "*.tar.gz" \) -print | sort
