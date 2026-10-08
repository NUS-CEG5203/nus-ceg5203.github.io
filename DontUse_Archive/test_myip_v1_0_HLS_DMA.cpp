/*
----------------------------------------------------------------------------------
--	(c) Rajesh C Panicker, NUS,
--  Description : Self-checking testbench for AXI DMA Coprocessor (HLS) implementing the sum of 4 numbers
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

#include <stdio.h>
#include "ap_int.h"

/***************** Coprocessor function declaration *********************/

void myip_v1_0_HLS_DMA(
    ap_uint<32>* input_buffer,
    ap_uint<32>* output_buffer,
    ap_uint<32> input_addr,
    ap_uint<32> output_addr,
    ap_uint<1> start,
    ap_uint<1>* done
);

/***************** Macros *********************/
#define NUMBER_OF_INPUT_WORDS 4
#define NUMBER_OF_OUTPUT_WORDS 4
#define NUMBER_OF_TEST_VECTORS 2
#define MEMORY_SIZE 1024  // Size of simulated memory

/************************** Variable Definitions *****************************/
ap_uint<32> test_input_memory[NUMBER_OF_TEST_VECTORS*NUMBER_OF_INPUT_WORDS] = {0x01, 0x02, 0x03, 0x04, 0x02, 0x03, 0x04, 0x05};
ap_uint<32> test_result_expected_memory[NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS];
ap_uint<32> result_memory[NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS];

// Simulated system memory
ap_uint<32> system_memory[MEMORY_SIZE];

/*****************************************************************************
* Main function
******************************************************************************/
int main()
{
    int word_cnt, test_case_cnt = 0;
    int success;
    ap_uint<1> done;
    
    // Initialize system memory
    for(int i = 0; i < MEMORY_SIZE; i++) {
        system_memory[i] = 0;
    }

    /************** Run a software version of the hardware function to validate results ************/
    // instead of hard-coding the results in test_result_expected_memory
    ap_uint<32> sum;
    for (test_case_cnt = 0; test_case_cnt < NUMBER_OF_TEST_VECTORS; test_case_cnt++) {
        sum = 0;
        for (word_cnt = 0; word_cnt < NUMBER_OF_INPUT_WORDS; word_cnt++) {
            sum += test_input_memory[word_cnt + test_case_cnt * NUMBER_OF_INPUT_WORDS];
        }
        for (word_cnt = 0; word_cnt < NUMBER_OF_OUTPUT_WORDS; word_cnt++) {
            test_result_expected_memory[word_cnt + test_case_cnt * NUMBER_OF_OUTPUT_WORDS] = sum + word_cnt;
        }
    }

    for (test_case_cnt = 0; test_case_cnt < NUMBER_OF_TEST_VECTORS; test_case_cnt++) {

        /******************** Setup memory for test case ***********************/
        
        // Calculate memory addresses for this test case
        ap_uint<32> input_base_addr = test_case_cnt * NUMBER_OF_INPUT_WORDS * 4;  // byte address
        ap_uint<32> output_base_addr = (NUMBER_OF_TEST_VECTORS * NUMBER_OF_INPUT_WORDS + 
                                       test_case_cnt * NUMBER_OF_OUTPUT_WORDS) * 4;  // byte address
        
        printf("Test case %d: Input addr = 0x%08X, Output addr = 0x%08X\n", 
               test_case_cnt, (unsigned int)input_base_addr, (unsigned int)output_base_addr);

        /******************** Load input data into system memory ***********************/
        
        printf("Loading input data for test case %d...\n", test_case_cnt);
        
        for (word_cnt = 0; word_cnt < NUMBER_OF_INPUT_WORDS; word_cnt++) {
            int memory_index = (input_base_addr / 4) + word_cnt;
            system_memory[memory_index] = test_input_memory[word_cnt + test_case_cnt * NUMBER_OF_INPUT_WORDS];
            printf("  Input[%d] = 0x%08X at memory[%d]\n", 
                   word_cnt, (unsigned int)system_memory[memory_index], memory_index);
        }

        /********************* Call the hardware function (invoke the co-processor) ***************/
        
        printf("Calling hardware function for test case %d...\n", test_case_cnt);
        
        // Reset done flag
        done = 0;
        
        // Call the coprocessor function
        myip_v1_0_HLS_DMA(
            system_memory,          // input_buffer (points to system memory)
            system_memory,          // output_buffer (same memory, different offset)
            input_base_addr,        // input_addr
            output_base_addr,       // output_addr
            1,                      // start = 1
            &done                   // done flag
        );
        
        // Check if processing completed
        if (done == 1) {
            printf("Processing completed for test case %d\n", test_case_cnt);
        } else {
            printf("ERROR: Processing not completed for test case %d\n", test_case_cnt);
            return 1;
        }

        /******************** Read output data from system memory ***********************/
        
        printf("Reading output data for test case %d...\n", test_case_cnt);
        
        for (word_cnt = 0; word_cnt < NUMBER_OF_OUTPUT_WORDS; word_cnt++) {
            int memory_index = (output_base_addr / 4) + word_cnt;
            result_memory[word_cnt + test_case_cnt * NUMBER_OF_OUTPUT_WORDS] = system_memory[memory_index];
            printf("  Output[%d] = 0x%08X from memory[%d]\n", 
                   word_cnt, (unsigned int)result_memory[word_cnt + test_case_cnt * NUMBER_OF_OUTPUT_WORDS], memory_index);
        }
    }

    /************************** Checking correctness of results *****************************/

    success = 1;

    /* Compare the expected results with the actual results */
    printf("\nComparing data...\n");
    for (word_cnt = 0; word_cnt < NUMBER_OF_TEST_VECTORS * NUMBER_OF_OUTPUT_WORDS; word_cnt++) {
        printf("Expected[%d] = 0x%08X, Actual[%d] = 0x%08X\n", 
               word_cnt, (unsigned int)test_result_expected_memory[word_cnt],
               word_cnt, (unsigned int)result_memory[word_cnt]);
        
        if (result_memory[word_cnt] != test_result_expected_memory[word_cnt]) {
            success = 0;
            printf("MISMATCH at index %d!\n", word_cnt);
        }
    }

    if (success != 1) {
        printf("\nTest Failed\n");
        return 1;
    }

    printf("\nTest Success\n");
    return 0;
}
