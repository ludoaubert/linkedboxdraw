/* MyRect.h
*
* Copyright (c) 2005-2017 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _MYRECT_
#define _MYRECT_


#include <vector>
#include <initializer_list>
#include <algorithm>
#include <stdint.h>


enum Direction
{
	EAST_WEST,
	NORTH_SOUTH,
	NR_DIRECTIONS
};

enum Sens
{
	DECREASE=-1,
	INCREASE=+1
};

const Direction directions[2] = { EAST_WEST, NORTH_SOUTH };

const int CHAR_RECT_HEIGHT = 16 ;

extern const int THIN_FRAME_BORDER ;
extern const int FRAME_BORDER ;
extern const int RECT_BORDER ;

struct MyPoint
{
	int16_t x, y ;
} ;


inline MyPoint operator-(const MyPoint& p)
{
        MyPoint result ;
        result.x = -p.x ;
        result.y = -p.y ;
        return result ;
}

inline bool operator==(const MyPoint& p1, const MyPoint& p2)
{
        return p1.x==p2.x && p1.y==p2.y ;
}

inline bool operator!=(const MyPoint& p1, const MyPoint& p2)
{
        return p1.x!=p2.x || p1.y!=p2.y ;
}

inline MyPoint operator-(const MyPoint& p1, const MyPoint& p2)
{
        MyPoint p ;
        p.x = p1.x - p2.x ;
        p.y = p1.y - p2.y ;
        return p ;
}

inline MyPoint operator+(const MyPoint& p1, const MyPoint& p2)
{
        MyPoint p ;
        p.x = p1.x + p2.x ;
        p.y = p1.y + p2.y ;
        return p ;
}



enum RectCorner
{
	TOP_LEFT=0,
	BOTTOM_LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT
} ;


const RectCorner RectCorners[4]={TOP_LEFT, BOTTOM_LEFT, TOP_RIGHT, BOTTOM_RIGHT} ;


enum RectDim 
{
  LEFT,
  RIGHT,
  TOP,
  BOTTOM,
  NR_RECT_DIMENSIONS
} ;

const RectDim RectDims[NR_RECT_DIMENSIONS]=
{
   RectDim::LEFT, 
   RectDim::RIGHT,
   RectDim::TOP,
   RectDim::BOTTOM
} ;


struct MyRect
{
	MyRect(std::initializer_list<int> i);
	MyRect();
	int16_t m_left=0, m_right=0, m_top=0, m_bottom=0 ;
	int16_t no_sequence=0 ;
	int16_t i=-1 ;
	bool selected=false ;
} ;


struct TranslatedBox{int16_t id; MyPoint translation;};

bool operator==(const TranslatedBox& tb1, const TranslatedBox& tb2);


bool operator==(const MyRect& r1, const MyRect& r2) ;
bool operator<(const MyRect& r1, const MyRect& r2);


inline int width(const MyRect& r)
{
        return r.m_right - r.m_left ;
}

inline int height(const MyRect& r)
{
        return r.m_bottom - r.m_top ;
}


int16_t dim_max(const MyRect& r) ;
float rectangle_diameter(const MyRect& r) ;
float frame_diameter(const std::vector<MyRect>& rectangles) ;
int16_t frame_dim_max(const std::vector<MyRect>& rectangles) ;

inline MyRect compute_frame(const std::vector<MyRect>& rectangles)
{
        MyRect frame = {+INT16_MAX,-INT16_MAX,+INT16_MAX,-INT16_MAX};

        for (const MyRect& r : rectangles)
        {
                frame.m_left = std::min(frame.m_left, r.m_left) ;
                frame.m_right = std::max(frame.m_right, r.m_right) ;
                frame.m_top = std::min(frame.m_top, r.m_top) ;
                frame.m_bottom = std::max(frame.m_bottom, r.m_bottom) ;
        }

        return frame ;
}


inline int rectangle_diameter_(const MyRect& r)
{
        return width(r) + height(r) ;
}


inline int frame_diameter_(const std::vector<MyRect>& rectangles)
{
        return rectangle_diameter_(compute_frame(rectangles)) ;
}


MyPoint dimensions(const MyRect& r) ;

std::vector<MyRect> operator-(const std::vector<MyRect>& rectangles, const MyRect& r) ;

bool is_on_rect_border(const MyRect& r, const MyPoint& p) ;

MyPoint min(const MyRect& r) ;
MyPoint max(const MyRect& r) ;
MyPoint center(const MyRect& r) ;
void expand_by(MyRect& r, int border) ;
void rect_swap_dimensions(MyRect& r) ;

bool check_rectangle(const MyRect& r) ;


inline int16_t distance_between_ranges(int16_t left1, int16_t right1, int16_t left2, int16_t right2)
{
  if (left2 > right1)
    return left2 - right1 ;
  else if (left1 > right2)
    return left1 - right2 ;
  else
    return 0 ;
}



bool range_intersect_strict(int16_t left1, int16_t right1, int16_t left2, int16_t right2) ;

inline int range_overlap(int16_t left1, int16_t right1, int16_t left2, int16_t right2)
{
  if (left2 >= right1)
    return 0 ;
  else if (left1 >= right2)
    return 0 ;
  else 
    return std::min(right1,right2) - std::max(left1, left2) ;
}


inline bool intersect(const MyRect& r1, const MyRect& r2)
{
        return !(r1.m_left > r2.m_right || r1.m_right < r2.m_left || r1.m_top > r2.m_bottom || r1.m_bottom < r2.m_top) ;
}

inline bool intersect_strict(const MyRect& r1, const MyRect& r2)
{
        return !(r1.m_left >= r2.m_right || r1.m_right <= r2.m_left || r1.m_top >= r2.m_bottom || r1.m_bottom <= r2.m_top) ;
}


int edge_overlap(const MyRect& r1, const MyRect& r2) ;

//this function is 'inspired' by the intersect() function
inline float rectangle_distance(const MyRect& r1, const MyRect& r2)
{
        if (r1.m_left > r2.m_right)
        {
                return r1.m_left - r2.m_right + distance_between_ranges(r1.m_top, r1.m_bottom, r2.m_top, r2.m_bottom) ;
        }
        else if (r1.m_right < r2.m_left)
        {
                return r2.m_left - r1.m_right + distance_between_ranges(r1.m_top, r1.m_bottom, r2.m_top, r2.m_bottom) ;
        }
        else if (r1.m_top > r2.m_bottom)
        {
                return r1.m_top - r2.m_bottom + distance_between_ranges(r1.m_left, r1.m_right, r2.m_left, r2.m_right) ;
        }
        else if (r1.m_bottom < r2.m_top)
        {
                return r2.m_top - r1.m_bottom + distance_between_ranges(r1.m_left, r1.m_right, r2.m_left, r2.m_right) ;
        }
        else
        {
//the two rectangles intersect.
                return 0 ;
        }
}

bool is_inside(const MyRect& r1, const MyRect& r2) ;


inline void translate(MyRect& r, const MyPoint& translation)
{
        r.m_left += translation.x ;
        r.m_right += translation.x ;
        r.m_top += translation.y ;
        r.m_bottom += translation.y ;
}


inline MyRect translate(const MyRect& r, const MyPoint& translation)
{
        MyRect rr = r ;
        rr.m_left += translation.x ;
        rr.m_right += translation.x ;
        rr.m_top += translation.y ;
        rr.m_bottom += translation.y ;
        return rr ;
}


inline MyRect enveloppe(const MyRect& r1, const MyRect& r2)
{
        MyRect r ;
        r.m_left = std::min(r1.m_left, r2.m_left) ;
        r.m_right = std::max(r1.m_right, r2.m_right) ;
        r.m_top = std::min(r1.m_top, r2.m_top) ;
        r.m_bottom = std::max(r1.m_bottom, r2.m_bottom) ;
        return r ;
}


void symmetric_diff(const MyRect &r, const MyRect &s, std::vector<MyRect> (&result2)[2]) ;

void test_rectangle_diff() ;


namespace std
{
	template <>
	struct hash<MyRect>
	{
		size_t operator()(const MyRect &r) const
		{
			return r.m_left ^ r.m_right ^ r.m_top ^ r.m_bottom ;
		}
	} ;
}


inline MyPoint corner(const MyRect& r, RectCorner rect_corner)
{
        MyPoint p ;

        switch (rect_corner)
        {
        case TOP_LEFT:
                p.y = r.m_top ;
                p.x = r.m_left ;
                break ;
        case BOTTOM_LEFT:
                p.y = r.m_bottom ;
                p.x = r.m_left ;
                break ;
        case TOP_RIGHT:
                p.y = r.m_top ;
                p.x = r.m_right ;
                break ;
        case BOTTOM_RIGHT:
                p.y = r.m_bottom ;
                p.x = r.m_right ;
                break ;
        }

        return p ;
}



inline int rectangle_intersection_dimension(const MyRect& r1, const MyRect& r2)
{
        int width = range_overlap(r1.m_left, r1.m_right, r2.m_left, r2.m_right) ;
        int height = range_overlap(r1.m_top, r1.m_bottom, r2.m_top, r2.m_bottom) ;
//attention, si une des dimensions est petites, l'intersection est petite !
        return std::min(width, height) ;
}



bool detect_collision(const std::vector<MyRect> &rectangles) ;

int16_t& value(MyPoint& p, Direction direction);
int16_t& value(MyRect& r, Direction direction, Sens sens);
int16_t value(const MyRect& r, Direction direction, Sens sens);

Sens reverse(Sens sens);
Direction transpose(Direction direction);
int16_t middle(const MyRect& r, Direction direction);
MyRect symmetric(const MyRect& r, Direction direction, int coord);

int16_t& min(MyRect& r, Direction direction);
int16_t& max(MyRect& r, Direction direction);
int16_t min(const MyRect& r, Direction direction);
int16_t max(const MyRect& r, Direction direction);


#endif
