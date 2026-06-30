#!/usr/bin/env bash
# Cross-build Switch portlib dependencies for nyan-doom:
#   libzip, libsndfile, FluidLite
#
# Prerequisites: devkitPro with devkitA64, Ninja, CMake
# Run from anywhere; clones/builds into <repo-root>/.build-deps/
# Installs to $DEVKITPRO/portlibs/switch (same as official portlibs).

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
WORK="$REPO_ROOT/.build-deps"

export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITPRO/tools/bin:$DEVKITA64/bin:$PATH
TC=$DEVKITPRO/cmake/Switch.cmake
PFX=$DEVKITPRO/portlibs/switch

mkdir -p "$WORK"
cd "$WORK"

# --- libzip ---
if [ ! -d libzip ]; then
  git clone --depth 1 --branch v1.10.1 https://github.com/nih-at/libzip.git
fi
cmake -S libzip -B libzip-build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TC" \
  -DCMAKE_INSTALL_PREFIX="$PFX" \
  -DBUILD_SHARED_LIBS=OFF \
  -DENABLE_BZIP2=ON -DENABLE_LZMA=OFF -DENABLE_ZSTD=OFF \
  -DENABLE_OPENSSL=OFF -DENABLE_GNUTLS=OFF -DENABLE_MBEDTLS=OFF \
  -DLIBZIP_DO_INSTALL=ON -DBUILD_TOOLS=OFF -DBUILD_REGRESS=OFF \
  -DBUILD_EXAMPLES=OFF -DBUILD_DOC=OFF
cmake --build libzip-build
cmake --install libzip-build

# --- libsndfile ---
if [ ! -d libsndfile ]; then
  git clone --depth 1 --branch 1.2.2 https://github.com/libsndfile/libsndfile.git
fi
cmake -S libsndfile -B libsndfile-build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TC" \
  -DCMAKE_INSTALL_PREFIX="$PFX" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DBUILD_SHARED_LIBS=OFF \
  -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF \
  -DENABLE_EXTERNAL_LIBS=ON -DENABLE_MPEG=OFF
cmake --build libsndfile-build
cmake --install libsndfile-build

# --- FluidLite (drop-in FluidSynth replacement, no glib dependency) ---
if [ ! -d fluidlite ]; then
  git clone --depth 1 https://github.com/divideconcept/FluidLite.git fluidlite
fi
cmake -S fluidlite -B fluidlite-build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$TC" \
  -DCMAKE_INSTALL_PREFIX="$PFX" \
  -DBUILD_SHARED_LIBS=OFF \
  -DFLUIDLITE_BUILD_STATIC=ON \
  -DFLUIDLITE_BUILD_TESTS=OFF \
  -DENABLE_SF3=TRUE \
  -DSTB_VORBIS=TRUE
cmake --build fluidlite-build
cmake --install fluidlite-build

echo "DONE: portlibs installed to $PFX"
