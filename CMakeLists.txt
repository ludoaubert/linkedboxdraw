cmake_minimum_required(VERSION 3.0)
project (linkedboxdraw)
set(CMAKE_BUILD_TYPE Release)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fopenmp -O3" )

add_executable(bombix bombix.cpp)

find_package (Eigen3 3.3 REQUIRED NO_MODULE)

#include_directories(/usr/include/eigen3)

set(SOURCES latuile.cpp binpack.cpp compact_frame.cpp compact_rectangles.cpp fit_together.cpp KMeansRexCore.cpp MPD_Arc.cpp MyRect.cpp optimize_rectangle_positions.cpp permutation.cpp stair_steps.cpp swap_rectangles.cpp WidgetContext.cpp FunctionTimer.cpp)

add_executable(latuile ${SOURCES})
target_link_libraries (latuile Eigen3::Eigen)

enable_testing()

add_test(latuile latuile)
add_test(bombix bombix)
