g++ bombix.cpp -std=c++14 -DNDEBUG -fopenmp -O3 -o bombix
g++ -I /usr/include/eigen3 latuile.cpp binpack.cpp compact_frame.cpp compact_rectangles.cpp fit_together.cpp KMeansRexCore.cpp MPD_Arc.cpp MyRect.cpp optimize_rectangle_positions.cpp permutation.cpp stair_steps.cpp swap_rectangles.cpp WidgetContext.cpp FunctionTimer.cpp -std=c++14 -DNDEBUG -fopenmp -O3 -o latuile
