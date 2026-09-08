# Vitis HLS Optimization Practice

## 1. Introduction

### Goal of the Experiments

Vitis High-Level Synthesis (HLS) provides a higher level of abstraction for
hardware design compared with traditional hardware description languages
(HDLs), such as Verilog. Instead of explicitly describing the behavior of a
circuit at the register-transfer level (RTL), Vitis HLS allows engineers to
describe hardware functionality using C/C++ and automatically synthesizes the
design into RTL.

This approach allows engineers to focus more on the overall architecture and
algorithm without manually specifying the behavior of every hardware component
on every clock cycle. In addition, C++ features such as functions, templates,
and generic programming can improve the modularity and reusability of hardware
designs.

However, using a higher-level programming model does not eliminate the
importance of the underlying hardware implementation. Scheduling, resource
sharing, memory bandwidth, parallelism, and hardware resource utilization still
have significant effects on the performance and cost of the synthesized
circuit.

Therefore, the goal of these experiments is to understand the connection
between high-level C/C++ descriptions and their resulting hardware
implementations. Through a series of small experiments, this project studies
how different HLS optimization techniques affect latency, throughput, memory
architecture, and FPGA resource utilization.

### Experimental Environment

- **Target FPGA:** AMD/Xilinx Artix-7 XC7A100T
- **Tool:** Vitis HLS 2025.1
- **Target clock period:** 10 ns

### Topics

The experiments focus on several fundamental concepts in Vitis HLS:

- Scheduling and binding
- Loop pipelining
- Loop unrolling
- Array partitioning and memory bandwidth
- Task-level dataflow
- Streaming architecture with `hls::stream`
- FIFO buffering and backpressure

## 2. Baseline Dot Product
### 2.1 Implementation
```cpp
void dot(
    int a[16],
    int b[16], 
    int *result
){
    int sum = 0;

    for(int i = 0; i < 16; i++){
#pragma HLS PIPELINE off
        sum += a[i] * b[i];
    }

    *result =  sum;
}
```
The function takes two memory array as input. Its goal is to compute the dot product of these two arrays. In each iteration loop, address increments to fetch the correct element, then add the product of two elements to sum. After the loop finishes, sum is write to the output register port. `#pragma HLS PIPELINE off` is to avoid HLS pipeline optimization between iteration i and iteration i+1. This is the simplest baseline of a dot product hardware implementation.   

The function takes two arrays of 16 integers as inputs and compute their dot products. In each iteration loop, the corresponding elements `a[i]` and `b[i]` are read from the input arrays, multiplied together, and accumulated into `sum`.

After all 16 iterations are completed, the final value of `sum` is written to the output through `result` pointer.

The `#pragma HLS PIPELINE off` directive explicitly disables loop pipelining. Therefore, HLS does not attempt to overlap the execution of consecutive loop iterations. This implementation serves as the baseline design for studying how HLS scheduling, pipelining, and resource allocation affect the generated hardware.

### 2.2 Performance

The target clock period is 10 ns, corresponding to a clock frequency of 100 MHz. The synthesis report shows that the top-level function has a latency of 81 cycles, or approximately 0.81 μs.

| Metric                 |    Result |
| ---------------------- | --------: |
| Target Clock Period    |     10 ns |
| Estimated Clock Period |   6.86 ns |
| Function Latency       | 81 cycles |
| Loop Latency           | 80 cycles |
| Iteration Latency      |  5 cycles |
| Trip Count             |        16 |
| Pipelined              |        No |

The loop executes 16 iterations, and each iteration has a latency of 5 cycles. Since loop pipelining is disabled, consecutive iterations are not overlapped. Therefore, the total loop latency is

$$
16 \times 5 = 80 \text{ cycles}.
$$

The additional cycle outside the loop results in a total function latency of 81 cycles.

This result provides the baseline performance for later experiments. In the following sections, loop pipelining will be introduced to overlap consecutive iterations and reduce the total execution latency.

### 2.3 Resource Utilization

The synthesized baseline design uses the following FPGA resources:

| Resource | Used | Available |
| -------- | ---: | --------: |
| BRAM_18K |    0 |       270 |
| DSP      |    3 |       240 |
| FF       |  304 |    126800 |
| LUT      |  163 |     63400 |

The main arithmetic hardware is a single 32-bit signed multiplier, reported as `mul_32s_32s_32_2_1_U1`. Although there is only one multiplier instance, it consumes three DSP resources to implement the 32-bit multiplication.

The accumulation operation `sum += ...` is implemented separately using LUTs. Since loop pipelining is disabled, the same multiplier and accumulator hardware can be reused across the 16 loop iterations instead of requiring separate arithmetic hardware for every iteration.

This demonstrates an important characteristic of HLS: the number of operations in the C code does not directly correspond to the number of hardware units. Scheduling and resource sharing determine how the operations are mapped onto the FPGA.

### 2.4 Interface

Vitis HLS synthesizes the C function arguments into RTL interfaces. The generated interfaces for the baseline design are:

| C Object         | Interface Protocol | Description                            |
| ---------------- | ------------------ | -------------------------------------- |
| `a[16]`          | `ap_memory`        | Input memory interface                 |
| `b[16]`          | `ap_memory`        | Input memory interface                 |
| `result`         | `ap_vld`           | Output data with a valid signal        |
| Function control | `ap_ctrl_hs`       | Controls the execution of the function |

The input arrays `a` and `b` are implemented using the `ap_memory` protocol. For example, array `a` is represented by the RTL signals `a_address0`, `a_ce0`, and `a_q0`. The generated hardware provides an address and chip-enable signal to access the memory and receives the corresponding 32-bit data through `a_q0`.

Similarly, array `b` uses its own `ap_memory` interface.

The output pointer `result` is synthesized using the `ap_vld` protocol. The 32-bit `result` port carries the computed dot product, while `result_ap_vld` indicates when the output value is valid.

The function itself uses the `ap_ctrl_hs` control protocol, which generates signals such as `ap_start`, `ap_done`, `ap_idle`, and `ap_ready` to control and monitor the execution of the hardware module.


## 3. Loop Pipelining
### 3.1 PIPELINE Directive
### 3.2 Pipeline Disabled
### 3.3 Comparison
### 3.4 Temporal Parallelism

## 4. Loop Unrolling
### 4.1 UNROLL factor=2
### 4.2 UNROLL factor=4
### 4.3 Spatial Parallelism
### 4.4 Memory Bandwidth Bottleneck

## 5. Array Partitioning
### 5.1 Complete Partition
### 5.2 Cyclic Partition
### 5.3 Complete vs Cyclic
### 5.4 Memory Banking and Access Pattern

## 6. Task-Level Dataflow
### 6.1 Load-Compute-Store Architecture
### 6.2 Sequential Execution
### 6.3 DATAFLOW
### 6.4 Latency vs Interval

## 7. Streaming Architecture
### 7.1 Array Buffer vs hls::stream
### 7.2 FIFO
### 7.3 Blocking Read/Write
### 7.4 Backpressure
### 7.5 Pipeline Bottleneck

## 8. Summary