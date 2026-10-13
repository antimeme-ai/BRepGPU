#ifndef BREPGPU_RUNTIME_TEST_HOOKS_H
#define BREPGPU_RUNTIME_TEST_HOOKS_H
#include "brepgpu.h"
// Compiled only with BREPGPU_ENABLE_TEST_HOOKS. Not part of the public ABI.
// Hooks are configured while callers are quiescent; data outlives all calls.
// Admission runs after shared promotion but before context locking. Publication
// runs after completed backend work, before registry insertion. A hook may throw
// to model allocation failure; it must never reenter the locked same context.
enum { BG_TEST_ADMISSION=1, BG_TEST_PUBLICATION=2 };
using bg_test_hook_fn = void (*)(uint32_t,bg_handle,void*);
extern "C" void bg_test_set_hook(bg_test_hook_fn,void*);
#endif
