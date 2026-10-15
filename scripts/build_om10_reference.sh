#!/usr/bin/env bash
# Headless pinned OCCT reference adapter prerequisite; execute on the fleet.
set -euo pipefail
if [[ "$(uname -s)" != Linux ]]; then
  echo 'Reference CAD builds execute on the Linux fleet.' >&2
  exit 2
fi
if [[ $# != 3 ]]; then
  echo "usage: $0 OCCT_SOURCE BUILD_DIRECTORY INSTALL_PREFIX" >&2
  exit 2
fi
om10_source=$1
om10_build=$2
om10_prefix=$3
cmake -S "$om10_source" -B "$om10_build" \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_LIBRARY_TYPE=Shared \
  -DCMAKE_INSTALL_PREFIX="$om10_prefix" -DINSTALL_DIR="$om10_prefix" \
  -DBUILD_MODULE_FoundationClasses=OFF -DBUILD_MODULE_ModelingData=OFF \
  -DBUILD_MODULE_ModelingAlgorithms=OFF -DBUILD_MODULE_Visualization=OFF \
  -DBUILD_MODULE_ApplicationFramework=OFF -DBUILD_MODULE_DataExchange=OFF \
  -DBUILD_MODULE_Draw=OFF -DBUILD_ADDITIONAL_TOOLKITS='TKDESTEP TKMesh' \
  -DUSE_OPENGL=OFF -DUSE_GLES2=OFF -DUSE_XLIB=OFF -DUSE_TK=OFF \
  -DUSE_FREETYPE=OFF -DUSE_FREEIMAGE=OFF -DUSE_FFMPEG=OFF \
  -DUSE_OPENVR=OFF -DUSE_RAPIDJSON=OFF -DUSE_DRACO=OFF \
  -DUSE_TBB=OFF -DUSE_EIGEN=OFF -DBUILD_SAMPLES_QT=OFF \
  -DBUILD_DOC_Overview=OFF -DBUILD_GTEST=OFF
cmake --build "$om10_build" --parallel "${BREPGPU_REFERENCE_JOBS:-32}"
cmake --install "$om10_build"
