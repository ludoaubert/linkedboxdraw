#ifndef _OPTIMIZE_POSITIONS_
#define _OPTIMIZE_POSITIONS_


#include "MyRect.h"
#include "MPD_Arc.h"
#include <vector>


int measure(const std::vector<MyRect> &rectangles, const std::vector<MPD_Arc>& edges, int& diameter, int& distance, int& intersection_penalty) ;

void optimize_rectangle_positions(std::vector<MyRect> &rectangles, const std::vector<std::vector<MPD_Arc> >& adjacency_list) ;

void test_optimize_rectangle_positions() ;


#endif