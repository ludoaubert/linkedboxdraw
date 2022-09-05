#ifndef _FIT_
#define _FIT_

#include "MyRect.h"
#include <vector>

std::vector<MyPoint> compute_fit_to_hole_transform_(const std::vector<MyRect>& input_rectangles);
void test_fit();


#endif
