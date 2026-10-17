#ifndef BREPGPU_TEST_ADAPTIVE_CAP_WORK_HPP
#define BREPGPU_TEST_ADAPTIVE_CAP_WORK_HPP
#include "../native/mesher/adaptive_cap_work.hpp"

// Called only with the real producer's captured view/domain/construction.
// Its link RED precedes implementation; execution waits for that producer.
[[nodiscard]] bgmesh::adaptive::CapFacetBatch
check_captured_cap_work(bgmesh::JobContext &, bgmesh::adaptive::SurfaceBoundView,
                         bgmesh::adaptive::FaceConstructionResult,
                         bgmesh::adaptive::FaceDomainLease,
                         bgmesh::adaptive::FacetAllowances, bgmesh::ExactTableKey);
#endif
