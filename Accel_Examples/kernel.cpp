// kernel.cpp
// Simple Vitis HLS vector addition: one pipelined loop, all arrays on one m_axi bundle.
// Compare its synthesis report with kernel_burst.cpp.
//
// Setup:
//   source ~/Xilinx/2025.1/Vitis/settings64.sh   # adjust to your install; sets $XILINX_VITIS
// Synthesis (Vitis HLS, KV260 part):
//   v++ -c --mode hls --part xck26-sfvc784-2LV-c --hls.flow_target vivado \
//       --hls.syn.file kernel.cpp --hls.syn.top vadd --hls.clock 10ns --work_dir vadd_simple
//   # Report: vadd_simple/hls/syn/report/vadd_csynth.rpt
// C simulation: test_Kernel.cpp declares the kernel_burst.cpp signature. To use it here,
//   change its declaration to: extern "C" void vadd(const int*, const int*, int*, int);
//   then: g++ -O2 -I$XILINX_VITIS/include -o tb_vadd kernel.cpp test_Kernel.cpp && ./tb_vadd

#include <hls_stream.h>
#include <ap_int.h>

// Vector addition kernel
// Each pointer is mapped to AXI4 memory (global memory)
extern "C" {
void vadd(const int *A, const int *B, int *C, int size) {
#pragma HLS INTERFACE m_axi port=A offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=B offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=C offset=slave bundle=gmem

#pragma HLS INTERFACE s_axilite port=A bundle=control
#pragma HLS INTERFACE s_axilite port=B bundle=control
#pragma HLS INTERFACE s_axilite port=C bundle=control
#pragma HLS INTERFACE s_axilite port=size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

  // Main loop
  for (int i = 0; i < size; i++) {
  #pragma HLS PIPELINE II=1
    C[i] = A[i] + B[i];
  }
}
}
