#ifndef BREPGPU_TOPOLOGY_TEST_HOOKS_H
#define BREPGPU_TOPOLOGY_TEST_HOOKS_H
#include <stdint.h>
/* Tooling-build-only controls; configure while calls are quiescent. Fail the
   nth occurrence (one based) of a point; secondary_cleanup additionally injects
   a reported cleanup failure after actual cleanup. Reset with point=0.
   Point IDs match topology_internal.hpp. Live resources count real CUDA buffers
   and stream ownership, never synthetic claims of allocation. */
extern "C" void bg5_test_fail_after(uint32_t point,uint64_t occurrence,
                                    uint32_t secondary_cleanup);
extern "C" uint64_t bg5_test_live_resources();
#endif
