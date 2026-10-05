# OpenCL Introduction

<!-- !!! danger "Work in Progress"-->

!!! success "Final"
    This assignment description is now complete. There could still be minor updates, which will be <span style="color: brown;">highlighted</span>. 

In this assignment, we explore OpenCL and Pynq. 

We start with configuring the computer (PC) and IDE to run OpenCL code. [Instructions](2_OpenCL_Instructions.md) for Intel GPUs in Windows and Linux are provided. [Instructions](3_Pynq_OpenCL_Kria.md) to run OpenCL on the Kria board are also provided.

## Assignment 3

This assignment involves

1) Accelerating the same computational problem as what you did in the previous 2 assignments - to be able to multiply two matrices A and B and to produce the result RES = A*B/256. This has to be done via

* OpenCL (GPU) on PC and 
* Kria board (CPU) via Portable OpenCL (PoCL). 

2) You must also report the time taken for multiplication in pure software (run without hardware acceleration / OpenCL) as well as with OpenCL.

Note: Since the size of the problem you are dealing with is small, the acceleration you obtain won't be indicative. Also, operating system context switches make performance evaluation hard to do deterministically. It is also affected by caching, and generally, it is faster the second time you run it.

If you have both onboard graphics (e.g. Intel) as well as a discrete GPU, it would be interesting to compare the performance of the two vs pure software. This is not a requirement.

3) Implement a Pynq-based acceleration on Kria board. Time measurement is optional, but recommended. The time reported should not include the time to program the bitstream.

## Submission Info

Assignment 3 (10 marks)

Upload a .zip file containing the

* the .c host code
* the .cl kernel code if it is not embedded into the host code
* the .py file used with Pynq
* A text file containing the information printed on the console in each of the cases
    1) OpenCL (GPU) on PC, with a comparison with plain C
    2) OpenCL (CPU) on Kria, with a comparison with plain C
    3) Pynq on Kria

to Canvas by **11:59 PM, 19 Nov 2026**.

It should be as a single .zip archive, with the filename <Team member 1 Name\>\_<Team member 2 Name\>\_3.zip. One submission per team is sufficient. It will show as not submitted for the other person, but that is ok. If you are doing solo, do not include \_<Team member 2 Name\> in the filename.

Please DO NOT upload the whole project!

You will also need to do a demonstration to a teaching assistant (arrangements will be made known in due course) - arrangements will be made known in due course.

## Tips

### Multiple Platforms

If you have multiple GPUs (for example, integrated/onboard and discrete), the idea below could be useful. The idea could be extended for cases where there are multiple devices per platform.

``` c
cl_platform_id cpPlatform[2]; // If you have 2 platforms

// Get IDs of both platforms
err = clGetPlatformIDs(2, cpPlatform, NULL);

// Discrete GPU is likely cpPlatform[0] - you can find this info using clinfo. Change this to cpPlatform[1] to change to onboard graphics.
err = clGetDeviceIDs(cpPlatform[0], CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
```

### Printing Kernel Program Build Log

If you experience issues related to your device OpenCL code compilation failure, you should inspect the kernel build log.

The most likely reason is that the .cl isn't accessible by the main program at runtime.

Another reason is the use of `double` - OpenCL extension `cl_khr_fp64` - is not supported by Intel integrated GPUs. You can see the supported extensions via clinfo - if you have only one GPU, see if `clinfo | grep cl_khr_fp64` gives some output. Such issues can be debugged by inspecting the kernel program build log.

### Setting OpenCL Version

You may need to have `#define CL_TARGET_OPENCL_VERSION 120` at the beginning of the example code covered in the lecture, else you will get warnings such as `clCreateCommandQueue` being deprecated. `clCreateCommandQueue` usage is discouraged in versions >=2.0, and the OpenCL version will default to the highest supported (likely 3.0 on Intel GPUs). This is just a warning though, not an error.
