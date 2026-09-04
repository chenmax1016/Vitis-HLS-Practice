#include "dot.h"

void dot(
    int a[16],
    int b[16], 
    int *result
){
    #pragma HLS ARRAY_PARTITION variable=a complete
    #pragma HLS ARRAY_PARTITION variable=b complete
    int sum = 0;

    for(int i = 0; i < 16; i++){
    #pragma HLS PIPELINE Off
    #pragma HLS UNROLL factor=4
        sum += a[i] * b[i];
    }

    *result =  sum;
}