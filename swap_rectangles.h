/* swap_rectangles.h
*
* Copyright (c) 2005-2015 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _SWAP_RECTANGLES_
#define _SWAP_RECTANGLES_


#include "MyRect.h"
#include "MPD_Arc.h"
#include <vector>
#include <tuple>


void swap_rectangles(std::vector<MyRect> &rectangles, const std::vector<MPD_Arc>& edges, std::vector<std::tuple<int,RectCorner,int,RectCorner> >& swaps) ;
void test_swap_rectangles() ;


#endif
