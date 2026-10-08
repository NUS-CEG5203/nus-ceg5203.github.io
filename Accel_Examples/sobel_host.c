// sobel_host.c
// Compare naive vs tiled Sobel OpenCL kernels
//
// Build:  gcc -O2 -o sobel_host sobel_host.c -lOpenCL -lm
// Run:    ./sobel_host [width] [height] [block]
//         Run from this directory: sobel_naive.cl and sobel_tiled.cl are loaded at runtime.
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

static double tdiff(struct timespec a, struct timespec b) {
    return (a.tv_sec - b.tv_sec) * 1000.0 + (a.tv_nsec - b.tv_nsec) / 1.0e6;
}

char* load_source(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); exit(1); }
    fseek(f, 0, SEEK_END); long len = ftell(f); rewind(f);
    char *src = malloc(len + 1);
    fread(src, 1, len, f); src[len] = 0;
    fclose(f); return src;
}

void fill_random(unsigned char *buf, int w, int h) {
    for (int i=0;i<w*h;i++) buf[i] = rand() & 0xFF;
}

void run_kernel(cl_context ctx, cl_command_queue q, cl_device_id dev,
                const char *srcfile, const char *kname,
                unsigned char *in, unsigned char *out,
                int width, int height, size_t block)
{
    size_t bytes = (size_t)width * height;
    cl_int err;
    char *src = load_source(srcfile);
    cl_program prog = clCreateProgramWithSource(ctx, 1, (const char**)&src, NULL, &err);
    if (err != CL_SUCCESS) { fprintf(stderr, "CreateProgram %d\n", err); exit(1); }
    err = clBuildProgram(prog, 1, &dev, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t logsz; clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &logsz);
        char *log = malloc(logsz+1); clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, logsz, log, NULL);
        log[logsz]=0; fprintf(stderr, "Build error in %s:\n%s\n", srcfile, log); free(log); exit(1);
    }

    cl_kernel k = clCreateKernel(prog, kname, &err);
    if (err != CL_SUCCESS) { fprintf(stderr, "CreateKernel %d\n", err); exit(1); }

    cl_mem bufIn  = clCreateBuffer(ctx, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, bytes, in, &err);
    cl_mem bufOut = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, NULL, &err);

    clSetKernelArg(k, 0, sizeof(cl_mem), &bufIn);
    clSetKernelArg(k, 1, sizeof(cl_mem), &bufOut);
    clSetKernelArg(k, 2, sizeof(int), &width);
    clSetKernelArg(k, 3, sizeof(int), &height);

    size_t g[2] = { (size_t)width, (size_t)height };
    size_t l[2] = { block, block };

    // If kernel expects __local arg (tiled version)
    cl_kernel_info info;
    if (clGetKernelArgInfo(k, 4, CL_KERNEL_ARG_ADDRESS_QUALIFIER, sizeof(info), &info, NULL) == CL_SUCCESS)
    {
        size_t lmem = (block + 2) * (block + 2);
        clSetKernelArg(k, 4, lmem, NULL);
    }

    cl_event evt;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    err = clEnqueueNDRangeKernel(q, k, 2, NULL, g, l, 0, NULL, &evt);
    clFinish(q);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    cl_ulong s=0,e=0; clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_START, sizeof(s), &s, NULL);
    clGetEventProfilingInfo(evt, CL_PROFILING_COMMAND_END, sizeof(e), &e, NULL);
    printf("%-12s kernel: %.3f ms  (total wall: %.3f ms)\n", kname,
           (e-s)/1.0e6, tdiff(t1,t0));

    clEnqueueReadBuffer(q, bufOut, CL_TRUE, 0, bytes, out, 0, NULL, NULL);

    clReleaseEvent(evt);
    clReleaseMemObject(bufIn);
    clReleaseMemObject(bufOut);
    clReleaseKernel(k);
    clReleaseProgram(prog);
    free(src);
}

int main(int argc, char **argv) {
    int w=1024, h=1024, block=16;
    if (argc>1) w=atoi(argv[1]);
    if (argc>2) h=atoi(argv[2]);
    if (argc>3) block=atoi(argv[3]);
    printf("Running Sobel %dx%d, block %d\n", w,h,block);

    unsigned char *in = malloc(w*h);
    unsigned char *out = malloc(w*h);
    fill_random(in, w, h);

    cl_uint np; clGetPlatformIDs(0,NULL,&np);
    cl_platform_id pf[np]; clGetPlatformIDs(np,pf,NULL);
    cl_device_id dev;
    clGetDeviceIDs(pf[0], CL_DEVICE_TYPE_DEFAULT, 1, &dev, NULL);
    char name[128]; clGetDeviceInfo(dev, CL_DEVICE_NAME, 128, name, NULL);
    printf("Device: %s\n", name);

    cl_int err;
    cl_context ctx = clCreateContext(NULL, 1, &dev, NULL, NULL, &err);
    cl_command_queue q = clCreateCommandQueue(ctx, dev, CL_QUEUE_PROFILING_ENABLE, &err);

    run_kernel(ctx, q, dev, "sobel_naive.cl", "sobel_naive", in, out, w, h, block);
    run_kernel(ctx, q, dev, "sobel_tiled.cl", "sobel_tiled", in, out, w, h, block);

    clReleaseCommandQueue(q);
    clReleaseContext(ctx);
    free(in); free(out);
}

