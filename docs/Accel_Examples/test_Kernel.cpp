// test_Kernel.cpp
// Testbench for the vadd kernel in kernel_burst.cpp (compares against a software reference).
//
// Setup:
//   source ~/Xilinx/2025.1/Vitis/settings64.sh   # adjust to your install; sets $XILINX_VITIS
// C simulation:
//   g++ -O2 -I$XILINX_VITIS/include -o tb_vadd_burst kernel_burst.cpp test_Kernel.cpp
//   ./tb_vadd_burst
// See kernel.cpp for using this testbench with the simple kernel.

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>  // For INT_MAX, INT_MIN
#include <time.h>

// Declare your kernel function
void vadd(volatile int* A, volatile int* B, volatile int* C, int N);

// Simple reference implementation
void vadd_reference(int* A, int* B, int* C, int N) {
    for (int i = 0; i < N; i++) {
        C[i] = A[i] + B[i];
    }
}

// Verify results
int verify_results(int* C_hw, int* C_ref, int N) {
    for (int i = 0; i < N; i++) {
        if (C_hw[i] != C_ref[i]) {
            printf("ERROR at index %d: HW=%d, REF=%d\n", i, C_hw[i], C_ref[i]);
            return 0;  // Failed
        }
    }
    return 1;  // Passed
}

int main() {
    printf("=== Simple Vector Addition Testbench ===\n");
    
    // Test parameters
    int N = 256;
    
    // Allocate arrays
    int* A = (int*)malloc(N * sizeof(int));
    int* B = (int*)malloc(N * sizeof(int));
    int* C_hw = (int*)malloc(N * sizeof(int));
    int* C_ref = (int*)malloc(N * sizeof(int));
    
    // Initialize test data
    for (int i = 0; i < N; i++) {
        A[i] = i;
        B[i] = i * 2;
        C_hw[i] = 0;
        C_ref[i] = 0;
    }
    
    printf("Testing with N = %d\n", N);
    
    // Run reference
    vadd_reference(A, B, C_ref, N);
    
    // Run HLS kernel
    vadd(A, B, C_hw, N);
    
    // Verify
    if (verify_results(C_hw, C_ref, N)) {
        printf("✓ Test PASSED\n");
    } else {
        printf("✗ Test FAILED\n");
    }
    
    // Print first few results for inspection
    printf("\nFirst 10 results:\n");
    for (int i = 0; i < 10; i++) {
        printf("A[%d]=%d, B[%d]=%d, C[%d]=%d\n", i, A[i], i, B[i], i, C_hw[i]);
    }
    
    // Cleanup
    free(A);
    free(B);
    free(C_hw);
    free(C_ref);
    
    return 0;
}


/*#include <iostream>
#include "kernel.cpp"

int main() {
    const int N = 1024; // <= MAX_SIZE
    int A[MAX_SIZE], B[MAX_SIZE], C[MAX_SIZE], C_ref[MAX_SIZE];

    // Initialize arrays
    for (int i = 0; i < N; i++) {
        A[i] = i;
        B[i] = 2*i;
        C[i] = 0;
        C_ref[i] = A[i] + B[i];
    }

    // Call HLS kernel
    vadd(A, B, C, N);

    // Verify results
    int errors = 0;
    for (int i = 0; i < N; i++) {
        if (C[i] != C_ref[i]) {
            if (errors < 10) std::cout << "Mismatch at " << i << std::endl;
            errors++;
        }
    }

    if (errors == 0) std::cout << "PASSED cosimulation\n";
    else std::cout << "FAILED cosimulation with " << errors << " errors\n";

    return 0;
}
*/