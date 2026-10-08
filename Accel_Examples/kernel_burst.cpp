// kernel_burst.cpp
// Burst-optimised Vitis HLS vector addition: separate m_axi bundles, 64-element
// read -> compute -> write chunks through local buffers. Testbench: test_Kernel.cpp
//
// Setup:
//   source ~/Xilinx/2025.1/Vitis/settings64.sh   # adjust to your install; sets $XILINX_VITIS
// C simulation:
//   g++ -O2 -I$XILINX_VITIS/include -o tb_vadd_burst kernel_burst.cpp test_Kernel.cpp
//   ./tb_vadd_burst
// Synthesis (Vitis HLS, KV260 part):
//   v++ -c --mode hls --part xck26-sfvc784-2LV-c --hls.flow_target vivado \
//       --hls.syn.file kernel_burst.cpp --hls.syn.top vadd --hls.clock 10ns --work_dir vadd_burst
//   # Report: vadd_burst/hls/syn/report/vadd_csynth.rpt

#include <ap_int.h>
#include <hls_stream.h>

#define MAX_SIZE 1024
#define BURST_LEN 64

// Optimized vector addition kernel
void vadd(volatile int* A, volatile int* B, volatile int* C, int N) {
#pragma HLS INTERFACE m_axi port=A offset=slave bundle=gmem0 depth=1024
#pragma HLS INTERFACE m_axi port=B offset=slave bundle=gmem1 depth=1024
#pragma HLS INTERFACE m_axi port=C offset=slave bundle=gmem2 depth=1024
#pragma HLS INTERFACE s_axilite port=N
#pragma HLS INTERFACE s_axilite port=return

    // Local buffers for burst transfers
    int buffer_A[BURST_LEN];
    int buffer_B[BURST_LEN];
    int buffer_C[BURST_LEN];
    
#pragma HLS ARRAY_PARTITION variable=buffer_A complete
#pragma HLS ARRAY_PARTITION variable=buffer_B complete
#pragma HLS ARRAY_PARTITION variable=buffer_C complete

    // Process data in bursts
    burst_loop: for (int base = 0; base < N; base += BURST_LEN) {
        int chunk_size = (base + BURST_LEN > N) ? N - base : BURST_LEN;
        
        // Burst read from A and B
        read_loop: for (int i = 0; i < chunk_size; i++) {
#pragma HLS PIPELINE II=1
            buffer_A[i] = A[base + i];
            buffer_B[i] = B[base + i];
        }
        
        // Compute vector addition
        compute_loop: for (int i = 0; i < chunk_size; i++) {
#pragma HLS PIPELINE II=1
            buffer_C[i] = buffer_A[i] + buffer_B[i];
        }
        
        // Burst write to C
        write_loop: for (int i = 0; i < chunk_size; i++) {
#pragma HLS PIPELINE II=1
            C[base + i] = buffer_C[i];
        }
    }
}