// sobel_image_host.c
// FRAGMENT ONLY - not a standalone program; it does not compile on its own.
// Shows the image-object host setup for sobel_image.cl. The complete, buildable
// version is run_image_kernel() in sobel_all_host.c:
//   gcc -O2 -o sobel_all_host sobel_all_host.c -lOpenCL -lm && ./sobel_all_host

// Replace the buffer-based code with image setup:
cl_image_format fmt = { CL_R, CL_UNORM_INT8 };
cl_image_desc desc;
memset(&desc, 0, sizeof(desc));
desc.image_type = CL_MEM_OBJECT_IMAGE2D;
desc.image_width = width;
desc.image_height = height;

cl_mem imgIn = clCreateImage(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             &fmt, &desc, in, &err);
cl_mem imgOut = clCreateImage(ctx, CL_MEM_WRITE_ONLY,
                              &fmt, &desc, NULL, &err);

cl_kernel k = clCreateKernel(prog, "sobel_image", &err);
clSetKernelArg(k, 0, sizeof(cl_mem), &imgIn);
clSetKernelArg(k, 1, sizeof(cl_mem), &imgOut);

size_t g[2] = { width, height };
cl_event evt;
struct timespec t0, t1;
clock_gettime(CLOCK_MONOTONIC, &t0);
clEnqueueNDRangeKernel(q, k, 2, NULL, g, NULL, 0, NULL, &evt);
clFinish(q);
clock_gettime(CLOCK_MONOTONIC, &t1);
printf("sobel_image kernel: %.3f ms\n", tdiff(t1, t0));

// Read back result
size_t origin[3] = {0,0,0}, region[3] = {width,height,1};
clEnqueueReadImage(q, imgOut, CL_TRUE, origin, region, 0, 0, out, 0, NULL, NULL);

