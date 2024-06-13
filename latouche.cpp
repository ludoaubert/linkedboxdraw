#include <vector>
#include <set>
#include <array>
#include <string>
#include <bitset>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <ranges>
#include <span>
#include <generator>
#include <execution>
#include <stdio.h>
#include <sys/stat.h>
#include <filesystem>
#include <cstring>
#include <cassert>
using namespace std;
using namespace std::literals;
namespace fs = std::filesystem;

//TODO: merge apply_mirror() and apply_trim_mirror()

//./holes2 | grep -e rectangles -e translations -e selectors | grep -e id=1 -e id=0

//#define _TRACE_

#ifdef _TRACE_
#  define D(x) x
#else
#  define D(x)
#endif

enum Direction
{
	EAST_WEST,
	NORTH_SOUTH
};

const int NR_DIRECTIONS=2;

const int RECT_BORDER = 20 ;

struct MyPoint
{
	int x, y;
	friend bool operator==(const MyPoint&, const MyPoint&) = default;
	
    template <class Self>
	auto&& operator[](this Self&& self, Direction direction)
	{
		switch(direction)
		{
		case EAST_WEST:
				return self.x;
		case NORTH_SOUTH:
				return self.y;
		}
	}
};

enum RectDim
{
  LEFT,
  RIGHT,
  TOP,
  BOTTOM
} ;

struct RectDimRange{ RectDim min, max;};

const RectDimRange rectDimRanges[2] = {
	{ LEFT, RIGHT },
	{ TOP, BOTTOM }
};

struct MyRect
{
	int m_left, m_right, m_top, m_bottom ;
	
	friend bool operator==(const MyRect&, const MyRect&) = default;
	
	template <class Self>
	auto&& operator[](this Self&& self, RectDim rd)
	{
		switch(rd)
		{
		case LEFT:
				return self.m_left;
		case RIGHT:
				return self.m_right;
		case TOP:
				return self.m_top;
		case BOTTOM:
				return self.m_bottom;
		}
	}
	
	MyRect& operator+=(const MyPoint& p)
	{
			m_left += p.x;
			m_right += p.x;
			m_top += p.y;
			m_bottom += p.y;
			return *this;
	}
};

inline int width(const MyRect& r)
{
        return r.m_right - r.m_left ;
}

inline int height(const MyRect& r)
{
        return r.m_bottom - r.m_top ;
}

int dim_max(const MyRect& r)
{
	return max(height(r), width(r)) ;
}

int distance_between_ranges(int left1, int right1, int left2, int right2)
{
	if (left2 > right1)
		return left2 - right1 ;
	else if (left1 > right2)
		return left1 - right2 ;
	else
		return 0 ;
}


int rectangle_distance(const MyRect& r1, const MyRect& r2)
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

inline bool intersect_strict(const MyRect& r1, const MyRect& r2)
{
        return !(r1.m_left >= r2.m_right || r1.m_right <= r2.m_left || r1.m_top >= r2.m_bottom || r1.m_bottom <= r2.m_top) ;
}

inline int range_overlap(int left1, int right1, int left2, int right2)
{
  if (left2 >= right1)
    return 0 ;
  else if (left1 >= right2)
    return 0 ;
  else
    return std::min(right1,right2) - std::max(left1, left2) ;
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

MyRect compute_frame(span<const MyRect> rectangles)
{
	return MyRect{
		.m_left = ranges::min(rectangles | views::transform(&MyRect::m_left)),
		.m_right = ranges::max(rectangles | views::transform(&MyRect::m_right)),
		.m_top = ranges::min(rectangles | views::transform(&MyRect::m_top)),
		.m_bottom = ranges::max(rectangles | views::transform(&MyRect::m_bottom))
	};
}

MyPoint dimensions(const MyRect& r)
{
	return MyPoint{.x = width(r), .y = height(r)} ;
}


struct Edge {
	int from;
	int to;
	auto operator<=>(const Edge&) const = default;
};

//Cf compute_box_rectangles.js
const int RECTANGLE_BOTTOM_CAP=200;


struct DecisionTreeNode
{
	int index=0;
	int parent_index=-1;
	int depth;
	int sigma_edge_distance;
	int i_emplacement_source, i_emplacement_destination;
	
	friend bool operator==(const DecisionTreeNode&, const DecisionTreeNode&) = default;
};

struct TranslationRangeItem
{
	int id;
	int ri;
	MyPoint tr;

	friend bool operator==(const TranslationRangeItem&, const TranslationRangeItem&) = default;
};


enum EtatEmplacement
{
	LIBRE,
	OCCUPE
};


struct SweepLineItem
{
	int sweep_value;
	RectDim rectdim;
	int ri;

	auto operator<=>(const SweepLineItem&) const = default;
};


/*
		  |
		  |
		  |
--------------------->tr
		  |
		  |
		  V
		sweep

		  |
		  |
		  |
--------------------->sweep
		  |
		  |
		  V
		 tr
*/

struct CustomLess
{
	inline bool operator()(const SweepLineItem& a, const SweepLineItem& b) const
	{
		if (a.sweep_value != b.sweep_value)
			return a.sweep_value < b.sweep_value;
		if (a.rectdim != b.rectdim)
			return a.rectdim > b.rectdim;   //RIGHT < LEFT and BOTTOM < TOP
		return a.ri < b.ri;
	}
};


struct RectLink
{
	int i;
	int j;
	int min_sweep_value, max_sweep_value=INT_MAX;
	auto operator<=>(const RectLink&) const = default;
};

/*
links[0] ------>
links[1] <------
*/
struct ActiveLineItem
{
	int i;
	RectLink* links[2]={0,0};
};


struct Score{
	int id, sigma_edge_distance, width, height, total;
};


struct TestInput
{
	int testid;
	vector<MyRect> input_rectangles;
//bi directional edges
	vector<Edge> edges;
};


const vector<TestInput> test_input={
{
	.testid=0,
	.input_rectangles = {
		{.m_left=406, .m_right=608, .m_top=20, .m_bottom=164},
		{.m_left=330, .m_right=552, .m_top=340, .m_bottom=451},
		{.m_left=463, .m_right=608, .m_top=228, .m_bottom=340},
		{.m_left=608, .m_right=774, .m_top=20, .m_bottom=212},
		{.m_left=608, .m_right=774, .m_top=212, .m_bottom=340},
		{.m_left=760, .m_right=947, .m_top=356, .m_bottom=516},
		{.m_left=283, .m_right=463, .m_top=164, .m_bottom=324},
		{.m_left=552, .m_right=760, .m_top=340, .m_bottom=516},
		{.m_left=345, .m_right=553, .m_top=516, .m_bottom=676},
		{.m_left=566, .m_right=753, .m_top=516, .m_bottom=660},
		{.m_left=774, .m_right=947, .m_top=196, .m_bottom=356},
		{.m_left=753, .m_right=940, .m_top=516, .m_bottom=724},
		{.m_left=103, .m_right=283, .m_top=163, .m_bottom=291},
		{.m_left=88, .m_right=283, .m_top=291, .m_bottom=451},
		{.m_left=130, .m_right=345, .m_top=451, .m_bottom=627}
	},
//bi directional edges
	.edges = {
		{.from=0,.to=3},
		{.from=1,.to=7},
		{.from=2,.to=7},
		{.from=3,.to=0},
		{.from=3,.to=4},
		{.from=4,.to=3},
		{.from=4,.to=7},
		{.from=5,.to=7},
		{.from=6,.to=7},
		{.from=6,.to=12},
		{.from=7,.to=1},
		{.from=7,.to=2},
		{.from=7,.to=4},
		{.from=7,.to=5},
		{.from=7,.to=6},
		{.from=7,.to=8},
		{.from=7,.to=9},
		{.from=7,.to=10},
		{.from=7,.to=11},
		{.from=8,.to=7},
		{.from=9,.to=7},
		{.from=10,.to=7},
		{.from=11,.to=7},
		{.from=12,.to=6},
		{.from=12,.to=13},
		{.from=13,.to=12},
		{.from=13,.to=14},
		{.from=14,.to=13}
	}
},
{
	.testid=1,
	.input_rectangles = {
		{.m_left=0+977-RECT_BORDER, .m_right=119+977+RECT_BORDER, .m_top=0+426-RECT_BORDER, .m_bottom=152+426+RECT_BORDER},//0
		{.m_left=0+977-RECT_BORDER, .m_right=119+977+RECT_BORDER, .m_top=0+250-RECT_BORDER, .m_bottom=136+250+RECT_BORDER},//1
		{.m_left=0+818-RECT_BORDER, .m_right=119+818+RECT_BORDER, .m_top=0+266-RECT_BORDER, .m_bottom=168+266+RECT_BORDER},//2
		{.m_left=0+942-RECT_BORDER, .m_right=154+942+RECT_BORDER, .m_top=0+10-RECT_BORDER, .m_bottom=200+10+RECT_BORDER},//3
		{.m_left=0+176-RECT_BORDER, .m_right=154+176+RECT_BORDER, .m_top=0+74-RECT_BORDER, .m_bottom=200+74+RECT_BORDER},//4
		{.m_left=0+624-RECT_BORDER, .m_right=126+624+RECT_BORDER, .m_top=0+26-RECT_BORDER, .m_bottom=200+26+RECT_BORDER},//6
		{.m_left=0+479-RECT_BORDER, .m_right=154+479+RECT_BORDER, .m_top=0+378-RECT_BORDER, .m_bottom=200+378+RECT_BORDER},//12
		{.m_left=0+388-RECT_BORDER, .m_right=154+388+RECT_BORDER, .m_top=0+10-RECT_BORDER, .m_bottom=88+10+RECT_BORDER},//13
		{.m_left=0+176-RECT_BORDER, .m_right=154+176+RECT_BORDER, .m_top=0+314-RECT_BORDER, .m_bottom=200+314+RECT_BORDER},//14
		{.m_left=0+388-RECT_BORDER, .m_right=196+388+RECT_BORDER, .m_top=0+138-RECT_BORDER, .m_bottom=200+138+RECT_BORDER},//15
		{.m_left=0+10-RECT_BORDER, .m_right=126+10+RECT_BORDER, .m_top=0+266-RECT_BORDER, .m_bottom=120+266+RECT_BORDER},//19
		{.m_left=0+10-RECT_BORDER, .m_right=126+10+RECT_BORDER, .m_top=0+154-RECT_BORDER, .m_bottom=72+154+RECT_BORDER},//20
		{.m_left=0+10-RECT_BORDER, .m_right=126+10+RECT_BORDER, .m_top=0+426-RECT_BORDER, .m_bottom=72+426+RECT_BORDER},//21
		{.m_left=0+673-RECT_BORDER, .m_right=105+673+RECT_BORDER, .m_top=0+266-RECT_BORDER, .m_bottom=200+266+RECT_BORDER},//26
		{.m_left=0+369-RECT_BORDER, .m_right=70+369+RECT_BORDER, .m_top=0+378-RECT_BORDER, .m_bottom=88+378+RECT_BORDER}//30
	},
	.edges = {
		{.from=0, .to=2},
		{.from=1, .to=2},
		{.from=2, .to=0},
		{.from=2, .to=1},
		{.from=2, .to=3},
		{.from=2, .to=9},
		{.from=3, .to=2},
		{.from=4, .to=5},
		{.from=4, .to=6},
		{.from=4, .to=9},
		{.from=4, .to=11},
		{.from=5, .to=4},
		{.from=6, .to=4},
		{.from=6, .to=8},
		{.from=7, .to=9},
		{.from=8, .to=6},
		{.from=8, .to=9},
		{.from=8, .to=12},
		{.from=9, .to=2},
		{.from=9, .to=4},
		{.from=9, .to=7},
		{.from=9, .to=8},
		{.from=9, .to=10},
		{.from=9, .to=13},
		{.from=9, .to=14},
		{.from=10, .to=9},
		{.from=10, .to=11},
		{.from=10, .to=12},
		{.from=11, .to=4},
		{.from=11, .to=10},
		{.from=12, .to=8},
		{.from=12, .to=10},
		{.from=13, .to=9},
		{.from=14, .to=9}
	}
},
{
	.testid=2,
	.input_rectangles = {
		{.m_left=750-RECT_BORDER, .m_right=960+RECT_BORDER, .m_top=282-RECT_BORDER, .m_bottom=434+RECT_BORDER},
		{.m_left=23-RECT_BORDER, .m_right=163+RECT_BORDER, .m_top=314-RECT_BORDER, .m_bottom=434+RECT_BORDER},
		{.m_left=1215-RECT_BORDER, .m_right=1341+RECT_BORDER, .m_top=90-RECT_BORDER, .m_bottom=226+RECT_BORDER},
		{.m_left=1049-RECT_BORDER, .m_right=1175+RECT_BORDER, .m_top=122-RECT_BORDER, .m_bottom=242+RECT_BORDER},
		{.m_left=1166-RECT_BORDER, .m_right=1341+RECT_BORDER, .m_top=298-RECT_BORDER, .m_bottom=386+RECT_BORDER},
		{.m_left=1000-RECT_BORDER, .m_right=1126+RECT_BORDER, .m_top=282-RECT_BORDER, .m_bottom=418+RECT_BORDER},
		{.m_left=203-RECT_BORDER, .m_right=336+RECT_BORDER, .m_top=154-RECT_BORDER, .m_bottom=274+RECT_BORDER},
		{.m_left=708-RECT_BORDER, .m_right=876+RECT_BORDER, .m_top=154-RECT_BORDER, .m_bottom=242+RECT_BORDER},
		{.m_left=370-RECT_BORDER, .m_right=475+RECT_BORDER, .m_top=10-RECT_BORDER, .m_bottom=114+RECT_BORDER},
		{.m_left=204-RECT_BORDER, .m_right=330+RECT_BORDER, .m_top=10-RECT_BORDER, .m_bottom=114+RECT_BORDER},
		{.m_left=10-RECT_BORDER, .m_right=164+RECT_BORDER, .m_top=10-RECT_BORDER, .m_bottom=114+RECT_BORDER},
		{.m_left=85-RECT_BORDER, .m_right=162+RECT_BORDER, .m_top=154-RECT_BORDER, .m_bottom=210+RECT_BORDER},
		{.m_left=376-RECT_BORDER, .m_right=474+RECT_BORDER, .m_top=201-RECT_BORDER, .m_bottom=273+RECT_BORDER},
		{.m_left=708-RECT_BORDER, .m_right=869+RECT_BORDER, .m_top=10-RECT_BORDER, .m_bottom=114+RECT_BORDER},
		{.m_left=514-RECT_BORDER, .m_right=668+RECT_BORDER, .m_top=154-RECT_BORDER, .m_bottom=258+RECT_BORDER},
		{.m_left=203-RECT_BORDER, .m_right=386+RECT_BORDER, .m_top=314-RECT_BORDER, .m_bottom=418+RECT_BORDER},
		{.m_left=514-RECT_BORDER, .m_right=641+RECT_BORDER, .m_top=298-RECT_BORDER, .m_bottom=370+RECT_BORDER}
	},
	.edges = {
		{.from=0, .to=5},
		{.from=0, .to=6},
		{.from=1, .to=6},
		{.from=2, .to=3},
		{.from=3, .to=2},
		{.from=3, .to=4},
		{.from=3, .to=5},
		{.from=4, .to=3},
		{.from=5, .to=0},
		{.from=5, .to=3},
		{.from=5, .to=7},
		{.from=6, .to=0},
		{.from=6, .to=1},
		{.from=6, .to=7},
		{.from=6, .to=8},
		{.from=6, .to=9},
		{.from=6, .to=10},
		{.from=6, .to=11},
		{.from=6, .to=15},
		{.from=7, .to=5},
		{.from=7, .to=6},
		{.from=8, .to=6},
		{.from=9, .to=6},
		{.from=10, .to=6},
		{.from=11, .to=6},
		{.from=12, .to=14},
		{.from=12, .to=15},
		{.from=12, .to=16},
		{.from=13, .to=14},
		{.from=14, .to=12},
		{.from=14, .to=13},
		{.from=14, .to=16},
		{.from=15, .to=6},
		{.from=15, .to=12},
		{.from=16, .to=12},
		{.from=16, .to=14}
	}
},
{
	.testid=3,
	.input_rectangles = {
		{.m_left=10-RECT_BORDER,.m_right=129+RECT_BORDER,.m_top=538-RECT_BORDER,.m_bottom=690+RECT_BORDER},
		{.m_left=10-RECT_BORDER,.m_right=129+RECT_BORDER,.m_top=730-RECT_BORDER,.m_bottom=866+RECT_BORDER},
		{.m_left=169-RECT_BORDER,.m_right=288+RECT_BORDER,.m_top=538-RECT_BORDER,.m_bottom=706+RECT_BORDER},
		{.m_left=328-RECT_BORDER,.m_right=482+RECT_BORDER,.m_top=618-RECT_BORDER,.m_bottom=818+RECT_BORDER},
		{.m_left=564-RECT_BORDER,.m_right=718+RECT_BORDER,.m_top=138-RECT_BORDER,.m_bottom=338+RECT_BORDER},
		{.m_left=564-RECT_BORDER,.m_right=690+RECT_BORDER,.m_top=378-RECT_BORDER,.m_bottom=578+RECT_BORDER},
		{.m_left=370-RECT_BORDER,.m_right=524+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=176-RECT_BORDER,.m_right=330+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=338+RECT_BORDER},
		{.m_left=176-RECT_BORDER,.m_right=330+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=328-RECT_BORDER,.m_right=524+RECT_BORDER,.m_top=378-RECT_BORDER,.m_bottom=578+RECT_BORDER},
		{.m_left=162-RECT_BORDER,.m_right=288+RECT_BORDER,.m_top=378-RECT_BORDER,.m_bottom=498+RECT_BORDER},
		{.m_left=370-RECT_BORDER,.m_right=496+RECT_BORDER,.m_top=266-RECT_BORDER,.m_bottom=338+RECT_BORDER},
		{.m_left=10-RECT_BORDER,.m_right=136+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=322+RECT_BORDER},
		{.m_left=522-RECT_BORDER,.m_right=627+RECT_BORDER,.m_top=618-RECT_BORDER,.m_bottom=818+RECT_BORDER},
		{.m_left=52-RECT_BORDER,.m_right=122+RECT_BORDER,.m_top=376-RECT_BORDER,.m_bottom=464+RECT_BORDER}
	},
	.edges = {
		{.from=0,.to=2},
		{.from=1,.to=2},
		{.from=2,.to=0},
		{.from=2,.to=1},
		{.from=2,.to=3},
		{.from=2,.to=9},
		{.from=3,.to=2},
		{.from=4,.to=5},
		{.from=4,.to=6},
		{.from=4,.to=9},
		{.from=4,.to=11},
		{.from=5,.to=4},
		{.from=6,.to=4},
		{.from=6,.to=8},
		{.from=7,.to=9},
		{.from=8,.to=6},
		{.from=8,.to=9},
		{.from=8,.to=12},
		{.from=9,.to=2},
		{.from=9,.to=4},
		{.from=9,.to=7},
		{.from=9,.to=8},
		{.from=9,.to=10},
		{.from=9,.to=13},
		{.from=9,.to=14},
		{.from=10,.to=9},
		{.from=10,.to=11},
		{.from=10,.to=12},
		{.from=11,.to=4},
		{.from=11,.to=10},
		{.from=12,.to=8},
		{.from=12,.to=10},
		{.from=13,.to=9},
		{.from=14,.to=9}
	}
},
{
	.testid=4,
	.input_rectangles = {
		{.m_left=108-RECT_BORDER,.m_right=234+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=454-RECT_BORDER,.m_right=594+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=66-RECT_BORDER,.m_right=171+RECT_BORDER,.m_top=490-RECT_BORDER,.m_bottom=674+RECT_BORDER},
		{.m_left=274-RECT_BORDER,.m_right=414+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=211-RECT_BORDER,.m_right=414+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=450+RECT_BORDER},
		{.m_left=648-RECT_BORDER,.m_right=803+RECT_BORDER,.m_top=282-RECT_BORDER,.m_bottom=450+RECT_BORDER},
		{.m_left=211-RECT_BORDER,.m_right=344+RECT_BORDER,.m_top=730-RECT_BORDER,.m_bottom=850+RECT_BORDER},
		{.m_left=211-RECT_BORDER,.m_right=456+RECT_BORDER,.m_top=490-RECT_BORDER,.m_bottom=690+RECT_BORDER},
		{.m_left=31-RECT_BORDER,.m_right=171+RECT_BORDER,.m_top=714-RECT_BORDER,.m_bottom=914+RECT_BORDER},
		{.m_left=454-RECT_BORDER,.m_right=608+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=450+RECT_BORDER},
		{.m_left=634-RECT_BORDER,.m_right=760+RECT_BORDER,.m_top=730-RECT_BORDER,.m_bottom=930+RECT_BORDER},
		{.m_left=746-RECT_BORDER,.m_right=929+RECT_BORDER,.m_top=490-RECT_BORDER,.m_bottom=658+RECT_BORDER},
		{.m_left=10-RECT_BORDER,.m_right=171+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=450+RECT_BORDER},
		{.m_left=496-RECT_BORDER,.m_right=706+RECT_BORDER,.m_top=490-RECT_BORDER,.m_bottom=690+RECT_BORDER},
		{.m_left=454-RECT_BORDER,.m_right=594+RECT_BORDER,.m_top=730-RECT_BORDER,.m_bottom=930+RECT_BORDER}
	},
	.edges = {
		{.from=0,.to=4},
		{.from=1,.to=4},
		{.from=1,.to=9},
		{.from=2,.to=4},
		{.from=2,.to=12},
		{.from=3,.to=4},
		{.from=3,.to=9},
		{.from=4,.to=0},
		{.from=4,.to=1},
		{.from=4,.to=2},
		{.from=4,.to=3},
		{.from=4,.to=7},
		{.from=5,.to=13},
		{.from=6,.to=7},
		{.from=6,.to=8},
		{.from=7,.to=4},
		{.from=7,.to=6},
		{.from=7,.to=13},
		{.from=8,.to=6},
		{.from=9,.to=1},
		{.from=9,.to=3},
		{.from=9,.to=13},
		{.from=10,.to=13},
		{.from=11,.to=13},
		{.from=12,.to=2},
		{.from=13,.to=5},
		{.from=13,.to=7},
		{.from=13,.to=9},
		{.from=13,.to=10},
		{.from=13,.to=11},
		{.from=13,.to=14},
		{.from=14,.to=13}
	}
},
{
	.testid=5,
	.input_rectangles = {
		{.m_left=128-RECT_BORDER,.m_right=234+RECT_BORDER,.m_top=138-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=190-RECT_BORDER,.m_right=316+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=338+RECT_BORDER},
		{.m_left=760-RECT_BORDER,.m_right=900+RECT_BORDER,.m_top=217-RECT_BORDER,.m_bottom=289+RECT_BORDER},
		{.m_left=16-RECT_BORDER,.m_right=150+RECT_BORDER,.m_top=330-RECT_BORDER,.m_bottom=498+RECT_BORDER},
		{.m_left=357-RECT_BORDER,.m_right=448+RECT_BORDER,.m_top=329-RECT_BORDER,.m_bottom=481+RECT_BORDER},
		{.m_left=190-RECT_BORDER,.m_right=317+RECT_BORDER,.m_top=378-RECT_BORDER,.m_bottom=482+RECT_BORDER},
		{.m_left=274-RECT_BORDER,.m_right=366+RECT_BORDER,.m_top=138-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=940-RECT_BORDER,.m_right=1052+RECT_BORDER,.m_top=217-RECT_BORDER,.m_bottom=273+RECT_BORDER},
		{.m_left=614-RECT_BORDER,.m_right=720+RECT_BORDER,.m_top=217-RECT_BORDER,.m_bottom=273+RECT_BORDER},
		{.m_left=488-RECT_BORDER,.m_right=642+RECT_BORDER,.m_top=313-RECT_BORDER,.m_bottom=401+RECT_BORDER},
		{.m_left=490-RECT_BORDER,.m_right=574+RECT_BORDER,.m_top=185-RECT_BORDER,.m_bottom=273+RECT_BORDER},
		{.m_left=710-RECT_BORDER,.m_right=858+RECT_BORDER,.m_top=329-RECT_BORDER,.m_bottom=385+RECT_BORDER},
		{.m_left=898-RECT_BORDER,.m_right=1192+RECT_BORDER,.m_top=393-RECT_BORDER,.m_bottom=497+RECT_BORDER},
		{.m_left=718-RECT_BORDER,.m_right=858+RECT_BORDER,.m_top=425-RECT_BORDER,.m_bottom=497+RECT_BORDER},
		{.m_left=614-RECT_BORDER,.m_right=971+RECT_BORDER,.m_top=41-RECT_BORDER,.m_bottom=177+RECT_BORDER},
		{.m_left=1011-RECT_BORDER,.m_right=1193+RECT_BORDER,.m_top=57-RECT_BORDER,.m_bottom=177+RECT_BORDER},
		{.m_left=128-RECT_BORDER,.m_right=352+RECT_BORDER,.m_top=26-RECT_BORDER,.m_bottom=98+RECT_BORDER},
		{.m_left=10-RECT_BORDER,.m_right=150+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=290+RECT_BORDER},
		{.m_left=17-RECT_BORDER,.m_right=88+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=98+RECT_BORDER},
		{.m_left=10-RECT_BORDER,.m_right=88+RECT_BORDER,.m_top=138-RECT_BORDER,.m_bottom=210+RECT_BORDER}
	},
	.edges = {
		{.from=0,.to=1},
		{.from=0,.to=2},
		{.from=0,.to=19},
		{.from=1,.to=0},
		{.from=1,.to=3},
		{.from=1,.to=4},
		{.from=1,.to=5},
		{.from=1,.to=6},
		{.from=2,.to=0},
		{.from=2,.to=7},
		{.from=2,.to=8},
		{.from=2,.to=11},
		{.from=3,.to=1},
		{.from=4,.to=1},
		{.from=5,.to=1},
		{.from=6,.to=1},
		{.from=7,.to=2},
		{.from=7,.to=14},
		{.from=7,.to=15},
		{.from=8,.to=2},
		{.from=8,.to=9},
		{.from=8,.to=10},
		{.from=9,.to=8},
		{.from=10,.to=8},
		{.from=11,.to=2},
		{.from=11,.to=12},
		{.from=11,.to=13},
		{.from=12,.to=11},
		{.from=13,.to=11},
		{.from=14,.to=7},
		{.from=15,.to=7},
		{.from=16,.to=19},
		{.from=17,.to=19},
		{.from=18,.to=19},
		{.from=19,.to=0},
		{.from=19,.to=16},
		{.from=19,.to=17},
		{.from=19,.to=18}
	}
}};


struct TranslationRangesTestContext
{
	int testid;
	vector<DecisionTreeNode> decision_tree;
	vector<TranslationRangeItem> expected_translation_ranges;
};


const vector<TranslationRangesTestContext> TRTestContexts={
{
	.testid=0,
	.decision_tree={
		{.index=822, .parent_index=-1, .depth=0, .sigma_edge_distance=0, .i_emplacement_source=0, .i_emplacement_destination=41}
	},
	.expected_translation_ranges={}
},
{
	.testid=1,
	.decision_tree={
		{.index=3492, .parent_index=-1, .depth=0, .sigma_edge_distance=0, .i_emplacement_source=5, .i_emplacement_destination=33}
	},
	.expected_translation_ranges={}
},
{
	.testid=2,
	.decision_tree={
		{.index=0, .parent_index=-1, .depth=0, .sigma_edge_distance=0, .i_emplacement_source=0, .i_emplacement_destination=35},
		{.index=1, .parent_index=0, .depth=1, .sigma_edge_distance=0, .i_emplacement_source=1, .i_emplacement_destination=33},
		{.index=2, .parent_index=1, .depth=2, .sigma_edge_distance=0, .i_emplacement_source=2, .i_emplacement_destination=1},
		{.index=3, .parent_index=2, .depth=3, .sigma_edge_distance=0, .i_emplacement_source=6, .i_emplacement_destination=2},
		{.index=4, .parent_index=3, .depth=4, .sigma_edge_distance=0, .i_emplacement_source=12, .i_emplacement_destination=6},
		{.index=5, .parent_index=4, .depth=5, .sigma_edge_distance=0, .i_emplacement_source=13, .i_emplacement_destination=0},
		{.index=6, .parent_index=5, .depth=6, .sigma_edge_distance=0, .i_emplacement_source=14, .i_emplacement_destination=22},
		{.index=7, .parent_index=5, .depth=6, .sigma_edge_distance=0, .i_emplacement_source=14, .i_emplacement_destination=26},
		{.index=8, .parent_index=5, .depth=6, .sigma_edge_distance=0, .i_emplacement_source=14, .i_emplacement_destination=27},
		{.index=9, .parent_index=5, .depth=6, .sigma_edge_distance=0, .i_emplacement_source=14, .i_emplacement_destination=32}
	},
	.expected_translation_ranges={
		{.id=1, .ri=1, .tr={.x=0, .y=111}},
		{.id=1, .ri=8, .tr={.x=0, .y=46}},
		{.id=1, .ri=14, .tr={.x=0, .y=111}},
		{.id=2, .ri=0, .tr={.x=0, .y=-112}},
		{.id=2, .ri=1, .tr={.x=0, .y=-1}},
		{.id=2, .ri=2, .tr={.x=-56, .y=222}},
		{.id=2, .ri=6, .tr={.x=0, .y=-112}},
		{.id=2, .ri=8, .tr={.x=0, .y=46}},
		{.id=2, .ri=14, .tr={.x=0, .y=111}},
		{.id=3, .ri=0, .tr={.x=0, .y=-272}},
		{.id=3, .ri=1, .tr={.x=0, .y=-161}},
		{.id=3, .ri=2, .tr={.x=-56, .y=222}},
		{.id=3, .ri=6, .tr={.x=89, .y=126}},
		{.id=3, .ri=8, .tr={.x=0, .y=46}},
		{.id=3, .ri=14, .tr={.x=0, .y=111}},
		{.id=4, .ri=0, .tr={.x=0, .y=-272}},
		{.id=4, .ri=1, .tr={.x=0, .y=-161}},
		{.id=4, .ri=2, .tr={.x=-56, .y=222}},
		{.id=4, .ri=6, .tr={.x=89, .y=126}},
		{.id=4, .ri=8, .tr={.x=0, .y=46}},
		{.id=4, .ri=12, .tr={.x=89, .y=127}},
		{.id=4, .ri=13, .tr={.x=-180, .y=0}},
		{.id=4, .ri=14, .tr={.x=0, .y=111}},
		{.id=5, .ri=0, .tr={.x=0, .y=-272}},
		{.id=5, .ri=1, .tr={.x=0, .y=-161}},
		{.id=5, .ri=2, .tr={.x=-56, .y=222}},
		{.id=5, .ri=6, .tr={.x=89, .y=126}},
		{.id=5, .ri=8, .tr={.x=0, .y=46}},
		{.id=5, .ri=12, .tr={.x=89, .y=127}},
		{.id=5, .ri=13, .tr={.x=123, .y=-543}},
		{.id=5, .ri=14, .tr={.x=0, .y=111}},
		{.id=6, .ri=0, .tr={.x=0, .y=-272}},
		{.id=6, .ri=1, .tr={.x=0, .y=-161}},
		{.id=6, .ri=2, .tr={.x=-56, .y=222}},
		{.id=6, .ri=6, .tr={.x=89, .y=126}},
		{.id=6, .ri=8, .tr={.x=0, .y=46}},
		{.id=6, .ri=12, .tr={.x=89, .y=127}},
		{.id=6, .ri=13, .tr={.x=123, .y=-543}},
		{.id=6, .ri=14, .tr={.x=-62, .y=-879}},
		{.id=7, .ri=0, .tr={.x=0, .y=-272}},
		{.id=7, .ri=1, .tr={.x=0, .y=-161}},
		{.id=7, .ri=2, .tr={.x=-56, .y=222}},
		{.id=7, .ri=6, .tr={.x=89, .y=126}},
		{.id=7, .ri=8, .tr={.x=0, .y=46}},
		{.id=7, .ri=12, .tr={.x=89, .y=127}},
		{.id=7, .ri=13, .tr={.x=123, .y=-543}},
		{.id=7, .ri=14, .tr={.x=-42, .y=-879}},
		{.id=8, .ri=0, .tr={.x=0, .y=-272}},
		{.id=8, .ri=1, .tr={.x=0, .y=-161}},
		{.id=8, .ri=2, .tr={.x=-56, .y=222}},
		{.id=8, .ri=6, .tr={.x=89, .y=126}},
		{.id=8, .ri=8, .tr={.x=0, .y=46}},
		{.id=8, .ri=12, .tr={.x=89, .y=127}},
		{.id=8, .ri=13, .tr={.x=123, .y=-543}},
		{.id=8, .ri=14, .tr={.x=-42, .y=-879}},
		{.id=9, .ri=0, .tr={.x=0, .y=-272}},
		{.id=9, .ri=1, .tr={.x=0, .y=-161}},
		{.id=9, .ri=2, .tr={.x=-56, .y=222}},
		{.id=9, .ri=6, .tr={.x=89, .y=126}},
		{.id=9, .ri=8, .tr={.x=0, .y=46}},
		{.id=9, .ri=12, .tr={.x=89, .y=127}},
		{.id=9, .ri=13, .tr={.x=123, .y=-543}},
		{.id=9, .ri=14, .tr={.x=182, .y=-879}}
	}
}
};


enum TrimAlgo
{
	SPLIT,
	NOTCH,
	TRIM,
	CORNER
};


const int NR_TRIM_ALGO=4;

const char* TrimAlgoStrings[NR_TRIM_ALGO]={
	"SPLIT",
	"NOTCH",
	"TRIM",
	"CORNER"
};


enum TrimMirrorDirection
{
	HORIZONTAL_MIRROR,
	VERTICAL_MIRROR,
	TILTED_MIRROR
};

const int NR_TRIM_MIRROR_DIRECTIONS=3;

enum MirroringState
{
	ACTIVE,
	IDLE
};

const int NR_MIRRORING_STATES=2;

const char* MirroringStateString[2]={
	"ACTIVE",
	"IDLE"
};



/*      by
      +------+
      |      |
+=====+======+=====+
|     |      |     |
|     |      |     | r
|     |      |     |
+=====+======+=====+
      |      |
      +------+
*/

void split(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (r.m_left < by.m_left && by.m_right < r.m_right && by.m_top < r.m_top && by.m_bottom > r.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=by.m_left, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom}
		};
	}
}

/*
+==================+
|                  |
|     +------+     | r
|     |      |     |
+=====+======+=====+
      |      |
      +------+
         by
*/

void notch(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (r.m_left < by.m_left && by.m_right < r.m_right && by.m_bottom > r.m_bottom && by.m_top < r.m_bottom && by.m_top > r.m_top)
	{
		rects = {
			{.m_left=r.m_left, .m_right=by.m_left, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=by.m_top}
		};
	}
}

/*
      +==================+
      |                  |
      |                  | r
+-----+------------------+----+
|     +==================+    |
|                             |
+-----------------------------+
         by
*/
void trim(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (by.m_left < r.m_left && by.m_right > r.m_right && r.m_top < by.m_top && r.m_bottom > by.m_top && r.m_bottom < by.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=by.m_top}
		};
	}
}

/*
      +==================+
      |                  |
      |                  | r
+-----+-----+            |
|     +=====+============+
|           |
+-----------+
         by
*/
void corner(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (by.m_left < r.m_left && by.m_right > r.m_left && by.m_right < r.m_right &&
		by.m_bottom > r.m_bottom && by.m_top > r.m_top && by.m_top < r.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=by.m_top},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom}
		};
	}
}


void apply_trim_algo(TrimAlgo trim_algo, const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	switch (trim_algo)
	{
	case SPLIT:
		split(r, by, rects);
		break;
	case NOTCH:
		notch(r, by, rects);
		break;
	case TRIM:
		trim(r, by, rects);
		break;
	case CORNER:
		corner(r, by, rects);
		break;
	}
}

void apply_trim_mirror(const TrimMirrorDirection mirroring_direction, MyRect& r)
{
	switch(mirroring_direction)
	{
	case VERTICAL_MIRROR:
		r.m_left *= -1;
		r.m_right *= -1;
		swap(r.m_left, r.m_right);
		break;
	case HORIZONTAL_MIRROR:
		r.m_top *= -1;
		r.m_bottom *= -1;
		swap(r.m_top, r.m_bottom);
		break;
	case TILTED_MIRROR:
		swap(r.m_left, r.m_top);
		swap(r.m_right, r.m_bottom);
		break;
	}
}


vector<MyRect> trimmed(MyRect r, MyRect by)
{
	auto rg1 = views::iota(0, NR_TRIM_ALGO);
	auto rg2 = views::iota(0, NR_MIRRORING_STATES);
	
	for (const auto [trim_algo, a, b, c] : views::cartesian_product(rg1, rg2, rg2, rg2))
	{
		int states[NR_TRIM_MIRROR_DIRECTIONS] = { a, b, c } ;
		
		auto rg = views::iota(0,NR_TRIM_MIRROR_DIRECTIONS);
		
		vector<MyRect> rects;

		for (int k : rg)
		{
			if (states[k] == ACTIVE)
			{
				TrimMirrorDirection mirroring_direction = (TrimMirrorDirection)k;
				apply_trim_mirror(mirroring_direction, r);
				apply_trim_mirror(mirroring_direction, by);
			}				
		}
		
		apply_trim_algo((TrimAlgo)trim_algo, r, by, rects);
		
		for (int k : rg | views::reverse)
		{
			if (states[k] == ACTIVE)
			{
				TrimMirrorDirection mirroring_direction = (TrimMirrorDirection)k;
				apply_trim_mirror(mirroring_direction, r);
				apply_trim_mirror(mirroring_direction, by);
				ranges::for_each(rects, [&](MyRect& rec){apply_trim_mirror(mirroring_direction,rec);});
			}				
		}
		
		if (!rects.empty())
		{
			D(printf("trim_algo=%u, mirroring=%u%u%u\n", trim_algo, a,b,c));
			D(printf("TrimAlgoString = %s\n", TrimAlgoStrings[trim_algo]));
			D(printf("TrimMirroringString = %s\n", TrimMirroringStrings[a+2*b+2*2*c]));
			string buffer = rects |
							views::transform([](const MyRect& r){
								return format("R({{.m_left={}, .m_right={}, .top={}, .m_bottom={}}})", r.m_left, r.m_right, r.m_top, r.m_bottom);
								}) |
							views::join_with(",\n"s) |
							ranges::to<string>();

			D(printf("%s", buffer));

			return rects;
		}
	}
	return {};
}


MyRect trimmed(const MyRect& r, span<const MyRect> rectangles)
{
	D(printf("begin trimmed\n"));

	auto next=[](vector<MyRect> previous, const MyRect& ri){

		return previous |
				views::transform([&](const MyRect& r){
					const vector<MyRect> rects = trimmed(r, ri);
					return rects.empty() ? vector{r} : rects;
				}) |
				views::join |
				ranges::to<vector>() ;
	};

	vector<MyRect> rects = ranges::fold_left(rectangles, vector{r}, next);

	return ranges::max(rects, {}, [](const MyRect& r){return width(r)*height(r);});

	D(printf("end trimmed\n"));
}


struct RectTrimTestContext{
	int testid;
	MyRect r;
	vector<MyRect> input_rectangles;
	MyRect expected;
};


const vector<RectTrimTestContext> rect_trim_test_contexts={
/*
             80  120   180  220         300
              | | |     |    |           |
               100
-80					+----+
					| 1  |
-100			+===+====+===============+
				|   |    |               |
-120			|   +----+               | r
-180	      +-+-+                      |
-200	      | 0 |======================+
-220	      +---+
*/
	{
		.testid=0,
		.r={.m_left=100, .m_right=300, .m_top=100, .m_bottom=200},
		.input_rectangles={
			{.m_left=80, .m_right=120, .m_top=180, .m_bottom=220},//0
			{.m_left=180, .m_right=220, .m_top=80, .m_bottom=120} //1
		},
		.expected={.m_left=120, .m_right=300, .m_top=120, .m_bottom=200}
	},
/*
           50  100 150               350 400 450
            |   |   |                 |   |   |

-50	        +-------+                 +-------+
	        |   2   |                 |   3   |
-100	    |	+===+=================+===+   |
	        |	|   |                 |   |   |
-150	    +---+---+                 +---+---+
				|           r             |
-200	    +---+---+                 +---+---+
            |	|   |                 |   |   |
-250	    |	+===+=================+===+   |
            |   0   |                 |   1   |
-300        +-------+                 +-------+
*/
	{
		.testid=1,
		.r={.m_left=100, .m_right=400, .m_top=100, .m_bottom=250},
		.input_rectangles={
			{.m_left=50, .m_right=150, .m_top=200, .m_bottom=300},//0
			{.m_left=350, .m_right=450, .m_top=200, .m_bottom=300},//1
			{.m_left=50, .m_right=150, .m_top=50, .m_bottom=150},//2
			{.m_left=350, .m_right=450, .m_top=50, .m_bottom=150} //3
		},
		.expected={.m_left=150, .m_right=350, .m_top=100, .m_bottom=250}
	},
/*
           100                    350 400 450
            |                      |   |   |

-100        +==========================+
            |                          |
-150		|                      +---+---+
            |             r        |   | 0 |
-200		|                      +---+---+
            |                          |
-250		+==========================+
*/
        {
			.testid=2,
			.r={.m_left=100, .m_right=400, .m_top=100, .m_bottom=250},
			.input_rectangles={
				{.m_left=350, .m_right=450, .m_top=150, .m_bottom=200}//0
			},
			.expected={.m_left=100, .m_right=350, .m_top=100, .m_bottom=250}
        },
/*
           50  100                        400 450
            |   |                          |   |
                              r
-100            +==========================+
                |                          |
-150        +---+--------------------------+---+
            |   |             0            |   |
-200        +---+------------------------- +---+
                |                          |
                |                          |
                |                          |
-300            +==========================+
*/
        {
			.testid=3,
			.r={.m_left=100, .m_right=400, .m_top=100, .m_bottom=300},
			.input_rectangles={
				{.m_left=50, .m_right=450, .m_top=150, .m_bottom=200}//0
			},
			.expected={.m_left=100, .m_right=400, .m_top=200, .m_bottom=300}
        },
/*
           50  100 150                    400 450
            |   |   |                      |   |

-50         +-------+
            |       |
-100        |   +===+======================+
            |   |   |                      |
-150        |   |   |                      |
            |   |   |         r            |
-200        |   |   |                      |
            |   |   |                      |
-250        |   +===+======================+
            |   0   |
-300        +-------+
*/
		{
			.testid=4,
			.r={.m_left=100, .m_right=400, .m_top=100, .m_bottom=250},
			.input_rectangles={
				{.m_left=50, .m_right=150, .m_top=50, .m_bottom=300}//0
			},
			.expected={.m_left=150, .m_right=400, .m_top=100, .m_bottom=250}
		}
};


void test_rect_trim()
{
	for (const auto& [testid, r, input_rectangles, expected] : rect_trim_test_contexts)
	{
		MyRect result = trimmed(r, input_rectangles);
		D(printf("result={.m_left=%d, .m_right=%d, .m_top=%d, .m_bottom=%d}\n", result.m_left, result.m_right, result.m_top, result.m_bottom));
		bool bOK = result == expected;
		printf("rect trim testid=%d : %s\n", testid, bOK ? "OK" : "KO");
	}
}


vector<RectLink> sweep(Direction update_direction, const span<MyRect>& rectangles)
{
	const int N=100;
	int n = rectangles.size();

	if (n >= N)
	{
		D(printf("unsufficient compile time size. Will crash silently...\n"));
		fflush(stdout);
	}

	const MyPoint translation2[2]={{.x=1, .y=0}, {.x=0, .y=1}};

	SweepLineItem sweep_line_buffer[2*N];
	span sweep_line(sweep_line_buffer, 2*n);

	ActiveLineItem active_line[N];
	RectLink rect_links_buffer[256];

//use the sweep_line that is not impacted by selected translation
	Direction sweep_direction = Direction(1-update_direction);

	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
	auto [minSweepRectDim, maxSweepRectDim] = rectDimRanges[sweep_direction];

		//sweep_line.reserve(2*n);

	for (int ri=0; ri < n; ri++)
	{
		sweep_line_buffer[2*ri]={.sweep_value=rectangles[ri][minSweepRectDim], .rectdim=minSweepRectDim, .ri=ri};
		sweep_line_buffer[2*ri+1]={.sweep_value=rectangles[ri][maxSweepRectDim], .rectdim=maxSweepRectDim, .ri=ri};
	}

	const MyPoint& translation = translation2[update_direction] ;

	ranges::sort(sweep_line, CustomLess());

	int active_line_size=0;
	int rect_links_size=0;

	auto cmp=[&](int i, int j){
		return rectangles[i][minCompactRectDim]<rectangles[j][minCompactRectDim];
	};

	auto erase=[&](SweepLineItem& sweep_line_item){

		auto& [sweep_value, rectdim, i] = sweep_line_item;

		ActiveLineItem& lower = *ranges::lower_bound(active_line,active_line+active_line_size, i, cmp, &ActiveLineItem::i);
		D(printf("lower = %d\n", lower.i));
		int pos = distance(active_line, &lower);
		D(printf("pos = %d\n", pos));

		for (RectLink* rl : active_line[pos].links)
		{
			if (rl != 0)
				rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
		}

		for (int ii=pos; ii<active_line_size; ii++)
			swap(active_line[ii], active_line[ii+1]);
		active_line_size -= 1;

		if (pos > 0 && pos < active_line_size)
		{
			rect_links_buffer[rect_links_size++] = RectLink{
				.i=active_line[pos-1].i,
				.j=active_line[pos].i,
				.min_sweep_value=sweep_value,
				.max_sweep_value=INT_MAX
			};

			if (RectLink *rl=active_line[pos-1].links[1]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			if (RectLink *rl=active_line[pos].links[0]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			active_line[pos-1].links[1] = active_line[pos].links[0] = & rect_links_buffer[rect_links_size - 1];
		}
	};

	auto insert=[&](SweepLineItem& sweep_line_item){

		auto& [sweep_value, rectdim, i] = sweep_line_item;

		ActiveLineItem& upper = *ranges::upper_bound(active_line,active_line+active_line_size, i, cmp, &ActiveLineItem::i);
		D(printf("upper = %d\n", upper.i));
		int pos = distance(active_line, &upper);
		D(printf("pos = %d\n", pos));

		for (int ii=active_line_size-1; ii>=pos; ii--)
			swap(active_line[ii],active_line[ii+1]);
		active_line_size += 1;
		active_line[pos].i=i;

		if (pos > 0)
		{
			rect_links_buffer[rect_links_size++] = RectLink{
				.i=active_line[pos-1].i,
				.j=active_line[pos].i,
				.min_sweep_value=sweep_value,
				.max_sweep_value=INT_MAX
			};

			if (RectLink *rl=active_line[pos].links[0]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			if (RectLink *rl=active_line[pos-1].links[1]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			active_line[pos].links[0] = active_line[pos-1].links[1] = & rect_links_buffer[rect_links_size - 1];
		}
		if (pos+1 < active_line_size)
		{
			rect_links_buffer[rect_links_size++] = RectLink{
				.i=active_line[pos].i,
				.j=active_line[pos+1].i,
				.min_sweep_value=sweep_value,
				.max_sweep_value=INT_MAX
			};

			if (RectLink *rl=active_line[pos].links[1]; rl!=0)
				rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
			if (RectLink *rl=active_line[pos+1].links[0]; rl!=0)
				rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
			active_line[pos].links[1] = active_line[pos+1].links[0] = &rect_links_buffer[rect_links_size - 1];
		}
	};

	auto print_active_line=[&](){
		char buffer[5000];
		int pos=0;
		pos += sprintf(buffer + pos, ".active_line={");
		for (auto& [i, links] : span(active_line, active_line_size))
		{
			pos += sprintf(buffer + pos, " %d,", i);
		}
		pos += sprintf(buffer + --pos, "}\n");
		buffer[pos]=0;
		printf("%s", buffer);
	};

	for (SweepLineItem& item : sweep_line)
	{
		const auto& [sweep_value, rectdim, ri] = item;
		switch(rectdim)
		{
		case LEFT:
		case TOP:
			D(printf("sweep reaching %d %s\n", ri, RectDimString[rectdim]));
			D(printf("before insert\n"));
//			print_active_line();
			insert(item);
			D(printf("after insert\n"));
			D(print_active_line());
			break;
		case RIGHT:
		case BOTTOM:
			D(printf("sweep leaving %d %s\n", ri, RectDimString[rectdim]));
			D(printf("before erase\n"));
//			print_active_line();
			erase(item);
			D(printf("after erase\n"));
			D(print_active_line());
			break;
		}
	}

	for (const auto [sweep_value, rectdim, ri] : sweep_line)
	{
		D(printf("{.sweep_value=%d, .rectdim=%s, .ri=%d},\n", sweep_value, RectDimString[rectdim], ri));
	}

	sort(rect_links_buffer, rect_links_buffer + rect_links_size);

    auto rg = views::counted(rect_links_buffer, rect_links_size) |
                views::filter([](const RectLink& rl){return rl.min_sweep_value != rl.max_sweep_value;});
#ifdef _TRACE_
	D(printf("rect_links:\n"));
	for (const auto& [i, j, min_sweep_value, max_sweep_value] : rg)
	{
		D(printf("{.i=%d, .j=%d, .%s=%d, .%s=%d},\n", i, j,
			RectDimString[minSweepRectDim], min_sweep_value, RectDimString[maxSweepRectDim], max_sweep_value));
	}
#endif

	return rg | ranges::to<vector>();
}


vector<MyRect> compute_holes(const vector<MyRect>& input_rectangles)
{
	const MyRect frame={
		.m_left=ranges::min(input_rectangles | views::transform(&MyRect::m_left)),
		.m_right=ranges::max(input_rectangles | views::transform(&MyRect::m_right)),
		.m_top=ranges::min(input_rectangles | views::transform(&MyRect::m_top)),
		.m_bottom=ranges::max(input_rectangles | views::transform(&MyRect::m_bottom))
	};

	const vector<MyRect> borders = {
		{.m_left=frame.m_left-10, .m_right=frame.m_left, .m_top=frame.m_top, .m_bottom=frame.m_bottom},
		{.m_left=frame.m_right, .m_right=frame.m_right+10, .m_top=frame.m_top, .m_bottom=frame.m_bottom},
		{.m_left=frame.m_left, .m_right=frame.m_right, .m_top=frame.m_top-10, .m_bottom=frame.m_top},
		{.m_left=frame.m_left, .m_right=frame.m_right, .m_top=frame.m_bottom, .m_bottom=frame.m_bottom+10}
	};

	auto next=[&](const vector<MyRect>& in_holes){

		auto il = {input_rectangles, in_holes, borders};
		vector<MyRect> rectangles = il | views::join | ranges::to<vector>() ;

		const Direction update_directions[2] = {EAST_WEST, NORTH_SOUTH};

		vector<MyRect> holes = update_directions |
			views::transform([&](Direction  update_direction){
				const vector<RectLink> rect_links = sweep(update_direction, rectangles);
				auto rg = rect_links |
					views::transform([&](const RectLink& lnk)->MyRect{
						const auto [i, j, min_sweep_value, max_sweep_value] = lnk;
						const MyRect &ri=rectangles[i], &rj=rectangles[j];
						switch(update_direction)
						{
						case EAST_WEST:
							return MyRect{.m_left=ri.m_right, .m_right=rj.m_left, .m_top=min_sweep_value, .m_bottom=max_sweep_value};
						case NORTH_SOUTH:
							return MyRect{.m_left=min_sweep_value, .m_right=max_sweep_value, .m_top=ri.m_bottom, .m_bottom=rj.m_top};
						}
					}) | 
					views::filter([](const MyRect& r){
						return r.m_left != r.m_right && r.m_top != r.m_bottom;
					}) | 
					views::filter([](const MyRect& r){
						return 5*min<int>(width(r), height(r)) >= RECTANGLE_BOTTOM_CAP;
					});
				return rg;
			}) |
			views::join |
			ranges::to<vector>();

		int n = holes.size();
		const vector<float> dim_spread = holes |
									views::transform([](const MyRect& r)->float{
										const int dim[2] = {width(r), height(r)};
										auto [min,Max] = ranges::minmax(dim);
										return (float)(Max - min) / (float)(Max + min);
									}) |
									ranges::to<vector>();
			
		auto rg = views::iota(0, n);
		vector<Edge> inter = views::cartesian_product(rg, rg) |
							views::transform([](auto arg){
								auto [i, j] = arg ;	return Edge{.from=i, .to=j};
							}) |
							views::filter([&](const Edge& e){
								return e.from != e.to && intersect_strict(holes[e.from], holes[e.to]);
							}) |
							ranges::to<vector>() ;

		ranges::sort(inter, {}, [&](const Edge& e){return dim_spread[e.to];}) ;

		vector<int> suppressed(holes.size(), 0);
		suppressed = ranges::fold_left(inter | views::reverse,
										suppressed,
										[](vector<int> suppressed, const Edge& e){
											if (suppressed[e.from]==0)
												suppressed[e.to] = 1;
											return suppressed;
										}
					);
					
		holes = holes |
					views::enumerate |
					views::filter([&](auto arg){
						auto [i, r] = arg;
						return suppressed[i]==0;
					}) |
					views::elements<1> |
					ranges::to<vector>() ;
		return holes;
	};

	vector<vector<MyRect> > vv(3);
	partial_sum(vv.begin(), vv.end(), vv.begin(),
		[&](const vector<MyRect>& prev, const vector<MyRect>&){
			D(printf("calling computation of next_holes\n"));
			fflush(stdout);
			vector<MyRect> next_holes = next(prev);
			D(printf("next_holes={\n"));
			for (const MyRect& r : next_holes)
				D(printf("{.m_left=%d, .m_right=%d, .top=%d, .m_bottom=%d}\n",r.m_left,r.m_right,r.m_top,r.m_bottom));
			D(printf("}\n"));
			fflush(stdout);
			return next_holes;
		});
	D(printf("returned from partial_sum()\n"));
	fflush(stdout);

	const vector<MyRect> holes = vv | views::join | ranges::to<vector>();
	
	const string buffer = holes |
		views::transform([](const MyRect& r){
			return format(R"({{"m_left":{},"m_right":{},"m_top":{},"m_bottom":{}}})",r.m_left, r.m_right, r.m_top, r.m_bottom);}) | 
		views::join_with(",\n"s) |
		ranges::to<string>() ;

	FILE *f=fopen("holes.json", "w");
	fprintf(f, "{\n\"holes\":[%s]}", buffer.c_str());
	fclose(f);

	return holes;
}


void spread(Direction update_direction, const vector<RectLink>& rect_links, span<MyRect> rectangles)
{
//TODO: use chunk_by C++23
	const int N=30;
	int n = rectangles.size();

	MyPoint translations[N];

	ranges::fill(translations, MyPoint{0,0});

	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}

	auto rec_push_hole=[&](this auto&& self, int ri, int tr)->void{

		D(printf("entering rec_push_hole(ri=%d ,tr=%d)\n", ri, tr));

		for (const RectLink& rl : ranges::equal_range(rect_links, ri, {}, &RectLink::i))
		{
			int tr2= rectangles[ri][maxCompactRectDim] - rectangles[rl.j][minCompactRectDim];

			if (tr2 < 0)
				tr2 = 0;

			self(rl.j, tr+tr2);
		}
		int &tri = translations[ri][update_direction];
		tri = max<int16_t>(tri, tr) ;
	};


	vector<RectLink> index = rect_links;
	ranges::sort(index, {}, &RectLink::j);
	vector<int> root_nodes;
	ranges::set_difference(views::iota(0,n),
				index | views::transform(&RectLink::j),
				back_inserter(root_nodes)
				);
	for (int j : root_nodes)
	{
		int tr=0;
		rec_push_hole(j, tr);
	}


	for (const auto& [x, y] : views::counted(translations, n))
	{
		D(printf("{.x=%d, .y=%d},", x, y));
	}
	D(printf("\n"));

	for (int ri=0; ri < n; ri++)
	{
		rectangles[ri] += translations[ri];
	}
}


void compact(Direction update_direction, const vector<RectLink>& rect_links, const vector<Edge>& edges, span<const MyRect> input_rectangles, span<MyRect> rectangles)
{
	D(printf("begin compact\n"));
	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}

	const int n = rectangles.size();

	vector<vector<TranslationRangeItem> > vv(10);
	int id=0;

	auto next=[&](const vector<TranslationRangeItem>& prev)->vector<TranslationRangeItem>
	{
		ranges::copy(input_rectangles, begin(rectangles));
		ranges::for_each(prev, [&](const TranslationRangeItem& item){
			const auto& [id, ri, tr] = item;
			rectangles[ri] += tr;
		});

		id++;

		auto rg = rectangles | views::transform([&](const MyRect& r){return r[minCompactRectDim];});
		const int frame_min = ranges::min(rg);
		const int next_min = ranges::min(rg | views::filter([&](int value){return value != frame_min;}));

		D(printf("frame_min=%d\n", frame_min));
		D(printf("next_min=%d\n", next_min));

		bitset<30> partition;

		auto selected_rect_links = rect_links | views::filter([&](const RectLink& lnk){return rectangles[lnk.i][maxCompactRectDim] == rectangles[lnk.j][minCompactRectDim];});
		vector<vector<int> > vv(20);
		vv[0] = views::iota(0,n) | 
				views::filter([&](int i){return rectangles[i][minCompactRectDim]==frame_min;}) |
				ranges::to<vector>();

		partial_sum(vv.begin(), vv.end(), vv.begin(),
				[&](const vector<int>& prev, const vector<int>&){
						vector<int> next = prev | 
								views::transform([&](int i){
									auto r = ranges::equal_range(selected_rect_links, i, {}, &RectLink::i) |
												views::transform(&RectLink::j);
									return r;
								}) |
								views::join |
								ranges::to<vector>();
						return next;
					}
				);

		ranges::for_each(vv | views::join, [&](int i){partition[i]=1;});


		auto r = rect_links |
				views::filter([&](const RectLink& e){return partition[e.i] > partition[e.j];}) |
				views::transform([&](const RectLink& e){return rectangles[e.j][minCompactRectDim]-rectangles[e.i][maxCompactRectDim];}) ;

		vector<TranslationRangeItem> translation_ranges;

		if (ranges::empty(r))
			return translation_ranges;

		MyPoint tr={.x=0, .y=0};
		tr[update_direction] = min<int>(ranges::min(r), next_min - frame_min);

		auto rg2 = views::iota(0,n) |
					views::filter([&](int i){return partition[i]==1;}) |
					views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i,.tr=tr};});

		ranges::for_each(rg2, [&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});

		translation_ranges = views::iota(0,n) |
					views::filter([&](int i){return rectangles[i][minCompactRectDim] != input_rectangles[i][minCompactRectDim];}) |
					views::transform([&](int i){
						MyPoint tr;
						tr[update_direction] = rectangles[i][minCompactRectDim] - input_rectangles[i][minCompactRectDim];
						return TranslationRangeItem{.id=id, .ri=i, .tr=tr};
					}) |
					ranges::to<vector>();
		for (const auto [id, ri, tr] : translation_ranges)
		{
			D(printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y));
		}
		return translation_ranges;
	};

//10: just had to choose a number. Should not be needed with C++23 partial_fold()
// Cf https://stackoverflow.com/questions/74042325/listing-all-intermediate-recurrence-results

	partial_sum(vv.begin(), vv.end(), vv.begin(),
				[&](const vector<TranslationRangeItem>& prev, const vector<TranslationRangeItem>&){
					return next(prev);}
				);

	auto rg = vv | views::join;

	D(printf("rg = vv | views::join\n"));

	for (const auto [id, ri, tr] : rg)
	{
		D(printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y));
	}

	if (ranges::empty(rg))
	{
		D(printf("end compact\n"));
		return ;
	}

//TODO: use C++23 chunk_by()
//TODO: structured binding in Lambda would simplify the code.

	const int nb = 1 + ranges::max(rg | views::transform(&TranslationRangeItem::id));

	auto cost_fn=[&](int id){

		ranges::copy(input_rectangles, begin(rectangles));
		ranges::for_each(ranges::equal_range(rg, id, {}, &TranslationRangeItem::id),
				[&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});

		const int sigma_edge_distance = ranges::fold_left(edges |
			views::transform([&](const Edge& le){ return rectangle_distance(rectangles[le.from],rectangles[le.to]);  }),
			0, plus<int>());

		const int sigma_translation = ranges::fold_left(ranges::equal_range(rg, id, {}, &TranslationRangeItem::id) |
			views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);}),
			0, plus<int>());

		const auto [width, height] = dimensions(compute_frame(rectangles));

		const int sigma_edge_overlap = ranges::fold_left(edges |
			views::transform([&](const Edge& le){    return edge_overlap(rectangles[le.from],rectangles[le.to]);  }),
			0, plus<int>());

		D(printf("id = %d\n", id));
		D(printf("sigma_edge_distance = %d\n", sigma_edge_distance));
		D(printf("sigma_translation = %d\n", sigma_translation));
		D(printf("[.width=%d, .height=%d]\n", width, height));
		D(printf("sigma_edge_overlap = %d\n", sigma_edge_overlap));

		int cost = width + height + width*height + sigma_edge_distance + sigma_translation - sigma_edge_overlap;

		D(printf("cost = %d\n", cost));

		return cost;
	};

	vector<int> costs = views::iota(0,nb) |
		views::transform(cost_fn) |
		ranges::to<vector>();

	auto it = ranges::min_element(costs);
	id = std::distance(costs.begin(), it);
	D(printf("id=%d\n", id));

	ranges::copy(input_rectangles, begin(rectangles));
	ranges::for_each(ranges::equal_range(rg, id, {}, &TranslationRangeItem::id),
			[&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});
	D(printf("end compact\n"));
}


struct Mirror
{
	MirroringState mirroring_state;
	Direction mirroring_direction;
};

enum Algo
{
	SPREAD,
	COMPACT
};

struct Job
{
	Algo algo;
	Direction update_direction;
};



const int NR_JOB_PIPELINES=2;

const Job pipelines[NR_JOB_PIPELINES][1]={
	{
		{.algo=SPREAD, .update_direction=EAST_WEST}
	},
	{
		{.algo=SPREAD, .update_direction=NORTH_SOUTH}
	}
};

const int NR_RECT_CORNERS=4;

const RectDim corners[NR_RECT_CORNERS][2]={
	{LEFT, TOP},
	{LEFT, BOTTOM},
	{RIGHT, TOP},
	{RIGHT, BOTTOM}
};

const char* CornerStrings[NR_RECT_CORNERS]={
	"{LEFT, TOP}",
	"{LEFT, BOTTOM}",
	"{RIGHT, TOP}",
	"{RIGHT, BOTTOM}"
};

//4 mirrors X 4 corners X 2 job pipelines

void apply_mirror(const Mirror& mirror, span<MyRect> rectangles)
{
	const auto& [mirroring_state, mirroring_direction] = mirror;

	if (mirroring_state == ACTIVE)
	{
		ranges::for_each(rectangles, [&](MyRect &r){

			switch(mirroring_direction)
			{
			case EAST_WEST:
				r.m_left *= -1;
				r.m_right *= -1;
				swap(r.m_left, r.m_right);
				break;
			case NORTH_SOUTH:
				r.m_top *= -1;
				r.m_bottom *= -1;
				swap(r.m_top, r.m_bottom);
				break;
			}
		});
	}
}


/*
TODO: use C++26 submdspan()
float* data = ...;

std::mdspan matrix(data, 2, 5);

std::mdspan row0 = std::submdspan(matrix, 0, std::full_extent);
std::span vector0(row0.data_handle(), row0.size()); // row 0 of matrix

std::mdspan row1 = std::submdspan(matrix, 1, std::full_extent);
std::span vector1(row1.data_handle(), row1.size()); // row 1 of matrix

needs to be rechecked later, but unfortunately it seems mdspan is not a range and cannot be iterated on.
*/
void compute_decision_tree_translations(const vector<DecisionTreeNode>& decision_tree,
										const vector<MyRect>& input_rectangles,
										const vector<MyRect>& holes,
										const vector<Edge>& edges,
										vector<MyRect>& emplacements_by_id)
{
	auto il = {input_rectangles, holes};
	vector<MyRect> input_emplacements = il | views::join | ranges::to<vector>() ;
// TODO: vector<MyRect> input_emplacements = views::concat(input_rectangles, holes) | ranges::to<vector>() ;

	int m = input_emplacements.size();
	int n = input_rectangles.size();

	emplacements_by_id.resize(m*decision_tree.size());

	auto tf=[&](int id, unsigned pipeline, unsigned a, unsigned b, unsigned match_corner){

		const int parent_index = decision_tree[id].parent_index;
//TODO: use submdspan.
		span<MyRect> emplacements(begin(emplacements_by_id)+m*id, m);
		ranges::copy(parent_index == -1 ? span(input_emplacements) : span(begin(emplacements_by_id)+m*parent_index,m), begin(emplacements));

		span<MyRect> rectangles(begin(emplacements), n);

		D(printf("calling tf(id=%d, pipeline=%u, a=%u, b=%u, match_corner=%u)\n", id, pipeline, a, b, match_corner));

		const DecisionTreeNode& node = decision_tree[id];

		MyRect &r1 = emplacements[node.i_emplacement_source],
			r2 = input_emplacements[node.i_emplacement_destination];
		const auto [RectDimX, RectDimY] = corners[match_corner];

		r2 = trimmed(r2, rectangles);
		r1 += MyPoint{.x=r2[RectDimX] - r1[RectDimX], .y=r2[RectDimY] - r1[RectDimY]};

		const Mirror mirrors[2]={
			Mirror{.mirroring_state=(MirroringState)a, .mirroring_direction=(Direction)0},
			Mirror{.mirroring_state=(MirroringState)b, .mirroring_direction=(Direction)1}
		};
		for (const Mirror& mirror : mirrors)
			apply_mirror(mirror, rectangles);
		for (const auto& [algo, update_direction] : pipelines[pipeline])
		{
			const vector<RectLink> rect_links = sweep(update_direction, rectangles);
			assert(algo == SPREAD);
			spread(update_direction, rect_links, rectangles);
		}
		for (const Mirror& mirror : mirrors)
			apply_mirror(mirror, rectangles);

		const string buffer = rectangles |
					views::enumerate |
					views::transform([&](const auto& arg)->TranslationRangeItem{
						const auto& [i, r] = arg;
						const MyRect &ir = emplacements[i];
						return {.id=id, .ri=(int)i, .tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top}};
						}
					) |
					views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
					views::transform([&](const TranslationRangeItem& item)->string{
						const auto& [id,i,tr]=item;
						if (i < n)
							return format(R"({{id={}, ri={}, tr={{x={}, y={}}} )", id, i, tr.x, tr.y);
						else
							return format(R"({{id={}, ri=h{}, tr={{x={}, y={}}} )", id, i-n, tr.x, tr.y);
						}
					) |
					views::join |
					ranges::to<string>();

        D(printf("[id=%d pipeline=%u, a=%u, b=%u, match_corner=%u] tf output translations=", id, pipeline, a, b, match_corner));
		D(printf("%s\n", buffer.c_str()));
	};

	auto cdtt = [&](int id){

		span<MyRect> emplacements(begin(emplacements_by_id)+m*id, m);
		span<MyRect> rectangles(begin(emplacements), n);

		auto cost_fn = [&](auto arg){
			const auto& [pipeline, a, b, match_corner] = arg;
			D(printf("pipeline=%u\n", pipeline));
			D(printf("MirroringStrings[%u%u]=%s%s\n", MirroringStateString[a], MirroringStateString[b]));
			D(printf("CornerStrings[match_corner]=%s\n", CornerStrings[match_corner]));

			tf(id, pipeline, a, b, match_corner);

			const int sigma_edge_distance = ranges::fold_left(edges |
				views::transform([&](const auto& le){ return rectangle_distance(rectangles[le.from],rectangles[le.to]); }),
				0, plus<int>());
				
//TODO: structured binding inside Lambda would simplify the expressions.

			const int sigma_translation = ranges::fold_left(views::iota(0,n) |
					views::transform([&](int i)->TranslationRangeItem{
						const MyRect &ir = input_rectangles[i], &r = rectangles[i];
						MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
						return {id, i, tr};}) |
					views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
					views::filter([&](const TranslationRangeItem& item){return item.ri != decision_tree[id].i_emplacement_source;}) |
					views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);}),
				0, plus<int>());

			const auto [width, height] = dimensions(compute_frame(rectangles));

			D(printf("sigma_edge_distance = %d\n", sigma_edge_distance));
			D(printf("sigma_translation = %d\n", sigma_translation));
			D(printf("[.width=%d, .height=%d]\n", width, height));

			int cost = width + height + sigma_edge_distance + sigma_translation ;

			D(printf("cost=%d\n", cost));
			return cost;
		};
		
		auto rng1 = views::iota(0, NR_JOB_PIPELINES);
		auto rng2 = views::iota(0, NR_MIRRORING_STATES);
		auto rng3 = views::iota(0, NR_RECT_CORNERS);

		const auto [pipeline, a, b, match_corner] = ranges::min(views::cartesian_product(rng1, rng2, rng2, rng3), {}, cost_fn);

		D(printf("selectors[id=%d] = {pipeline=%u, mirroring=%u%u, match_corner=%u}\n", id, pipeline, a, b, match_corner));

		D(printf("MirroringStrings[%u%u]=%s%s\n", MirroringStateString[a], MirroringStateString[b]));
		D(printf("CornerStrings[match_corner]=%s\n", CornerStrings[match_corner]));

		tf(id, pipeline, a, b, match_corner);
	};

	for (int depth=0; depth<10; depth++)
	{
		vector<int> input = views::iota(0, (int)decision_tree.size()) |
			views::filter([&](int id){return decision_tree[id].depth==depth;}) |
			ranges::to<vector>();

		for_each(execution::par_unseq, input.begin(), input.end(), cdtt);
	}

	auto rg1 = views::iota(0, n);
	auto rg2 = views::iota(0, (int)decision_tree.size());
	
	string buffer = views::cartesian_product(rg1, rg2) |
		views::transform([&](auto arg)->TranslationRangeItem{
			const auto& [i, id] = arg ;
//TODO: use submdspan.
			span<MyRect> emplacements(begin(emplacements_by_id)+m*id, m);
			span<MyRect> rectangles(begin(emplacements), n);
			const MyRect &ir = input_emplacements[i], &r = emplacements[i];
			MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
			return {id, i, tr};}) |
		views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
		views::transform([](const TranslationRangeItem& item){
			const auto& [id, ri, tr] = item;
			return format(R"({{"id":{}, "ri":{}, "x":{}, "y":{}}})", id, ri, tr.x, tr.y);
		}) |
		views::join_with(",\n"s) |
		ranges::to<string>();

	FILE *f=fopen("translation_ranges.json", "w");
	fprintf(f, "[%s\n]\n", buffer.c_str());
	fclose(f);
}

struct JobMirror
{
	Job job;
	Mirror mirror;
};

const int NR_JOB_PIPELINES2=2;

const JobMirror pipelines2[NR_JOB_PIPELINES2][4]={
	{
		{
			.job={.algo=COMPACT, .update_direction=EAST_WEST},
			.mirror={.mirroring_state=IDLE, .mirroring_direction=EAST_WEST}
		},
		{
			.job={.algo=COMPACT, .update_direction=EAST_WEST},
			.mirror={.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST}
		},
		{
			.job={.algo=COMPACT, .update_direction=NORTH_SOUTH},
			.mirror={.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
		},
		{
			.job={.algo=COMPACT, .update_direction=NORTH_SOUTH},
			.mirror={.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
		}
	},
	{
		{
			.job={.algo=COMPACT, .update_direction=NORTH_SOUTH},
			.mirror={.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
		},
		{
			.job={.algo=COMPACT, .update_direction=NORTH_SOUTH},
			.mirror={.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
		},
		{
			.job={.algo=COMPACT, .update_direction=EAST_WEST},
			.mirror={.mirroring_state=IDLE, .mirroring_direction=EAST_WEST}
		},
		{
			.job={.algo=COMPACT, .update_direction=EAST_WEST},
			.mirror={.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST}
		}
	}
};


//./holes2 --dt --skip | awk '/begin cmpt_tr2 id=4 /,/end cmpt_tr2 id=4 /'
//TODO: use views::chunk_by() C++23

void compute_decision_tree_translations2(const vector<DecisionTreeNode>& decision_tree,
					int n,
					const vector<Edge>& edges,
					const vector<MyRect>& emplacements_by_id,
					vector<MyRect>& emplacements2_by_id)
{
	vector<MyRect> emplacements1_by_id = emplacements_by_id;

	auto cdtt = [&](int id){

	//decision_tree.size() can be zero, in which case the division will never execute.
        	const int m = emplacements_by_id.size() / decision_tree.size();

	// rectangles holds the constant reference. rectangles1 holds the accumulation of selected changes during the pipeline.
	// rectangles2 holds live output
		span<const MyRect> rectangles(begin(emplacements_by_id)+m*id, n);
		span<MyRect> rectangles1(begin(emplacements1_by_id)+m*id, n),
					rectangles2(begin(emplacements2_by_id)+m*id, n);

		auto tf=[&](unsigned pipeline){

			D(printf("calling tf(pipeline=%u)\n", pipeline));

			ranges::copy(rectangles, begin(rectangles1));

			for (const auto& [job, mirror] : pipelines2[pipeline])
			{
				ranges::copy(rectangles1, begin(rectangles2));

				apply_mirror(mirror, rectangles1);
				apply_mirror(mirror, rectangles2);

				const auto& [algo, update_direction] = job;
				const vector<RectLink> rect_links = sweep(update_direction, rectangles2);
				assert(algo == COMPACT);
				compact(update_direction, rect_links, edges, rectangles1, rectangles2);

				apply_mirror(mirror, rectangles1);
				apply_mirror(mirror, rectangles2);

				ranges::copy(rectangles2, begin(rectangles1));
			}
		};

		D(printf("begin cmpt_tr2 id=%d \n", id));

		auto cost_fn = [&](int pipeline){
			D(printf("pipeline=%u\n", pipeline));

			tf(pipeline);

			const int sigma_edge_distance = ranges::fold_left(edges |
				views::transform([&](const auto& le){ return rectangle_distance(rectangles2[le.from],rectangles2[le.to]);}),
				0, plus<int>());

			const int sigma_translation = ranges::fold_left(views::iota(0,n) |
				views::transform([&](int i)->TranslationRangeItem{
						const MyRect &ir = rectangles[i], &r = rectangles2[i];
						MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
						return {id, i, tr};}) |
				views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
				views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);}),
				0, plus<int>());

			const auto [width, height] = dimensions(compute_frame(rectangles2));

			D(printf("sigma_edge_distance = %d\n", sigma_edge_distance));
			D(printf("sigma_translation = %d\n", sigma_translation));
			D(printf("[.width=%d, .height=%d]\n", width, height));

			int cost = width + height + sigma_edge_distance + sigma_translation ;

			D(printf("cost=%d\n", cost));
			return cost;
		};

		int costs[NR_JOB_PIPELINES2];
		auto rng = views::iota(0,NR_JOB_PIPELINES2);
		transform(rng.begin(), rng.end(), costs, cost_fn);
		int *it = ranges::min_element(costs);
		int pipeline = it - costs;

//		const auto pipeline = ranges::min(views::iota(0,NR_JOB_PIPELINES2), {}, cost_fn);

		D(printf("selection[id=%d] = {pipeline=%u}\n", id, pipeline));

		tf(pipeline);

		D(printf("end cmpt_tr2 id=%d \n", id));
	};

	const vector<int> input = views::iota(0, (int)decision_tree.size()) | ranges::to<vector>() ;

	for_each(execution::par_unseq, input.begin(), input.end(), cdtt);

	auto rg1 = views::iota(0, n);
	auto rg2 = views::iota(0, (int)decision_tree.size());
	string buffer = views::cartesian_product(rg1, rg2) |
		views::transform([&](auto arg)->TranslationRangeItem{
        //decision_tree.size() can be zero, in which case the division will never execute.
			const int m = emplacements_by_id.size() / decision_tree.size();
			const auto& [i, id] = arg ;
			span<const MyRect> rectangles(begin(emplacements_by_id)+m*id, n),
					rectangles2(begin(emplacements2_by_id)+m*id, n);
			const MyRect &ir = rectangles[i], &r = rectangles2[i];
			MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
			return {id, i, tr};
		}) |
		views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
		views::transform([](const TranslationRangeItem& item){
			const auto& [id, ri, tr] = item;
			return format(R"({{"id":{}, "ri":{}, "x":{}, "y":{}}})", id, ri, tr.x, tr.y);
		}) |
		views::join_with(",\n"s) |
		ranges::to<string>();

	FILE *f=fopen("translation_ranges2.json", "w");
	fprintf(f, "[%s\n]\n", buffer.c_str());
	fclose(f);
}


struct TranslationItem {
	int i, x, y;
	friend bool operator==(const TranslationItem&, const TranslationItem&) = default;
};

struct TestContext {
	int testid;
	vector<MyRect> input_rectangles;
	vector<Job> pipeline;
	vector<TranslationItem> expected_translations;
};

const vector<TestContext> test_contexts={
/*
       +-------+
       |       |
+------+   1   +------+
|      |       |      |
|  0   +---+---+  2   +------+
|      |rh |   |      |      |
+------+---+---+------+  3   |
|      |       |      |      |
|  4   |   5   |      +------+
|      |       |
+------+-------+
3 => rh
*/
	{
		.testid=0,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
			{.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
			{.m_left=300-200, .m_right=400-200, .m_top=100, .m_bottom=200},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=150, .m_bottom=250}
		},
		.pipeline = {
			{.algo=SPREAD,.update_direction=EAST_WEST}
		},
		.expected_translations={
			{.i=5, .x=100, .y=0}
		}
	},
/*
       +-------+
       |       |
+------+   1   +------+
|      |       |      |
|  0   +---+---+  2   +---+
|      |       |      | 3 |
+------+   rh  +------+---+
|      |       |
|  4   +-------+
|      |       |
+------+   5   |
       |       |
       +-------+
3 => rh
*/
	{
		.testid=1,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
			{.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
			{.m_left=300-200, .m_right=350-200, .m_top=100, .m_bottom=150},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=200, .m_bottom=300}
		},
		.pipeline = {
			{.algo=COMPACT,.update_direction=NORTH_SOUTH}
		},
		.expected_translations={
			{.i=1, .x=0, .y=50},
			{.i=3, .x=0, .y=50}
		}
	},
/*
       +-------+
       |       |
+------+   1   |
|      |       |
|  0   +---+---+------+---+
|      |       |      | 3 |
+------+   rh  |  2   +---+
|      |       |      |
|  4   +-------+------+
|      |       |
+------+   5   |
       |       |
       +-------+
3 => rh
*/
	{
		.testid=2,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
			{.m_left=200, .m_right=300, .m_top=100, .m_bottom=200},
			{.m_left=300-200, .m_right=350-200, .m_top=100, .m_bottom=150},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=200, .m_bottom=300}
		},
		.pipeline = {
			{.algo=COMPACT,.update_direction=EAST_WEST}
		},
		.expected_translations={
			{.i=0, .x=50, .y=0},
			{.i=1, .x=50, .y=0},
			{.i=3, .x=50, .y=0},
			{.i=4, .x=50, .y=0},
			{.i=5, .x=50, .y=0}
		}
	},
/*
       +-------+
       |       |
+------+   1   +------+
|      |       |      |
|  0   +-------+  2   +------+
|      |       |      |      |
+------+-------+---+--+  3   |
|      |       |rh |  |      |
|  4   |   5   +---+  +------+
|      |       |
+------+-------+
3 => rh
*/
	{
		.testid=3,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
			{.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
			{.m_left=300-100, .m_right=400-100, .m_top=100+50, .m_bottom=200+50},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=150, .m_bottom=250}
		},
		.pipeline = {
			{.algo=COMPACT,.update_direction=NORTH_SOUTH}
		},
		.expected_translations={
			{.i=1, .x=0, .y=50}
		}
	},

/*
       +---+
       |rh |
+------+---+   +------+
|      |       |      |
|  0   +       +  1   +---------+
|      |       |      |         |
+------+       +------+  2      |
                      |         |
                      +---------+
2 => rh
*/
	{
		.testid=4,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
			{.m_left=300-200, .m_right=450-200, .m_top=100-100, .m_bottom=200-100}
		},
		.pipeline = {
			{.algo=SPREAD,.update_direction=EAST_WEST}
		},
		.expected_translations={
			{.i=1, .x=50, .y=0}
		}
	},

/*
+------+       +------+------+
|      |       |      |      |
|      |       |      |  2   |
|      |       |      |      |
|      |       |      +------+
|      +---+   |      |
|      |rh |   |      |
|  0   +---+   |  1   |
|      |       |      |
|      |       |      |
|      |       |      |
|      |       |      |
+------+       +------+
2 => rh
*/
	{
		.testid=5,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=0, .m_bottom=700},
			{.m_left=300, .m_right=400, .m_top=0, .m_bottom=700},
			{.m_left=400-300, .m_right=450-300, .m_top=0+250, .m_bottom=100+250}
		},
		.pipeline = {
			{.algo=COMPACT,.update_direction=EAST_WEST}
		},
		.expected_translations={
			{.i=0, .x=150, .y=0},
			{.i=2, .x=150, .y=0}
		}
	},
	{
		.testid=6,
		.input_rectangles = {
			{.m_left=328, .m_right=530, .m_top=10, .m_bottom=154},
			{.m_left=252, .m_right=474, .m_top=490, .m_bottom=601},
			{.m_left=385, .m_right=530, .m_top=218, .m_bottom=330},
			{.m_left=530, .m_right=696, .m_top=10, .m_bottom=202},
			{.m_left=530, .m_right=696, .m_top=202, .m_bottom=330},
			{.m_left=682, .m_right=869, .m_top=346, .m_bottom=506},
			{.m_left=267, .m_right=447, .m_top=601, .m_bottom=761},
			{.m_left=474, .m_right=682, .m_top=330, .m_bottom=506},
			{.m_left=266, .m_right=474, .m_top=330, .m_bottom=490},
			{.m_left=488, .m_right=675, .m_top=506, .m_bottom=650},
			{.m_left=744, .m_right=917, .m_top=186, .m_bottom=346},
			{.m_left=675, .m_right=862, .m_top=506, .m_bottom=714},
			{.m_left=25, .m_right=205, .m_top=153, .m_bottom=281},
			{.m_left=10, .m_right=205, .m_top=281, .m_bottom=441},
			{.m_left=37, .m_right=252, .m_top=441, .m_bottom=617}
		},
		.pipeline = {
			{.algo=COMPACT,.update_direction=EAST_WEST}
		},
			.expected_translations={
		}
	}
};


void test_fit()
{
	for (const auto& [testid, input_rectangles, pipeline, expected_translations] : test_contexts)
	{
		vector<MyRect> rectangles = input_rectangles;
		int dm1 = dim_max(compute_frame(input_rectangles));
		for (const auto& [algo, update_direction] : pipeline)
		{
			assert(algo == SPREAD);
			const vector<RectLink> rect_links = sweep(update_direction, rectangles);
			spread(update_direction, rect_links, rectangles);
		}

		int n=rectangles.size();
		auto rg = views::iota(0, n) |
			views::filter([&](int i){return input_rectangles[i] != rectangles[i];}) |
			views::transform([&](int i)->TranslationItem{
				const MyRect &r1 = input_rectangles[i], &r2 = rectangles[i];
				return {.i=i,.x=r2.m_left - r1.m_left,.y=r2.m_top - r1.m_top};
			});

		int dm2 = dim_max(compute_frame(rectangles));
		bool bOK = ranges::equal(rg, expected_translations);
		printf("fit_to_hole testid=%d : %s\n", testid, bOK ? "OK" : "KO");
		printf("dim_max(frame) : %d => %d\n", dm1, dm2);
//		(bOK ? nbOK : nbKO)++;
	}
}


void test_translations()
{
	const auto& [testid, input_rectangles, edges] = test_input[0];

	for (const auto [testid, decision_tree, expected_translation_ranges] : TRTestContexts)
	{
		const vector<MyRect> holes = compute_holes(input_rectangles);
		vector<MyRect> emplacements_by_id;
		compute_decision_tree_translations(decision_tree,
						input_rectangles,
						holes,
						edges,
						emplacements_by_id);
//		bool bOK = translation_ranges == expected_translation_ranges;
//		printf("translation ranges testid=%d : %s\n", testid, bOK ? "OK" : "KO");
	}
};


vector<DecisionTreeNode> compute_decision_tree(const vector<Edge>& edges, const vector<MyRect>& input_rectangles, const vector<MyRect>& holes)
{
	const int nr_input_rectangles = input_rectangles.size();

	auto il = {input_rectangles, holes};
	const vector<MyRect> rectangles = il | views::join | ranges::to<vector>();
//TODO
//	const vector<MyRect> rectangles = concat(input_rectangles, holes) | ranges::to<vector>();

	const int nr_emplacements = rectangles.size();
	
	const int N = rectangles.size();
	
	const vector<int> distance_matrix = views::cartesian_product(rectangles, rectangles) |
										views::transform([](auto arg){const auto [r1, r2]=arg;	return rectangle_distance(r1, r2);}) |
										ranges::to<vector>();
										
//TODO: use mdspan
	
	vector<DecisionTreeNode> decision_tree;
	
	auto walk_up_from = [&](int parent_index)->generator<int>{
		for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
		{
			co_yield index;
		}		
	};

    auto index2emplacement = [&](int idx){
		vector<int> emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		for (int ix : walk_up_from(idx) | ranges::to<vector>() | views::reverse)
		{
			const DecisionTreeNode& n = decision_tree[ix];
			swap(emplacement[n.i_emplacement_source], emplacement[n.i_emplacement_destination]);
		}
        return emplacement;
    };
	
	auto child_nodes = [&](int parent_index, int depth){

		vector<int> emplacement = index2emplacement(parent_index);

		return views::cartesian_product( views::iota(nr_input_rectangles, nr_emplacements),
									  views::iota(0, nr_input_rectangles) |
											views::filter([&](int r){return emplacement[r]==r;}) 
									) |
			views::transform([&](const auto arg) {
				auto const [h, r] = arg;
				auto swap_r_h = [&](int u){
					if (u==r)
						return h;
					if (u==h)
						return r;
					else
						return u;
				};
			
				int sigma_edge_distance = ranges::fold_left(edges | views::transform([&](const Edge& e){return distance_matrix[emplacement[swap_r_h(e.from)] * N + emplacement[swap_r_h(e.to)]];}),
					0, plus<int>()) ;
				
				return DecisionTreeNode{
					.index = -1,
					.parent_index = parent_index,
					.depth=depth,
					.sigma_edge_distance = sigma_edge_distance,
					.i_emplacement_source = r, 
					.i_emplacement_destination = h
				};
			}) |
			ranges::to<vector>();
	};
	
	auto build_decision_tree = [&](){
		printf("enter build_decision_tree()\n");
		
		for (int depth=0; depth<=7; depth++)
		{	
			int size = decision_tree.size();
			
			vector<int> indexes = decision_tree 
						| views::filter([&](const DecisionTreeNode& n){return n.depth==depth-1;})
						| views::transform(&DecisionTreeNode::index)
						| ranges::to<vector>() ;
						
			ranges::sort(indexes, {}, [&](int idx){return decision_tree[idx].sigma_edge_distance;});
			if (indexes.size() > 2000)
				indexes.resize(2000);

			if (depth==0)
				indexes.push_back(-1);

			vector<vector<DecisionTreeNode> > vv(indexes.size());
			transform(execution::par_unseq, begin(indexes), end(indexes), begin(vv), [&](int parent_index){return child_nodes(parent_index, depth);});
			
			ranges::copy( vv | views::join, back_inserter(decision_tree));

			for (int i=size; i<decision_tree.size(); i++)
				decision_tree[i].index = i;
		}
		
		printf("exit build_decision_tree()\n");
	};
	
	build_decision_tree();
	
	return decision_tree;
}


vector<DecisionTreeNode> compute_decision_subtree(const vector<DecisionTreeNode>& decision_tree, int count)
{
	auto walk_up_from = [&](int parent_index)->generator<int>{
		for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
		{
			co_yield index;
		}		
	};
	
    //const vector<int> v = {6,9,9,6};
    //const auto dense_rank = views::zip(v | ranges::to<set>(), views::iota(0)) | ranges::to<unordered_map>() ;
	
	const int n = decision_tree.size();
	vector<int> index = views::iota(0, n) | ranges::to<vector>();
	ranges::sort(index, {}, [&](int id){return decision_tree[id].sigma_edge_distance;});
	
	vector<int> subtree_index = index | 
					views::take(count) |
					views::transform(walk_up_from) |
					views::join |
					ranges::to<vector>() |	//doesn't compile without this
					ranges::to<set>() |
					ranges::to<vector>() ;

	ranges::sort(subtree_index, {}, [&](int id){return decision_tree[id].sigma_edge_distance;});
	
	map<int, long int> subtree_index_map = subtree_index |
					views::enumerate |
					views::transform([](auto arg){const auto [i, id]=arg; return make_pair(id,i);}) |
					ranges::to<map>();
	
	vector<DecisionTreeNode> dst =  subtree_index |
		views::transform([&](int id){
			DecisionTreeNode node = decision_tree[id];
			assert(node.index == id);
			node.index = subtree_index_map[node.index];
			node.parent_index = subtree_index_map[node.parent_index];
			return node;}
		) |
		ranges::to<vector>();
	
	auto walk_up_from_ = [&](int parent_index)->generator<int>{
		for (int index=parent_index; index != -1; index = dst[index].parent_index)
		{
			co_yield index;
		}		
	};
		
	string buffer = views::iota(0, count) |
		views::transform([&](int id){
			return walk_up_from_(id) | 
				ranges::to<vector>() | 
				views::reverse |
				views::transform([&](int id){return dst[id];}) |
				views::transform([](const DecisionTreeNode& n){
					return format(R"({{"depth":{},"sigma_edge_distance":{},"i_emplacement_source":{},"i_emplacement_destination":{}}})",
						n.depth, n.sigma_edge_distance, n.i_emplacement_source, n.i_emplacement_destination);
				}) |
				views::join_with(",\n"s) ;
		}) |
		views::join_with("},\n{"s) |
		ranges::to<string>();
		
//	FILE* f=fopen("decision_tree.json", "w");
//	fprintf(f, "{%s}", buffer.c_str());
	printf("{%s}", buffer.c_str());
//	fclose(f);
	
	return dst;
}


void compute_scores(const vector<DecisionTreeNode>& decision_tree,
		const vector<MyRect>& emplacements_by_id,
		const vector<MyRect>& input_rectangles,
		const vector<Edge>& edges)
{
	int n = input_rectangles.size();

	string buffer = views::iota(0, (int)decision_tree.size()) |
		views::transform([&](int id)->Score{

	//decision_tree.size() can be zero. In this case this instruction will not get executed
        	const int m = emplacements_by_id.size() / decision_tree.size();

			span<const MyRect> rectangles(begin(emplacements_by_id)+m*id, n);

			const int sigma_edge_distance = ranges::fold_left(edges | views::transform([&](const auto& le){
					return rectangle_distance(rectangles[le.from],rectangles[le.to]); }),
				0, plus<int>());

			const auto [width, height] = dimensions(compute_frame(rectangles));

			return {
				.id=id,
				.sigma_edge_distance=sigma_edge_distance,
				.width=width,
				.height=height,
				.total= width + height + sigma_edge_distance
			};
		}) |
		views::transform([](const Score& score)->string{
			const auto [id, sigma_edge_distance, width, height, total] = score;
			return format(R"({{"id":{}, "sigma_edge_distance":{}, "width":{}, "height":{}, "total":{}}})",
				id, sigma_edge_distance, width, height, total);
		}) |
		views::join_with(",\n"s) |
		ranges::to<string>();


	FILE *f=fopen("scores.json", "w");
	fprintf(f, "[%s\n]", buffer.c_str());
	fclose(f);
}


// ./holes2 --dt --skip | awk '/begin cmpt_tr2 id=4 /,/end cmpt_tr2 id=4 /'
// ./holes2 --dt | awk '/begin testid=1 /,/end testid=1 /' | awk '/begin compute_decision_tree/,/end compute_decision_tree/' > holes2.log

int main(int argc, char* argv[])
{
for (const auto& [testid, input_rectangles, edges] : test_input)
{
	D(printf("begin testid=%d \n", testid));
	char file_name[50];

	if (argc==2 && strcmp(argv[1], "--dt")==0)
	{
		const vector<MyRect> holes = compute_holes(input_rectangles);

		D(printf("begin compute_decision_tree()\n"));
		vector<DecisionTreeNode> decision_tree = compute_decision_tree(edges, input_rectangles, holes);
		D(printf("end compute_decision_tree()\n"));
		fflush(stdout);
		
		int count=20;
		vector<DecisionTreeNode> decision_subtree = compute_decision_subtree(decision_tree, count) ;

		sprintf(file_name, "logical_graph%d.json", testid);
		fs::copy("logical_graph.json", file_name, fs::copy_options::update_existing);
		sprintf(file_name, "holes%d.json", testid);
		fs::copy("holes.json", file_name, fs::copy_options::update_existing);
		sprintf(file_name, "decision_tree%d.json", testid);
		fs::copy("decision_tree.json", file_name, fs::copy_options::update_existing);

		sprintf(file_name, "decision_tree%d.dat", testid);
		if(FILE* f = fopen(file_name, "wb")) {
			fwrite(decision_subtree.data(), sizeof (DecisionTreeNode), decision_subtree.size(), f);
			fclose(f);
		}

		vector<MyRect> emplacements_by_id;
		D(printf("begin compute_decision_tree_translations()\n"));
		compute_decision_tree_translations(decision_subtree, input_rectangles, holes, edges, emplacements_by_id);
		D(printf("end compute_decision_tree_translations()\n"));
		fflush(stdout);

		sprintf(file_name, "translation_ranges_%d.json", testid);
		fs::copy("translation_ranges.json", file_name, fs::copy_options::update_existing);

		vector<MyRect> emplacements2_by_id = emplacements_by_id;
		D(printf("begin compute_decision_tree_translations2()\n"));
		compute_decision_tree_translations2(decision_subtree, input_rectangles.size(), edges, emplacements_by_id, emplacements2_by_id);
		D(printf("end compute_decision_tree_translations2()\n"));
		fflush(stdout);

		sprintf(file_name, "translation_ranges2_%d.json", testid);
		fs::copy("translation_ranges2.json", file_name, fs::copy_options::update_existing);

		D(printf("begin compute_scores()\n"));
		compute_scores(decision_subtree, emplacements_by_id, input_rectangles, edges);
		D(printf("end compute_scores()\n"));
		fflush(stdout);

		sprintf(file_name, "scores%d.json", testid);
		fs::copy("scores.json", file_name, fs::copy_options::update_existing);
	}

	if (argc==3 && strcmp(argv[1], "--dt")==0 && strcmp(argv[2], "--skip")==0)
	{
		vector<DecisionTreeNode> decision_tree ;

		sprintf(file_name, "decision_tree%d.dat", testid);
		if(FILE* f = fopen(file_name, "rb")) {
			struct stat stat_buf;
			int rc = stat(file_name, &stat_buf);
			int n = stat_buf.st_size / sizeof(DecisionTreeNode);
			decision_tree.resize(n);
			size_t ret_code = fread(&decision_tree[0], sizeof (DecisionTreeNode), n, f);
					fclose(f);
		}

		vector<TranslationRangeItem> translation_ranges ;

		sprintf(file_name, "translation_ranges%d.dat", testid);
		if(FILE* f = fopen(file_name, "rb")) {
			struct stat stat_buf;
			int rc = stat(file_name, &stat_buf);
			int n = stat_buf.st_size / sizeof(TranslationRangeItem);
			translation_ranges.resize(n);
			size_t ret_code = fread(&translation_ranges[0], sizeof (TranslationRangeItem), n, f);
			fclose(f);
		}

		vector<MyRect> emplacements_by_id, emplacements2_by_id;
		compute_decision_tree_translations2(decision_tree, input_rectangles.size(), edges, emplacements_by_id,emplacements2_by_id);

		compute_scores(decision_tree, emplacements_by_id, input_rectangles, edges);
	}
	D(printf("end testid=%d \n", testid));
}

	if (argc == 1)
	{
		test_rect_trim();

		test_fit();

		test_translations();
	}

	return 0;
}
