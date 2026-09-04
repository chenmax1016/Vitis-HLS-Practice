#include "dataflow.h"

void load(int in[16], int buffer[16]){
    for(int i = 0; i < 16; i++){
#pragma HLS PIPELINE II=1
        buffer[i] = in[i];
    }
}

void compute(int in_buffer[16], int out_buffer[16]){
    for(int i = 0; i < 16; i++){
#pragma HLS PIPELINE II=1
        out_buffer[i] = in_buffer[i] * 2;
    }
}

void store(int buffer[16], int out[16]){
    for(int i = 0; i < 16; i++){
#pragma HLS PIPELINE II=1
        out[i] = buffer[i];
    }

}


void dataflow(int in[16], int out[16]){
#pragma HLS DATAFLOW
    int buffer1[16];
    int buffer2[16];

    load(in, buffer1);
    compute(buffer1, buffer2);
    store(buffer2, out);
}