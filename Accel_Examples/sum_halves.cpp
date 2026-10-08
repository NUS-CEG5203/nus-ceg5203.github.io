// sum_halves.cpp
// HLS exercise: BRAM port conflicts and ARRAY_PARTITION. Try the commented-out pragma and
// the three-way average, re-synthesise, and compare the achieved II in the report.
// There is no main(), so this file only synthesises (no standalone executable).
//
// Setup:
//   source ~/Xilinx/2025.1/Vitis/settings64.sh   # adjust to your install; sets $XILINX_VITIS
// Synthesis (Vitis HLS, KV260 part):
//   v++ -c --mode hls --part xck26-sfvc784-2LV-c --hls.flow_target vivado \
//       --hls.syn.file sum_halves.cpp --hls.syn.top sum_halves --hls.clock 10ns --work_dir sum_halves
//   # Report: sum_halves/hls/syn/report/sum_halves_csynth.rpt
// Syntax check only (no HLS tools needed):  g++ -c sum_halves.cpp

#define N 2048

void sum_halves(int a[N], int out[1024]) {
#pragma HLS INTERFACE bram port=a
#pragma HLS INTERFACE bram port=out
//#pragma HLS ARRAY_PARTITION variable=a ? factor=?

    for (int i = 0; i < 1024; i++) {
#pragma HLS PIPELINE
        out[i] = (a[i] + a[i + 1024]) / 2;
        //out[i] = (a[i] + a[i + 512] + a[i + 1024]) / 3;
    }
}