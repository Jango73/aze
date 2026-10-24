#!/bin/bash
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"

EXIT_SUCCESS=0
EXIT_FAILURE=1

BUILD_TYPE="Release"
HTTP_ONLY=0
HTTP_PORT="8080"
HTTPS_PORT="8443"
CERT_FILE=""
KEY_FILE=""
CERT_SET=0
KEY_SET=0

DEFAULT_CERT_DIR="${PROJECT_ROOT_FOLDER}/tmp/certs"

usage() {
  cat <<'USAGE'
Usage: ./serve.sh [--debug|--release] [-d|-r] [-c Debug|Release] [--http-only]
                  [--port <http>] [--https-port <https>] [--cert <file>] [--key <file>]
                  [-- <extra aze serve args>]
  --debug,   -d  Build type Debug
  --release, -r  Build type Release (default)
  -c             Build type (Debug or Release)
  --http-only    Plain HTTP only, no TLS (default: HTTP+HTTPS)
  --port         HTTP listening port (default: 8080)
  --https-port   HTTPS listening port (default: 8443)
  --cert         SSL certificate file in PEM format (default: tmp/certs/localhost.crt)
  --key          SSL private key file in PEM format (default: tmp/certs/localhost.key)
  Extra arguments after -- are forwarded to "aze serve".
  The server binds 127.0.0.1 and serves the current folder explicitly:
  pass --bind-address <ip> and --base-path <folder> after -- to override.
  Without --cert/--key, a self-signed dev certificate is generated in
  tmp/certs when missing. Never use it in production.
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
    --http-only)
      HTTP_ONLY=1
      shift
      ;;
    --port)
      if [[ -z "${2:-}" ]]; then
        echo "Option --port requires an argument." >&2
        usage >&2
        exit "${EXIT_FAILURE}"
      fi
      HTTP_PORT="$2"
      shift 2
      ;;
    --https-port)
      if [[ -z "${2:-}" ]]; then
        echo "Option --https-port requires an argument." >&2
        usage >&2
        exit "${EXIT_FAILURE}"
      fi
      HTTPS_PORT="$2"
      shift 2
      ;;
    --cert)
      if [[ -z "${2:-}" ]]; then
        echo "Option --cert requires an argument." >&2
        usage >&2
        exit "${EXIT_FAILURE}"
      fi
      CERT_FILE="$2"
      CERT_SET=1
      shift 2
      ;;
    --key)
      if [[ -z "${2:-}" ]]; then
        echo "Option --key requires an argument." >&2
        usage >&2
        exit "${EXIT_FAILURE}"
      fi
      KEY_FILE="$2"
      KEY_SET=1
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

if [[ "${CERT_SET}" -eq 0 ]]; then
  CERT_FILE="${DEFAULT_CERT_DIR}/localhost.crt"
fi

if [[ "${KEY_SET}" -eq 0 ]]; then
  KEY_FILE="${DEFAULT_CERT_DIR}/localhost.key"
fi

if [[ "${HTTP_ONLY}" -eq 0 && (! -f "${CERT_FILE}" || ! -f "${KEY_FILE}") ]]; then
  if [[ "${CERT_SET}" -eq 1 || "${KEY_SET}" -eq 1 ]]; then
    echo "[serve] Certificate or key not found: ${CERT_FILE} ${KEY_FILE}" >&2
    exit "${EXIT_FAILURE}"
  fi
  MAKE_DEV_CERT="${PROJECT_ROOT_FOLDER}/qt-plus/scripts/linux/make-dev-cert.sh"
  if [ ! -f "${MAKE_DEV_CERT}" ]; then
    echo "[serve] make-dev-cert.sh not found, run ./scripts/linux/deps.sh first." >&2
    exit "${EXIT_FAILURE}"
  fi
  echo "[serve] Generating self-signed dev certificate..."
  "${MAKE_DEV_CERT}" "${DEFAULT_CERT_DIR}"
fi

BUILD_FOLDER="${PROJECT_ROOT_FOLDER}/build-${BUILD_TYPE,,}"
LOG_FILE="${PROJECT_ROOT_FOLDER}/tmp/serve.log"

if [[ "${BUILD_TYPE}" == "Debug" ]]; then
  BINARY="${BUILD_FOLDER}/Aze/bin/aze-dbg"
else
  BINARY="${BUILD_FOLDER}/Aze/bin/aze"
fi

NEEDS_BUILD=0

if [ ! -f "${BINARY}" ]; then
  echo "[serve] Binary not found, running build first..."
  NEEDS_BUILD=1
elif [ -n "$(find "${PROJECT_ROOT_FOLDER}/Aze" "${PROJECT_ROOT_FOLDER}/AzeLib" "${PROJECT_ROOT_FOLDER}/qt-plus" -path "*/.git" -prune -o -path "${PROJECT_ROOT_FOLDER}/build-*" -prune -o \( -name '*.pro' -o -name '*.pri' -o -name '*.cpp' -o -name '*.h' \) -newer "${BINARY}" -print -quit 2>/dev/null)" ]; then
  echo "[serve] Sources changed, running build first..."
  NEEDS_BUILD=1
fi

if [[ "${NEEDS_BUILD}" -eq 1 ]]; then
  "${SCRIPT_FOLDER}/build.sh" -c "${BUILD_TYPE}"
fi

if [ ! -f "${BINARY}" ]; then
  echo "[serve] Binary not found at ${BINARY}" >&2
  exit "${EXIT_FAILURE}"
fi

SERVE_ARGS=(serve --port "${HTTP_PORT}" --bind-address "127.0.0.1" --base-path "${PWD}")

if [[ "${HTTP_ONLY}" -eq 0 ]]; then
  SERVE_ARGS+=(--https-port "${HTTPS_PORT}" --ssl-cert "${CERT_FILE}" --ssl-key "${KEY_FILE}")
fi

SERVE_ARGS+=("${FORWARD_ARGS[@]}")

mkdir -p "${PROJECT_ROOT_FOLDER}/tmp"
echo "[serve] Logging to ${LOG_FILE}"
LD_LIBRARY_PATH="${BUILD_FOLDER}/AzeLib/bin" \
  exec "${BINARY}" "${SERVE_ARGS[@]}" 2>&1 | tee "${LOG_FILE}"
