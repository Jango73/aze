#!/bin/bash
# Perf test: builds a repository with a very large linear history.
# Work folder is <repository>/tmp/aze-perf (gitignored scratch, never committed).
set -euo pipefail

SCRIPT_FOLDER="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT_FOLDER="$(cd "${SCRIPT_FOLDER}/../.." && pwd)"

EXIT_SUCCESS=0
EXIT_FAILURE=1

BUILD_TYPE="Release"
COMMITS=1000
MIN_SIZE_STR="2k"
MAX_SIZE_STR="20k"
KEEP=0
WORK_DIR_NAME="aze-perf"

usage() {
  cat <<'USAGE'
Usage: ./perf-history.sh [options]
  -n, --commits <count>    Number of linear commits to create (default: 1000)
  --min-size <size>        Minimum file size per commit (default: 2k)
  --max-size <size>        Maximum file size per commit (default: 20k)
                           Sizes accept a plain byte count or a k/K/m/M suffix (e.g. 2048, 2k, 1M)
  --debug, -d              Build type Debug
  --release, -r            Build type Release (default)
  -c                       Build type (Debug or Release)
  --keep                   Keep existing work folder content instead of starting fresh
  -h, --help               Show this help
Measures per-operation times (init, stage, commit, log, status, diff) and total time.
History is generated in <repository>/tmp/aze-perf.
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -n|--commits)
      if [[ -z "${2:-}" ]]; then
        echo "Option $1 requires an argument." >&2
        usage >&2
        exit "${EXIT_FAILURE}"
      fi
      COMMITS="$2"
      shift 2
      ;;
    --min-size)
      if [[ -z "${2:-}" ]]; then
        echo "Option --min-size requires an argument." >&2
        usage >&2
        exit "${EXIT_FAILURE}"
      fi
      MIN_SIZE_STR="$2"
      shift 2
      ;;
    --max-size)
      if [[ -z "${2:-}" ]]; then
        echo "Option --max-size requires an argument." >&2
        usage >&2
        exit "${EXIT_FAILURE}"
      fi
      MAX_SIZE_STR="$2"
      shift 2
      ;;
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
    --keep)
      KEEP=1
      shift
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

# Convert a size like 2048, 2k, 20k, 1M to bytes.
parse_size() {
  local sValue="$1"
  local sNumber="${sValue%[kKmM]}"
  local sSuffix="${sValue:${#sNumber}}"

  if [[ -z "${sNumber}" || ! "${sNumber}" =~ ^[0-9]+$ ]]; then
    echo "Invalid size: ${sValue}" >&2
    exit "${EXIT_FAILURE}"
  fi

  case "${sSuffix}" in
    "")
      printf '%s' "${sNumber}"
      ;;
    k|K)
      printf '%s' "$((sNumber * 1024))"
      ;;
    m|M)
      printf '%s' "$((sNumber * 1024 * 1024))"
      ;;
    *)
      echo "Invalid size suffix: ${sValue} (use k/K/m/M)" >&2
      exit "${EXIT_FAILURE}"
      ;;
  esac
}

if [[ ! "${COMMITS}" =~ ^[0-9]+$ ]] || [[ "${COMMITS}" -lt 1 ]]; then
  echo "Invalid commit count: ${COMMITS} (use an integer >= 1)" >&2
  exit "${EXIT_FAILURE}"
fi

MIN_SIZE="$(parse_size "${MIN_SIZE_STR}")"
MAX_SIZE="$(parse_size "${MAX_SIZE_STR}")"

if [[ "${MIN_SIZE}" -lt 1 ]]; then
  echo "Invalid --min-size: ${MIN_SIZE_STR} (must be >= 1 byte)" >&2
  exit "${EXIT_FAILURE}"
fi

if [[ "${MAX_SIZE}" -lt "${MIN_SIZE}" ]]; then
  echo "Invalid sizes: max (${MAX_SIZE_STR}) is smaller than min (${MIN_SIZE_STR})" >&2
  exit "${EXIT_FAILURE}"
fi

BUILD_FOLDER="${PROJECT_ROOT_FOLDER}/build-${BUILD_TYPE,,}"

if [[ "${BUILD_TYPE}" == "Debug" ]]; then
  BINARY="${BUILD_FOLDER}/Aze/bin/aze-dbg"
else
  BINARY="${BUILD_FOLDER}/Aze/bin/aze"
fi

if [ ! -f "${BINARY}" ]; then
  echo "[perf-history] Binary not found, running build first..."
  "${SCRIPT_FOLDER}/build.sh" -c "${BUILD_TYPE}"
fi

if [ ! -f "${BINARY}" ]; then
  echo "[perf-history] Binary not found at ${BINARY}" >&2
  exit "${EXIT_FAILURE}"
fi

export LD_LIBRARY_PATH="${BUILD_FOLDER}/AzeLib/bin"

WORK_DIR="${PROJECT_ROOT_FOLDER}/tmp/${WORK_DIR_NAME}"
LOG_FILE="${PROJECT_ROOT_FOLDER}/tmp/perf-history.log"
mkdir -p "${PROJECT_ROOT_FOLDER}/tmp"

now_ns() {
  date +%s%N
}

# Elapsed milliseconds between two nanosecond timestamps.
elapsed_ms() {
  local iStart="$1"
  local iEnd="$2"
  printf '%s' "$(((iEnd - iStart) / 1000000))"
}

SCRIPT_START_NS="$(now_ns)"

if [[ "${KEEP}" -eq 0 ]]; then
  rm -rf "${WORK_DIR}"
fi
mkdir -p "${WORK_DIR}/Files"

cd "${WORK_DIR}"

echo "[perf-history] Commits: ${COMMITS}, file size: ${MIN_SIZE_STR} (${MIN_SIZE} B) to ${MAX_SIZE_STR} (${MAX_SIZE} B)"
echo "[perf-history] Work folder: ${WORK_DIR}"
echo "[perf-history] Binary: ${BINARY} (${BUILD_TYPE})"

INIT_START_NS="$(now_ns)"
"${BINARY}" init --silent
INIT_END_NS="$(now_ns)"
INIT_MS="$(elapsed_ms "${INIT_START_NS}" "${INIT_END_NS}")"
echo "[perf-history] init: ${INIT_MS} ms"

PERF_FILE="Files/Perf-Data.txt"
SIZE_RANGE=$((MAX_SIZE - MIN_SIZE + 1))

STAGE_TOTAL_MS=0
STAGE_MIN_MS=""
STAGE_MAX_MS=0
COMMIT_TOTAL_MS=0
COMMIT_MIN_MS=""
COMMIT_MAX_MS=0
GEN_TOTAL_MS=0

PROGRESS_STEP=100
if [[ "${COMMITS}" -lt 100 ]]; then
  PROGRESS_STEP=10
fi

for ((iCommit = 1; iCommit <= COMMITS; iCommit++)); do
  # Variable file size in [MIN_SIZE, MAX_SIZE], full rewrite each commit so
  # every commit stores new content. Fill character cycles with the commit
  # index to defeat delta caching.
  iSize=$((MIN_SIZE + RANDOM % SIZE_RANGE))
  sHeader="PERF commit ${iCommit}/${COMMITS} size ${iSize}"
  iHeaderLen=$((${#sHeader} + 1))
  iFillSize=$((iSize - iHeaderLen))
  if [[ "${iFillSize}" -lt 0 ]]; then
    iFillSize=0
  fi
  # shellcheck disable=SC2184
  sFillChar="$(printf "\\$(printf '%03o' $((65 + iCommit % 26)))")"

  GEN_START_NS="$(now_ns)"
  printf '%s\n' "${sHeader}" > "${PERF_FILE}"
  if [[ "${iFillSize}" -gt 0 ]]; then
    head -c "${iFillSize}" /dev/zero | tr '\0' "${sFillChar}" >> "${PERF_FILE}"
  fi
  GEN_END_NS="$(now_ns)"
  GEN_TOTAL_MS=$((GEN_TOTAL_MS + $(elapsed_ms "${GEN_START_NS}" "${GEN_END_NS}")))

  STAGE_START_NS="$(now_ns)"
  "${BINARY}" stage --silent "${PERF_FILE}"
  STAGE_END_NS="$(now_ns)"
  iStageMs="$(elapsed_ms "${STAGE_START_NS}" "${STAGE_END_NS}")"
  STAGE_TOTAL_MS=$((STAGE_TOTAL_MS + iStageMs))
  if [[ -z "${STAGE_MIN_MS}" || "${iStageMs}" -lt "${STAGE_MIN_MS}" ]]; then
    STAGE_MIN_MS="${iStageMs}"
  fi
  if [[ "${iStageMs}" -gt "${STAGE_MAX_MS}" ]]; then
    STAGE_MAX_MS="${iStageMs}"
  fi

  COMMIT_START_NS="$(now_ns)"
  "${BINARY}" commit --silent -m "Perf commit ${iCommit}" -a "perf-history" >/dev/null
  COMMIT_END_NS="$(now_ns)"
  iCommitMs="$(elapsed_ms "${COMMIT_START_NS}" "${COMMIT_END_NS}")"
  COMMIT_TOTAL_MS=$((COMMIT_TOTAL_MS + iCommitMs))
  if [[ -z "${COMMIT_MIN_MS}" || "${iCommitMs}" -lt "${COMMIT_MIN_MS}" ]]; then
    COMMIT_MIN_MS="${iCommitMs}"
  fi
  if [[ "${iCommitMs}" -gt "${COMMIT_MAX_MS}" ]]; then
    COMMIT_MAX_MS="${iCommitMs}"
  fi

  if [[ $((iCommit % PROGRESS_STEP)) -eq 0 || "${iCommit}" -eq "${COMMITS}" ]]; then
    echo "[perf-history] ${iCommit}/${COMMITS} commits (last stage ${iStageMs} ms, last commit ${iCommitMs} ms)"
  fi
done

# Read operations on the full history.
LOG_START_NS="$(now_ns)"
"${BINARY}" log --silent > /dev/null
LOG_END_NS="$(now_ns)"
LOG_MS="$(elapsed_ms "${LOG_START_NS}" "${LOG_END_NS}")"

LOG_COUNT_START_NS="$(now_ns)"
"${BINARY}" log --silent -c 100 > /dev/null
LOG_COUNT_END_NS="$(now_ns)"
LOG_COUNT_MS="$(elapsed_ms "${LOG_COUNT_START_NS}" "${LOG_COUNT_END_NS}")"

STATUS_START_NS="$(now_ns)"
"${BINARY}" status --silent > /dev/null
STATUS_END_NS="$(now_ns)"
STATUS_MS="$(elapsed_ms "${STATUS_START_NS}" "${STATUS_END_NS}")"

DIFF_START_NS="$(now_ns)"
"${BINARY}" diff --silent "tip~1" "tip" > /dev/null
DIFF_END_NS="$(now_ns)"
DIFF_MS="$(elapsed_ms "${DIFF_START_NS}" "${DIFF_END_NS}")"

SCRIPT_END_NS="$(now_ns)"
TOTAL_MS="$(elapsed_ms "${SCRIPT_START_NS}" "${SCRIPT_END_NS}")"

STAGE_AVG_MS=$((STAGE_TOTAL_MS / COMMITS))
COMMIT_AVG_MS=$((COMMIT_TOTAL_MS / COMMITS))
GEN_AVG_MS=$((GEN_TOTAL_MS / COMMITS))

REPO_SIZE="$(du -sh .aze 2>/dev/null | cut -f1)"
WORK_SIZE="$(du -sh . 2>/dev/null | cut -f1)"

{
  echo ""
  echo "perf-history results (${COMMITS} commits, file size ${MIN_SIZE_STR}-${MAX_SIZE_STR})"
  echo "operation | count | total_ms | avg_ms | min_ms | max_ms"
  printf 'init | 1 | %s | %s | %s | %s\n' "${INIT_MS}" "${INIT_MS}" "${INIT_MS}" "${INIT_MS}"
  printf 'generate-file | %s | %s | %s | - | -\n' "${COMMITS}" "${GEN_TOTAL_MS}" "${GEN_AVG_MS}"
  printf 'stage | %s | %s | %s | %s | %s\n' "${COMMITS}" "${STAGE_TOTAL_MS}" "${STAGE_AVG_MS}" "${STAGE_MIN_MS}" "${STAGE_MAX_MS}"
  printf 'commit | %s | %s | %s | %s | %s\n' "${COMMITS}" "${COMMIT_TOTAL_MS}" "${COMMIT_AVG_MS}" "${COMMIT_MIN_MS}" "${COMMIT_MAX_MS}"
  printf 'log (full) | 1 | %s | %s | %s | %s\n' "${LOG_MS}" "${LOG_MS}" "${LOG_MS}" "${LOG_MS}"
  printf 'log -c 100 | 1 | %s | %s | %s | %s\n' "${LOG_COUNT_MS}" "${LOG_COUNT_MS}" "${LOG_COUNT_MS}" "${LOG_COUNT_MS}"
  printf 'status | 1 | %s | %s | %s | %s\n' "${STATUS_MS}" "${STATUS_MS}" "${STATUS_MS}" "${STATUS_MS}"
  printf 'diff tip~1 tip | 1 | %s | %s | %s | %s\n' "${DIFF_MS}" "${DIFF_MS}" "${DIFF_MS}" "${DIFF_MS}"
  printf 'total (script) | - | %s | - | - | -\n' "${TOTAL_MS}"
  echo ".aze size: ${REPO_SIZE}, work folder size: ${WORK_SIZE}"
} | tee "${LOG_FILE}"

echo "[perf-history] Done in ${TOTAL_MS} ms, details in ${LOG_FILE}"
