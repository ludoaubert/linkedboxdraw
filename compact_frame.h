#ifndef _COMPACT_FRAME_
#define _COMPACT_FRAME_

#include "MyRect.h"
#include "MPD_Arc.h"
#include <vector>


void compact_frame(std::vector<MyRect>& rectangles, const std::vector<std::vector<MPD_Arc> > &adjacency_list) ;

void test_compact_frame() ;


#endif