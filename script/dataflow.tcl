open_project dataflow_hls

set_top dataflow

add_files src/dataflow.cpp
add_files -tb test_bench/dataflow_tb.cpp

open_solution "solution_1"

set_part {xc7a100t-csg324-1}

create_clock -period 10 -name default

csim_design
csynth_design

exit