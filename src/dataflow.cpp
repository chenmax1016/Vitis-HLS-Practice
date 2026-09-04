#include "dataflow.h"
#include <hls_stream.h>

void load(int in[16], hls::stream<int>& out_stream){
    for(int i = 0; i < 16; i++){
#pragma HLS PIPELINE II=1
        out_stream.write(in[i]);
    }
}

void compute(hls::stream<int> &in_stream, hls::stream<int> &out_stream){
    for(int i = 0; i < 16; i++){
#pragma HLS PIPELINE II=2
        int x = in_stream.read();

        volatile int state=x;
        for(int j = 0; j < 2; j++){
#pragma HLS PIPELINE off
            state = state;
        }

        out_stream.write(x*2);
    }
}

void store(hls::stream<int> &in_stream, int out[16]){
    for(int i = 0; i < 16; i++){
#pragma HLS PIPELINE II=1
        out[i] = in_stream.read();
    }

}


void dataflow(int in[16], int out[16]){
#pragma HLS DATAFLOW
    hls::stream<int> stream1;
    hls::stream<int> stream2;

#pragma HLS STREAM variable=stream1 depth=2
#pragma HLS STREAM variable=stream2 depth=2

    load(in, stream1);
    compute(stream1, stream2);
    store(stream2, out);
}