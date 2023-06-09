cmake_minimum_required(VERSION 3.0)
project (linkedboxdraw)
set(CMAKE_BUILD_TYPE Release)

SET(CMAKE_CXX_COMPILER "g++-12" )
SET(CMAKE_CXX_FLAGS "-fmodules-ts -std=c++2b -fopenmp -O3" )

list(APPEND CMAKE_MODULE_PATH "deps/tbb/cmake/")
find_package(TBB REQUIRED)

add_executable(bombix bombix-origine.cpp)
add_executable(bombix-experimental bombix-origine-experimental.cpp)

set (SOURCES holes.cpp MyRect.cpp compact_frame.cpp FunctionTimer.cpp latuile_test_json_output.cpp thread_pool.cpp)
add_executable(holes ${SOURCES})

set (SOURCES rect_union.cpp)
add_executable(rect_union ${SOURCES})

set (SOURCES holes2.cpp MyRect.cpp FunctionTimer.cpp)
add_executable(holes2 ${SOURCES})

set (SOURCES holes3.cpp MyRect.cpp FunctionTimer.cpp)
add_executable(holes3 ${SOURCES})
target_link_libraries(holes3 PUBLIC TBB::tbb)

set (SOURCES holes4.cpp MyRect.cpp FunctionTimer.cpp)
add_executable(holes4 ${SOURCES})

set (SOURCES holes5.cpp MyRect.cpp FunctionTimer.cpp)
add_executable(holes5 ${SOURCES})
target_link_libraries(holes5 PUBLIC TBB::tbb)

set (SOURCES holes6.cpp MyRect.cpp FunctionTimer.cpp)
add_executable(holes6 ${SOURCES})
target_link_libraries(holes6 PUBLIC TBB::tbb)

set (SOURCES holes7.cpp MyRect.cpp FunctionTimer.cpp)
add_executable(holes7 ${SOURCES})
target_link_libraries(holes7 PUBLIC TBB::tbb)

set (SOURCES holes8.cpp MyRect.cpp FunctionTimer.cpp)
add_executable(holes8 ${SOURCES})
target_link_libraries(holes8 PUBLIC TBB::tbb)

set (SOURCES par_unseq.cpp)
add_executable(par_unseq ${SOURCES})
target_link_libraries(par_unseq PUBLIC TBB::tbb)

set (SOURCES snap.cpp MyRect.cpp)
add_executable(snap ${SOURCES})

find_package (Eigen3 3.3 REQUIRED NO_MODULE)

#include_directories(/usr/include/eigen3)

set(SOURCES latuile.cpp latuile_test_json_output.cpp thread_pool.cpp binpack.cpp compact_frame.cpp compact_rectangles.cpp fit_together.cpp KMeansRexCore.cpp MyRect.cpp optimize_rectangle_positions.cpp permutation.cpp stair_steps.cpp swap_rectangles.cpp WidgetContext.cpp FunctionTimer.cpp)

add_executable(latuile ${SOURCES})
target_link_libraries (latuile Eigen3::Eigen)

enable_testing()

add_test(latuile latuile)
add_test(bombix bombix-experimental)
add_test(holes holes)
