/* binpack.h
*
* Copyright (c) 2005-2014 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _BINPACK_
#define _BINPACK_


#include "MyRect.h"
#include <vector>


struct BPBlock
{
	BPBlock(int w, int h): placed(false)
	{
		this->w=w ;
		this->h=h ;
	}
	int w,h,x,y ;
	bool placed ;
	int i ;
} ;

void binpack(std::vector<BPBlock>& blocks, int& w, int& h) ;
void test_binpack() ;
void test_split_and_fit() ;

void binpack(std::vector<MyRect>& rectangles, int& w, int& h) ;
void gravity(std::vector<MyRect> &rectangles) ;
void collapse(std::vector<MyRect> &rectangles) ;


#endif
