# Open-ended Project

<!-- !!! danger "Work in Progress" -->

The project is open-ended - you choose the application to accelerate as well as the exact stuff to do.

Minimally, implement and compare the time taken when your application is

- run entirely on x86_64 CPU using plain C or PoCL vs
- hardware accelerated via OpenCL on integrated and/or discrete GPU.

and

- run entirely on Kria (Zynq Ultrascale+) PS, i.e., Cortex A53 in plain C or PoCL vs
- accelerated using an FPGA/PL-based accelerator using HLS/HDL + AXI DMA / AXI Slave / AXI Master-based DMA-capable coprocessor. The host code can be plain C (Standalone/FreeRTOS/Linux) or Pynq.

Comparison can be either based on profiling and/or based on a timer (hardware/software timer on FPGA, software timer on PC).

## Sample Applications

Some sample applications are given below. These are just to provide a very rough idea of the stuff you can do.

- Easiest: Increasing or decreasing the brightness of images.
- Moderate: Edge detection using Sobel filters, Machine learning / classification (inference) using linear classifiers, etc.
- Harder: Object recognition in an image, deep learning based image processing, etc.

As the focus of the module is not on algorithms themselves, but on how they can be accelerated, the complexity of the algorithm is not of great importance. Just that an algorithm that is too simple offers you little chance to try out optimizations.

## Enhancements

Try out various configurations and optimizations.

HLS/HDL/OpenCL optimizations for FPGA such as unroll, pipeline, array partition, dataflow (applicable if you have multiple workgroups in the NDRange),
Possible OpenCL optimizations for GPU such as vectorization and different workgroup sizes,
Bit width optimizations (for both FPGA and GPU, but more applicable for FPGAs), etc.
Present your results in a presentable manner - for example, as a proper image on PC irrespective of whether you process it on FPGA or PC. It is also possible to show the image on a monitor connected to FPGA directly using the TFT IP provided by Xilinx (if using standalone) or via the GUI of Linux!.

For large data sizes, if using FIFO for testing (using DMA is strongly recommended), you will need to either 1) increase the FIFO transmit and/or receive buffer size as appropriate or 2) send/receive the data as multiple chunks, each not exceeding the respective buffer size.

If you have large local arrays, you may need to increase the stack size. If you have large dynamic arrays, you will need to increase the heap size. This can be done by double-clicking the .ld file in the sources folder.

Serial communication can be used for FPGA, with data sent via UART, but you can also consider exploring Ethernet-based communications. It is also possible to load the files from an SD card, though this requires either using FAT file system drivers (if using standalone) or running Linux.

## Deliverables

Make a short 10-minute presentation (7-8 minutes for presentation, 2-3 minutes for QnA) on your application, approaches, and results. Some of you will have to present in the Week 13 lecture, others will present at other time slots to the teaching assistants. The criterion for deciding who will have to present during the lecture will be announced later. A quick demo of your system during the presentation is appreciated, though not absolutely necessary. However, we reserve the right to require a quick demo after the presentation if we deem it necessary.

No report is necessary, but the slides to be used for the presentation, all the codes (only source files - .c/.cpp/.h/.py you have created/edited) + .xdc files/files + data files + any PC-side visualization code should be uploaded to Canvas by 6 PM of Week 13 Monday.
It should be as a single .zip archive, with the filename <Team member 1 Name\>\_<Team member 2 Name\>\_1.zip. One submission per team is sufficient. It will show as not submitted for the other person, but that is ok. If you are doing solo, do not include \_<Team member 2 Name\> in the filename.


You should clearly demarcate and acknowledge the codes that are not written yourself, in the source code as well as in the presentation.