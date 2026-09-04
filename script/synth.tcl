open_project dot_hls

set_top dot

add_files src/dot.cpp
add_files -tb test_bench/dot_tb.cpp

open_solution "solution_1"

set_part {xc7a100t-csg324-1}

create_clock -period 10 -name default

csim_design
csynth_design

exit