#include "dot.h"

void dot(
    int a[16],
    int b[16], 
    int *result
){
    int sum = 0;

    for(int i = 0; i < 16; i++){
        sum += a[i] * b[i];
    }

    *result =  sum;
}