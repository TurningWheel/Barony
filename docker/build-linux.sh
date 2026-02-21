#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-linux-all"

: "${FMOD_DIR:=${ROOT_DIR}/deps/fmod/linux}"
: "${STEAMWORKS_ROOT:=${ROOT_DIR}/deps/steamworks}"
: "${NFD_DIR:=/opt/nfd}"
: "${THEORAPLAYER_DIR:=/opt/theoraplayer}"
: "${OPUS_DIR:=/usr}"
: "${BARONY_BUILD_JOBS:=4}"

require_file() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    echo "Missing required file: $file" >&2
    exit 1
  fi
}

require_file "${FMOD_DIR}/api/core/inc/fmod.hpp"
require_file "${STEAMWORKS_ROOT}/sdk/public/steam/steam_api.h"
require_file "${STEAMWORKS_ROOT}/sdk/redistributable_bin/linux64/libsteam_api.so"
require_file "${NFD_DIR}/include/nfd.h"
require_file "${THEORAPLAYER_DIR}/include/theoraplayer/theoraplayer.h"

# FindFMOD.cmake searches for libfmod.so; FMOD Linux SDK often ships only versioned SONAME files.
FMOD_LIB_DIR="${FMOD_DIR}/api/core/lib/x86_64"
if [[ ! -f "${FMOD_LIB_DIR}/libfmod.so" ]]; then
  FMOD_VERSIONED_LIB="$(find "${FMOD_LIB_DIR}" -maxdepth 1 -type f -name 'libfmod.so.*' | sort | head -n1 || true)"
  if [[ -n "${FMOD_VERSIONED_LIB}" ]]; then
    ln -sfn "$(basename "${FMOD_VERSIONED_LIB}")" "${FMOD_LIB_DIR}/libfmod.so"
  fi
fi
require_file "${FMOD_LIB_DIR}/libfmod.so"

export FMOD_DIR
export STEAMWORKS_ROOT
export NFD_DIR
export THEORAPLAYER_DIR
export OPUS_DIR

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.10 \
  -DCMAKE_BUILD_TYPE=Release \
  -DFMOD_ENABLED=ON \
  -DOPENAL_ENABLED=OFF \
  -DSTEAMWORKS=ON \
  -DEOS=OFF \
  -DPLAYFAB=OFF \
  -DTHEORAPLAYER=ON \
  -DCURL=ON \
  -DOPUS=ON

cmake --build "${BUILD_DIR}" -j"${BARONY_BUILD_JOBS}"

echo "Linux build complete: ${BUILD_DIR}/barony"
