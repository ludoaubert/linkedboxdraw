cmake_minimum_required(VERSION 2.8.9)
project (linkedboxdraw)
set(CMAKE_BUILD_TYPE Release)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fopenmp -O3" )

add_executable(bombix bombix.cpp)

target_include_directories(/usr/include/eigen3)

set(SOURCES latuile.cpp binpack.cpp compact_frame.cpp compact_rectangles.cpp fit_together.cpp KMeansRexCore.cpp MPD_Arc.cpp MyRect.cpp optimize_rectangle_positions.cpp permutation.cpp stair_steps.cpp swap_rectangles.cpp WidgetContext.cpp FunctionTimer.cpp)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fopenmp -O3" )

add_executable(latuile ${SOURCES})
