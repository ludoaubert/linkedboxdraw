#ifndef _COMPACT_FRAME_
#define _COMPACT_FRAME_

#include "MyRect.h"
#include "MPD_Arc.h"
#include <vector>


void compute_stress_line(const std::vector<MyRect>& rectangles, std::vector<int> (&stress_line)[2]);

std::vector<MyPoint> compute_compact_frame_transform(const std::vector<MyRect>& rectangles);

std::vector<RectTranslation> compute_compact_frame_transform_(const std::vector<MyRect>& rectangles);

void compact_frame(std::vector<MyRect>& rectangles, const std::vector<std::vector<MPD_Arc> > &adjacency_list) ;

void test_compact_frame() ;


#endif
