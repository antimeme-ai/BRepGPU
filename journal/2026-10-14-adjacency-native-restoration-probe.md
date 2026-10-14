# Direct CUDA device restoration witness

Root recorded literal greens on CPU1/CPU24/both P100s and native checks on all
four backends. Root requested a direct previous-device restoration witness
before qualification. Expanded the native check executable only: CUDA-enabled
checks link cudart, select a prior device different from the requested device
when two are available, call the real adjacency operation, then query and assert
the previous device after success. The test restores its own original device.

Guarded CUDA tooling also queries the current device after real allocation/phase
cleanup branches, after synthetic cleanup/restore reports, and after healthy
retry. The fault campaign's primary/secondary failure calls now assert the same
restoration property. Synthetic restore reporting still occurs after actual
restoration and is labeled accordingly; it does not emulate a failed CUDA runtime
restore instruction. Ordinary product native sources and library behavior are
unchanged by this test addition. Root serializes fleet recompilation and runs;
this lane performed static inspection only.
