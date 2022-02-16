cmake_minimum_required(VERSION 3.0)
project (linkedboxdraw)
set(CMAKE_BUILD_TYPE Release)

SET(CMAKE_CXX_COMPILER "g++-11" )
SET(CMAKE_CXX_FLAGS "-fmodules-ts -std=c++2b -fopenmp -O3" )

add_executable(bombix bombix-origine.cpp)

find_package (Eigen3 3.3 REQUIRED NO_MODULE)

#include_directories(/usr/include/eigen3)

set(SOURCES latuile.cpp latuile_test_json_output.cpp binpack.cpp compact_frame.cpp compact_rectangles.cpp fit_together.cpp KMeansRexCore.cpp MPD_Arc.cpp MyRect.cpp optimize_rectangle_positions.cpp permutation.cpp stair_steps.cpp swap_rectangles.cpp WidgetContext.cpp FunctionTimer.cpp)

add_executable(latuile ${SOURCES})
target_link_libraries (latuile Eigen3::Eigen)

enable_testing()

add_test(latuile latuile)
add_test(bombix bombix)
