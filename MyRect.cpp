/* MyRect.cpp
*
* Copyright (c) 2005-2022 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#include "MyRect.h"
#include <vector>
#include <algorithm>
#include <ranges>
#include <assert.h>
#include <stdio.h>
#include <iterator>
#include <cstdint>
#include <cstring>
#include <cmath>
using namespace std ;


const char* DirectionString[2]={"EAST_WEST", "NORTH_SOUTH"};

const char* AlgorithmString[3]={"QUERY_COMPACT_DIMENSION","COMPACT_FRAME","FIT_HOLE"};


vector<MyPoint> operator+(const vector<MyPoint> m1, const vector<MyPoint>& m2)
{
	assert(m1.size() == m2.size());
	int n = m1.size();
	vector<MyPoint> m(n);
	for (int i=0; i < n; i++)
	{
			m[i] = m1[i] + m2[i];
	}
	return m;
}

const char* RectDimString[4]={"LEFT","RIGHT","TOP", "BOTTOM"} ;

const char* RectCornerString[4]={"TOP_LEFT", "BOTTOM_LEFT", "TOP_RIGHT", "BOTTOM_RIGHT"};


vector<MyRect> operator+(const vector<MyRect> m1, const vector<MyRect>& m2)
{
	assert(m1.size() == m2.size());
	int n = m1.size();
	vector<MyRect> m(n);
	for (int i=0; i < n; i++)
	{
			m[i] = m1[i] + m2[i];
	}
	return m;
}

vector<MyRect> operator-(const vector<MyRect> m1, const vector<MyRect>& m2)
{
        assert(m1.size() == m2.size());
        int n = m1.size();
        vector<MyRect> m(n);
        for (int i=0; i < n; i++)
        {
                        m[i] = m1[i] - m2[i];
        }
        return m;
}

vector<MyRect> operator+(const vector<MyRect> m1, const vector<MyPoint>& m2)
{
	assert(m1.size() == m2.size());
	int n = m1.size();
	vector<MyRect> m(n);
	for (int i=0; i < n; i++)
	{
			m[i] = m1[i] + m2[i];
	}
	return m;
}


vector<MyRect> operator+(const vector<MyPoint> m1, const vector<MyRect>& m2)
{
	assert(m1.size() == m2.size());
	int n = m1.size();
	vector<MyRect> m(n);
	for (int i=0; i < n; i++)
	{
			m[i] = m1[i] + m2[i];
	}
	return m;
}


/*
vector rects = {{1,2,3,4},{10,20,30,40}};
auto r = rects | ranges::views::transform([](const MyRect& r){return vector{r.left, r.right};}) | views::join ;
for (int const e : r) std::cout << e << ' ';
std::cout << '\n';
int min = ranges::min(r);
int max = ranges::max(r);
*/

MyRect compute_frame(span<const MyRect> rectangles)
{
	MyRect frame ;

	if (rectangles.size()==0)
		return frame;

	return {
		ranges::min(rectangles | views::transform(&MyRect::m_left)),
		ranges::max(rectangles | views::transform(&MyRect::m_right)),
		ranges::min(rectangles | views::transform(&MyRect::m_top)),
		ranges::max(rectangles | views::transform(&MyRect::m_bottom))
	};
}


MyPoint compute_center_frame_translation(const vector<MyRect>& rectangles)
{
	MyRect frame = compute_frame(rectangles);
	const MyPoint translation = {FRAME_BORDER-frame.m_left, FRAME_BORDER-frame.m_top};
	return translation;
}


void expand_by(MyRect& r, int border)
{
	r.m_left -= border ;
	r.m_right += border ;
	r.m_top -= border ;
	r.m_bottom += border ;
}

MyRect expanded_by(const MyRect& r, int border)
{
	MyRect rec = r;
        rec.m_left -= border ;
        rec.m_right += border ;
        rec.m_top -= border ;
        rec.m_bottom += border ;
	return rec;
}

int16_t dim_max(const MyRect& r)
{
	return max(height(r), width(r)) ;
}

float rectangle_diameter(const MyRect& r)
{
	return sqrt(width(r)*width(r) + height(r)*height(r)) ;
}


float frame_diameter(const vector<MyRect>& rectangles)
{
	return rectangle_diameter(compute_frame(rectangles)) ;
}

int16_t frame_dim_max(const vector<MyRect>& rectangles)
{
	return dim_max(compute_frame(rectangles)) ;
}


vector<MyRect> operator-(const vector<MyRect>& rectangles, const MyRect& r)
{
	vector<MyRect> rectangles_ ;
	ranges::remove_copy_if(
		rectangles,
		back_inserter(rectangles_), [&](const MyRect& rr){return rr == r;}
	) ;
	return rectangles_ ;
}


MyPoint dimensions(const MyRect& r)
{
	MyPoint dim ;
	dim.x = width(r) ;
	dim.y = height(r) ;
	return dim ;
}


bool is_on_rect_border(const MyRect& r, const MyPoint& p)
{
	return (
		((p.x == r.m_left || p.x == r.m_right) && (r.m_top < p.y && p.y < r.m_bottom))
		  ||
	    ((p.y == r.m_top || p.y == r.m_bottom) && (r.m_left < p.x && p.x < r.m_right))
	) ;
}

void rect_swap_dimensions(MyRect& r)
{
	swap(r.m_left, r.m_top) ;
	swap(r.m_right, r.m_bottom) ;
	assert(check_rectangle(r)) ;
}


bool check_rectangle(const MyRect& r)
{
	if (r.m_top > r.m_bottom)
		return false ;
	if (r.m_right < r.m_left)
		return false ;
	return true ;
}



MyPoint min(const MyRect& r)
{
	MyPoint p ;
	p.x = r.m_left ;
	p.y = r.m_top ;
	return p ;
}

MyPoint max(const MyRect& r)
{
	MyPoint p ;
	p.x = r.m_right ;
	p.y = r.m_bottom ;
	return p ;
}


MyPoint center(const MyRect& r)
{
	MyPoint p ;
	p.x = (r.m_left + r.m_right) / 2 ;
	p.y = (r.m_bottom + r.m_top) / 2 ;
	return p ;
}


bool range_intersect_strict(int16_t left1, int16_t right1, int16_t left2, int16_t right2)
{
  if (left2 >= right1)
    return false ;
  else if (left1 >= right2)
    return false ;
  else
    return true ;
}



int edge_overlap(const MyRect& r1, const MyRect& r2)
{
	if (r1.m_left == r2.m_right || r1.m_right == r2.m_left)
		return range_overlap(r1.m_top, r1.m_bottom, r2.m_top, r2.m_bottom) ;
	else if (r1.m_top == r2.m_bottom || r1.m_bottom == r2.m_top)
		return range_overlap(r1.m_left, r1.m_right, r2.m_left, r2.m_right) ;
	else
		return 0 ;
}


//is r1 inside r2 ?
bool is_inside(const MyRect& r1, const MyRect& r2)
{
	return r1.m_right <= r2.m_right && r1.m_left >= r2.m_left && r1.m_top >= r2.m_top && r1.m_bottom <= r2.m_bottom ;
}


/*
 Finds the difference between two intersecting rectangles.
 return An array of rectangle areas that are covered by either r or s, but
 not both.
*/
void symmetric_diff(const MyRect &r, const MyRect &s, vector<MyRect> (&result2)[2])
{
	assert(intersect(r,s)) ;

	int16_t a = min( r.m_left, s.m_left );
	int16_t b = max( r.m_left, s.m_left );
	int16_t c = min( r.m_right, s.m_right );
	int16_t d = max( r.m_right, s.m_right );

	int16_t e = min( r.m_top, s.m_top );
	int16_t f = max( r.m_top, s.m_top );
	int16_t g = min( r.m_bottom, s.m_bottom );
	int16_t h = max( r.m_bottom, s.m_bottom );

	// X = intersection, 0-7 = possible difference areas
	// e +-+-+-+
	// . |5|6|7|
	// f +-+-+-+
	// . |3|X|4|
	// g +-+-+-+
	// . |0|1|2|
	// h +-+-+-+
	// . a b c d

	vector<MyRect> result ;

	// we'll always have rectangles 1, 3, 4 and 6
	if (b < c && g < h)
		result.push_back({ b, c, g, h }) ;
	if (a < b && f < g)
		result.push_back({ a, b, f, g }) ;
	if (c < d && f < g)
		result.push_back({ c, d, f, g }) ;
	if (b < c && e < f)
		result.push_back({ b, c, e, f }) ;

	// decide which corners
	if( r.m_left == a && r.m_top == e || s.m_left == a && s.m_top == e )
	{
		if (a < b && e < f)
			result.push_back({ a, b, e, f }) ;
	}
	if( r.m_right == d && r.m_top == e || s.m_right == d && s.m_top == e )
	{
		if (c < d && e < f)
			result.push_back({ c, d, e, f }) ;
	}
	if( r.m_right == d && r.m_bottom == h || s.m_right == d && s.m_bottom == h )
	{
		if (c < d && g < h)
			result.push_back({ c, d, g, h }) ;
	}
	if ( r.m_left == a && r.m_bottom == h || s.m_left == a && s.m_bottom == h )
	{
		if (a < b && g < h)
			result.push_back({ a, b, g, h }) ;
	}

	for (MyRect &rec: result)
	{
		if (is_inside(rec, r))
			result2[0].push_back(rec) ;
		if (is_inside(rec, s))
			result2[1].push_back(rec) ;
	}

#ifdef _DEBUG
	FILE *fp = fopen("rd.txt", "w") ;
	int LEGS[2]={0,1} ;
	for (int LEG : LEGS)
	{
		fprintf(fp, "LEG=%d\n", LEG) ;
		for (MyRect &r : result2[LEG])
			fprintf(fp, "{%d,%d,%d,%d},\n", r.m_left, r.m_right, r.m_top, r.m_bottom) ;
	}
	fclose(fp) ;
#endif
}


void test_rectangle_diff()
{
	//test1
	{
		vector<MyRect> result[2] ;
		symmetric_diff({0,2,0,2}, {1,3,1,3}, result) ;
		int tab1[3][4]={
			{0,1,1,2},
			{1,2,0,1},
			{0,1,0,1}
		} ;
		int tab2[3][4]={
			{1,2,2,3},
			{2,3,1,2},
			{2,3,2,3}
		} ;
		//assert(std::equal(tab1, tab1+3, &result[0][0])) ;
		//assert(std::equal(tab2, tab2+3, &result[1][0])) ;
	}

	//test2
	{
		vector<MyRect> result[2] ;
		symmetric_diff({680, 1005, 520, 768}, {960, 1025, 440, 1272}, result) ;
		int tab1[1][4] = {
			{680,960,520,768}
		} ;
		int tab2[5][4]={
			{960,1005,768,1272},
			{1005,1025,520,768},
			{960,1005,440,520},
			{1005,1025,440,520},
			{1005,1025,768,1272}
		} ;
		//assert(std::equal(tab1, tab1+1, &result[0][0])) ;
		//assert(std::equal(tab2, tab2+5, &result[1][0])) ;
	}
}



bool detect_collision(const vector<MyRect> &rectangles)
{
	for (const MyRect& r1 : rectangles)
	{
		for (const MyRect& r2 : rectangles)
		{
			if (r1.i >= r2.i)
				continue ;
			if (intersect_strict(r1, r2))
				return true ;
		}
	}
	return false ;
}


int16_t& value(MyPoint& p, Direction direction)
{
	switch (direction)
	{
	case EAST_WEST:
		return p.x;
	case NORTH_SOUTH:
		return p.y;
	}
}


int16_t& value(MyRect& r, Direction direction, Sens sens)
{
	switch ((direction << 1) + (sens == INCREASE ? 1 : 0))
	{
	case (EAST_WEST << 1) + 1/*INCREASE*/:
		return r.m_right;
	case (EAST_WEST << 1) + 0/*DECREASE*/:
		return r.m_left;
	case (NORTH_SOUTH << 1) + 1/*INCREASE*/:
		return r.m_bottom;
	case (NORTH_SOUTH << 1) + 0/*DECREASE*/:
		return r.m_top;
	}
}

int16_t value(const MyRect& r, Direction direction, Sens sens)
{
	switch ((direction << 1) + (sens == INCREASE ? 1 : 0))
	{
	case (EAST_WEST << 1) + 1/*INCREASE*/:
		return r.m_right;
	case (EAST_WEST << 1) + 0/*DECREASE*/:
		return r.m_left;
	case (NORTH_SOUTH << 1) + 1/*INCREASE*/:
		return r.m_bottom;
	case (NORTH_SOUTH << 1) + 0/*DECREASE*/:
		return r.m_top;
	}
}


Sens reverse(Sens sens)
{
	switch (sens)
	{
	case INCREASE:
		return DECREASE;
	case DECREASE:
		return INCREASE;
	}
}


Direction transpose(Direction direction)
{
	switch (direction)
	{
	case EAST_WEST:
		return NORTH_SOUTH;
	case NORTH_SOUTH:
		return EAST_WEST;
	}
}


int16_t middle(const MyRect& r, Direction direction)
{
	switch (direction)
	{
	case EAST_WEST:
		return (r.m_left + r.m_right) / 2;
	case NORTH_SOUTH:
		return (r.m_top + r.m_bottom) / 2;
	}
}


MyRect symmetric(const MyRect& r, Direction direction, int coord)
{
	MyRect rsym = r;
	Direction other_direction = transpose(direction);
	value(rsym, other_direction, DECREASE) = 2 * coord - value(r, other_direction, INCREASE);
	value(rsym, other_direction, INCREASE) = 2 * coord - value(r, other_direction, DECREASE);
	return rsym;
}


int16_t& min(MyRect& r, Direction direction)
{
	switch (direction)
	{
	case EAST_WEST:
		return r.m_left;
	case NORTH_SOUTH:
		return r.m_top;
	}
}

int16_t& max(MyRect& r, Direction direction)
{
	switch (direction)
	{
	case EAST_WEST:
		return r.m_right;
	case NORTH_SOUTH:
		return r.m_bottom;
	}
}

int16_t min(const MyRect& r, Direction direction)
{
	switch (direction)
	{
	case EAST_WEST:
		return r.m_left;
	case NORTH_SOUTH:
		return r.m_top;
	}
}

int16_t max(const MyRect& r, Direction direction)
{
	switch (direction)
	{
	case EAST_WEST:
		return r.m_right;
	case NORTH_SOUTH:
		return r.m_bottom;
	}
}
