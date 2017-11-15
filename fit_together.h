/* fit_together.h
*
* Copyright (c) 2005-2015 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _FIT_TOGETHER_
#define _FIT_TOGETHER_


#include "MyRect.h"
#include <vector>


struct WidgetContext ;


void fit_together(std::vector<WidgetContext*> (&composite_partition)[2], 
				  Direction direction,
				  std::vector<MyRect> &B,
				  MyPoint& translation_B,
				  int &diameter) ;


void fit_together(std::vector<MyRect> A, 
				  std::vector<MyRect> B, 
				  Direction direction, 
				  MyPoint& translation_B) ;


void test_fit_together() ;


#endif
