// sobel_all_host.c
// Times the naive, tiled and image-object Sobel OpenCL kernels on a 1024x1024 image.
//
// Build:  gcc -O2 -o sobel_all_host sobel_all_host.c -lOpenCL -lm
// Run:    ./sobel_all_host        (or: bash run_sobel_all_host.sh)
//         Run from this directory: sobel_naive.cl, sobel_tiled.cl and sobel_image.cl are
//         loaded at runtime. Image size and work-group size are fixed in main().

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>

#define CHECK(e, msg) if(e!=CL_SUCCESS){fprintf(stderr,"%s: %d\n",msg,e);exit(1);}
double tdiff(struct timespec a, struct timespec b){
    return (a.tv_sec-b.tv_sec)*1e3 + (a.tv_nsec-b.tv_nsec)/1e6;
}

char* load_kernel(const char *fname){
    FILE *f=fopen(fname,"rb");
    if(!f){perror(fname);exit(1);}
    fseek(f,0,SEEK_END);size_t sz=ftell(f);rewind(f);
    char *src=malloc(sz+1);fread(src,1,sz,f);src[sz]=0;fclose(f);
    return src;
}

void run_buffer_kernel(cl_context ctx, cl_command_queue q, cl_program prog,
                       const char *kname, const unsigned char *in, unsigned char *out,
                       int width, int height, size_t local_xy)
{
    cl_int err;
    size_t sz = width * height;
    cl_mem bufIn = clCreateBuffer(ctx, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sz, (void*)in, &err);
    cl_mem bufOut = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, sz, NULL, &err);
    cl_kernel k = clCreateKernel(prog, kname, &err);

    if(strcmp(kname,"sobel_tiled")==0){
        size_t local_bytes = (local_xy+2)*(local_xy+2);
        clSetKernelArg(k, 0, sizeof(cl_mem), &bufIn);
        clSetKernelArg(k, 1, sizeof(cl_mem), &bufOut);
        clSetKernelArg(k, 2, sizeof(int), &width);
        clSetKernelArg(k, 3, sizeof(int), &height);
        clSetKernelArg(k, 4, local_bytes, NULL);
    } else {
        clSetKernelArg(k, 0, sizeof(cl_mem), &bufIn);
        clSetKernelArg(k, 1, sizeof(cl_mem), &bufOut);
        clSetKernelArg(k, 2, sizeof(int), &width);
        clSetKernelArg(k, 3, sizeof(int), &height);
    }

    size_t gsz[2] = {width, height};
    size_t lsz[2] = {local_xy, local_xy};

    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    clEnqueueNDRangeKernel(q,k,2,NULL,gsz,lsz,0,NULL,NULL);
    clFinish(q);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    printf("%s: %.3f ms\n", kname, tdiff(t1,t0));

    clEnqueueReadBuffer(q, bufOut, CL_TRUE, 0, sz, out, 0, NULL, NULL);
    clReleaseMemObject(bufIn);
    clReleaseMemObject(bufOut);
    clReleaseKernel(k);
}

void run_image_kernel(cl_context ctx, cl_command_queue q, cl_program prog,
                      const unsigned char *in, unsigned char *out, int width, int height)
{
    cl_int err;
    cl_image_format fmt = { CL_R, CL_UNORM_INT8 };
    cl_image_desc desc = {0};
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = width;
    desc.image_height = height;

    cl_mem imgIn = clCreateImage(ctx, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, &fmt, &desc, (void*)in, &err);
    cl_mem imgOut = clCreateImage(ctx, CL_MEM_WRITE_ONLY, &fmt, &desc, NULL, &err);
    cl_kernel k = clCreateKernel(prog, "sobel_image", &err);

    clSetKernelArg(k, 0, sizeof(cl_mem), &imgIn);
    clSetKernelArg(k, 1, sizeof(cl_mem), &imgOut);

    size_t g[2] = {width, height};
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    clEnqueueNDRangeKernel(q,k,2,NULL,g,NULL,0,NULL,NULL);
    clFinish(q);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    printf("sobel_image: %.3f ms\n", tdiff(t1,t0));

    size_t origin[3]={0,0,0}, region[3]={width,height,1};
    clEnqueueReadImage(q,imgOut,CL_TRUE,origin,region,0,0,out,0,NULL,NULL);

    clReleaseMemObject(imgIn);
    clReleaseMemObject(imgOut);
    clReleaseKernel(k);
}

int main(){
    int width=1024, height=1024;
    size_t N = width*height;
    unsigned char *in = malloc(N), *out = malloc(N);
    for(size_t i=0;i<N;i++) in[i] = rand()%256;

    cl_int err;
    cl_platform_id p; cl_device_id d;
    clGetPlatformIDs(1,&p,NULL);
    clGetDeviceIDs(p,CL_DEVICE_TYPE_GPU,1,&d,NULL);
    cl_context ctx = clCreateContext(NULL,1,&d,NULL,NULL,&err);
    cl_command_queue q = clCreateCommandQueue(ctx,d,0,&err);

    char *src = load_kernel("sobel_naive.cl");
    char *src2 = load_kernel("sobel_tiled.cl");
    char *src3 = load_kernel("sobel_image.cl");
    const char *allsrcs[] = {src, src2, src3};
    cl_program prog = clCreateProgramWithSource(ctx,3,allsrcs,NULL,&err);
    err = clBuildProgram(prog,0,NULL,NULL,NULL,NULL);
    if(err!=CL_SUCCESS){
        size_t log_size; clGetProgramBuildInfo(prog,d,CL_PROGRAM_BUILD_LOG,0,NULL,&log_size);
        char *log=malloc(log_size); clGetProgramBuildInfo(prog,d,CL_PROGRAM_BUILD_LOG,log_size,log,NULL);
        printf("%s\n",log); free(log); exit(1);
    }

    run_buffer_kernel(ctx,q,prog,"sobel_naive",in,out,width,height,16);
    run_buffer_kernel(ctx,q,prog,"sobel_tiled",in,out,width,height,16);
    run_image_kernel(ctx,q,prog,in,out,width,height);

    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);
    free(in); free(out);
    free(src); free(src2); free(src3);
    return 0;
}

