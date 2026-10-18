#!/usr/bin/env bash
# Build the delivered-clock probe that scripts/measurement_lane.py runs for
# every context row. Execute on the fleet; the result lands where the lane
# module looks by default (build/measurement/brepgpu-delivered-clock).
set -euo pipefail
if [[ "$(uname -s)" != Linux ]]; then
  echo 'The measurement probe is built and run on the Linux fleet.' >&2
  exit 2
fi
root=$(cd "$(dirname "$0")/.." && pwd)
cmake -S "$root/native/measurement" -B "$root/build/measurement" -DCMAKE_BUILD_TYPE=Release
cmake --build "$root/build/measurement"
