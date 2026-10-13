# First iteration runtime study, 2026-10-13

Read the project instructions, primary CPython/ctypes/buffer documentation, CUDA compiler/runtime/support documentation and pybind11 reference documentation. Inspected the reference kernel's scalar Python bindings and borrowed-geometry preflight. Performed the authorized read-only execution-host inventory using narrowly scoped compiler, GPU, package, loader and interpreter queries; no installations or host files changed.

Observed two P100 PCIe 16 GB devices, driver 580.178.04 and compute capability 6.0, with GCC 13.3, CMake 3.28.3 and Python 3.12.3. No accessible CUDA compiler/toolkit was found in the bounded search. Reported that blocker early to the integrated design owner.

Wrote the public-safe runtime report proposing an owned C ABI, standard-library ctypes, optional CUDA backend loading, checked process-local handles, explicit native layouts, synchronous completion and resident device data with explicit download. Distinguished borrowed buffer retention from prevention of concurrent mutation and fatal CUDA process errors from ordinary per-query numerical failures. No product code, adopted dependency, Git mutation, external message or GPU performance claim resulted from this lane.
