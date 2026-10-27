// matmul_opencl_autotune.c
// OpenCL GPU with auto tile tuning + OpenMP CPU fallback
//
// Build:  gcc -O2 -fopenmp -o matmul_autotile "matmul_opencl_autoselTileSize.c" -lOpenCL
// Run:    ./matmul_autotile [N]          (default N=1024; tile size auto-selected from 8/16/32/64)

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#ifdef _OPENMP
#include <omp.h>
#endif

static const char *kernelSource =
"__kernel void matmul_tiled(const int N, __global const float* A, __global const float* B, __global float* C, const int tile) {\n"
"    const int row = get_global_id(1);\n"
"    const int col = get_global_id(0);\n"
"    __local float Asub[64][64];\n"
"    __local float Bsub[64][64];\n"
"    float acc = 0.0f;\n"
"    int numTiles = (N + tile - 1) / tile;\n"
"    for (int t = 0; t < numTiles; ++t) {\n"
"        int tiledRow = row;\n"
"        int tiledCol = t * tile + get_local_id(0);\n"
"        if (tiledRow < N && tiledCol < N) Asub[get_local_id(1)][get_local_id(0)] = A[tiledRow * N + tiledCol];\n"
"        else Asub[get_local_id(1)][get_local_id(0)] = 0.0f;\n"
"        tiledRow = t * tile + get_local_id(1);\n"
"        tiledCol = col;\n"
"        if (tiledRow < N && tiledCol < N) Bsub[get_local_id(1)][get_local_id(0)] = B[tiledRow * N + tiledCol];\n"
"        else Bsub[get_local_id(1)][get_local_id(0)] = 0.0f;\n"
"        barrier(CLK_LOCAL_MEM_FENCE);\n"
"        for (int k = 0; k < tile; ++k) acc += Asub[get_local_id(1)][k] * Bsub[k][get_local_id(0)];\n"
"        barrier(CLK_LOCAL_MEM_FENCE);\n"
"    }\n"
"    if (row < N && col < N) C[row * N + col] = acc;\n"
"}\n";

static double time_diff_ms(struct timespec a, struct timespec b) {
    return (a.tv_sec - b.tv_sec) * 1000.0 + (a.tv_nsec - b.tv_nsec) / 1.0e6;
}

void fill_rand(float *M, int N) {
    for (int i = 0; i < N*N; ++i) M[i] = (float)(rand() % 100) / 10.0f;
}

void matmul_cpu_omp(const float *A, const float *B, float *C, int N) {
#ifdef _OPENMP
    #pragma omp parallel for collapse(2)
#endif
    for (int i=0; i<N; i++)
        for (int j=0; j<N; j++) {
            float acc = 0.0f;
            for (int k=0; k<N; k++) acc += A[i*N+k] * B[k*N+j];
            C[i*N+j] = acc;
        }
}

// Run OpenCL kernel on given device, return kernel time in ms
int run_on_device(cl_device_type dev_type, int N, int tile, double *out_total_ms, double *out_kernel_ms) {
    cl_int err;
    cl_uint numPlatforms=0;
    clGetPlatformIDs(0,NULL,&numPlatforms);
    if(numPlatforms==0) return -1;
    cl_platform_id *platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id)*numPlatforms);
    clGetPlatformIDs(numPlatforms, platforms, NULL);

    cl_device_id dev = NULL;
    for(cl_uint p=0; p<numPlatforms && dev==NULL; ++p){
        cl_uint numDevices=0;
        if(clGetDeviceIDs(platforms[p], dev_type, 0,NULL,&numDevices)!=CL_SUCCESS || numDevices==0) continue;
        cl_device_id *devices=(cl_device_id*)malloc(sizeof(cl_device_id)*numDevices);
        clGetDeviceIDs(platforms[p], dev_type, numDevices, devices, NULL);
        dev = devices[0];
        free(devices);
    }
    free(platforms);
    if(!dev) return -2;

    char devName[256]; clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(devName), devName,NULL);
    printf("=== Running on device: %s ===\n", devName);

    cl_context context = clCreateContext(NULL,1,&dev,NULL,NULL,&err);
    cl_command_queue queue = clCreateCommandQueue(context, dev, CL_QUEUE_PROFILING_ENABLE,&err);
    cl_program program = clCreateProgramWithSource(context,1,&kernelSource,NULL,&err);
    clBuildProgram(program,1,&dev,"",NULL,NULL);
    cl_kernel kernel = clCreateKernel(program,"matmul_tiled",&err);

    size_t bytes = (size_t)N*N*sizeof(float);
    float *A=(float*)malloc(bytes), *B=(float*)malloc(bytes), *C=(float*)malloc(bytes);
    srand(12345); fill_rand(A,N); fill_rand(B,N);

    cl_mem bufA=clCreateBuffer(context,CL_MEM_READ_ONLY,bytes,NULL,&err);
    cl_mem bufB=clCreateBuffer(context,CL_MEM_READ_ONLY,bytes,NULL,&err);
    cl_mem bufC=clCreateBuffer(context,CL_MEM_WRITE_ONLY,bytes,NULL,&err);

    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    clEnqueueWriteBuffer(queue,bufA,CL_TRUE,0,bytes,A,0,NULL,NULL);
    clEnqueueWriteBuffer(queue,bufB,CL_TRUE,0,bytes,B,0,NULL,NULL);

    clSetKernelArg(kernel,0,sizeof(int),&N);
    clSetKernelArg(kernel,1,sizeof(cl_mem),&bufA);
    clSetKernelArg(kernel,2,sizeof(cl_mem),&bufB);
    clSetKernelArg(kernel,3,sizeof(cl_mem),&bufC);
    clSetKernelArg(kernel,4,sizeof(int),&tile);

    size_t global[2]={(size_t)((N+tile-1)/tile)*tile,(size_t)((N+tile-1)/tile)*tile};
    size_t local[2]={(size_t)tile,(size_t)tile};

    cl_event k_event;
    clEnqueueNDRangeKernel(queue,kernel,2,NULL,global,local,0,NULL,&k_event);
    clFinish(queue);
    clEnqueueReadBuffer(queue,bufC,CL_TRUE,0,bytes,C,0,NULL,NULL);
    clock_gettime(CLOCK_MONOTONIC,&t1);

    double total_ms = time_diff_ms(t1,t0);
    double kernel_ms = -1.0;
    cl_ulong start,end;
    if(clGetEventProfilingInfo(k_event,CL_PROFILING_COMMAND_START,sizeof(start),&start,NULL)==CL_SUCCESS &&
       clGetEventProfilingInfo(k_event,CL_PROFILING_COMMAND_END,sizeof(end),&end,NULL)==CL_SUCCESS) kernel_ms=(end-start)*1.0e-6;

    printf("Total time (including transfer): %.3f ms\n",total_ms);
    if(kernel_ms>=0) printf("Kernel execution: %.3f ms\n",kernel_ms);

    *out_total_ms = total_ms; *out_kernel_ms = kernel_ms;

    clReleaseMemObject(bufA); clReleaseMemObject(bufB); clReleaseMemObject(bufC);
    clReleaseKernel(kernel); clReleaseProgram(program); clReleaseCommandQueue(queue);
    clReleaseContext(context); clReleaseEvent(k_event);
    free(A); free(B); free(C);

    return 0;
}

// Auto-tune GPU tile size
int autotune_tile(cl_device_id dev, int N) {
    int tiles[] = {8,16,32,64};
    int best_tile=16;
    double best_time=1e30;

    for(int i=0;i<4;i++){
        int tile=tiles[i];
        size_t max_wg; clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_GROUP_SIZE,sizeof(max_wg),&max_wg,NULL);
        if((size_t)(tile*tile)>max_wg) continue;

        double kernel_ms=0,total_ms=0;
        run_on_device(CL_DEVICE_TYPE_GPU,N,tile,&total_ms,&kernel_ms);
        if(kernel_ms<best_time){best_time=kernel_ms; best_tile=tile;}
    }
    printf("Auto-selected tile size: %d\n",best_tile);
    return best_tile;
}

int main(int argc,char **argv){
    int N=1024,tile=16;
    if(argc>=2) N=atoi(argv[1]);
    if(argc>=3) tile=atoi(argv[2]);
    printf("Matrix multiply N=%d\n",N);

    double gpu_total=0,gpu_kernel=0,cpu_total=0,cpu_kernel=0;

    // GPU autotune
    cl_uint numPlatforms=0; clGetPlatformIDs(0,NULL,&numPlatforms);
    cl_platform_id *platforms=(cl_platform_id*)malloc(sizeof(cl_platform_id)*numPlatforms);
    clGetPlatformIDs(numPlatforms,platforms,NULL);
    cl_device_id dev=0;
    for(cl_uint p=0;p<numPlatforms && !dev;p++){
        cl_uint numDevices=0;
        if(clGetDeviceIDs(platforms[p],CL_DEVICE_TYPE_GPU,0,NULL,&numDevices)!=CL_SUCCESS || numDevices==0) continue;
        cl_device_id *devices=(cl_device_id*)malloc(sizeof(cl_device_id)*numDevices);
        clGetDeviceIDs(platforms[p],CL_DEVICE_TYPE_GPU,numDevices,devices,NULL);
        dev=devices[0]; free(devices);
    }
    free(platforms);
    if(dev) tile=autotune_tile(dev,N);

    int rc_gpu=run_on_device(CL_DEVICE_TYPE_GPU,N,tile,&gpu_total,&gpu_kernel);

    int rc_cpu=run_on_device(CL_DEVICE_TYPE_CPU,N,tile,&cpu_total,&cpu_kernel);
    if(rc_cpu==-2){
        printf("No OpenCL CPU. Running plain C CPU with OpenMP.\n");
        size_t bytes=(size_t)N*N*sizeof(float);
        float *A=(float*)malloc(bytes),*B=(float*)malloc(bytes),*C=(float*)malloc(bytes);
        srand(12345); fill_rand(A,N); fill_rand(B,N);
        struct timespec t0,t1;
        clock_gettime(CLOCK_MONOTONIC,&t0);
        matmul_cpu_omp(A,B,C,N);
        clock_gettime(CLOCK_MONOTONIC,&t1);
        cpu_total=time_diff_ms(t1,t0); cpu_kernel=cpu_total;
        printf("CPU (OpenMP) time: %.3f ms\n",cpu_total);
        free(A); free(B); free(C);
    }

    printf("\n===== Summary =====\n");
    if(rc_gpu==0) printf("GPU: Total %.3f ms, Kernel %.3f ms\n",gpu_total,gpu_kernel);
    printf("CPU: Total %.3f ms, Kernel %.3f ms\n",cpu_total,cpu_kernel);
    if(rc_gpu==0) printf("Speedup (CPU/GPU): %.2fx\n",cpu_total/gpu_total);

    return 0;
}

