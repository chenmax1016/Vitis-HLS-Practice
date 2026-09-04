#include "dot.h"

void dot(
    int a[N],
    int b[N], 
    int *result
){
    #pragma HLS ARRAY_PARTITION variable=a cyclic factor=4
    #pragma HLS ARRAY_PARTITION variable=b cyclic factor=4
    int sum = 0;

    for(int i = 0; i < N; i++){
    #pragma HLS PIPELINE Off
    #pragma HLS UNROLL factor=4
        sum += a[i] * b[i];
    }

    *result =  sum;
}