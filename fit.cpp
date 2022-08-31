#include <vector>
#include <algorithm>
#include <span>
#include <ranges>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "MyRect.h"
#include "FunctionTimer.h"
using namespace std;

#define _TRACE_

#ifdef _TRACE_
#  define D(x) x
#else
#  define D(x)
#endif


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


vector<RectLink> sweep(Direction update_direction, const vector<MyRect>& rectangles)
{
	FunctionTimer ft("sweep");

	const int N=20;
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
			printf("before insert\n");
//			print_active_line();
			insert(item);
			printf("after insert\n");
			print_active_line();
			break;
		case RIGHT:
		case BOTTOM:
			D(printf("sweep leaving %d %s\n", ri, RectDimString[rectdim]));
			printf("before erase\n");
//			print_active_line();
			erase(item);
			printf("after erase\n");
			print_active_line();
			break;
		}
	}

	for (const auto [sweep_value, rectdim, ri] : sweep_line)
	{
		printf("{.sweep_value=%d, .rectdim=%s, .ri=%d},\n",
			sweep_value, RectDimString[rectdim], ri);
	}
}

{
        FunctionTimer ft("cft_rectlinks_sort");
	sort(rect_links_buffer, rect_links_buffer + rect_links_size);
}
#ifdef _TRACE_
	D(printf("rect_links:\n"));
	for (const auto& [i, j, min_sweep_value, max_sweep_value] : span(rect_links_buffer, rect_links_size))
	{
		D(printf("{.i=%d, .j=%d, .%s=%d, .%s=%d},\n", i, j,
			RectDimString[minSweepRectDim], min_sweep_value, RectDimString[maxSweepRectDim], max_sweep_value));
	}
#endif

	return vector<RectLink>(rect_links_buffer, rect_links_buffer + rect_links_size);
}


void spread(Direction update_direction, const vector<RectLink>& rect_links, vector<MyRect>& rectangles)
{
//TODO: use chunk_by C++23
	const int N=20;
	int n = rectangles.size();

        MyPoint translations[N];

        ranges::fill(translations, MyPoint{0,0});

	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}

{
        FunctionTimer ft("cft_rec_push");
	auto rec_push_hole=[&](int ri, int tr, auto&& rec_push_hole)->void{

		span adj_list = ranges::equal_range(rect_links, ri, {}, &RectLink::i);

		for (const RectLink& rl : adj_list)
		{
			int tr2= rectangles[ri][maxCompactRectDim] - rectangles[rl.j][minCompactRectDim];

			if (tr2 < 0)
				tr2 = 0;

			rec_push_hole(rl.j, tr+tr2, rec_push_hole);
		}
		translations[ri][update_direction] = tr ;
	};

	auto push_hole=[&](){
		for (int j : views::iota(0,n) | views::filter([&](int j){return ranges::count(rect_links,j,&RectLink::j)==0;}))
		{
			int tr=0;
			rec_push_hole(j, tr, rec_push_hole);
		}
	};
	push_hole();
}

	for (int ri=0; ri < n; ri++)
	{
		rectangles[ri] += translations[ri];
	}
}


void compact(Direction update_direction, const vector<RectLink>& rect_links, vector<MyRect>& rectangles)
{
//TODO: use chunk_by C++23
        const int N=20;
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
                int value = translations[ri][update_direction];
//                if (value != 0)
//                     rectangles[ri] += translations[FORWARD_LINKS][ri];
         }
}
}


vector<MyPoint> compute_fit_to_hole_transform_(const vector<MyRect>& input_rectangles)
{
	FunctionTimer ft("compute_fht_");

	vector<MyRect> rectangles = input_rectangles;
	for (Direction update_direction : {EAST_WEST, NORTH_SOUTH})
	{
		vector<RectLink> rect_links = sweep(update_direction, rectangles);
		spread(update_direction, rect_links, rectangles);
	}
	return vector<MyPoint>();
}


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

const unsigned CORNER_MASK=0xC;
const unsigned PIPELINE_MASK=0x3;

Job pipelines[4][4]={
	{
		{SPREAD, EAST_WEST}, {SPREAD, NORTH_SOUTH}, {COMPACT, EAST_WEST}, {COMPACT, NORTH_SOUTH}
	},
	{
		{SPREAD, EAST_WEST}, {SPREAD, NORTH_SOUTH}, {COMPACT, NORTH_SOUTH}, {COMPACT, EAST_WEST}
	},
	{
		{SPREAD, NORTH_SOUTH}, {SPREAD, EAST_WEST}, {COMPACT, EAST_WEST}, {COMPACT, NORTH_SOUTH}
	},
	{
		{SPREAD, NORTH_SOUTH}, {SPREAD, EAST_WEST}, {COMPACT, NORTH_SOUTH}, {COMPACT, EAST_WEST}
	}
};

RectDim corners[4][2]={
	{LEFT, TOP},
	{LEFT, BOTTOM},
	{RIGHT, TOP},
	{RIGHT, BOTTOM}
};

//4 corners X 4 job pipelines


void apply_job(const Job& job, vector<MyRect>& rectangles)
{
	const auto [algo, update_direction] = job;

}

void apply_job_pipeline(const Job (&jobs)[4], vector<MyRect>& rectangles)
{
	for (const Job& job : jobs)
	{
		apply_job(job, rectangles);
	}
}

void translate_rectangles(int id, const RectDim (&corner)[2], vector<MyRect>& rectangles)
{
	const auto [RectDimX, RectDimY] = corner;

}

void main_69()
{
	int code = ranges::min(views::iota(0, 4*4), {}, [&](unsigned code){
		unsigned corner=code & CORNER_MASK;
		unsigned pipeline=code & PIPELINE_MASK;
		vector<MyRect> rectangles /*= input_rectangles*/;
		int id=0;
		translate_rectangles(id, corners[corner], rectangles);
		apply_job_pipeline(pipelines[pipeline], rectangles);
		return dim_max(compute_frame(rectangles));
	});
}


void test_fit()
{
	FunctionTimer::MAX_NESTING=1;
	FunctionTimer ft("holes");

	struct MyVector{float x=0.0f; float y=0.0f;};
	struct RectHole {int ri; int rj; RectCorner corner; MyVector direction; int value; MyRect rec;};
	struct SingleHoleTestContext {int testid; vector<MyRect> input_rectangles; RectHole rect_hole; vector<MyPoint> expected_translations;};

	const vector<SingleHoleTestContext> single_hole_test_contexts={
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
			{.m_left=300, .m_right=400, .m_top=100, .m_bottom=200},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=150, .m_bottom=250}
		},
		.rect_hole = {
			.ri=3, .rj=1, .corner=BOTTOM_LEFT, .direction={.x=1.0, .y=1.0}, .value=50,
			.rec={.m_left=100, .m_right=150, .m_top=100, .m_bottom=150}
		},
		.expected_translations={
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=50},
			{.x=0, .y=0}
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
			{.m_left=300, .m_right=350, .m_top=100, .m_bottom=150},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=200, .m_bottom=300}
		},
		.rect_hole = {
			.ri=3, .rj=1, .corner=BOTTOM_LEFT, .direction={.x=1.0, .y=1.0}, .value=100,
			.rec={.m_left=100, .m_right=200, .m_top=100, .m_bottom=200}
		},
		.expected_translations={
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=-50},
			{.x=0, .y=0}
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
			{.m_left=300, .m_right=350, .m_top=100, .m_bottom=150},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=200, .m_bottom=300}
		},
		.rect_hole = {
			.ri=3, .rj=1, .corner=BOTTOM_LEFT, .direction={.x=1.0, .y=1.0}, .value=100,
			.rec={.m_left=100, .m_right=200, .m_top=100, .m_bottom=200}
		},
		.expected_translations={
			{.x=0, .y=0},
			{.x=-50, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=-50},
			{.x=0, .y=0}
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
			{.m_left=300, .m_right=400, .m_top=100, .m_bottom=200},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=150, .m_bottom=250}
		},
		.rect_hole = {
			.ri=3, .rj=5, .corner=TOP_RIGHT, .direction={.x=1.0, .y=1.0}, .value=50,
			.rec={.m_left=200, .m_right=250, .m_top=150, .m_bottom=200}
		},
		.expected_translations={
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0},
			{.x=0, .y=0}
		}
	},

/*
       +---+
       |rh |
+------+---+   +------+
|      |       |      |
|  0   +       +  1   +------+
|      |       |      |      |
+------+       +------+  2   |
                      |      |
                      +------+
2 => rh
*/
        {
                .testid=4,
                .input_rectangles = {
                        {.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
                        {.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
                        {.m_left=300, .m_right=400, .m_top=100, .m_bottom=200}
                },
                .rect_hole = {
                        .ri=2, .rj=0, .corner=TOP_RIGHT, .direction={.x=1.0, .y=-1.0}, .value=100,
                        .rec={.m_left=100, .m_right=200, .m_top=0, .m_bottom=100}
                },
                .expected_translations={
                        {.x=0, .y=0},
                        {.x=0, .y=0},
                        {.x=0, .y=0}
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
                        {.m_left=200, .m_right=300, .m_top=0, .m_bottom=700},
                        {.m_left=300, .m_right=400, .m_top=0, .m_bottom=100}
                },
                .rect_hole = {
                        .ri=2, .rj=0, .corner=TOP_RIGHT, .direction={.x=1.0, .y=-1.0}, .value=100,//this line is wrong but not used
                        .rec={.m_left=100, .m_right=150, .m_top=300, .m_bottom=350}
                },
                .expected_translations={
                        {.x=0, .y=0},
                        {.x=0, .y=0},
                        {.x=0, .y=0}
                }
        }
	};

	for (const auto& [testid, input_rectangles, rect_hole, expected_translations] : single_hole_test_contexts)
	{
		const auto& [ri, rj, corner, direction, value, rec] = rect_hole;
		int dm1 = dim_max(compute_frame(input_rectangles));
		vector<MyRect> input_rectangles_ = input_rectangles;
		MyRect& r = input_rectangles_[ri];
		r += MyPoint{rec.m_left - r.m_left, rec.m_top - r.m_top};
		vector<MyPoint> translations = compute_fit_to_hole_transform_(input_rectangles_);
		int dm2 = dim_max(compute_frame(input_rectangles_ + translations));
		bool bOK = translations == expected_translations;
#ifdef _TRACE_
        printf("fit_to_hole testid=%d : %s\n", testid, bOK ? "OK" : "KO");
		printf("dim_max(frame) : %d => %d\n", dm1, dm2);
#endif
//		(bOK ? nbOK : nbKO)++;
	}
}
