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
### 2.2 Synthesis Result
### 2.3 Latency, Interval, and II
### 2.4 Hardware Resource Mapping

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