#ifndef COMPACT_RECTANGLES
#define COMPACT_RECTANGLES


#include "MyRect.h"
#include "MPD_Arc.h"
#include <vector>


bool compact_rectangles(std::vector<MyRect> &rectangles, const std::vector<std::vector<MPD_Arc> >& adjacency_list) ;

void test_compact_rectangles() ;


#endif