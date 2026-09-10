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

The multiplication is mapped to a single DSP-based multiplier, which uses three DSP slices, while the 32-bit accumulation addition is implemented using LUTs. The accumulated value sum is stored in a 32-bit register. Since loop pipelining is disabled, consecutive loop iterations do not overlap in execution. As a result, the loop processes one iteration at a time.

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

In C++, loop iterations are executed sequentially according to the program
semantics. In hardware, however, the operations within an iteration can be
scheduled into multiple stages. Loop pipelining allows different iterations
to overlap their execution across these stages, improving the throughput of
the loop.

The initiation interval (II) represents the number of clock cycles between
the start of two consecutive loop iterations. The following directive requests
an II of one cycle:
```cpp
#pragma HLS PIPELINE II=1
```
### 3.2 Performance Comparison

To observe the effect of loop pipelining, the dot-product loop was synthesized
with pipelining disabled and with a target initiation interval of one.

| Configuration   | Loop Latency | Iteration Latency | Achieved II | Function Latency | Function Interval |
| --------------- | -----------: | ----------------: | ----------: | ---------------: | ----------------: |
| `PIPELINE off`  |    80 cycles |          5 cycles |         N/A |        81 cycles |         82 cycles |
| `PIPELINE II=1` |    19 cycles |          5 cycles |           1 |        21 cycles |         19 cycles |


Without pipelining, each iteration completes before the next iteration starts.
Since each iteration has a latency of 5 cycles and the loop has a trip count of
16, the total loop latency is 80 cycles.

After pipelining is enabled, the iteration latency remains 5 cycles, but the
achieved II becomes 1. Therefore, a new iteration can start every clock cycle
while previous iterations are still in progress. This overlap reduces the loop
latency from 80 cycles to 19 cycles and the overall function latency from 81
cycles to 21 cycles.

This demonstrates that pipelining improves throughput by overlapping loop
iterations rather than reducing the latency of an individual iteration.


### 3.3 Utilization Comparison

Loop pipelining improves performance at the cost of additional control and
register resources.

| Configuration   | BRAM | DSP |  FF | LUT |
| --------------- | ---: | --: | --: | --: |
| `PIPELINE off`  |    0 |   3 | 304 | 163 |
| `PIPELINE II=1` |    0 |   3 | 375 | 198 |
| Difference      |    0 |   0 | +71 | +35 |


The number of DSP slices remains unchanged at three in both designs. The
synthesis reports also show the same single multiplier instance in both
configurations. Therefore, loop pipelining does not duplicate the multiplier
in this experiment.

However, the pipelined design uses more FFs and LUTs. The FF count increases
from 304 to 375, while the LUT count increases from 163 to 198. These additional
resources are associated with the registers and control logic required to
support overlapped execution of loop iterations.

Therefore, this experiment shows an important HLS trade-off: loop pipelining
can significantly improve throughput and latency without necessarily
duplicating arithmetic units, but additional registers and control logic may
be required.

## 4. Loop Unrolling

### 4.1 UNROLL Directive

In the previous experiments, only one multiplier was synthesized regardless of
whether loop pipelining was enabled. Pipelining improved performance by
overlapping the execution of different iterations, but it did not increase the
number of multiplier instances.

Another approach to increasing parallelism is loop unrolling. The dot-product
loop contains multiple multiplication operations from different iterations.
By unrolling the loop, multiple original iterations are expanded into the same
scheduling scope, giving HLS the opportunity to execute multiple operations in
parallel.

For example, the following directive unrolls the loop by a factor of two:

```cpp
#pragma HLS UNROLL factor=2
```

### 4.2 UNROLL factor=2

The loop was first synthesized with UNROLL factor=2 while loop pipelining
remained disabled.

| Metric            | `PIPELINE off` | `UNROLL factor=2` |
| ----------------- | -------------: | ----------------: |
| Trip Count        |             16 |                 8 |
| Iteration Latency |       5 cycles |          5 cycles |
| Loop Latency      |      80 cycles |         40 cycles |
| Function Latency  |      81 cycles |         41 cycles |
| Function Interval |      82 cycles |         42 cycles |

The trip count decreases from 16 to 8 because each unrolled iteration represents
two iterations of the original loop. The iteration latency remains at 5 cycles,
while the loop latency decreases from 80 to 40 cycles. As a result, the overall
function latency is reduced from 81 to 41 cycles.

In this case, the unroll factor of two achieves almost the expected 2×
improvement in latency because the hardware is able to process the additional
operations without increasing the iteration latency.

| Resource | `PIPELINE off` | `UNROLL factor=2` |
| -------- | -------------: | ----------------: |
| BRAM     |              0 |                 0 |
| DSP      |              3 |                 6 |
| FF       |            304 |               565 |
| LUT      |            163 |               224 |

The number of DSP slices increases from 3 to 6. The synthesis report shows two
multiplier instances, with each 32-bit multiplier using three DSP slices.
Therefore, unlike loop pipelining, unrolling by a factor of two results in two
multiplier datapaths in this experiment.

The FF and LUT usage also increases because additional datapath, registers,
multiplexing, and control logic are required to support the unrolled operations.
This demonstrates the basic trade-off of spatial parallelism: lower latency is
achieved by using more hardware resources.


### 4.3 UNROLL factor=4

The unroll factor was then increased from two to four.

| Metric            | `UNROLL factor=2` | `UNROLL factor=4` |
| ----------------- | ----------------: | ----------------: |
| Trip Count        |                 8 |                 4 |
| Iteration Latency |          5 cycles |          7 cycles |
| Loop Latency      |         40 cycles |         28 cycles |
| Function Latency  |         41 cycles |         29 cycles |
| Function Interval |         42 cycles |         30 cycles |

As expected, increasing the unroll factor from two to four further reduces the
trip count from 8 to 4. However, the iteration latency increases from 5 to 7
cycles. Therefore, the loop latency decreases from 40 to 28 cycles rather than
the ideal 20 cycles.

This result shows that increasing the unroll factor does not necessarily produce
a proportional performance improvement. Although four original iterations are
now exposed to the scheduler at once, the available hardware cannot execute all
of their operations fully in parallel.

| Resource | `UNROLL factor=2` | `UNROLL factor=4` |
| -------- | ----------------: | ----------------: |
| BRAM     |                 0 |                 0 |
| DSP      |                 6 |                 6 |
| FF       |               565 |               761 |
| LUT      |               224 |               414 |

Interestingly, the DSP usage remains at 6. The synthesis report still contains
only two multiplier instances instead of four. Therefore, increasing the unroll
factor from two to four does not result in four physical multipliers.

Nevertheless, the design is still faster because the trip count is reduced from
8 to 4. HLS schedules the operations exposed by the four-way unrolled loop over
the available hardware, resulting in a longer iteration latency of 7 cycles but
a lower total loop latency of 28 cycles.

The FF usage increases from 565 to 761 and the LUT usage increases from 224 to
414, indicating that additional logic is still required for the more heavily
unrolled datapath even though the number of multiplier instances does not
increase.

One important limitation is the memory interface. The input arrays still have
limited memory ports, restricting the number of elements that can be read
simultaneously. Therefore, computation parallelism exposed by loop unrolling
cannot necessarily be fully utilized without sufficient memory bandwidth. This
motivates the use of array partitioning in the next experiment.



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