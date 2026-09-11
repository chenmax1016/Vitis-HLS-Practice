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

### 4.2 UNROLL Factor = 2

The loop was first synthesized with `UNROLL factor=2` while loop pipelining
remained disabled.

#### Resource Utilization

| Resource | `PIPELINE off` | `UNROLL factor=2` |
|---|---:|---:|
| BRAM | 0 | 0 |
| DSP | 3 | 6 |
| FF | 304 | 565 |
| LUT | 163 | 224 |

The most significant change is the DSP usage, which increases from 3 to 6.
The synthesis report shows two multiplier instances, with each 32-bit
multiplier using three DSP slices. Therefore, unrolling the loop by a factor
of two results in two multiplier datapaths in this experiment.

The FF usage also increases from 304 to 565, while the LUT usage increases
from 163 to 224. These additional resources are required by the expanded
datapath, registers, multiplexing, and control logic introduced by loop
unrolling.

Unlike loop pipelining, which improved throughput without increasing the
number of multiplier instances in the previous experiment, loop unrolling
exposes operations from multiple iterations to HLS simultaneously and can
therefore result in additional arithmetic hardware. This is an example of
spatial parallelism.

#### Performance Comparison

| Metric | `PIPELINE off` | `UNROLL factor=2` |
|---|---:|---:|
| Trip Count | 16 | 8 |
| Iteration Latency | 5 cycles | 5 cycles |
| Loop Latency | 80 cycles | 40 cycles |
| Function Latency | 81 cycles | 41 cycles |
| Function Interval | 82 cycles | 42 cycles |

The trip count decreases from 16 to 8 because each unrolled iteration now
contains the operations from two original loop iterations. At the same time,
the iteration latency remains at 5 cycles.

Since two multiplier datapaths are available, the additional multiplication
exposed by the two-way unrolling can be executed without increasing the
iteration latency. Consequently, the loop latency decreases from 80 to 40
cycles, and the overall function latency decreases from 81 to 41 cycles.

In this case, an unroll factor of two achieves approximately a 2× reduction
in latency at the cost of increased hardware resource utilization.


### 4.3 UNROLL Factor = 4

The unroll factor was then increased from two to four to expose more operations
from the original loop to HLS.

#### Resource Utilization

| Resource | `UNROLL factor=2` | `UNROLL factor=4` |
|---|---:|---:|
| BRAM | 0 | 0 |
| DSP | 6 | 6 |
| FF | 565 | 761 |
| LUT | 224 | 414 |

Unlike the previous experiment, increasing the unroll factor from two to four
does not increase the DSP usage. The synthesis report still shows only two
multiplier instances, corresponding to a total of six DSP slices.

Therefore, an unroll factor of four does not imply that four physical
multipliers will be synthesized. The directive exposes the operations from
four original iterations to the HLS scheduler, while the actual number of
hardware units is determined by scheduling and binding.

Although the number of multipliers remains unchanged, the FF usage increases
from 565 to 761 and the LUT usage increases from 224 to 414. This indicates
that additional datapath and control logic is introduced to schedule the more
heavily unrolled loop over the available hardware.

#### Performance Comparison

| Metric | `UNROLL factor=2` | `UNROLL factor=4` |
|---|---:|---:|
| Trip Count | 8 | 4 |
| Iteration Latency | 5 cycles | 7 cycles |
| Loop Latency | 40 cycles | 28 cycles |
| Function Latency | 41 cycles | 29 cycles |
| Function Interval | 42 cycles | 30 cycles |

The trip count decreases from 8 to 4 as expected. However, because the design
still contains only two multiplier instances, the four multiplication
operations exposed by each unrolled iteration cannot all be mapped to four
separate multipliers.

As a result, the iteration latency increases from 5 to 7 cycles. Nevertheless,
the total loop latency still decreases from 40 to 28 cycles because only four
unrolled iterations are required instead of eight. Therefore, `UNROLL
factor=4` is faster than `factor=2`, even though it does not synthesize
additional multipliers.

The improvement is smaller than the ideal 2× speedup from increasing the
unroll factor from two to four. If the iteration latency had remained at
5 cycles, the expected loop latency would have been approximately 20 cycles.
Instead, the measured loop latency is 28 cycles.

One important limitation is the available memory bandwidth. The input arrays
still provide a limited number of simultaneous memory accesses, while a
four-way unrolled iteration exposes four pairs of input elements that could
potentially be read and processed in parallel. Therefore, the available
computation parallelism cannot necessarily be fully utilized by simply
increasing the unroll factor.

This motivates the next optimization, `ARRAY_PARTITION`, which increases
memory-level parallelism by dividing an array into multiple independently
accessible banks or elements.

## 5. Array Partitioning

### 5.1 Complete Partition

One possible limitation of loop unrolling is memory bandwidth. In the
`UNROLL factor=4` experiment, the input arrays `a` and `b` use the `ap_memory`
interface. The memory interface provides only a limited number of read ports,
so four pairs of input elements cannot necessarily be fetched simultaneously.

Although four-way unrolling exposes four multiplication operations to the HLS
scheduler, the previous synthesis result contains only two multiplier
instances. Therefore, increasing computational parallelism alone does not
guarantee that the exposed parallelism can be fully utilized when the memory
system cannot provide enough data in parallel.

One solution is to partition an array into independently accessible storage
elements or memory banks. Complete partitioning can be applied using:

```cpp
#pragma HLS ARRAY_PARTITION variable=a complete
#pragma HLS ARRAY_PARTITION variable=b complete
```
Complete partitioning separates every element of an array. In this experiment,
the original ap_memory array interfaces are transformed into individual
scalar ap_none ports such as a_0, a_1, ..., a_15. As a result, multiple
array elements can be accessed independently without competing for the same
memory ports.

| Resource | `UNROLL factor=4` | Complete Partition |
| -------- | ----------------: | -----------------: |
| BRAM     |                 0 |                  0 |
| DSP      |                 6 |                 12 |
| FF       |               761 |               2143 |
| LUT      |               414 |                538 |

After complete partitioning, the DSP usage increases from 6 to 12. The
synthesis report now shows four multiplier instances, with each 32-bit
multiplier using three DSP slices. This indicates that the four-way unrolled
loop can now be mapped to four multiplier datapaths.

However, this additional parallelism comes with a significant hardware cost.
The FF usage increases from 761 to 2143, while the LUT usage increases from
414 to 538. In particular, complete partitioning removes the original memory
structure and exposes all array elements as independently accessible values,
which requires substantially more register and interface resources in this
experiment.

| Metric            | `UNROLL factor=4` | Complete Partition |
| ----------------- | ----------------: | -----------------: |
| Trip Count        |                 4 |                  4 |
| Iteration Latency |          7 cycles |           5 cycles |
| Loop Latency      |         28 cycles |          20 cycles |


The trip count remains at 4 because the unroll factor has not changed. However,
the iteration latency decreases from 7 to 5 cycles. With sufficient independent
data access, HLS is able to instantiate four multiplier datapaths and exploit
more of the parallelism exposed by loop unrolling.

Consequently, the loop latency decreases from 28 to 20 cycles. This experiment
demonstrates that computational parallelism and memory bandwidth must be
considered together. Increasing the unroll factor exposes more parallel
operations, while array partitioning can provide the memory-level parallelism
required to support them.

### 5.2 Cyclic Partition

Although complete partitioning provides the highest degree of independent
access, its hardware cost is significant. In the previous experiment, all
16 elements of each input array were completely separated, resulting in a
large number of registers.

However, the four-way unrolled loop does not require all 16 elements to be
accessed simultaneously. Each unrolled iteration only requires four consecutive
elements from each input array. Therefore, completely partitioning the array is
unnecessary.

A more resource-efficient solution is cyclic partitioning:

```cpp
#pragma HLS ARRAY_PARTITION variable=a cyclic factor=4
#pragma HLS ARRAY_PARTITION variable=b cyclic factor=4
```

With a cyclic partition factor of four, the array is divided into four
independently accessible memory banks:

```text
Bank 0: a[0],  a[4],  a[8],  a[12]
Bank 1: a[1],  a[5],  a[9],  a[13]
Bank 2: a[2],  a[6],  a[10], a[14]
Bank 3: a[3],  a[7],  a[11], a[15]
```

Four consecutive elements are therefore distributed across four different
banks and can be accessed in parallel.

#### Resource Utilization

| Resource | Complete Partition | Cyclic Factor=4 |
|---|---:|---:|
| BRAM | 0 | 0 |
| DSP | 12 | 12 |
| FF | 2143 | 1120 |
| LUT | 538 | 406 |

Both implementations use 12 DSP slices, indicating that cyclic partitioning
still provides sufficient memory bandwidth for the four multiplier datapaths.

The major advantage of cyclic partitioning is the reduction in storage and
control resources. With complete partitioning, the synthesis report shows
1483 FFs under the `Register` category because the array elements are exposed
as independently accessible scalar values. In contrast, the cyclic-partitioned
design uses only 460 FFs in the `Register` category because the arrays remain
organized as four memory banks instead of being completely separated.

As a result, the total FF usage decreases from 2143 to 1120, while LUT usage
decreases from 538 to 406. Therefore, cyclic partitioning achieves the required
four-way memory parallelism with substantially lower hardware cost.

#### Performance Comparison

| Metric | Complete Partition | Cyclic Factor=4 |
|---|---:|---:|
| Trip Count | 4 | 4 |
| Iteration Latency | 5 cycles | 6 cycles |
| Loop Latency | 20 cycles | 24 cycles |
| Function Latency | 21 cycles | 25 cycles |
| Function Interval | 22 cycles | 26 cycles |

The cyclic-partitioned design has a slightly higher iteration latency than the
complete-partitioned design. The iteration latency increases from 5 to 6 cycles,
and the loop latency increases from 20 to 24 cycles.

This is the trade-off of using a banked memory structure. Complete partitioning
provides direct independent access to every element, while cyclic partitioning
retains address generation and banked memory access. Although this introduces
a small latency overhead, it significantly reduces FF and LUT usage while still
supporting four parallel multiplier datapaths.

Therefore, cyclic partitioning provides a better balance between memory
parallelism and hardware cost for this access pattern.

## 6. Task-Level Dataflow

### 6.1 Load-Compute-Store Architecture

The previous experiments mainly focused on loop-level parallelism. Operations
within a loop were optimized using pipelining, unrolling, and memory
partitioning. However, realistic applications usually consist of multiple
processing stages rather than a single computational loop.

A common organization is the **Load-Compute-Store** architecture, which divides
the processing flow into three tasks:

```text
Input
  |
  v
Load
  |
  v
Local Input Buffer
  |
  v
Compute
  |
  v
Local Output Buffer
  |
  v
Store
  |
  v
Output
```

The **Load** task reads input data from an external interface and writes it into
a local buffer. After the required input data is available, the **Compute** task
performs the arithmetic operations and writes the results into another local
buffer. Finally, the **Store** task reads the output buffer and transfers the
results to the output interface.

Separating the design into multiple tasks provides an opportunity for
**task-level parallelism**. Instead of optimizing only the iterations inside
each task, HLS can potentially execute Load, Compute, and Store concurrently.


### 6.2 Sequential Execution

As a baseline, the loops inside the Load, Compute, and Store tasks are
pipelined with:

```cpp
#pragma HLS PIPELINE II=1
```

However, `DATAFLOW` is not enabled at the top level. Therefore, although the
loops inside each task are pipelined, the three tasks themselves do not overlap
at the task level.

Conceptually, one transaction is processed as:

```text
Time ---------------------------------------------------------->

Load        [----------------]
Compute                       [----------------]
Store                                           [----------------]
```

The synthesis report gives the following performance:

| Stage | Latency | Interval |
|---|---:|---:|
| Load | 18 cycles | 17 cycles |
| Compute | 18 cycles | 17 cycles |
| Store | 18 cycles | 17 cycles |
| **Top Level** | **59 cycles** | **60 cycles** |

Each individual task has a latency of 18 cycles. However, because the tasks
execute without task-level overlapping, the top-level function requires 59
cycles to process one transaction and has an interval of 60 cycles.

Therefore, pipelining the loops inside individual tasks improves their internal
throughput, but does not by itself create parallel execution between different
tasks.

#### Resource Utilization

| Resource | Sequential |
|---|---:|
| BRAM | 0 |
| DSP | 0 |
| FF | 111 |
| LUT | 310 |

The three processing stages are synthesized as separate hardware modules.
However, without task-level dataflow, these modules are not used concurrently
for different transactions. For example, while the Compute stage is operating,
the Load and Store stages cannot simultaneously process the next and previous
transactions at the top level.

This leaves an opportunity to improve hardware utilization over time by
overlapping the execution of the three tasks.


### 6.3 DATAFLOW

In the sequential implementation, only one stage of a transaction is active at
a time. For example, after Load finishes processing an input array, Compute
starts processing that array before the design proceeds to Store.

However, these tasks operate on different data buffers. Once the first input
has been loaded and moves to the Compute stage, the Load stage can begin
processing the next input. Similarly, while Compute processes the second input,
Store can write the result of the previous input.

Task-level overlapping can be enabled by placing the following directive in the
top-level function:

```cpp
#pragma HLS DATAFLOW
```

The execution can then be represented as:

```text
Time ---------------------------------------------------------->

Load       [ Data 1 ][ Data 2 ][ Data 3 ]
Compute             [ Data 1 ][ Data 2 ][ Data 3 ]
Store                          [ Data 1 ][ Data 2 ][ Data 3 ]
```

Different transactions can now occupy different stages simultaneously.

#### Performance Comparison

| Metric | Sequential | DATAFLOW |
|---|---:|---:|
| Top Latency | 59 cycles | 56 cycles |
| Top Interval | 60 cycles | 16 cycles |
| Load Latency | 18 cycles | 18 cycles |
| Compute Latency | 18 cycles | 18 cycles |
| Store Latency | 18 cycles | 18 cycles |

The latency of the individual tasks remains unchanged at 18 cycles. The
top-level latency also changes only slightly, from 59 to 56 cycles.

The major improvement is the **top-level interval**, which decreases from
60 cycles to 16 cycles. This means that after the dataflow pipeline reaches
steady-state operation, the design can start processing a new 16-element
transaction every 16 cycles instead of every 60 cycles.

Therefore, the main benefit of `DATAFLOW` in this experiment is not reducing
the latency of an individual transaction, but significantly increasing the
throughput by overlapping the execution of different tasks.

#### Resource Utilization

| Resource | Sequential | DATAFLOW |
|---|---:|---:|
| BRAM | 0 | 2 |
| DSP | 0 | 0 |
| FF | 111 | 98 |
| LUT | 310 | 222 |

The most noticeable architectural change is the BRAM usage, which increases
from 0 to 2. The dataflow implementation requires intermediate storage between
concurrently executing tasks so that one stage can produce data while another
stage consumes previously produced data.

Interestingly, the total FF and LUT usage does not increase in this experiment.
FF usage decreases from 111 to 98, while LUT usage decreases from 310 to 222.
This shows that enabling `DATAFLOW` does not simply duplicate the existing
hardware. Instead, HLS changes the scheduling, communication, buffering, and
control structure of the top-level design.

The result demonstrates the difference between loop-level and task-level
pipelining:

- `PIPELINE` overlaps **iterations within a task**.
- `DATAFLOW` overlaps **different tasks across transactions**.

By combining both techniques, each individual stage can process data
efficiently while multiple stages operate concurrently.
## 7. Streaming Architecture
### 7.1 Array Buffer vs hls::stream
### 7.2 FIFO
### 7.3 Blocking Read/Write
### 7.4 Backpressure
### 7.5 Pipeline Bottleneck

## 8. Summary