#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// #define USE_XCLBIN  // Uncomment this line when using Xilinx FPGA with .xclbin file. 
                        // Also ensure to set PLATFORM_INDEX appropriately below.

#define CHECK(status, msg) \
    if (status != CL_SUCCESS) { fprintf(stderr, "%s Error: %d\n", msg, status); return -1; }

int main() {
    const size_t N = 8*1024*1024; // 8 million+ elements
    float *A = (float*)malloc(sizeof(float) * N);
    float *B = (float*)malloc(sizeof(float) * N);
    float *C = (float*)malloc(sizeof(float) * N);

    for (size_t i = 0; i < N; i++) {
        A[i] = 1.0f;
        B[i] = 2.0f;
    }

    // ---------------- CPU baseline ----------------
    clock_t c1 = clock();
    for (size_t i = 0; i < N; i++) {
        C[i] = A[i] + B[i];
    }
    clock_t c2 = clock();
    double cpu_time = (double)(c2 - c1) / CLOCKS_PER_SEC;
    printf("CPU (pure C) time: %f seconds\n", cpu_time);

    // ---------------- GPU OpenCL ----------------
    cl_int status;

    // Get all platforms. Assuming 2 here.
    cl_platform_id platform[2]; // assuming a total of 2 platforms.
    status = clGetPlatformIDs(2, platform, NULL);
    CHECK(status, "clGetPlatformIDs failed");

    // Get CPU/GPU/FPGA device
    // Change CL_DEVICE_TYPE_SEL to CL_DEVICE_TYPE_CPU if using PoCL, CL_DEVICE_TYPE_GPU if using GPU.
    // For PC, GPU is generally the first platform (PLATFORM_INDEX is 0), CPU (PoCL) second. 
    // For Kria, CPU (PoCL, if installed) is generally the first platform (PLATFORM_INDEX is 0), FPGA second.
    // **Important**: Check the order via `clinfo` and make selections appropropriately.
    #ifdef USE_XCLBIN
        #define CL_DEVICE_TYPE_SEL CL_DEVICE_TYPE_ACCELERATOR
        #define PLATFORM_INDEX 1
    #else
        #define CL_DEVICE_TYPE_SEL CL_DEVICE_TYPE_GPU
        #define PLATFORM_INDEX 0 
    #endif
    
    cl_device_id device;
    status = clGetDeviceIDs(platform[PLATFORM_INDEX], CL_DEVICE_TYPE_SEL, 1, &device, NULL); 
    CHECK(status, "clGetDeviceIDs failed");

    // Context + queue
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &status);
    CHECK(status, "clCreateContext failed");

    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &status);
    CHECK(status, "clCreateCommandQueue failed");

    // Load kernel file
    #ifdef USE_XCLBIN
        const char* filename = "vadd.xclbin";
        const char* mode = "rb";
    #else
        const char* filename = "kernel.cl";
        const char* mode = "r";
    #endif

    FILE* f = fopen(filename, mode);
    if (!f) { fprintf(stderr, "Failed to open %s\n", filename); return -1; }
    fseek(f, 0, SEEK_END);
    size_t src_size = ftell(f);
    rewind(f);

    #ifdef USE_XCLBIN
        unsigned char* src = (unsigned char*)malloc(src_size);
        fread(src, 1, src_size, f);
    #else
        char* src = (char*)malloc(src_size + 1);
        fread(src, 1, src_size, f);
        src[src_size] = '\0';
    #endif
    fclose(f);

    // Build program
    #ifdef USE_XCLBIN
        cl_program program = clCreateProgramWithBinary(context, 1, &device, &src_size, 
                                                    (const unsigned char**)&src, NULL, &status);
        CHECK(status, "clCreateProgramWithBinary failed");
    #else
        cl_program program = clCreateProgramWithSource(context, 1, (const char**)&src, NULL, &status);
        CHECK(status, "clCreateProgramWithSource failed");
    #endif

    status = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (status != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        fprintf(stderr, "Build log:\n%s\n", log);
        free(log);
        return -1;
    }

    cl_kernel kernel = clCreateKernel(program, "vec_add", &status);
    CHECK(status, "clCreateKernel failed");

    // ---------------- GPU timing ----------------
    // Full pipeline: buffer creation + transfers + kernel + readback
    clock_t tp_start = clock();

    // Buffers
    cl_mem bufA = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float)*N, NULL, &status);
    cl_mem bufB = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float)*N, NULL, &status);
    cl_mem bufC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*N, NULL, &status);

    // Host → Device transfers
    clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, sizeof(float)*N, A, 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, sizeof(float)*N, B, 0, NULL, NULL);

    // Set args
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &bufA);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &bufB);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &bufC);

    // Pure kernel timing
    size_t globalSize = N;
    size_t localSize = 512;
    c1 = clock();
    status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);
    CHECK(status, "clEnqueueNDRangeKernel failed");
    clFinish(queue);
    c2 = clock();
    double ocl_kernel_time = (double)(c2 - c1) / CLOCKS_PER_SEC;

    // Device → Host transfer
    clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, sizeof(float)*N, C, 0, NULL, NULL);

    clock_t tp_end = clock();
    double ocl_pipeline_time = (double)(tp_end - tp_start) / CLOCKS_PER_SEC;

    printf("OpenCL kernel-only time: %f seconds\n", ocl_kernel_time);
    printf("OpenCL full pipeline time (incl. transfers): %f seconds\n", ocl_pipeline_time);

    // Cleanup
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(bufA);
    clReleaseMemObject(bufB);
    clReleaseMemObject(bufC);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    printf("C[123] = %f (should be 3)\n", C[123]);

    free(A);
    free(B);
    free(C);
    free(src);

    return 0;
}
