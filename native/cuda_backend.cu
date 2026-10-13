#include <cuda_runtime.h>
#include "evaluator.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <vector>

namespace {

struct DevicePatch { uint32_t p, q; uint64_t offset; };
struct Context { int device; bool profile; cudaStream_t stream = nullptr; };
struct Patches { Context* context; DevicePatch* records = nullptr; double* controls = nullptr; uint64_t count; };
struct Queries { Context* context; Patches* patches; uint32_t* slots = nullptr; double* uv = nullptr; uint64_t count; };
struct Result { Context* context; double* jets = nullptr; uint32_t* statuses = nullptr; uint64_t count; uint32_t order; };

void append_bounded(char* target, size_t capacity, const char* text) noexcept {
    size_t used = 0;
    while (used < capacity && target[used] != '\0') ++used;
    if (used == capacity) return;
    while (used+1 < capacity && *text != '\0') target[used++] = *text++;
    target[used] = '\0';
}

bool fatal_cuda(cudaError_t code) noexcept {
    switch (code) {
    case cudaErrorIllegalAddress: case cudaErrorLaunchTimeout: case cudaErrorAssert:
    case cudaErrorHardwareStackError: case cudaErrorIllegalInstruction:
    case cudaErrorMisalignedAddress: case cudaErrorInvalidAddressSpace:
    case cudaErrorInvalidPc: case cudaErrorLaunchFailure: case cudaErrorECCUncorrectable:
    case cudaErrorUnknown:
        return true;
    default: return false;
    }
}

struct Operation {
    bg_status status = BG_OK;
    bg_diagnostic diagnostic{};
    Operation() noexcept {
        diagnostic.abi_version = BG_ABI_VERSION;
        diagnostic.struct_size = sizeof(diagnostic);
        diagnostic.query_index = -1;
    }
    bool ok() const noexcept { return status == BG_OK; }
    void error(bg_status code, uint32_t stage, const char* message,
               cudaError_t native = cudaSuccess) noexcept {
        if (fatal_cuda(native)) diagnostic.flags |= BG_DIAG_FATAL;
        if (ok()) {
            status = code;
            diagnostic.code = code;
            diagnostic.stage = stage;
            diagnostic.backend_code = static_cast<int32_t>(native);
            append_bounded(diagnostic.message, sizeof(diagnostic.message), message);
        } else {
            append_bounded(diagnostic.message, sizeof(diagnostic.message), "; also: ");
            append_bounded(diagnostic.message, sizeof(diagnostic.message), message);
        }
    }
    void cuda(cudaError_t code, uint32_t stage, const char* action) noexcept {
        if (code == cudaSuccess) return;
        char text[256]{};
        append_bounded(text, sizeof(text), action);
        append_bounded(text, sizeof(text), ": ");
        append_bounded(text, sizeof(text), cudaGetErrorString(code));
        const bg_status mapped = code == cudaErrorMemoryAllocation ? BG_OUT_OF_MEMORY :
            ((code == cudaErrorNoDevice || code == cudaErrorInsufficientDriver ||
              code == cudaErrorInitializationError) ? BG_BACKEND_UNAVAILABLE : BG_DEVICE_FAILURE);
        error(mapped, stage, text, code);
    }
};

/* Restoration is explicit and recorded before publishing any token. */
struct DeviceScope {
    Operation& operation;
    uint32_t stage;
    int previous = -1;
    bool selected = false;
    DeviceScope(int device, uint32_t where, Operation& op) noexcept : operation(op), stage(where) {
        const cudaError_t get = cudaGetDevice(&previous);
        operation.cuda(get, stage, "read current device");
        if (get == cudaSuccess) {
            const cudaError_t set = cudaSetDevice(device);
            selected = set == cudaSuccess;
            operation.cuda(set, stage, "select context device");
        }
    }
    ~DeviceScope() noexcept {
        if (previous >= 0) operation.cuda(cudaSetDevice(previous), stage, "restore prior device");
    }
};

template<class Function>
bg_status guarded(uint32_t stage, bg_diagnostic* diagnostic, Function function) noexcept {
    Operation operation;
    try { function(operation); }
    catch (const std::bad_alloc&) { operation.error(BG_OUT_OF_MEMORY, stage, "host allocation failed"); }
    catch (...) { operation.error(BG_INTERNAL_ERROR, stage, "unexpected backend exception"); }
    if (diagnostic) *diagnostic = operation.diagnostic;
    return operation.status;
}

bool bytes_for(uint64_t count, size_t size, size_t& bytes, Operation& op, uint32_t stage) noexcept {
    if (count > std::numeric_limits<size_t>::max()/size) {
        op.error(BG_INVALID_ARGUMENT, stage, "backend buffer size overflow");
        return false;
    }
    bytes = static_cast<size_t>(count)*size;
    return true;
}

template<class T>
void allocate(T*& pointer, uint64_t count, Context* context, Operation& op, uint32_t stage) noexcept {
    size_t bytes = 0;
    if (!op.ok() || !bytes_for(count, sizeof(T), bytes, op, stage) || bytes == 0) return;
    op.cuda(cudaMalloc(reinterpret_cast<void**>(&pointer), bytes), stage, "allocate device buffer");
    (void)context;
}

template<class T>
void upload(T* target, const T* source, uint64_t count, Context* context,
            Operation& op) noexcept {
    size_t bytes = 0;
    if (!op.ok() || !bytes_for(count, sizeof(T), bytes, op, BG_STAGE_UPLOAD) || bytes == 0) return;
    op.cuda(cudaMemcpyAsync(target, source, bytes, cudaMemcpyHostToDevice, context->stream),
            BG_STAGE_UPLOAD, "upload device buffer");
}

void finish(Context* context, Operation& op, uint32_t stage) noexcept {
    if (context->stream) op.cuda(cudaStreamSynchronize(context->stream), stage, "complete context stream");
}

void destroy(Context* context, Operation& op) noexcept {
    {
        DeviceScope device(context->device, BG_STAGE_CLOSE, op);
        if (device.selected) {
            finish(context, op, BG_STAGE_CLOSE);
            if (context->stream) op.cuda(cudaStreamDestroy(context->stream), BG_STAGE_CLOSE, "destroy stream");
        }
    }
    delete context;
}
void destroy(Patches* patches, Operation& op) noexcept {
    {
        DeviceScope device(patches->context->device, BG_STAGE_CLOSE, op);
        if (device.selected) {
            finish(patches->context, op, BG_STAGE_CLOSE);
            if (patches->records) op.cuda(cudaFree(patches->records), BG_STAGE_CLOSE, "free patch records");
            if (patches->controls) op.cuda(cudaFree(patches->controls), BG_STAGE_CLOSE, "free controls");
        }
    }
    delete patches;
}
void destroy(Queries* queries, Operation& op) noexcept {
    {
        DeviceScope device(queries->context->device, BG_STAGE_CLOSE, op);
        if (device.selected) {
            finish(queries->context, op, BG_STAGE_CLOSE);
            if (queries->slots) op.cuda(cudaFree(queries->slots), BG_STAGE_CLOSE, "free query slots");
            if (queries->uv) op.cuda(cudaFree(queries->uv), BG_STAGE_CLOSE, "free parameters");
        }
    }
    delete queries;
}
void destroy(Result* result, Operation& op) noexcept {
    {
        DeviceScope device(result->context->device, BG_STAGE_CLOSE, op);
        if (device.selected) {
            finish(result->context, op, BG_STAGE_CLOSE);
            if (result->jets) op.cuda(cudaFree(result->jets), BG_STAGE_CLOSE, "free result jets");
            if (result->statuses) op.cuda(cudaFree(result->statuses), BG_STAGE_CLOSE, "free result statuses");
        }
    }
    delete result;
}

template<class T> struct Pending {
    T* value;
    Operation& operation;
    Pending(T* object, Operation& op) noexcept : value(object), operation(op) {}
    ~Pending() noexcept { if (value) destroy(value, operation); }
    T* release() noexcept { T* out = value; value = nullptr; return out; }
};

__device__ __forceinline__ double interpolate(double a, double b, double t) {
    return __fma_rn(t, b, __fma_rn(-t, a, a));
}

__device__ __forceinline__ double curve_value(const double* c, unsigned degree, double t) {
    switch (degree) {
    case 0: return c[0];
    case 1: return interpolate(c[0], c[1], t);
    case 2: return interpolate(interpolate(c[0], c[1], t), interpolate(c[1], c[2], t), t);
    default: {
        const double a = interpolate(c[0], c[1], t);
        const double b = interpolate(c[1], c[2], t);
        const double d = interpolate(c[2], c[3], t);
        return interpolate(interpolate(a, b, t), interpolate(b, d, t), t);
    }
    }
}

template<unsigned Order>
__device__ __forceinline__ void channel_jet(const double* net, unsigned p, unsigned q,
                                           unsigned axis, double s, double t, double* out) {
    double row[4], row_s[4], row_ss[4], control[4], first[4], second[4];
    for (unsigned j = 0; j <= q; ++j) {
        for (unsigned i = 0; i <= p; ++i) control[i] = net[(i*(q+1)+j)*4+axis];
        row[j] = curve_value(control, p, s);
        if constexpr (Order >= 1) {
            for (unsigned i = 0; i < p; ++i) first[i] = p*(control[i+1]-control[i]);
            row_s[j] = p > 0 ? curve_value(first, p-1, s) : 0.0;
        }
        if constexpr (Order == 2) {
            for (unsigned i = 0; i+2 <= p; ++i)
                second[i] = p*(p-1)*(control[i+2]-2.0*control[i+1]+control[i]);
            row_ss[j] = p > 1 ? curve_value(second, p-2, s) : 0.0;
        }
    }
    out[0] = curve_value(row, q, t);
    if constexpr (Order >= 1) {
        out[1] = curve_value(row_s, q, t);
        for (unsigned j = 0; j < q; ++j) first[j] = q*(row[j+1]-row[j]);
        out[2] = q > 0 ? curve_value(first, q-1, t) : 0.0;
    }
    if constexpr (Order == 2) {
        out[3] = curve_value(row_ss, q, t);
        for (unsigned j = 0; j < q; ++j) first[j] = q*(row_s[j+1]-row_s[j]);
        out[4] = q > 0 ? curve_value(first, q-1, t) : 0.0;
        for (unsigned j = 0; j+2 <= q; ++j) second[j] = q*(q-1)*(row[j+2]-2.0*row[j+1]+row[j]);
        out[5] = q > 1 ? curve_value(second, q-2, t) : 0.0;
    }
}

template<unsigned Order>
__device__ __forceinline__ void failed_query(uint64_t request, uint64_t count,
                                             uint32_t status, double* jets, uint32_t* statuses) {
    const double invalid = __longlong_as_double(0x7ff8000000000000LL);
    for (unsigned k = 0; k < brepgpu::jet_terms(Order)*3; ++k) jets[k*count+request] = invalid;
    statuses[request] = status;
}

template<unsigned Order>
__global__ void evaluate_kernel(const DevicePatch* patches, uint64_t patch_count,
                               const double* controls, const uint32_t* slots,
                               const double* uv, uint64_t count, double* jets,
                               uint32_t* statuses) {
    const uint64_t stride = static_cast<uint64_t>(gridDim.x)*blockDim.x;
    for (uint64_t request = static_cast<uint64_t>(blockIdx.x)*blockDim.x+threadIdx.x;
         request < count; request += stride) {
        const uint32_t slot = slots[request];
        if (slot >= patch_count) {
            failed_query<Order>(request, count, BG_QUERY_BAD_PATCH, jets, statuses);
            continue;
        }
        const double s = uv[request*2], t = uv[request*2+1];
        if (!isfinite(s) || !isfinite(t) || s < 0.0 || s > 1.0 || t < 0.0 || t > 1.0) {
            failed_query<Order>(request, count, BG_QUERY_BAD_PARAMETER, jets, statuses);
            continue;
        }
        const DevicePatch patch = patches[slot];
        if (patch.p > 3 || patch.q > 3) {
            failed_query<Order>(request, count, BG_QUERY_UNSUPPORTED, jets, statuses);
            continue;
        }
        const double* net = controls+patch.offset*4;
        double weights[6]{};
        channel_jet<Order>(net, patch.p, patch.q, 3, s, t, weights);
        bool success = true;
        for (unsigned axis = 0; axis < 3; ++axis) {
            double numerator[6]{}, result[6]{};
            channel_jet<Order>(net, patch.p, patch.q, axis, s, t, numerator);
            if (!brepgpu::recover_channel(numerator, weights, Order, result)) {
                success = false;
                break;
            }
            for (unsigned k = 0; k < brepgpu::jet_terms(Order); ++k)
                jets[(k*3+axis)*count+request] = result[k];
        }
        if (success) statuses[request] = BG_QUERY_OK;
        else failed_query<Order>(request, count, BG_QUERY_ARITHMETIC, jets, statuses);
    }
}

bg_status context_create(const bg_context_desc* desc, void** out, bg_diagnostic* diag) noexcept {
    if (out) *out = nullptr;
    return guarded(BG_STAGE_CONTEXT, diag, [&](Operation& op) {
        if (!desc || !out) { op.error(BG_INVALID_ARGUMENT, BG_STAGE_CONTEXT, "missing context input/output"); return; }
        Pending<Context> pending(new Context{desc->device, (desc->flags & BG_CONTEXT_PROFILE) != 0}, op);
        {
            DeviceScope device(desc->device, BG_STAGE_CONTEXT, op);
            if (device.selected && op.ok()) {
                cudaDeviceProp properties{};
                op.cuda(cudaGetDeviceProperties(&properties, desc->device), BG_STAGE_CONTEXT, "read device capabilities");
                if (op.ok() && (properties.major != 6 || properties.minor != 0))
                    op.error(BG_BACKEND_UNAVAILABLE, BG_STAGE_CONTEXT, "this backend supports sm_60 devices");
                if (op.ok()) op.cuda(cudaStreamCreateWithFlags(&pending.value->stream, cudaStreamNonBlocking),
                                     BG_STAGE_CONTEXT, "create context stream");
            }
        }
        if (op.ok()) *out = pending.release();
    });
}
bg_status context_close(void* token, bg_diagnostic* diag) noexcept {
    return guarded(BG_STAGE_CLOSE, diag, [&](Operation& op) {
        if (!token) op.error(BG_INVALID_ARGUMENT, BG_STAGE_CLOSE, "missing context token");
        else destroy(static_cast<Context*>(token), op);
    });
}

bg_status patch_create(void* ctx, const bg_patch_desc* records, uint64_t count,
                       const double* controls, uint64_t doubles, void** out, bg_diagnostic* diag) noexcept {
    if (out) *out = nullptr;
    return guarded(BG_STAGE_UPLOAD, diag, [&](Operation& op) {
        if (!ctx || !out || (count && !records) || (doubles && !controls)) {
            op.error(BG_INVALID_ARGUMENT, BG_STAGE_UPLOAD, "missing patch input/output"); return;
        }
        auto* context = static_cast<Context*>(ctx);
        size_t record_bytes = 0;
        if (!bytes_for(count, sizeof(DevicePatch), record_bytes, op, BG_STAGE_UPLOAD)) return;
        std::vector<DevicePatch> compact;
        compact.reserve(static_cast<size_t>(count));
        for (uint64_t i = 0; i < count; ++i) {
            const auto& patch = records[i];
            if (patch.degree_u > 3 || patch.degree_v > 3 || doubles%4 != 0 ||
                patch.control_count != (patch.degree_u+1)*(patch.degree_v+1) ||
                patch.control_offset > doubles/4 || patch.control_count > doubles/4-patch.control_offset) {
                op.error(BG_INVALID_ARGUMENT, BG_STAGE_UPLOAD, "invalid private patch extent"); return;
            }
            compact.push_back(DevicePatch{patch.degree_u, patch.degree_v, patch.control_offset});
        }
        Pending<Patches> pending(new Patches{context, nullptr, nullptr, count}, op);
        {
            DeviceScope device(context->device, BG_STAGE_UPLOAD, op);
            if (device.selected && op.ok()) {
                allocate(pending.value->records, count, context, op, BG_STAGE_UPLOAD);
                allocate(pending.value->controls, doubles, context, op, BG_STAGE_UPLOAD);
                upload(pending.value->records, compact.data(), count, context, op);
                upload(pending.value->controls, controls, doubles, context, op);
                finish(context, op, BG_STAGE_UPLOAD);
            }
        }
        if (op.ok()) *out = pending.release();
    });
}
bg_status patch_close(void* ctx, void* token, bg_diagnostic* diag) noexcept {
    return guarded(BG_STAGE_CLOSE, diag, [&](Operation& op) {
        if (!ctx || !token) op.error(BG_INVALID_ARGUMENT, BG_STAGE_CLOSE, "missing patch close input");
        else destroy(static_cast<Patches*>(token), op);
    });
}

bg_status query_create(void* ctx, void* patch_token, const uint32_t* slots, uint64_t count,
                       const double* uv, void** out, bg_diagnostic* diag) noexcept {
    if (out) *out = nullptr;
    return guarded(BG_STAGE_UPLOAD, diag, [&](Operation& op) {
        if (!ctx || !patch_token || !out || (count && (!slots || !uv))) {
            op.error(BG_INVALID_ARGUMENT, BG_STAGE_UPLOAD, "missing query input/output"); return;
        }
        if (count > std::numeric_limits<uint64_t>::max()/2) {
            op.error(BG_INVALID_ARGUMENT, BG_STAGE_UPLOAD, "parameter count overflow"); return;
        }
        auto* context = static_cast<Context*>(ctx);
        auto* patches = static_cast<Patches*>(patch_token);
        if (patches->context != context) { op.error(BG_INVALID_ARGUMENT, BG_STAGE_UPLOAD, "foreign private patch context"); return; }
        Pending<Queries> pending(new Queries{context, patches, nullptr, nullptr, count}, op);
        {
            DeviceScope device(context->device, BG_STAGE_UPLOAD, op);
            if (device.selected && op.ok()) {
                allocate(pending.value->slots, count, context, op, BG_STAGE_UPLOAD);
                allocate(pending.value->uv, count*2, context, op, BG_STAGE_UPLOAD);
                upload(pending.value->slots, slots, count, context, op);
                upload(pending.value->uv, uv, count*2, context, op);
                finish(context, op, BG_STAGE_UPLOAD);
            }
        }
        if (op.ok()) *out = pending.release();
    });
}
bg_status query_close(void* ctx, void* token, bg_diagnostic* diag) noexcept {
    return guarded(BG_STAGE_CLOSE, diag, [&](Operation& op) {
        if (!ctx || !token) op.error(BG_INVALID_ARGUMENT, BG_STAGE_CLOSE, "missing query close input");
        else destroy(static_cast<Queries*>(token), op);
    });
}

bg_status evaluate(void* ctx, void* query_token, uint32_t order, void** out,
                   bg_backend_profile* profile, bg_diagnostic* diag) noexcept {
    if (out) *out = nullptr;
    if (profile) *profile = bg_backend_profile{0, 0, std::numeric_limits<double>::quiet_NaN()};
    return guarded(BG_STAGE_EVALUATE, diag, [&](Operation& op) {
        if (!ctx || !query_token || !out || !profile || order > 2) {
            op.error(BG_INVALID_ARGUMENT, BG_STAGE_EVALUATE, "invalid evaluation input/order"); return;
        }
        auto* context = static_cast<Context*>(ctx);
        auto* queries = static_cast<Queries*>(query_token);
        if (queries->context != context) { op.error(BG_INVALID_ARGUMENT, BG_STAGE_EVALUATE, "foreign private query context"); return; }
        const uint64_t fields = brepgpu::jet_terms(order)*3;
        if (queries->count > std::numeric_limits<uint64_t>::max()/fields) {
            op.error(BG_INVALID_ARGUMENT, BG_STAGE_EVALUATE, "result count overflow"); return;
        }
        Pending<Result> pending(new Result{context, nullptr, nullptr, queries->count, order}, op);
        float milliseconds = 0;
        bool measured = false;
        {
            DeviceScope device(context->device, BG_STAGE_EVALUATE, op);
            if (device.selected && op.ok()) {
                allocate(pending.value->jets, queries->count*fields, context, op, BG_STAGE_EVALUATE);
                allocate(pending.value->statuses, queries->count, context, op, BG_STAGE_EVALUATE);
                cudaEvent_t start = nullptr, stop = nullptr;
                if (op.ok() && context->profile && queries->count) {
                    op.cuda(cudaEventCreate(&start), BG_STAGE_EVALUATE, "create profiling start event");
                    if (op.ok()) op.cuda(cudaEventCreate(&stop), BG_STAGE_EVALUATE, "create profiling stop event");
                    if (op.ok()) op.cuda(cudaEventRecord(start, context->stream), BG_STAGE_EVALUATE, "record profiling start");
                }
                if (op.ok() && queries->count) {
                    constexpr unsigned threads = 128;
                    const uint64_t needed = queries->count/threads + (queries->count%threads != 0);
                    const unsigned blocks = static_cast<unsigned>(std::min<uint64_t>(needed, 4096));
                    Patches* patches = queries->patches;
                    if (order == 0) evaluate_kernel<0><<<blocks,threads,0,context->stream>>>(patches->records,
                        patches->count, patches->controls, queries->slots, queries->uv, queries->count,
                        pending.value->jets, pending.value->statuses);
                    else if (order == 1) evaluate_kernel<1><<<blocks,threads,0,context->stream>>>(patches->records,
                        patches->count, patches->controls, queries->slots, queries->uv, queries->count,
                        pending.value->jets, pending.value->statuses);
                    else evaluate_kernel<2><<<blocks,threads,0,context->stream>>>(patches->records,
                        patches->count, patches->controls, queries->slots, queries->uv, queries->count,
                        pending.value->jets, pending.value->statuses);
                    op.cuda(cudaGetLastError(), BG_STAGE_EVALUATE, "launch rational patch kernel");
                    if (op.ok() && stop) op.cuda(cudaEventRecord(stop, context->stream), BG_STAGE_EVALUATE, "record profiling stop");
                }
                finish(context, op, BG_STAGE_EVALUATE);
                if (op.ok() && start && stop) {
                    op.cuda(cudaEventElapsedTime(&milliseconds, start, stop), BG_STAGE_EVALUATE, "read profiling duration");
                    measured = op.ok();
                }
                if (start) op.cuda(cudaEventDestroy(start), BG_STAGE_CLOSE, "destroy profiling start");
                if (stop) op.cuda(cudaEventDestroy(stop), BG_STAGE_CLOSE, "destroy profiling stop");
            }
        }
        if (op.ok()) {
            if (measured) *profile = bg_backend_profile{BG_RESULT_PROFILED, 0, milliseconds};
            *out = pending.release();
        }
    });
}
bg_status result_close(void* ctx, void* token, bg_diagnostic* diag) noexcept {
    return guarded(BG_STAGE_CLOSE, diag, [&](Operation& op) {
        if (!ctx || !token) op.error(BG_INVALID_ARGUMENT, BG_STAGE_CLOSE, "missing result close input");
        else destroy(static_cast<Result*>(token), op);
    });
}

bg_status download(void* ctx, void* result_token, double* jets, uint64_t doubles,
                   uint32_t* statuses, uint64_t count, bg_diagnostic* diag) noexcept {
    return guarded(BG_STAGE_DOWNLOAD, diag, [&](Operation& op) {
        if (!ctx || !result_token) { op.error(BG_INVALID_ARGUMENT, BG_STAGE_DOWNLOAD, "missing download input"); return; }
        auto* context = static_cast<Context*>(ctx);
        auto* result = static_cast<Result*>(result_token);
        if (result->context != context || count != result->count ||
            doubles != result->count*brepgpu::jet_terms(result->order)*3 ||
            (count && (!jets || !statuses))) {
            op.error(BG_INVALID_ARGUMENT, BG_STAGE_DOWNLOAD, "invalid download capacities/context"); return;
        }
        size_t jet_bytes = 0, status_bytes = 0;
        if (!bytes_for(doubles, sizeof(double), jet_bytes, op, BG_STAGE_DOWNLOAD) ||
            !bytes_for(count, sizeof(uint32_t), status_bytes, op, BG_STAGE_DOWNLOAD)) return;
        {
            DeviceScope device(context->device, BG_STAGE_DOWNLOAD, op);
            if (device.selected && op.ok()) {
                if (jet_bytes) op.cuda(cudaMemcpyAsync(jets, result->jets, jet_bytes, cudaMemcpyDeviceToHost,
                                                      context->stream), BG_STAGE_DOWNLOAD, "download jets");
                if (op.ok() && status_bytes) op.cuda(cudaMemcpyAsync(statuses, result->statuses, status_bytes,
                    cudaMemcpyDeviceToHost, context->stream), BG_STAGE_DOWNLOAD, "download statuses");
                finish(context, op, BG_STAGE_DOWNLOAD);
            }
        }
    });
}

const bg_backend_v1 table = {
    BG_BACKEND_ABI_VERSION, sizeof(bg_backend_v1), BG_BACKEND_CAP_FP64,
    context_create, context_close, patch_create, patch_close, query_create,
    query_close, evaluate, result_close, download
};
}

extern "C" bg_status brepgpu_cuda_backend_v1(uint32_t requested_abi,
                                             const bg_backend_v1** out,
                                             bg_diagnostic* diag) {
    if (out) *out = nullptr;
    return guarded(BG_STAGE_PLUGIN, diag, [&](Operation& op) {
        if (!out || requested_abi != BG_BACKEND_ABI_VERSION)
            op.error(BG_UNSUPPORTED, BG_STAGE_PLUGIN, "unsupported backend ABI");
        else *out = &table;
    });
}
