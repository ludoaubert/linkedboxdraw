#include <vector>
#include <string>
#include <deque>
#include <map>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <span>
#include <stdio.h>
#include <cstring>
//#include <fmt/ranges.h>
//#include <format>
#include "FunctionTimer.h"
#include "MyRect.h"
using namespace std;

//#define _TRACE_

#ifdef _TRACE_
#  define D(x) x
#else
#  define D(x)
#endif

//Cf compute_box_rectangles.js
const int RECTANGLE_BOTTOM_CAP=200;


struct LogicalEdge {
	int from;
	int to;

	friend auto operator<=>(const LogicalEdge&, const LogicalEdge&) = default;
};


struct TopologicalEdge {
	int from;
	int to;
	int distance;

	friend auto operator<=>(const TopologicalEdge&, const TopologicalEdge&) = default;
};


struct MyVector
{
	float x=0.0f;
	float y=0.0f;

	operator MyPoint() const {
		return {x, y};
	}
};

inline MyVector operator*(int16_t value, const MyVector& vec)
{
	const auto& [x, y] = vec;
	return {value*x, value*y};
}


struct RectHole {int ri; RectCorner corner; MyVector direction; int value; MyRect rec;};

//	lightweight node

struct RectMap
{
	int i_emplacement_source, i_emplacement_destination;
};

struct DecisionTreeNode
{
	int i;
	int parent_index=-1;
	int depth;
	RectMap recmap;
	int match;
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
	int min_sweep_value, max_sweep_value=INT16_MAX;
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


const vector<MyRect> input_rectangles = {
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
};

//bi directional edges
const vector<LogicalEdge> logical_edges = {
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
};


const vector<MyRect> emplacements={
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
	{.m_left=130, .m_right=345, .m_top=451, .m_bottom=627},

	{.m_left=88, .m_right=130, .m_top=451, .m_bottom=493},
	{.m_left=88, .m_right=130, .m_top=585, .m_bottom=627},
	{.m_left=88, .m_right=130, .m_top=627, .m_bottom=669},
	{.m_left=103, .m_right=246, .m_top=20, .m_bottom=163},
	{.m_left=130, .m_right=227, .m_top=627, .m_bottom=724},
	{.m_left=140, .m_right=283, .m_top=20, .m_bottom=163},
	{.m_left=248, .m_right=345, .m_top=627, .m_bottom=724},
	{.m_left=263, .m_right=406, .m_top=20, .m_bottom=163},
	{.m_left=283, .m_right=330, .m_top=324, .m_bottom=371},
	{.m_left=283, .m_right=330, .m_top=340, .m_bottom=387},
	{.m_left=283, .m_right=330, .m_top=404, .m_bottom=451},
	{.m_left=283, .m_right=406, .m_top=40, .m_bottom=163},
	{.m_left=283, .m_right=406, .m_top=41, .m_bottom=164},
	{.m_left=296, .m_right=345, .m_top=627, .m_bottom=676},
	{.m_left=297, .m_right=345, .m_top=676, .m_bottom=724},
	{.m_left=345, .m_right=393, .m_top=676, .m_bottom=724},
	{.m_left=345, .m_right=410, .m_top=451, .m_bottom=516},
	{.m_left=463, .m_right=527, .m_top=164, .m_bottom=228},
	{.m_left=487, .m_right=552, .m_top=451, .m_bottom=516},
	{.m_left=505, .m_right=553, .m_top=676, .m_bottom=724},
	{.m_left=544, .m_right=608, .m_top=164, .m_bottom=228},
	{.m_left=553, .m_right=601, .m_top=676, .m_bottom=724},
	{.m_left=560, .m_right=608, .m_top=164, .m_bottom=212},
	{.m_left=566, .m_right=630, .m_top=660, .m_bottom=724},
	{.m_left=689, .m_right=753, .m_top=660, .m_bottom=724},
	{.m_left=774, .m_right=947, .m_top=20, .m_bottom=193},
	{.m_left=774, .m_right=947, .m_top=23, .m_bottom=196}
};


struct TranslationRangesTestContext
{
	int testid;
	vector<DecisionTreeNode> decision_tree;
	vector<TranslationRangeItem> expected_translation_ranges;
};


const TranslationRangesTestContext TRTestContexts[2]={
{
	.testid=0,
	.decision_tree={
		{.i=3492, .parent_index=-1, .depth=0, .recmap={.i_emplacement_source=5, .i_emplacement_destination=33}, .match=24}
	},
	.expected_translation_ranges={}
},
{
	.testid=1,
	.decision_tree={
		{.i=0, .parent_index=-1, .depth=0, .recmap={.i_emplacement_source=0, .i_emplacement_destination=35}, .match=24},
		{.i=1, .parent_index=0, .depth=1, .recmap={.i_emplacement_source=1, .i_emplacement_destination=33}, .match=24},
		{.i=2, .parent_index=1, .depth=2, .recmap={.i_emplacement_source=2, .i_emplacement_destination=1}, .match=24},
		{.i=3, .parent_index=2, .depth=3, .recmap={.i_emplacement_source=6, .i_emplacement_destination=2}, .match=24},
		{.i=4, .parent_index=3, .depth=4, .recmap={.i_emplacement_source=12, .i_emplacement_destination=6}, .match=26},
		{.i=5, .parent_index=4, .depth=5, .recmap={.i_emplacement_source=13, .i_emplacement_destination=0}, .match=24},
		{.i=6, .parent_index=5, .depth=6, .recmap={.i_emplacement_source=14, .i_emplacement_destination=22}, .match=26},
		{.i=7, .parent_index=5, .depth=6, .recmap={.i_emplacement_source=14, .i_emplacement_destination=26}, .match=26},
		{.i=8, .parent_index=5, .depth=6, .recmap={.i_emplacement_source=14, .i_emplacement_destination=27}, .match=26},
		{.i=9, .parent_index=5, .depth=6, .recmap={.i_emplacement_source=14, .i_emplacement_destination=32}, .match=26},
	},
	.expected_translation_ranges={}
}
};


vector<RectHole> compute_holes(const vector<MyRect>& input_rectangles)
{
	MyRect frame={
		.m_left=ranges::min(input_rectangles | views::transform(&MyRect::m_left)),
		.m_right=ranges::max(input_rectangles | views::transform(&MyRect::m_right)),
		.m_top=ranges::min(input_rectangles | views::transform(&MyRect::m_top)),
		.m_bottom=ranges::max(input_rectangles | views::transform(&MyRect::m_bottom))
	};

	int n = input_rectangles.size();

	vector<RectHole> holes;

	for (int i=0; i<n; i++)
	{
		const MyRect& r = input_rectangles[i];
/*
	TOP_LEFT=0,
	BOTTOM_LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT
*/
		const MyPoint rect_corners[4]={
			{.x=r.m_left, .y=r.m_top},
			{.x=r.m_left, .y=r.m_bottom},
			{.x=r.m_right, .y=r.m_top},
			{.x=r.m_right, .y=r.m_bottom}
		};

		const MyVector directions[4][3]={
				{{.x=-1, .y=-1},{.x=+1, .y=-1},{.x=-1, .y=+1}},
				{{.x=-1, .y=+1},{.x=+1, .y=+1},{.x=-1, .y=-1}},
				{{.x=+1, .y=+1},{.x=+1, .y=-1},{.x=-1, .y=-1}},
				{{.x=-1, .y=+1},{.x=+1, .y=+1},{.x=+1, .y=-1}}
		};

		for (int rc : views::iota(0,4))
		{
			const MyPoint& pt = rect_corners[rc] ;

			for (const MyVector& dir : directions[rc])
			{
				int intervalle[2]={2, INT16_MAX};
				auto& [m, M] = intervalle;
				while (M > 1+m)
				{
					int value = M==INT16_MAX ? 2*m : (m+M)/2 ;
					MyRect rec = rect(pt, pt + value*dir);
					auto rg = input_rectangles | views::filter([&](const MyRect& r){return intersect_strict(rec,r) /*|| is_inside(r, rec)*/;});
					(rg.empty() && is_inside(rec,frame) ? m : M) = value;
					printf("{.i=%d, .RectCorner=%s, .direction={.x=%.0f, .y=%.0f} [.m=%d .M=%d]}\n", i, RectCornerString[rc], dir.x, dir.y, m, M);
				}

				if (m > 2)
				{
 					MyRect rec = rect(pt, pt + m*dir);
					holes.push_back({.ri=i, .corner=(RectCorner)rc, .direction=dir, .value=m, .rec=rec});
				}
			}
		}
	}

	ranges::sort(holes, {}, &RectHole::rec);
	vector<RectHole> holes_dedup;
	ranges::unique_copy(holes, back_inserter(holes_dedup), {}, &RectHole::rec);

	holes.clear();

	for (const auto& [ri, corner, direction, value, rec] : holes_dedup)
	{
		if (5*value >= RECTANGLE_BOTTOM_CAP)
		{
			holes.push_back({ri, corner, direction, value, rec});
		}
	}

	return holes;
};



bool filter(const LogicalEdge& e){

	int dist = rect_distance(input_rectangles[e.from], input_rectangles[e.to]);
	return dist <= 20;
};


vector<int> compute_connected_components(size_t n,
					const vector<LogicalEdge>& logical_edges)
{
	assert(ranges::is_sorted(logical_edges));
	vector<int> connected_component(n, -1);

	auto rec_compute_connected_components = [&](int i, int c, auto&& rec_compute_connected_components)->void{
		connected_component[i] = c;

		span adj_list = ranges::equal_range(logical_edges, i, {}, &LogicalEdge::from);

		for (const LogicalEdge& e : adj_list)
		{
			if (connected_component[e.to] == -1 && filter(e))
				rec_compute_connected_components(e.to, c, rec_compute_connected_components);
		}
	};

	int c=0;
	while (true)
	{
		auto it=ranges::find(connected_component, -1);
		if (it == end(connected_component))
			break;
		int i = ranges::distance(begin(connected_component), it);
		rec_compute_connected_components(i, c++, rec_compute_connected_components);
	}
	return connected_component;
}

vector<RectLink> sweep(Direction update_direction, const vector<MyRect>& rectangles)
{
	FunctionTimer ft("sweep");

	const int N=30;
	int n = rectangles.size();

	const MyPoint translation2[2]={{.x=1, .y=0}, {.x=0, .y=1}};

	SweepLineItem sweep_line_buffer[2*N];
	span sweep_line(sweep_line_buffer, 2*n);

	ActiveLineItem active_line[N];
	RectLink rect_links_buffer[256];

//use the sweep_line that is not impacted by selected translation
	Direction sweep_direction = Direction(1-update_direction);

	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
	auto [minSweepRectDim, maxSweepRectDim] = rectDimRanges[sweep_direction];
{
        FunctionTimer ft("cft_fill_sweepline");
		//sweep_line.reserve(2*n);

	for (int ri=0; ri < n; ri++)
	{
		sweep_line_buffer[2*ri]={.sweep_value=rectangles[ri][minSweepRectDim], .rectdim=minSweepRectDim, .ri=ri};
		sweep_line_buffer[2*ri+1]={.sweep_value=rectangles[ri][maxSweepRectDim], .rectdim=maxSweepRectDim, .ri=ri};
	}
}

	const MyPoint& translation = translation2[update_direction] ;
{
        FunctionTimer ft("cft_sort_sweepline");
	ranges::sort(sweep_line, CustomLess());
}
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
			rect_links_buffer[rect_links_size++] = {
				.i=active_line[pos-1].i,
				.j=active_line[pos].i,
				.min_sweep_value=sweep_value
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
			rect_links_buffer[rect_links_size++] = {
				.i=active_line[pos-1].i,
				.j=active_line[pos].i,
				.min_sweep_value=sweep_value
			};

			if (RectLink *rl=active_line[pos].links[0]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			if (RectLink *rl=active_line[pos-1].links[1]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			active_line[pos].links[0] = active_line[pos-1].links[1] = & rect_links_buffer[rect_links_size - 1];
		}
		if (pos+1 < active_line_size)
		{
			rect_links_buffer[rect_links_size++] = {
				.i=active_line[pos].i,
				.j=active_line[pos+1].i,
				.min_sweep_value=sweep_value
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

{
        FunctionTimer ft("cft_sweep");
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
}

{
        FunctionTimer ft("cft_rectlinks_sort");
	sort(rect_links_buffer, rect_links_buffer + rect_links_size);
}

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

	// return rg | views::to<vector>; TODO C++23

	return vector(ranges::begin(rg), ranges::end(rg));
}


void spread(Direction update_direction, const vector<RectLink>& rect_links, vector<MyRect>& rectangles)
{
//TODO: use chunk_by C++23
	const int N=30;
	int n = rectangles.size();

        MyPoint translations[N];

        ranges::fill(translations, MyPoint{0,0});

	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}

	auto rec_push_hole=[&](int ri, int tr, auto&& rec_push_hole)->void{

		D(printf("entering rec_push_hole(ri=%d ,tr=%d)\n", ri, tr));

		span adj_list = ranges::equal_range(rect_links, ri, {}, &RectLink::i);

		for (const RectLink& rl : adj_list)
		{
			int tr2= rectangles[ri][maxCompactRectDim] - rectangles[rl.j][minCompactRectDim];

			if (tr2 < 0)
				tr2 = 0;

			rec_push_hole(rl.j, tr+tr2, rec_push_hole);
		}
		int16_t &tri = translations[ri][update_direction];
		tri = max<int16_t>(tri, tr) ;
	};

	auto push_hole=[&](){
		for (int j : views::iota(0,n) | views::filter([&](int j){return ranges::count(rect_links,j,&RectLink::j)==0;}))
		{
			int tr=0;
			rec_push_hole(j, tr, rec_push_hole);
		}
	};
	push_hole();

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


void compact(Direction update_direction, const vector<RectLink>& rect_links, vector<MyRect>& rectangles)
{
//TODO: use chunk_by C++23
        const int N=30;
        int n = rectangles.size();

        MyPoint translations[N];

        ranges::fill(translations, MyPoint{0,0});

        auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}

        int compact_dimension=0;
        int tr;

        MyRect frame={
                .m_left=ranges::min(rectangles | views::transform(&MyRect::m_left)),
                .m_right=ranges::max(rectangles | views::transform(&MyRect::m_right)),
                .m_top=ranges::min(rectangles | views::transform(&MyRect::m_top)),
                .m_bottom=ranges::max(rectangles | views::transform(&MyRect::m_bottom))
        };
{
        FunctionTimer ft("cft_query_compact_dim");
        auto rec_query_compact_dimension=[&](int ri, auto&& rec_query_compact_dimension)->int{
                span adj_list = ranges::equal_range(rect_links, ri, {}, &RectLink::i);
                if (adj_list.empty())
                {
                        return dimensions(rectangles[ri])[update_direction];
                }
                int tr = ranges::max(adj_list | views::transform([&](const RectLink& e){return rec_query_compact_dimension(e.j, rec_query_compact_dimension);}));
                return tr + dimensions(rectangles[ri])[update_direction];
        };

        auto query_compact_dimension=[&]()->int{
                return ranges::max(views::iota(0,n) |
				views::filter([&](int j){return ranges::count(rect_links,j,&RectLink::j)==0;}) |
				views::transform([&](int j){return rec_query_compact_dimension(j, rec_query_compact_dimension);})
		);
        };

	compact_dimension = query_compact_dimension();
	tr = dimensions(frame)[update_direction] - compact_dimension;
	D(printf("compact_dimension=%d tr=%d\n", compact_dimension, tr));
}
{
	FunctionTimer ft("cft_push");
	auto rec_push=[&](int ri, int tri, auto&& rec_push)->void{
		span adj = ranges::equal_range(rect_links, ri, {}, &RectLink::i);
		translations[ri][update_direction] = tri;
		for (const RectLink& e : adj)
		{
			int trj = tri - (rectangles[e.j][minCompactRectDim] - rectangles[ri][maxCompactRectDim]);
			if (trj > 0)
				rec_push(e.j, trj, rec_push);
		}
	};

        auto push=[&](int tr){
		for (int j : views::iota(0,n) | views::filter([&](int j){return ranges::count(rect_links,j,&RectLink::j)==0;}))
		{
			int trj = tr - (rectangles[j][minCompactRectDim] - frame[minCompactRectDim]);
			if (trj > 0)
				rec_push(j, trj, rec_push);
                }
         };
         push(tr);
}
{
#ifdef _TRACE_
         D(printf("translations={"));
         for (int ri=0; ri < n; ri++)
         {
                int tr = translations[ri][update_direction];
                if (tr != 0)
                        D(printf("{ri=%d, tr=%d},",ri, tr));
         }
         D(printf("}\n"));
#endif
         for (int ri=0; ri < n; ri++)
         {
                rectangles[ri] += translations[ri];
         }
}
}

enum MirroringState
{
	ACTIVE,
	IDLE
};

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


const unsigned NR_MIRRORING_OPTIONS=4;

const Mirror mirrors[NR_MIRRORING_OPTIONS][2]={
	{
		{.mirroring_state=IDLE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
	},
	{
		{.mirroring_state=IDLE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
	}
};

const char* MirroringStrings[NR_MIRRORING_OPTIONS]={
	"IDLE,IDLE",
	"IDLE,ACTIVE",
	"ACTIVE,IDLE",
	"ACTIVE,ACTIVE"
};

const unsigned NR_JOB_PIPELINES=2;

const Job pipelines[NR_JOB_PIPELINES][1]={
	{
		{.algo=SPREAD, .update_direction=EAST_WEST}
	},
	{
		{.algo=SPREAD, .update_direction=NORTH_SOUTH}
	}
};

const unsigned NR_RECT_CORNERS=4;

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

void apply_mirror(const Mirror& mirror, vector<MyRect>& rectangles)
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


void apply_job(const Job& job, vector<MyRect>& rectangles)
{
	const auto& [algo, update_direction] = job;

	vector<RectLink> rect_links = sweep(update_direction, rectangles);

	switch (algo)
	{
	case SPREAD:
		spread(update_direction, rect_links, rectangles);
		break;
	case COMPACT:
		compact(update_direction, rect_links, rectangles);
		break;
	}
}


struct ProcessSelector
{
	unsigned pipeline, mirroring, match_corner;
};


// TODO: use upcoming C++23 views::cartesian_product()
vector<ProcessSelector> cartesian_product()
{
	vector<ProcessSelector> result;

	for (int pipeline=0; pipeline < NR_RECT_CORNERS*NR_JOB_PIPELINES; pipeline++)
		for (int mirroring=0; mirroring < NR_MIRRORING_OPTIONS; mirroring++)
			for (int match_corner=0; match_corner < NR_RECT_CORNERS; match_corner++)
				result.push_back({pipeline, mirroring, match_corner});

	return result;
}


const vector<ProcessSelector> process_selectors = cartesian_product();


vector<TranslationRangeItem> compute_decision_tree_translations(const vector<DecisionTreeNode>& decision_tree,
																const vector<MyRect>& input_emplacements,
																const vector<MyRect>& input_rectangles)
{
	int m = input_emplacements.size();
	int n = input_rectangles.size();
	vector<ProcessSelector> selectors(decision_tree.size());
	vector<TranslationRangeItem> translation_ranges;
	vector<MyRect> emplacements(m);
	vector<MyRect> rectangles(n);

	auto tf=[&](int id, unsigned pipeline, unsigned mirroring, unsigned match_corner){

		D(printf("calling tf(id=%d, pipeline=%u, mirroring=%u, match_corner=%u)\n", id, pipeline, mirroring, match_corner));

		for (MyRect& r : rectangles)
			r = emplacements[r.i];

		const auto& [i_emplacement_source, i_emplacement_destination] = decision_tree[id].recmap;
		const auto [RectDimX, RectDimY] = corners[match_corner];
		MyRect &r1 = rectangles[i_emplacement_source],
				&r2 = *find_if(rectangles.rbegin(), rectangles.rend(), [&](const MyRect& r){return r.i==i_emplacement_destination;});
		const MyRect r = r1;
		r1 += MyPoint{.x=r2[RectDimX] - r1[RectDimX], .y=r2[RectDimY] - r1[RectDimY]};
		if (i_emplacement_destination >= n)
		{
			int i=r2.i;
			r2 = r;
			r2.i = i;
		}

		for (const Mirror& mirror : mirrors[mirroring])
			apply_mirror(mirror, rectangles);
		for (const Job& job : pipelines[pipeline])
			apply_job(job, rectangles);
		for (const Mirror& mirror : mirrors[mirroring])
			apply_mirror(mirror, rectangles);
	};

	auto rec_tf=[&](int id, auto&& rec_tf)->void{

		if (int parent_index = decision_tree[id].parent_index; parent_index != -1)
		{
			rec_tf(parent_index, rec_tf);
		}

		const auto [pipeline, mirroring, match_corner] = selectors[id];

		tf(id, pipeline, mirroring, match_corner);

		for (const MyRect& r : rectangles)
			emplacements[r.i] = r;
	};


	for (int id=0; id < decision_tree.size(); id++)
	{
		memcpy(&emplacements[0], &input_emplacements[0], sizeof(MyRect)*m);
		for (int i=0; i<m; i++)
			emplacements[i].i = i;

//gather the rectangles of interest
                rectangles.resize(n);
                memcpy(&rectangles[0], &emplacements[0], sizeof(MyRect)*n);
                for (int pid=id; pid != -1; pid=decision_tree[pid].parent_index)
                {
                        const auto& [i_emplacement_source, i_emplacement_destination] = decision_tree[pid].recmap;
                        if (i_emplacement_destination >= n)
                                rectangles.push_back( emplacements[i_emplacement_destination] );
                }

		if (int parent_index = decision_tree[id].parent_index; parent_index != -1)
                {
                        rec_tf(parent_index, rec_tf);
                }

/*
// TODO: use upcoming C++23 views::cartesian_product()
	auto rg = views::cartesian_product( views::iota(0, NR_JOB_PIPELINES),
										views::iota(0, NR_MIRRORING_OPTIONS),
										views::iota(0, NR_RECT_CORNERS));
	const auto& [pipeline, mirroring, match_corner] = ranges::min(rg, {}, [&](const auto [pipeline, mirroring, match_corner]{...

*/

		const auto [pipeline, mirroring, match_corner] = ranges::min(process_selectors, {}, [&](const ProcessSelector& ps){
			D(printf("MirroringStrings[mirroring]=%s\n", MirroringStrings[mirroring]));
			D(printf("CornerStrings[match_corner]=%s\n", CornerStrings[match_corner]));

			tf(id, ps.pipeline, ps.mirroring, ps.match_corner);

			auto rg1 = logical_edges |
				views::transform([&](const auto& le){ return rectangle_distance(rectangles[le.from],rectangles[le.to]);	});

			auto rg2 = views::iota(0,n) |
				views::transform([&](int i)->TranslationRangeItem{
					const MyRect &ir = input_rectangles[i], &r = rectangles[i];
					MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
					return {id, i, tr};}) |
				views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
				views::filter([&](const TranslationRangeItem& item){return item.ri != decision_tree[id].recmap.i_emplacement_source;}) |
				views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);});

			int cost = dim_max(compute_frame(rectangles)) +
						accumulate(ranges::begin(rg1), ranges::end(rg1),0) +
						accumulate(ranges::begin(rg2), ranges::end(rg2),0) ;

			D(printf("cost=%d\n", cost));
			return cost;
		});

		selectors[id] = {pipeline, mirroring, match_corner};

		D(printf("MirroringStrings[mirroring]=%s\n", MirroringStrings[mirroring]));
		D(printf("CornerStrings[match_corner]=%s\n", CornerStrings[match_corner]));

		tf(id, pipeline, mirroring, match_corner);

                for (const MyRect& r : rectangles)
                        emplacements[r.i] = r;

		auto rg = views::iota(0,n) |
                        views::transform([&](int i)->TranslationRangeItem{
                                        const MyRect &ir = input_emplacements[i], &r = emplacements[i];
                                        MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
                                        return {id, i, tr};}) |
			views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};});

		for (TranslationRangeItem item : rg)
		{
			translation_ranges.push_back(item);
		}
	}

{
        FILE *f=fopen("translation_ranges.json", "w");
        fprintf(f, "[\n");
        for (int i=0; i < translation_ranges.size(); i++)
        {
                const auto [id, ri, tr] = translation_ranges[i];
                fprintf(f, "{\"id\":%d, \"ri\":%d, \"x\":%d, \"y\":%d}%s\n", id, ri, tr.x, tr.y,
                        i+1 == translation_ranges.size() ? "": ",");
        }
        fprintf(f, "]\n");
        fclose(f);
}

	return translation_ranges;
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
        }
};


void test_fit()
{
	for (const auto& [testid, input_rectangles, pipeline, expected_translations] : test_contexts)
	{
		vector<MyRect> rectangles = input_rectangles;
		int dm1 = dim_max(compute_frame(input_rectangles));
		for (const Job& job : pipeline)
			apply_job(job, rectangles);

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
	for (const auto [testid, decision_tree, expected_translation_ranges] : TRTestContexts)
	{
		vector<TranslationRangeItem> translation_ranges = compute_decision_tree_translations(decision_tree,
                	                                                			emplacements,
                        	                                        			input_rectangles);
		bool bOK = translation_ranges == expected_translation_ranges;
		printf("translation ranges testid=%d : %s\n", testid, bOK ? "OK" : "KO");
return;
	}
};


vector<DecisionTreeNode> compute_decision_tree(const vector<MyRect>& input_rectangles,
						const vector<LogicalEdge>& logical_edges,
						vector<MyRect>& emplacements)
{
	const int n = input_rectangles.size();

{
/* TODO
	auto jv = input_rectangles |
		views::transform([&](const MyRect& r)->string{
			char buffer[200];
			sprintf(buffer, "\t{\"m_left\":%d, \"m_right\":%d, \"m_top\":%d, \"m_bottom\":%d},\n",r.m_left, r.m_right, r.m_top, r.m_bottom);
			return buffer;}) |
		views ::join;
*/
	char buffer[10*1000];
	int pos=0;
	pos += sprintf(buffer + pos,"{\n\"input_rectangles\":[");
	for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : input_rectangles)
        {
                pos += sprintf(buffer + pos, "\n\t{\"m_left\":%d, \"m_right\":%d, \"m_top\":%d, \"m_bottom\":%d},", m_left, m_right, m_top, m_bottom);
        }
	pos += sprintf(buffer + --pos,"\n],\n\"logical_edges\":[");
	for (const auto& [from, to] : logical_edges)
	{
		pos += sprintf(buffer + pos, "\n\t{\"from\":%d, \"to\":%d},", from, to);
	}
	pos += sprintf(buffer + --pos,"\n],\n\"topological_edges\":[");

        for (int ri=0; ri < n; ri++)
        {
                for (int rj : views::iota(0, n) | views::filter([&](int rj){return ri != rj && edge_overlap(input_rectangles[ri], input_rectangles[rj]);}))
                {
                        pos += sprintf(buffer + pos, "\n\t{\"from\":%d, \"to\":%d},", ri, rj);
                }
	}
	pos += sprintf(buffer + --pos,"\n]\n}");

        FILE *f=fopen("logical_graph.json", "w");
	fprintf(f, "%s", buffer);
	fclose(f);
}

	for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : input_rectangles)
	{
		printf("{.m_left=%d, .m_right=%d, .m_top=%d, .m_bottom=%d}\n", m_left, m_right, m_top, m_bottom);
	}
	vector<RectHole> holes = compute_holes(input_rectangles);

{
	char buffer[100*1000];
	int pos=0;
        pos += sprintf(buffer + pos,"{\n\"holes\":[");
	for (int hi=0; hi < holes.size(); hi++)
        {
                const auto& [ri, RectCorner, direction, value, rec] = holes[hi];
		pos += sprintf(buffer+pos, "\n\t{\"hi\":%d, \"ri\":%d, \"RectCorner\":%d, \"RectCornerString\":\"%s\", \"direction\":{\"x\":%.0f,\"y\":%.0f},\"value\":%d, \"rec\":{\"m_left\":%d,\"m_right\":%d,\"m_top\":%d,\"m_bottom\":%d}},",
			hi, ri, RectCorner, RectCornerString[RectCorner], direction.x, direction.y, value, rec.m_left, rec.m_right, rec.m_top, rec.m_bottom);
	}
	pos += sprintf(buffer + --pos,"\n],\n\"topological_contact\":[");

	for (int hi=0; hi < holes.size(); hi++)
	{
		RectHole& rh = holes[hi];
                for (int rj : views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rh.rec, input_rectangles[rj]);}))
                {
                        pos += sprintf(buffer+pos, "\n\t{\"hi\":%d, \"rj\":%d},", hi, rj);
                }
	}
	pos += sprintf(buffer + --pos, "\n]\n}");

        FILE *f=fopen("holes.json", "w");
        fprintf(f, "%s", buffer);
        fclose(f);
}

//La liste des rectangles et des trous devient une liste d'emplacements, et un graphe topologique.

	for (const MyRect &r : input_rectangles)
		emplacements.push_back(r);
	for (const RectHole &rh : holes)
		emplacements.push_back(rh.rec);

	vector<TopologicalEdge> topological_edges;
	for (int i=0; i < emplacements.size(); i++)
	{
		for (int j=0; j < emplacements.size(); j++)
		{
			int dist = rect_distance(emplacements[i], emplacements[j]);
			if (dist < 20)
				topological_edges.push_back({.from=i, .to=j, .distance=dist});
		}
	}

	ranges::sort(topological_edges);

	vector<int> connected_component = compute_connected_components(input_rectangles.size(), logical_edges);

//	fmt::print("connected_component: {}\n", connected_component);

	printf("connected_component: ");
	for (int c : connected_component)
		printf("%d, ", c);
	printf("\n");

	int nb = ranges::max(connected_component);
	vector<int> cc_size(nb, 0);
	for (int c : connected_component)
		cc_size[c]++;
	auto it = ranges::max_element(cc_size);
	int cmax = ranges::distance(begin(cc_size), it);
// si connected_component[i]==cmax alors i ne doit pas etre deplacé.

	vector<int> recmap(input_rectangles.size());
	for (int i=0; i < recmap.size(); i++)
		recmap[i] = i;

	vector<EtatEmplacement> etat_emplacement(emplacements.size());
	vector<int> mapping(input_rectangles.size());

	vector<DecisionTreeNode> decision_tree;

	auto build_decision_tree = [&](int parent_index, auto&& build_decision_tree)->void{

		for (int i=0; i < input_rectangles.size(); i++)
		{
			printf("i=%d\n", i);

// par default, les intput_rectangles sont des emplacements non libres, les autres emplacements etant libres
			ranges::fill(etat_emplacement, LIBRE);
			for (int ii=0; ii < input_rectangles.size(); ii++)
				etat_emplacement[ii] = OCCUPE;

			deque<RectMap> chemin;

			for (int pos=parent_index; pos != -1; pos = decision_tree[pos].parent_index)
			{
				chemin.push_front( decision_tree[pos].recmap );
			}

			printf("chemin=");
			for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
				printf("{.i_emplacement_source=%d, i_emplacement_destination=%d},", i_emplacement_source, i_emplacement_destination);
			printf("\n");

			int depth = chemin.size();

                        if (connected_component[i] == cmax && depth <= 2)
			{
				printf("connected_component[%d] == cmax && depth=%d <= 2\n", i, depth);
			}
			else if (connected_component[i] != cmax && depth > 2)
			{
                                printf("connected_component[%d] != cmax && depth=%d > 2\n", i, depth);
			}
			else
                        {
                                printf("connected_component[%d] == %d. depth=%d. skipping %d\n", i, cmax, depth, i);
                                continue;
                        }

			if (depth > 6)
				continue;

			for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
			{
				swap(etat_emplacement[i_emplacement_source], etat_emplacement[i_emplacement_destination]);
			}

			for (int i=0; i<input_rectangles.size(); i++)
				mapping[i]=i;
			for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
			{
				mapping[i_emplacement_source] = i_emplacement_destination;
			}

			if (mapping[i] != i)
			{
				printf("on ne mappe pas 2 fois un meme emplacement.\n");
				printf("mapping[%d] != %d\n", i, i);
				continue;
			}

			span le_adj_list = ranges::equal_range(logical_edges, i, {}, &LogicalEdge::from);

		//si tous les liens de i {e sont des liens dont e.j n'a pas ete mappé et e.j que l'on peut deplacer}, alors i n'est pas
		// stable
			if (ranges::all_of(le_adj_list,
					[&](const LogicalEdge& e){return mapping[e.to]==e.to && connected_component[e.to] != cmax;}
					)
			)
			{
				printf("tous les liens de %d {e sont des liens dont e.j n'a pas ete mappé et e.j que l'on peut deplacer}\n", i);
				printf("%d is not stable\n", i);
				continue;
			}

			for (int j=0; j < emplacements.size(); j++)
			{
				printf("i=%d j=%d h=%d\n", i, j, j-n);

				if (j == i)
					continue;

				if (etat_emplacement[j] == OCCUPE)
				{
					printf("etat_emplacement[%d] == OCCUPE\n", j);
					continue;
				}

			//on regarde si emplacements[j] intersecte un emplacement deja occupé
				if (ranges::any_of(views::iota(input_rectangles.size()) |
									views::take(emplacements.size() - input_rectangles.size()) |
									views::filter([&](int i){return i!=j;}) |
									views::filter([&](int i){return etat_emplacement[i]==OCCUPE;}),
									[&](int i){return intersect_strict(emplacements[i], emplacements[j]);}
									)
					)
				{
					printf("emplacements[%d] intersecte un emplacement deja occupé\n", j);
					continue;
				}

// l'emplacement j est-il topologiquement lié aux rectangles auxquels i est logiquement lié et que l'on ne peut pas deplacer ?
			//les rectangles auxquels i est logiquement lié et que l'on ne peut pas déplacer:
				auto rg1 = le_adj_list |
// si connected_component[i]==cmax alors i ne doit pas etre deplacé.
					views::filter([&](const LogicalEdge& e){return connected_component[e.to] == cmax;}) |
					views::transform([](const LogicalEdge& e){return e.to;});

				printf("rg1={");
				for (int i : rg1)
					printf(" %d,", i);
				printf("}\n");


				span te_adj_list = ranges::equal_range(topological_edges, j, {}, &TopologicalEdge::from);

			//les rectangles auxquels j est topologiquement lié:
				auto rg2 = te_adj_list |
					views::filter([&](const TopologicalEdge& e){return e.to < input_rectangles.size();}) |
					views::filter([&](const TopologicalEdge& e){return mapping[e.to]==e.to /*connected_component[e.to] == cmax*/;}) |
					views::transform([](const TopologicalEdge& e){return e.to;});

                                printf("rg2={");
                                for (int i : rg2)
                                        printf(" %d,", i);
                                printf("}\n");

				assert(ranges::is_sorted(rg1));
				assert(ranges::is_sorted(rg2));

				if (ranges::includes(rg2, rg1)==false)
				{
					printf("l'emplacement %d(h=%d) est-il topologiquement lié aux rectangles auxquels %d est logiquement lié et que l'on ne peut pas deplacer ?\n", j, j-n, i);
					printf("ranges::includes(rg2, rg1)==false\n");
					continue;
				}

			//ensuite on mappe les liens de i et on regarde si ils figurent bien dans les liens de j
			// en ne gardant que les liens de i {e dont e.j a deja ete mappé ou e.j que l'on ne peut deplacer}
				mapping[i]=j;
				auto rg3 = le_adj_list |
					views::filter([&](const LogicalEdge& e){return mapping[e.to]!=e.to || connected_component[e.to] == cmax;}) |
					views::transform([&](const LogicalEdge& e){return TopologicalEdge{.from=mapping[e.from], .to=mapping[e.to], .distance=0};});

				assert(ranges::is_sorted(te_adj_list));
			//rg4 est triée, mais pas rg3 because mapping shuffles the ordering
				auto it = ranges::find_if(rg3, [&](const TopologicalEdge& e){return ranges::count(te_adj_list, e)==0;});
				if (it != ranges::end(rg3))
				{
					const TopologicalEdge& e = *it;
					printf("ensuite on mappe les liens de %d et on regarde si ils figurent bien dans les liens de %d\n", i, j);
					printf("it != ranges::end(rg3)\n");
					printf("mapped TopologicalEdge={.from=%d, .to=%d} ne figure pas parmi les liens topologiques de %d\n", e.from, e.to, j);
					continue;
				}

				decision_tree.push_back({
					.parent_index=parent_index,
					.depth=depth,
					.recmap={
						.i_emplacement_source=i,
						.i_emplacement_destination=j
					}
				});

				build_decision_tree(decision_tree.size()-1, build_decision_tree);
			}
		}
	};

	build_decision_tree(-1, build_decision_tree);

	printf("decision_tree.size()=%ld\n", decision_tree.size());

	for (int i=0; i < decision_tree.size(); i++)
	{
		deque<RectMap> chemin;

		for (int pos=i; pos != -1; pos = decision_tree[pos].parent_index)
		{
			chemin.push_front( decision_tree[pos].recmap );
		}
		for (int ii=0; ii<input_rectangles.size(); ii++)
			mapping[ii]=ii;
		for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
		{
			mapping[i_emplacement_source] = i_emplacement_destination;
		}

		auto rg = logical_edges |
			views::transform([&](const LogicalEdge& e)->TopologicalEdge{return {
				.from=mapping[e.from],
				.to=mapping[e.to],
				.distance=0
			};});
		vector<TopologicalEdge> v(logical_edges.size()), inter;
		ranges::copy(rg, &v[0]);
		ranges::sort(v);
		ranges::set_intersection(v, topological_edges, back_inserter(inter));
                decision_tree[i].match = inter.size();
		printf("i=%d inter.size()=%ld\n", i, inter.size());
	}

{
        FILE *f=fopen("decision_tree.json", "w");
        fprintf(f, "[\n");
        for (int i=0; i < decision_tree.size(); i++)
        {
                const auto& [_, parent_index, depth, recmap, match] = decision_tree[i];
                const auto& [i_emplacement_source, i_emplacement_destination] = recmap;
                fprintf(f, "{\"i\":%d, \"parent_index\":%d, \"depth\":%d, \"i_emplacement_source\":%d, \"i_emplacement_destination\":%d, \"match\":%d}%s\n",
                        i, parent_index, depth, i_emplacement_source, i_emplacement_destination, match,
                        i+1 == decision_tree.size() ? "": ",");
        }
        fprintf(f, "]\n");
        fclose(f);
}
	return decision_tree;
}

int main(int argc, char* argv[])
{
	if (argc==2 && strcmp(argv[1], "--dt")==0)
	{
		vector<MyRect> emplacements;

		vector<DecisionTreeNode> decision_tree = compute_decision_tree(input_rectangles, logical_edges, emplacements);

		vector<TranslationRangeItem> translation_ranges = compute_decision_tree_translations(decision_tree, emplacements, input_rectangles);
	}

	if (argc == 1)
	{
		test_fit();

		test_translations();
	}

	return 0;
}
