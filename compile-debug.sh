g++ -std=c++14 -g -c latuile.cpp
g++ -std=c++14 -g -c binpack.cpp
g++ -std=c++14 -g -c compact_frame.cpp
g++ -std=c++14 -g -c compact_rectangles.cpp
g++ -std=c++14 -g -c fit_together.cpp
g++ -I /usr/include/eigen3 -std=c++14 -g -c KMeansRexCore.cpp
g++ -std=c++14 -g -c MPD_Arc.cpp
g++ -std=c++14 -g -c MyRect.cpp
g++ -std=c++14 -g -c optimize_rectangle_positions.cpp
g++ -std=c++14 -g -c permutation.cpp
g++ -I /usr/include/eigen3 -std=c++14 -g -c stair_steps.cpp
g++ -std=c++14 -g -c swap_rectangles.cpp
g++ -std=c++14 -g -c WidgetContext.cpp
g++ -std=c++14 -g -c FunctionTimer.cpp

g++ -o latuile-d latuile.o binpack.o compact_frame.o compact_rectangles.o fit_together.o KMeansRexCore.o MPD_Arc.o MyRect.o optimize_rectangle_positions.o permutation.o stair_steps.o swap_rectangles.o WidgetContext.o FunctionTimer.o

g++ bombix.cpp -std=c++14 -g -fopenmp -o bombix-d
