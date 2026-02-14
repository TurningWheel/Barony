#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INSTALL_PREFIX="${ROOT_DIR}/deps/theoraplayer"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
REPO_URL="https://github.com/AprilAndFriends/theoraplayer.git"
REPO_REF=""

print_usage() {
  cat <<'USAGE'
Usage: scripts/build_theoraplayer.sh [options]

Builds TheoraPlayer from source and installs files expected by
cmake/Modules/FindTheoraPlayer.cmake.

Options:
  --prefix <path>   Install prefix (default: deps/theoraplayer)
  --jobs <count>    Parallel build jobs (default: number of CPUs)
  --repo <url>      Git repository URL
  --ref <ref>       Git branch/tag/commit to checkout
  -h, --help        Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --prefix)
      INSTALL_PREFIX="$2"
      shift 2
      ;;
    --jobs)
      JOBS="$2"
      shift 2
      ;;
    --repo)
      REPO_URL="$2"
      shift 2
      ;;
    --ref)
      REPO_REF="$2"
      shift 2
      ;;
    -h|--help)
      print_usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      print_usage
      exit 1
      ;;
  esac
done

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

require_cmd git
require_cmd cmake
require_cmd pkg-config

for mod in ogg vorbis vorbisfile vorbisenc theora theoradec theoraenc; do
  if ! pkg-config --exists "$mod"; then
    echo "Missing pkg-config module: $mod" >&2
    exit 1
  fi
done

WRAPPER_DIR="${ROOT_DIR}/scripts/theoraplayer"
if [[ ! -f "${WRAPPER_DIR}/CMakeLists.txt" ]]; then
  echo "Missing shared TheoraPlayer wrapper: ${WRAPPER_DIR}/CMakeLists.txt" >&2
  exit 1
fi

TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/barony-theoraplayer-XXXXXX")"
SRC_DIR="${TMP_ROOT}/theoraplayer-src"
BUILD_DIR="${TMP_ROOT}/theoraplayer-build"

cleanup() {
  rm -rf "${TMP_ROOT}"
}
trap cleanup EXIT

echo "[theoraplayer] cloning ${REPO_URL}"
git clone --depth 1 "${REPO_URL}" "${SRC_DIR}"
if [[ -n "${REPO_REF}" ]]; then
  git -C "${SRC_DIR}" fetch --depth 1 origin "${REPO_REF}"
  git -C "${SRC_DIR}" checkout "${REPO_REF}"
fi

cmake -S "${WRAPPER_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEORAPLAYER_ROOT="${SRC_DIR}" \
  -DTHEORAPLAYER_DEP_BACKEND=pkgconfig

cmake --build "${BUILD_DIR}" -j"${JOBS}" --target theoraplayer

mkdir -p "${INSTALL_PREFIX}/include/theoraplayer" "${INSTALL_PREFIX}/lib"
cp -f "${SRC_DIR}/theoraplayer/include/theoraplayer/"*.h "${INSTALL_PREFIX}/include/theoraplayer/"

BUILT_LIB="$(find "${BUILD_DIR}" -maxdepth 1 -type f \( -name 'libtheoraplayer.so' -o -name 'libtheoraplayer.dylib' \) | head -n1 || true)"
if [[ -z "${BUILT_LIB}" ]]; then
  echo "Failed to find built TheoraPlayer shared library in ${BUILD_DIR}" >&2
  exit 1
fi
cp -f "${BUILT_LIB}" "${INSTALL_PREFIX}/lib/"

echo "[theoraplayer] installed headers and library under ${INSTALL_PREFIX}"
