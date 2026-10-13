#include <cuda_runtime.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(cudaError_t status, const char* operation) {
    if (status != cudaSuccess) {
        std::fprintf(stderr, "CUDA failure: %s: %s (%d)\n", operation,
                     cudaGetErrorString(status), static_cast<int>(status));
        std::exit(2);
    }
}
#define CHECK(operation) check((operation), #operation)

__global__ void fp64_polynomial(const double* input, double* output, int count) {
    int i = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    if (i < count) {
        double x = input[i];
        output[i] = fma(x, x, fma(3.0, x, 2.0));
    }
}

int main(int argc, char** argv) {
    if (argc != 2) return 64;
    char* tail = nullptr;
    long requested = std::strtol(argv[1], &tail, 10);
    if (!tail || *tail || requested < 0 || requested > 1) return 64;
    int device = static_cast<int>(requested), device_count = 0;
    CHECK(cudaGetDeviceCount(&device_count));
    if (device_count != 2) {
        std::fprintf(stderr, "Expected two GPUs, observed %d\n", device_count);
        return 1;
    }
    CHECK(cudaSetDevice(device));
    cudaDeviceProp property{};
    CHECK(cudaGetDeviceProperties(&property, device));
    if (property.major != 6 || property.minor != 0) return 1;
    constexpr int count = 257;
    std::vector<double> input(count), output(count);
    for (int i = 0; i < count; ++i) input[i] = (i - 128) / 16.0;
    double *device_input = nullptr, *device_output = nullptr;
    cudaStream_t stream = nullptr;
    CHECK(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));
    CHECK(cudaMalloc(&device_input, count * sizeof(double)));
    CHECK(cudaMalloc(&device_output, count * sizeof(double)));
    CHECK(cudaMemcpyAsync(device_input, input.data(), count * sizeof(double),
                          cudaMemcpyHostToDevice, stream));
    fp64_polynomial<<<(count + 63) / 64, 64, 0, stream>>>(device_input, device_output, count);
    CHECK(cudaGetLastError());
    CHECK(cudaMemcpyAsync(output.data(), device_output, count * sizeof(double),
                          cudaMemcpyDeviceToHost, stream));
    CHECK(cudaStreamSynchronize(stream));
    double max_error = 0.0;
    for (int i = 0; i < count; ++i) {
        int numerator = i - 128;
        double expected = (numerator * numerator + 48 * numerator + 512) / 256.0;
        if (!std::isfinite(output[i]) || output[i] != expected) {
            std::fprintf(stderr, "Analytic mismatch: i=%d observed=%.17g expected=%.17g\n",
                         i, output[i], expected);
            return 1;
        }
        max_error = std::fmax(max_error, std::fabs(output[i] - expected));
    }
    cudaFuncAttributes attributes{};
    CHECK(cudaFuncGetAttributes(&attributes, fp64_polynomial));
    if (attributes.binaryVersion != 60) return 1;
    int runtime_version = 0, driver_version = 0;
    CHECK(cudaRuntimeGetVersion(&runtime_version));
    CHECK(cudaDriverGetVersion(&driver_version));
    CHECK(cudaFree(device_output));
    CHECK(cudaFree(device_input));
    CHECK(cudaStreamDestroy(stream));
    std::printf("device=%d name=%s memory_bytes=%zu compute_capability=%d.%d "
                "kernel_binary_version=%d runtime_version=%d driver_api_version=%d "
                "queries=%d max_abs_error=%.17g result=PASS\n",
                device, property.name, property.totalGlobalMem, property.major, property.minor,
                attributes.binaryVersion, runtime_version, driver_version, count, max_error);
    return 0;
}
