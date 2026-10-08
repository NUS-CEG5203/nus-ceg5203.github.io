/*
----------------------------------------------------------------------------------
--	(c) Rajesh C Panicker, NUS,
--  Description : AXI DMA Coprocessor (HLS), implementing the sum of 4 numbers
--	License terms :
--	You are free to use this code as long as you
--		(i) DO NOT post a modified version of this on any public repository;
--		(ii) use it only for educational purposes;
--		(iii) accept the responsibility to ensure that your implementation does not violate any intellectual property of any entity.
--		(iv) accept that the program is provided "as is" without warranty of any kind or assurance regarding its suitability for any particular purpose;
--		(v) send an email to rajesh.panicker@ieee.org briefly mentioning its use (except when used for the course EE4218/CEG5203 at the National University of Singapore);
--		(vi) retain this notice in this file or any files derived from this.
----------------------------------------------------------------------------------
*/

#include "ap_int.h"

#define NUMBER_OF_INPUT_WORDS 4
#define NUMBER_OF_OUTPUT_WORDS 4

void myip_v1_0_HLS_DMA(
    ap_uint<32>* input_buffer,      // Input memory buffer
    ap_uint<32>* output_buffer,     // Output memory buffer
    ap_uint<32> input_addr,         // Input buffer address offset
    ap_uint<32> output_addr,        // Output buffer address offset
    //ap_uint<32> transfer_length,    // Number of words to transfer
    ap_uint<1> start,               // Start processing
    ap_uint<1>* done                // Processing complete flag
){
    // Interface pragmas
    #pragma HLS INTERFACE m_axi port=input_buffer offset=slave bundle=gmem0 depth=1024
    #pragma HLS INTERFACE m_axi port=output_buffer offset=slave bundle=gmem1 depth=1024
    #pragma HLS INTERFACE s_axilite port=input_addr bundle=control
    #pragma HLS INTERFACE s_axilite port=output_addr bundle=control
    //#pragma HLS INTERFACE s_axilite port=transfer_length bundle=control
    #pragma HLS INTERFACE s_axilite port=start bundle=control
    #pragma HLS INTERFACE s_axilite port=done bundle=control
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    // Local buffers for data processing
    static ap_uint<32> local_input[NUMBER_OF_INPUT_WORDS];
    static ap_uint<32> local_output[NUMBER_OF_OUTPUT_WORDS];
    
    #pragma HLS ARRAY_PARTITION variable=local_input complete
    #pragma HLS ARRAY_PARTITION variable=local_output complete

    *done = 0;

    if (start) {
        ap_uint<32> sum = 0;
        int word_cnt;

        // DMA Read: Transfer data from external memory to local buffer
        dma_read_loop: for(word_cnt = 0; word_cnt < NUMBER_OF_INPUT_WORDS; word_cnt++){
            #pragma HLS PIPELINE II=1
            #pragma HLS LOOP_TRIPCOUNT min=4 max=4
            local_input[word_cnt] = input_buffer[input_addr/4 + word_cnt];
        }

        // Process data: Calculate sum of input words
        process_loop: for(word_cnt = 0; word_cnt < NUMBER_OF_INPUT_WORDS; word_cnt++){
            #pragma HLS UNROLL
            #pragma HLS LOOP_TRIPCOUNT min=4 max=4
            sum += local_input[word_cnt];
        }

        // Generate output data
        generate_output_loop: for(word_cnt = 0; word_cnt < NUMBER_OF_OUTPUT_WORDS; word_cnt++){
            #pragma HLS UNROLL
            #pragma HLS LOOP_TRIPCOUNT min=4 max=4
            local_output[word_cnt] = sum + word_cnt;
        }

        // DMA Write: Transfer results from local buffer to external memory
        dma_write_loop: for(word_cnt = 0; word_cnt < NUMBER_OF_OUTPUT_WORDS; word_cnt++){
            #pragma HLS PIPELINE II=1
            #pragma HLS LOOP_TRIPCOUNT min=4 max=4
            output_buffer[output_addr/4 + word_cnt] = local_output[word_cnt];
        }

        *done = 1;
    }
}
