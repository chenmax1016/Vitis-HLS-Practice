#include "dot.h"

void dot(
    int a[16],
    int b[16], 
    int *result
){
    int sum = 0;

    for(int i = 0; i < 16; i++){
#pragma HLS PIPELINE off
#pragma HLS UNROLL factor=4
        sum += a[i] * b[i];
    }

    *result =  sum;
}