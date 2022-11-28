/* MyRect.h
*
* Copyright (c) 2005-2022 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _MYRECT_
#define _MYRECT_


#include <vector>
#include <span>
#include <stdint.h>
#include <stdio.h>
#include <algorithm>
#include <math.h>
#include <assert.h>


enum Algorithm
{
	QUERY_COMPACT_DIMENSION,
	COMPACT_FRAME,
	FIT_HOLE
};

extern const char* AlgorithmString[3];


enum Direction
{
	EAST_WEST,
	NORTH_SOUTH,
	NR_DIRECTIONS
};

extern const char* DirectionString[2];

enum Sens
{
	DECREASE=-1,
	INCREASE=+1
};

const Direction directions[2] = { EAST_WEST, NORTH_SOUTH };


const int CHAR_RECT_HEIGHT = 16 ;

const int THIN_FRAME_BORDER = 10 ;
const int FRAME_BORDER = 30 ;
const int RECT_BORDER = 20 ;

struct MyPoint
{
	int16_t x=0, y=0 ;
	bool operator==(const MyPoint&) const = default;

	int16_t& operator[](Direction direction)
	{
		switch(direction)
		{
		case EAST_WEST:
			return x;
		case NORTH_SOUTH:
			return y;
		}
	}

        int16_t operator[](Direction direction) const
        {
                switch(direction)
                {
                case EAST_WEST:
                        return x;
                case NORTH_SOUTH:
                        return y;
                }
        }

        MyPoint& operator+=(const MyPoint& p)
        {
                x += p.x;
                y += p.y;
                return *this;
        }
} ;


inline MyPoint operator-(const MyPoint& p)
{
        MyPoint result ;
        result.x = -p.x ;
        result.y = -p.y ;
        return result ;
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

inline MyPoint operator*(int16_t value, const MyPoint& p)
{
	const auto& [x, y] = p;
	return {value*x, value*y};
}

std::vector<MyPoint> operator+(const std::vector<MyPoint> m1, const std::vector<MyPoint>& m2);


struct RectTranslation
{
	Algorithm algorithm;
	Direction compact_direction;
	int ri;
	int value;

	bool operator==(const RectTranslation&) const = default;
} ;


enum RectCorner
{
	TOP_LEFT=0,
	BOTTOM_LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT
} ;


extern const char* RectCornerString[4];


const RectCorner RectCorners[4]={TOP_LEFT, BOTTOM_LEFT, TOP_RIGHT, BOTTOM_RIGHT} ;


enum RectDim
{
  LEFT,
  RIGHT,
  TOP,
  BOTTOM
} ;


const RectDim RectDims[4]={LEFT,RIGHT,TOP,BOTTOM} ;

extern const char* RectDimString[4];

const Direction RectDimDirection[4]={EAST_WEST,EAST_WEST,NORTH_SOUTH,NORTH_SOUTH};


const RectDim RectCornerDims[4][2]={
	{LEFT, TOP},
	{LEFT, BOTTOM},
	{RIGHT, TOP},
	{RIGHT, BOTTOM}
};

struct RectDimRange{ RectDim min, max;};

const RectDimRange rectDimRanges[2] = {
	{ LEFT, RIGHT },
	{ TOP, BOTTOM }
};

struct MySegment {
	int min, max;
        bool operator==(const MySegment&) const = default;
};


struct MyRect
{
	int16_t m_left=0, m_right=0, m_top=0, m_bottom=0 ;
	int16_t no_sequence=0 ;
	int16_t i=-1 ;
	bool selected=false ;

	MySegment operator[](Direction direction) const
	{
		switch(direction)
		{
		case EAST_WEST:
			return {m_left, m_right};
		case NORTH_SOUTH:
			return {m_top, m_bottom};
		}
	}

	MyPoint operator[](RectCorner rc) const
	{
		switch(rc)
		{
		case TOP_LEFT:
			return {m_left, m_top};
		case BOTTOM_LEFT:
			return {m_left, m_bottom};
		case TOP_RIGHT:
			return {m_right, m_top};
		case BOTTOM_RIGHT:
			return {m_right, m_bottom};
		}
	}

	inline int16_t operator[](RectDim rd) const
	{
		switch(rd)
		{
		case LEFT:
			return m_left;
		case RIGHT:
			return m_right;
		case TOP:
			return m_top;
		case BOTTOM:
			return m_bottom;
		}
	}

	inline int16_t& operator[](RectDim rd)
	{
		switch(rd)
		{
		case LEFT:
				return m_left;
		case RIGHT:
				return m_right;
		case TOP:
				return m_top;
		case BOTTOM:
				return m_bottom;
		}
	}

	MyRect& operator+=(const MyRect& r)
	{
		m_left += r.m_left;
		m_right += r.m_right;
		m_top += r.m_top;
		m_bottom += r.m_bottom;
		return *this;
	}

        MyRect& operator+=(const MyPoint& p)
        {
                m_left += p.x;
                m_right += p.x;
                m_top += p.y;
                m_bottom += p.y;
                return *this;
        }

	MyRect& operator-=(const MyPoint& p)
	{
		m_left -= p.x;
		m_right -= p.x;
		m_top -= p.y;
		m_bottom -= p.y;
		return *this;
	}

	MyRect operator-() const
	{
		return {.m_left=-m_left, .m_right=-m_right, .m_top=-m_top, .m_bottom=-m_bottom, .no_sequence=no_sequence, .i=i, .selected=selected};
	}

	MyRect& operator=(const MyPoint& p)
	{
		m_left = m_right = p.x;
		m_top = m_bottom = p.y;
		return *this;
	}

	auto operator<=>(const MyRect&) const = default;
} ;

inline bool operator==(const MyRect& r, const MyPoint& p)
{
	return r.m_left==p.x && r.m_right==p.x && r.m_top==p.y && r.m_bottom==p.y;
}

inline bool operator!=(const MyRect& r, const MyPoint& p)
{
	return r.m_left!=p.x || r.m_right!=p.x || r.m_top!=p.y || r.m_bottom!=p.y;
}

inline MyRect operator+(const MyRect& r1, const MyRect& r2)
{
	return {
		.m_left = r1.m_left + r2.m_left,
		.m_right = r1.m_right + r2.m_right,
		.m_top = r1.m_top + r2.m_top,
		.m_bottom = r1.m_bottom + r2.m_bottom,
		.no_sequence = r1.no_sequence,
		.i = r1.i
	};
}

inline MyRect operator-(const MyRect& r1, const MyRect& r2)
{
        return {
                .m_left = r1.m_left - r2.m_left,
                .m_right = r1.m_right - r2.m_right,
                .m_top = r1.m_top - r2.m_top,
                .m_bottom = r1.m_bottom - r2.m_bottom,
                .no_sequence = r1.no_sequence,
                .i = r1.i
        };
}

inline MyRect rect(const MyPoint& pt1, const MyPoint& pt2)
{
        const auto [x1, y1] = pt1;
        const auto [x2, y2] = pt2;
        return {
			.m_left = std::min(x1,x2),
        	.m_right = std::max(x1,x2),
        	.m_top = std::min(y1,y2),
        	.m_bottom = std::max(y1, y2)
	};
}

inline MyRect operator+(const MyRect& r, const MyPoint& p)
{
	return {
		.m_left = r.m_left + p.x,
		.m_right = r.m_right + p.x,
		.m_top = r.m_top + p.y,
		.m_bottom = r.m_bottom + p.y,
		.no_sequence = r.no_sequence,
		.i = r.i
	};
}

inline MyRect operator+(const MyPoint& p, const MyRect& r)
{
	return {
		.m_left = r.m_left + p.x,
		.m_right = r.m_right + p.x,
		.m_top = r.m_top + p.y,
		.m_bottom = r.m_bottom + p.y,
		.no_sequence = r.no_sequence,
		.i = r.i
	};
}

std::vector<MyRect> operator+(const std::vector<MyRect> m1, const std::vector<MyRect>& m2);

std::vector<MyRect> operator-(const std::vector<MyRect> m1, const std::vector<MyRect>& m2);

std::vector<MyRect> operator+(const std::vector<MyRect> m1, const std::vector<MyPoint>& m2);

std::vector<MyRect> operator+(const std::vector<MyPoint> m1, const std::vector<MyRect>& m2);


template <typename T>
struct MatWrap
{
        MatWrap(std::vector<T>& m):_m(m){}

	template <typename U>
        MatWrap& operator+=(const std::vector<U>& m)
        {
                assert(_m.size() == m.size());

                int n = _m.size();

                for (int i=0; i < n; i++)
                        _m[i] += m[i];

                return *this;
        }

        std::vector<T>& _m;
};

template <typename T>
MatWrap<T> matw(std::vector<T>& m)
{
	return MatWrap(m);
}

struct TranslatedBox
{
	int16_t id;
	MyPoint translation;
	bool operator==(const TranslatedBox&) const = default;
};


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

MyRect compute_frame(std::span<const MyRect> rectangles) ;

MyPoint compute_center_frame_translation(const std::vector<MyRect>& rectangles);

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
MyRect expanded_by(const MyRect& r, int border);
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


/*
dist is the euclidean distance between points
rect. 1 is formed by points (x1, y1) and (x1b, y1b)
rect. 2 is formed by points (x2, y2) and (x2b, y2b)
*/
inline float rect_distance(const MyRect& r1, const MyRect& r2)
{
	const auto& [x1, x1b, y1, y1b, i1, no_sequence1, selected1] = r1;
	const auto& [x2, x2b, y2, y2b, i2, no_sequence2, selected2] = r2;

	auto dist = [](const MyPoint& p1, const MyPoint& p2){
		const auto& [x1, y1] = p1;
		const auto& [x2, y2] = p2;
		return sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
	};

	bool left = x2b < x1;
    bool right = x1b < x2;
    bool bottom = y2b < y1;
    bool top = y1b < y2;

    if (top && left)
        return dist({x1, y1b}, {x2b, y2});
    else if (left && bottom)
        return dist({x1, y1}, {x2b, y2b});
    else if (bottom && right)
        return dist({x1b, y1}, {x2, y2b});
    else if (right && top)
        return dist({x1b, y1b}, {x2, y2});
    else if (left)
        return x1 - x2b;
    else if (right)
        return x2 - x1b;
    else if (bottom)
        return y1 - y2b;
    else if (top)
        return y2 - y1b;
    else             // rectangles intersect
        return 0;
}


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
