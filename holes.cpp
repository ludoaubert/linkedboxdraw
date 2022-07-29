#include <vector>
#include <algorithm>
#include <tuple>
#include <span>
#include <ranges>
#include <optional>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "MyRect.h"
#include "FunctionTimer.h"
#include "compact_frame.h"
using namespace std;

#define _TRACE_

#ifdef _TRACE_
#  define D(x) x
#else
#  define D(x)
#endif


enum LEG
{
	LEFT_LEG,
	RIGHT_LEG
};

const char* LegString[2]={"LEFT_LEG", "RIGHT_LEG"};

// should be replaced by views::set_union() when it becomes available.

template <typename Range, typename Cmp, typename F>
void set_union(Range& a, Range& b, Cmp&& cmp, F&& f)
{
	for (int i=0, j=0; i<a.size() || j<b.size();)
	{
		auto lag=[](Range&r ,int k){
			return k-1>=0 && k-1<r.size() ? &r[k-1] : 0;
		};

		if (i < a.size() && j<b.size())
		{
			if (cmp(a[i], b[j]))
			{
				f(&a[i++], lag(b,j), LEFT_LEG);
			}
			else if (cmp(b[j], a[i]))
			{
				f(&b[j++], lag(a,i), RIGHT_LEG);
			}
			else
			{
				f(&a[i++], lag(b,j), LEFT_LEG);
				f(&b[j++], lag(a,i), RIGHT_LEG);
			}
		}
		else if (i < a.size())
		{
			f(&a[i++], 0, LEFT_LEG);
		}
		else if (j < b.size())
		{
			f(&b[j++], 0, RIGHT_LEG);
		}
	}
}


struct RankingCap{
	int n;
	int RC1;
	int RC2;
};

const vector<RankingCap> ranking_cap={
	{.n=0, .RC1=INT16_MAX, .RC2=INT16_MAX},
	{.n=1, .RC1=INT16_MAX, .RC2=INT16_MAX},
	{.n=2, .RC1=INT16_MAX, .RC2=INT16_MAX},
	{.n=3, .RC1=INT16_MAX, .RC2=INT16_MAX},
	{.n=4, .RC1=INT16_MAX, .RC2=INT16_MAX},
	{.n=5, .RC1=INT16_MAX, .RC2=INT16_MAX},
	{.n=6, .RC1=INT16_MAX, .RC2=INT16_MAX},
	{.n=7, .RC1=15, .RC2=7},
	{.n=8, .RC1=15, .RC2=7},
	{.n=9, .RC1=15, .RC2=7},
	{.n=10, .RC1=15, .RC2=7},
	{.n=11, .RC1=15, .RC2=7},
	{.n=12, .RC1=15, .RC2=7},
	{.n=13, .RC1=15, .RC2=7},
	{.n=14, .RC1=15, .RC2=7},
	{.n=15, .RC1=15, .RC2=7},
	{.n=16, .RC1=15, .RC2=7},
	{.n=17, .RC1=15, .RC2=7},
	{.n=18, .RC1=15, .RC2=7},
	{.n=19, .RC1=15, .RC2=7},
	{.n=20, .RC1=15, .RC2=7},
	{.n=21, .RC1=15, .RC2=7},
	{.n=22, .RC1=15, .RC2=7},
	{.n=23, .RC1=15, .RC2=7},
	{.n=24, .RC1=15, .RC2=7},
	{.n=25, .RC1=15, .RC2=7}
};

struct Edge {
	int from;
	int to;

	friend auto operator<=>(const Edge&, const Edge&) = default;
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


struct RectHole {int ri; int rj; RectCorner corner; MyVector direction; int value; MyRect rec;};


//	lightweight node

struct DecisionTreeNode
{
	int parent_index=-1;
	int depth;
	RectHole rh;
//KPIs:
	MyPoint dim;
	float rect_distances;
	float potential;
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
	LEG LEG_i;
	int i;
	LEG LEG_j;
	int j;
	int min_sweep_value, max_sweep_value=INT16_MAX;
	auto operator<=>(const RectLink&) const = default;
};

struct ActiveLineItem
{
	int i;
	RectLink* links[2]={0,0};
};

RectLink shared_link_buffer[100];
int index_available=0;

struct SharedLinks
{
	static RectLink none;
	RectLink &left_link=none, &right_link=none;

	auto operator<=>(const SharedLinks&) const = default;
};

RectLink SharedLinks::none;


struct ActiveLineItemPOD
{
	int i;
	SharedLinks shared_links;
	optional<RectLink> links[2];

	auto operator<=>(const ActiveLineItemPOD&) const = default;
};

struct ActiveLineTableItem
{
	SweepLineItem sweep_line_item;
	int pos;
	ActiveLineItemPOD active_line[20];
	int active_line_size;

	auto operator<=>(const ActiveLineTableItem&) const = default;
};

ActiveLineTableItem item={
	.sweep_line_item={.rectdim=TOP, .ri=7},
	.active_line={
		{.i=3, .shared_links={.left_link=shared_link_buffer[index_available++], .right_link=shared_link_buffer[index_available++]}, .links={nullopt, optional<RectLink>{{.LEG_i=LEFT_LEG, .i=2, .LEG_j=LEFT_LEG, .j=4, .min_sweep_value=34, .max_sweep_value=INT16_MAX}}}}
	},
	.active_line_size=1
};


vector<MyPoint> compute_fit_to_hole_transform_(const vector<MyRect>& input_rectangles)
{
	FunctionTimer ft("compute_fht_");

	const int N=20;
	int n = input_rectangles.size();

	MyRect rectangles_buffer[N];
	span rectangles(rectangles_buffer, n);
	ranges::copy(input_rectangles, rectangles_buffer);

	const MyPoint translation2[2]={{.x=1, .y=0}, {.x=0, .y=1}};

	SweepLineItem sweep_line_buffer[2*N];
	span sweep_line(sweep_line_buffer, 2*n);

	ActiveLineItem active_line[N];
	RectLink rect_links_buffer[256];
	RectLink in_rect_links_buffer[N];
	int in_edge_count[N];
	int edge_partition[N+1];

	MyPoint translations[N];

	vector<SharedLinks> shared_links_array;
	for (int i=0; i < n; i++)
	{
		RectLink &left_link = shared_link_buffer[index_available++];
		RectLink &right_link = shared_link_buffer[index_available++];
		shared_links_array.push_back({left_link, right_link});
	}

	for (Direction compact_direction : {EAST_WEST, NORTH_SOUTH})
	{
        vector<ActiveLineTableItem> active_line_table;
        active_line_table.reserve(2*N);

		ranges::fill(translations, MyPoint{0,0});
		MyRect frame={
			.m_left=ranges::min(rectangles | views::transform(&MyRect::m_left)),
			.m_right=ranges::max(rectangles | views::transform(&MyRect::m_right)),
			.m_top=ranges::min(rectangles | views::transform(&MyRect::m_top)),
			.m_bottom=ranges::max(rectangles | views::transform(&MyRect::m_bottom))
		};

//use the sweep_line that is not impacted by selected translation
		Direction sweep_direction = Direction(1-compact_direction);

		auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[compact_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
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

		const MyPoint& translation = translation2[compact_direction] ;
{
        FunctionTimer ft("cft_sort_sweepline");
		ranges::sort(sweep_line, CustomLess());
}
		int active_line_size=0;
		int rect_links_size=0;

		auto cmp=[&](int i, int j){
			return rectangles[i][minCompactRectDim]<rectangles[j][minCompactRectDim];
		};

		auto erase=[&](int i, int sweep_value){
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
					.LEG_i = LEFT_LEG,
					.i=active_line[pos-1].i,
					.LEG_j = LEFT_LEG,
					.j=active_line[pos].i,
					.min_sweep_value=sweep_value
				};

				if (RectLink *rl=active_line[pos-1].links[1]; rl!=0)
					rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
				if (RectLink *rl=active_line[pos].links[0]; rl!=0)
					rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
				active_line[pos-1].links[1] = active_line[pos].links[0] = & rect_links_buffer[rect_links_size - 1];
			}

			ActiveLineTableItem item={
				.sweep_line_item={.sweep_value=sweep_value, .rectdim=maxSweepRectDim, .ri=i},
				.pos=pos,
				.active_line={},
				.active_line_size=0
			};

			for (auto& [i, links] : span(active_line, active_line_size))
			{
				ActiveLineItemPOD active_line_item={.i=i, .shared_links=shared_links_array[i]};
				for (int LEG : {0,1})
				{
					RectLink* lk = links[LEG];
					active_line_item.links[LEG]=
						lk==0 ? nullopt : optional<RectLink>{{
							.LEG_i=LEFT_LEG,
							.i=lk->i,
							.LEG_j=LEFT_LEG,
							.j=lk->j,
							.min_sweep_value=lk->min_sweep_value,
							.max_sweep_value=lk->max_sweep_value
						}};
				}
				memcpy ( &item.active_line[item.active_line_size++], &active_line_item, sizeof(ActiveLineItemPOD));
			}
			active_line_table.push_back(item);
		};

		auto insert=[&](int i, int sweep_value){
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
					.LEG_i = LEFT_LEG,
					.i=active_line[pos-1].i,
					.LEG_j = LEFT_LEG,
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
					.LEG_i=LEFT_LEG,
					.i=active_line[pos].i,
					.LEG_j=LEFT_LEG,
					.j=active_line[pos+1].i,
					.min_sweep_value=sweep_value};

				if (RectLink *rl=active_line[pos].links[1]; rl!=0)
					rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
				if (RectLink *rl=active_line[pos+1].links[0]; rl!=0)
					rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
				active_line[pos].links[1] = active_line[pos+1].links[0] = &rect_links_buffer[rect_links_size - 1];
			}

			ActiveLineTableItem item={
				.sweep_line_item={.sweep_value=sweep_value, .rectdim=minSweepRectDim, .ri=i},
				.pos=pos,
				.active_line={},
				.active_line_size=0
			};

			for (auto& [i, links] : span(active_line, active_line_size))
			{
				ActiveLineItemPOD active_line_item={.i=i .shared_links=shared_links_array[i]};
				for (int LEG : {0,1})
				{
					RectLink* lk = links[LEG];
					active_line_item.links[LEG] = lk==0 ? nullopt : optional<RectLink>{{
						.LEG_i=LEFT_LEG,
						.i=lk->i,
						.LEG_j=LEFT_LEG,
						.j=lk->j,
						.min_sweep_value=lk->min_sweep_value,
						.max_sweep_value=lk->max_sweep_value
					}};
				}
				memcpy ( &item.active_line[item.active_line_size++], &active_line_item, sizeof(ActiveLineItemPOD));
			}
			active_line_table.push_back(item);
		};

		auto print_active_line=[&](){
			char buffer[5000];
			int pos=0;
			pos += sprintf(buffer + pos, ".active_line={", active_line_size);
			for (auto& [i, links] : span(active_line, active_line_size))
			{
				pos += sprintf(buffer + pos, "{.i=%d, .shared_links=shared_links_array[%d], .links={", i, i);
				for (RectLink* prl : links)
				{
					if (prl == 0)
						pos += sprintf(buffer+pos,"nullopt,");
					else
					{
						pos += sprintf(buffer + pos, "{.LEG_i=%s, .i=%d, .LEG_j=%s, .j=%d, .min_sweep_value=%d, .max_sweep_value=%d},",
							LegString[prl->LEG_i], prl->i, LegString[prl->LEG_j], prl->j, prl->min_sweep_value, prl->max_sweep_value);
					}
				}
				pos += sprintf(buffer + --pos, "},");
			}
			pos += sprintf(buffer + --pos, "}\n");
			buffer[pos]=0;
			printf("%s", buffer);
		};

{
        FunctionTimer ft("cft_sweep");
		for (const SweepLineItem& item : sweep_line)
		{
			const auto& [sweep_value, rectdim, ri] = item;
			switch(rectdim)
			{
			case LEFT:
			case TOP:
				D(printf("sweep reaching %d %s\n", ri, RectDimString[rectdim]));
				printf("before insert\n");
//				print_active_line();
				insert(ri, rectangles[ri][rectdim]);
                                printf("after insert\n");
                                print_active_line();
				break;
			case RIGHT:
			case BOTTOM:
				D(printf("sweep leaving %d %s\n", ri, RectDimString[rectdim]));
				printf("before erase\n");
//				print_active_line();
				erase(ri, rectangles[ri][rectdim]);
                                printf("after erase\n");
                                print_active_line();
				break;
			}
		}
}
//TODO: C++23 reflexion ?
		for (const auto& [sweep_line_item, pos, active_line, active_line_size] : active_line_table)
		{
			char buffer[1000];
			int bp=0;

			auto [sweep_value, rectdim, ri] = sweep_line_item;
			bp += sprintf(buffer + bp, "{\n");
			bp += sprintf(buffer + bp, ".sweep_line_item={.sweep_value=%d, .rectdim=%s, .ri=%d},\n", sweep_value, RectDimString[rectdim], ri);
			bp += sprintf(buffer + bp, ".pos=%d,\n", pos);
			bp += sprintf(buffer + bp, ".active_line={", active_line_size);
			for (const auto& [i, shared_links, links] : span(active_line, active_line_size))
			{
				bp += sprintf(buffer + bp, "\n\t{.i=%d, .shared_links=shared_links_array[%d], .links={", i, i);
				for (optional<RectLink> rl : links)
				{
					if (rl)
					{
						const auto& [LEG_i, i, LEG_j, j, min_sweep_value, max_sweep_value] = rl.value();//double curly braces: outer braces for optional<>
						bp += sprintf(buffer + bp, "{{.LEG_i=%s, .i=%d, .LEG_j=%s .j=%d, .min_sweep_value=%d, .max_sweep_value=%d}},",
							LegString[LEG_i], i, LegString[LEG_j], j, min_sweep_value, max_sweep_value);
					}
					else
						bp += sprintf(buffer + bp,"nullopt,");
				}
				bp += sprintf(buffer + --bp, "}},");
			}
			bp += sprintf(buffer + --bp, "\n},\n");
			bp += sprintf(buffer + bp, ".active_line_size=%d\n", active_line_size);
			bp += sprintf(buffer + bp, "},\n");
			buffer[bp]=0;
			printf("%s", buffer);
		}

{
        FunctionTimer ft("cft_rectlinks_sort");
		sort(rect_links_buffer, rect_links_buffer + rect_links_size);
}
#ifdef _TRACE_
		D(printf("rect_links:\n"));
		for (const auto& [LEG_i, i, LEG_j, j, min_sweep_value, max_sweep_value] : span(rect_links_buffer, rect_links_size))
		{
			D(printf("{.LEG_i=%s, .i=%d, .LEG_j=%s, .j=%d, .%s=%d, .%s=%d},\n", LegString[LEG_i], i, LegString[LEG_j], j,
				RectDimString[minSweepRectDim], min_sweep_value, RectDimString[maxSweepRectDim], max_sweep_value));
		}
#endif
{
        FunctionTimer ft("cft_edge_part");
		edge_partition[0]=0;
		for (int pos=0, ii=0; ii<n; ii++)
		{
			int &start_pos = edge_partition[ii];
			int &end_pos = edge_partition[ii+1];
			end_pos = start_pos;
			for ( ; pos < rect_links_size && rect_links_buffer[pos].i==ii; pos++)
			{
				end_pos = max(end_pos, pos+1);
			}
		}
}
#ifdef _TRACE_
		D(printf("edge_partition: "));
		for (int pos : span(edge_partition,n+1))
			D(printf("%d,", pos));
		D(printf("\n"));
#endif

//TODO: use chunk_by C++23
		int in_rect_links_size=0;
{
		FunctionTimer ft("cft_in_edges");
		ranges::fill(in_edge_count, 0);
		for (const RectLink& rl : span(rect_links_buffer, rect_links_size))
			in_edge_count[rl.j] += 1;
		for (int ri : views::iota(0, n) | views::filter([&](int ri){return in_edge_count[ri]==0;}))
			in_rect_links_buffer[in_rect_links_size++] = {LEFT_LEG, -INT16_MAX, LEFT_LEG, ri};
#ifdef _TRACE_
		D(printf("in_edges: "));
		for (const RectLink& rl : span(in_rect_links_buffer, in_rect_links_size))
			D(printf("%d, ", rl.j));
		D(printf("\n"));
#endif
}

		auto adj_list=[&](int ri)->span<RectLink>{
			int i=edge_partition[ri];
			int j=edge_partition[ri+1];
			return span(&rect_links_buffer[i], j-i);
		};

{
        FunctionTimer ft("cft_rec_push");
		auto rec_push_hole=[&](int ri, int tr, auto&& rec_push_hole)->void{
			for (const RectLink& rl : adj_list(ri))
			{
				int tr2= rectangles[ri][maxCompactRectDim] - rectangles[rl.j][minCompactRectDim];

				if (tr2 < 0)
					tr2 = 0;

				rec_push_hole(rl.j, tr+tr2, rec_push_hole);
			}
			translations[ri][compact_direction] = tr ;
		};

		auto push_hole=[&](){
			span adj(in_rect_links_buffer, in_rect_links_size);
			for (const RectLink& e : adj)
			{
				int tr=0;
				rec_push_hole(e.j, tr, rec_push_hole);
			}
		};
		push_hole();
}

		for (int ri=0; ri < n; ri++)
		{
			rectangles[ri] += translations[ri];
		}
	}
{
	FunctionTimer ft("cft_return_result");
	vector<MyPoint> tf(n);
	for (int i=0; i<n; i++)
	{
		MyRect r = rectangles[i] - input_rectangles[i];
		tf[i] = {r.m_left, r.m_top};
	}

#ifdef _TRACE_
	printf("tf={");
	for (const auto& [x, y] : tf)
		printf("{.x=%d, .y=%d},", x, y);
	printf("\n");
#endif
	return tf;
}
}


int main()
{
	FunctionTimer::MAX_NESTING=1;
	FunctionTimer ft("holes");

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

const vector<MyRect> input_rectangles1 = {
	{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
	{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
	{.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
	{.m_left=300, .m_right=400, .m_top=100, .m_bottom=200},
	{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
	{.m_left=100, .m_right=200, .m_top=150, .m_bottom=250}
};

const vector<MyRect> input_rectangles2 = {
        {.m_left=100, .m_right=150, .m_top=100, .m_bottom=150}
};

const vector<MyRect>* rectangles2[2] = { &input_rectangles1, &input_rectangles2 };

/*
{
{
.sweep_line_item={.sweep_value=100, .rectdim=TOP, .ri=3},
.pos=0,
.active_line={
        {.i=3, .links={nullopt,nullopt}}
},
{
.sweep_line_item={.sweep_value=200, .rectdim=BOTTOM, .ri=3},
.pos=0,
.active_line={
        {}
}
}
*/
vector<ActiveLineTableItem> active_line_table={
{
.sweep_line_item={.sweep_value=0, .rectdim=TOP, .ri=1},
.pos=0,
.active_line={
        {.i=1, .links={nullopt,nullopt}}
},
.active_line_size=1
},
{
.sweep_line_item={.sweep_value=50, .rectdim=TOP, .ri=0},
.pos=0,
.active_line={
        {.i=0, .links={nullopt,{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=1, .min_sweep_value=50, .max_sweep_value=32767}}}},
        {.i=1, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=1, .min_sweep_value=50, .max_sweep_value=32767}},nullopt}}
},
.active_line_size=2
},
{
.sweep_line_item={.sweep_value=50, .rectdim=TOP, .ri=2},
.pos=2,
.active_line={
        {.i=0, .links={nullopt,{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=1, .min_sweep_value=50, .max_sweep_value=32767}}}},
        {.i=1, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=1, .min_sweep_value=50, .max_sweep_value=32767}},{{.LEG_i=LEFT_LEG, .i=1, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=50, .max_sweep_value=32767}}}},
        {.i=2, .links={{{.LEG_i=LEFT_LEG, .i=1, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=50, .max_sweep_value=32767}},nullopt}}
},
.active_line_size=3
},
{
.sweep_line_item={.sweep_value=100, .rectdim=BOTTOM, .ri=1},
.pos=1,
.active_line={
        {.i=0, .links={nullopt,{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=100, .max_sweep_value=32767}}}},
        {.i=2, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=100, .max_sweep_value=32767}},nullopt}}
},
.active_line_size=2
},
{
.sweep_line_item={.sweep_value=100, .rectdim=TOP, .ri=3},
.pos=1,
.active_line={
        {.i=0, .links={nullopt,{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=3, .min_sweep_value=100, .max_sweep_value=32767}}}},
        {.i=3, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=3, .min_sweep_value=100, .max_sweep_value=32767}},{{.LEG_i=LEFT_LEG, .i=3, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=100, .max_sweep_value=32767}}}},
        {.i=2, .links={{{.LEG_i=LEFT_LEG, .i=3, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=100, .max_sweep_value=32767}},nullopt}}
},
.active_line_size=3
},
{
.sweep_line_item={.sweep_value=150, .rectdim=BOTTOM, .ri=0},
.pos=0,
.active_line={
        {.i=3, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=3, .min_sweep_value=100, .max_sweep_value=150}},{{.LEG_i=LEFT_LEG, .i=3, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=100, .max_sweep_value=32767}}}},
        {.i=2, .links={{{.LEG_i=LEFT_LEG, .i=3, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=100, .max_sweep_value=32767}},nullopt}}
},
.active_line_size=2
},
{
.sweep_line_item={.sweep_value=150, .rectdim=BOTTOM, .ri=2},
.pos=1,
.active_line={
        {.i=3, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=3, .min_sweep_value=100, .max_sweep_value=150}},{{.LEG_i=LEFT_LEG, .i=3, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=100, .max_sweep_value=150}}}}
},
.active_line_size=1
},
{
.sweep_line_item={.sweep_value=150, .rectdim=TOP, .ri=4},
.pos=0,
.active_line={
        {.i=4, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=1, .min_sweep_value=50, .max_sweep_value=100}},{{.LEG_i=LEFT_LEG, .i=4, .LEG_j=LEFT_LEG, .j=3, .min_sweep_value=150, .max_sweep_value=32767}}}},
        {.i=3, .links={{{.LEG_i=LEFT_LEG, .i=4, .LEG_j=LEFT_LEG, .j=3, .min_sweep_value=150, .max_sweep_value=32767}},{{.LEG_i=LEFT_LEG, .i=3, .LEG_j=LEFT_LEG, .j=2, .min_sweep_value=100, .max_sweep_value=150}}}}
},
.active_line_size=2
},
{
.sweep_line_item={.sweep_value=150, .rectdim=TOP, .ri=5},
.pos=2,
.active_line={
        {.i=4, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=1, .min_sweep_value=50, .max_sweep_value=100}},{{.LEG_i=LEFT_LEG, .i=4, .LEG_j=LEFT_LEG, .j=3, .min_sweep_value=150, .max_sweep_value=32767}}}},
        {.i=3, .links={{{.LEG_i=LEFT_LEG, .i=4, .LEG_j=LEFT_LEG, .j=3, .min_sweep_value=150, .max_sweep_value=32767}},{{.LEG_i=LEFT_LEG, .i=3, .LEG_j=LEFT_LEG, .j=5, .min_sweep_value=150, .max_sweep_value=32767}}}},
        {.i=5, .links={{{.LEG_i=LEFT_LEG, .i=3, .LEG_j=LEFT_LEG, .j=5, .min_sweep_value=150, .max_sweep_value=32767}},nullopt}}
},
.active_line_size=3
},
{
.sweep_line_item={.sweep_value=200, .rectdim=BOTTOM, .ri=3},
.pos=1,
.active_line={
        {.i=4, .links={{{.LEG_i=LEFT_LEG, .i=0, .LEG_j=LEFT_LEG, .j=1, .min_sweep_value=50, .max_sweep_value=100}},{{.LEG_i=LEFT_LEG, .i=4, .LEG_j=LEFT_LEG, .j=5, .min_sweep_value=200, .max_sweep_value=32767}}}},
        {.i=5, .links={{{.LEG_i=LEFT_LEG, .i=4, .LEG_j=LEFT_LEG, .j=5, .min_sweep_value=200, .max_sweep_value=32767}},nullopt}}
},
.active_line_size=2
},
{
.sweep_line_item={.sweep_value=250, .rectdim=BOTTOM, .ri=4},
.pos=0,
.active_line={
        {.i=5, .links={{{.LEG_i=LEFT_LEG, .i=4, .LEG_j=LEFT_LEG, .j=5, .min_sweep_value=200, .max_sweep_value=250}},nullopt}}
},
.active_line_size=1
},
{
.sweep_line_item={.sweep_value=250, .rectdim=BOTTOM, .ri=5},
.pos=0,
.active_line={},
.active_line_size=0
}
};

vector<ActiveLineTableItem> active_line_table2={
{
.sweep_line_item={.sweep_value=100, .rectdim=TOP, .ri=3},
.pos=0,
.active_line={
        {.i=3, .links={nullopt,nullopt}}
},
.active_line_size=1
},
{
.sweep_line_item={.sweep_value=200, .rectdim=BOTTOM, .ri=3},
.pos=0,
.active_line={
        {}
},
.active_line_size=0
}
};

RectLink rect_links_buffer[256];
int rect_links_size=0;
RectDim minCompactRectDim=TOP;

auto cmp=[](const ActiveLineTableItem& a, const ActiveLineTableItem& b){
	return a.sweep_line_item < b.sweep_line_item;
};

set_union(active_line_table, active_line_table2,
cmp,
[&](ActiveLineTableItem* main_active_line_table_item, ActiveLineTableItem* optional_active_line_table_item, LEG active_LEG)
{
	auto& [sweep_line_item, pos, active_line, active_line_size] = * main_active_line_table_item;

	ActiveLineItemPOD* slide[3]={0,0,0};

	slide[1] = & active_line[pos];

	if (optional_active_line_table_item == 0) [[likely]]
	{
		if (0 < pos)
			slide[0] = & active_line[pos-1];
		if (pos+1 < active_line_size)
			slide[2] = &active_line[pos+1];
	}
	else
	{
		auto& [other_sweep_line_item, other_pos, other_active_line, other_active_line_size] = * optional_active_line_table_item;
		span r(other_active_line, other_active_line_size);
		auto lower = ranges::lower_bound(
			r,
			(*rectangles2[active_LEG])[active_line[pos].i][minCompactRectDim],
			{},
			[&](ActiveLineItemPOD& ali){return (*rectangles2[1-active_LEG])[ali.i][minCompactRectDim];});
		if (lower==ranges::end(r) && pos+1 >= active_line_size)
		{
			slide[2] = 0;
		}
		else if (lower!=ranges::end(r) && pos+1 >= active_line_size)
		{
			slide[2] = &*lower;
		}
		else if (lower==ranges::end(r) && pos+1 < active_line_size)
		{
			slide[2] = &active_line[pos+1];
		}
		else
		{
			if ( (*rectangles2[1-active_LEG])[lower->i][minCompactRectDim] < (*rectangles2[active_LEG])[ active_line[pos+1].i ][minCompactRectDim])
				slide[2] = &*lower;
			else
				slide[2] = & active_line[pos+1];
		}
/*
In a sorted container, the last element that is less than or equivalent to x, is the element before the first element that is greater than x.
Thus you can call std::upper_bound, and decrement the returned iterator once. (Before decrementing, you must of course check that it is not the begin iterator;
if it is, then there are no elements that are less than or equivalent to x.)
*/
		auto upper = ranges::upper_bound(
			r,
			(*rectangles2[active_LEG])[active_line[pos].i][minCompactRectDim],
			{},
			[&](ActiveLineItemPOD& ali){return (*rectangles2[1-active_LEG])[ali.i][minCompactRectDim];});
		if (upper > ranges::begin(r))
			upper--;
		else
			upper = ranges::end(r);

		if (upper==ranges::end(r) && pos-1 < 0)
		{
			slide[2] = 0;
		}
		else if (upper!=ranges::end(r) && pos-1 < 0)
		{
			slide[2] = &*upper;
		}
		else if (upper==ranges::end(r) && pos-1 >= 0)
		{
			slide[2] = &active_line[pos-1];
		}
		else
		{
			if ( (*rectangles2[1-active_LEG])[upper->i][minCompactRectDim] > (*rectangles2[active_LEG])[ active_line[pos-1].i ][minCompactRectDim])
				slide[2] = &*upper;
			else
				slide[2] = & active_line[pos-1];
		}
	}

        const auto& [sweep_value, rectdim, ri] = sweep_line_item;

	switch (rectdim)
	{
	case LEFT:
	case TOP:
		if (pos > 0)
		{
			rect_links_buffer[rect_links_size++] = {
				.LEG_i = LEFT_LEG,
				.i=active_line[pos-1].i,
				.LEG_j = LEFT_LEG,
				.j=active_line[pos].i,
				.min_sweep_value = sweep_value
			};

			if (RectLink &rl=active_line[pos].shared_links.left_link; &rl != &SharedLinks::none)
				rl.max_sweep_value = min(sweep_value,rl.max_sweep_value);
			if (RectLink &rl=active_line[pos-1].shared_links.right_link; &rl != &SharedLinks::none)
				rl.max_sweep_value = min(sweep_value,rl.max_sweep_value);
//			active_line[pos].shared_links.left_link = active_line[pos-1].shared_links.right_link = & rect_links_buffer[rect_links_size - 1];
		}
		if (pos+1 < active_line_size)
		{
			rect_links_buffer[rect_links_size++] = {
				.LEG_i=LEFT_LEG,
				.i=active_line[pos].i,
				.LEG_j=LEFT_LEG,
				.j=active_line[pos+1].i,
				.min_sweep_value=sweep_value};

			if (RectLink &rl=active_line[pos].shared_links.right_link; &rl != &SharedLinks::none)
				rl.max_sweep_value = min(sweep_value, rl.max_sweep_value);
			if (RectLink &rl=active_line[pos+1].shared_links.left_link; &rl != &SharedLinks::none)
				rl.max_sweep_value = min(sweep_value, rl.max_sweep_value);
//			active_line[pos].links[1] = active_line[pos+1].links[0] = &rect_links_buffer[rect_links_size - 1];
		}
		break;
	case RIGHT:
	case BOTTOM:
		for (RectLink* rl : {&active_line[pos].shared_links.left_link, &active_line[pos].shared_links.right_link})
		{
			if (rl != 0)
				rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
		}

//		for (int ii=pos; ii<active_line_size; ii++)
//			swap(active_line[ii], active_line[ii+1]);
//		active_line_size -= 1;

		if (pos > 0 && pos < active_line_size)
		{
			rect_links_buffer[rect_links_size++] = {
				.LEG_i = LEFT_LEG,
				.i=active_line[pos-1].i,
				.LEG_j = LEFT_LEG,
				.j=active_line[pos].i,
				.min_sweep_value=sweep_value
			};

			if (RectLink &rl=active_line[pos-1].shared_links.left_link; &rl != &SharedLinks::none)
				rl.max_sweep_value = min(sweep_value,rl.max_sweep_value);
			if (RectLink &rl=active_line[pos].shared_links.right_link; &rl != &SharedLinks::none)
				rl.max_sweep_value = min(sweep_value,rl.max_sweep_value);
//			active_line[pos-1].links[1] = active_line[pos].links[0] = & rect_links_buffer[rect_links_size - 1];
		}
		break;
	}
});

return 0;

	struct TestContext {int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<MyRect> expected_rectangles; };

	const vector<TestContext> test_contexts={
	{
		.testid=1,
		.input_rectangles = {
			{.m_left=20, .m_right=120, .m_top=20, .m_bottom=120, .i=0},
                        {.m_left=120, .m_right=220, .m_top=20, .m_bottom=220, .i=1},
                        {.m_left=220, .m_right=320, .m_top=120, .m_bottom=220, .i=2}
		},
		.edges = {
                        {.from=1,.to=0},
                        {.from=1,.to=2}
		},

		.expected_rectangles={
                        {.m_left=20, .m_right=120, .m_top=20, .m_bottom=120, .i=0},
                        {.m_left=120, .m_right=220, .m_top=20, .m_bottom=220, .i=1},
                        {.m_left=20, .m_right=120, .m_top=120, .m_bottom=220, .i=2}
		}
	},
	{
		.testid=2,
		.input_rectangles = {
			{.m_left=396-RECT_BORDER+FRAME_BORDER, .m_right=396+162+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+104+RECT_BORDER+FRAME_BORDER, .i=0},//8
			{.m_left=320-RECT_BORDER+FRAME_BORDER, .m_right=320+182+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+72+RECT_BORDER+FRAME_BORDER, .i=1},//9
			{.m_left=453-RECT_BORDER+FRAME_BORDER, .m_right=453+105+RECT_BORDER+FRAME_BORDER, .m_top=218-RECT_BORDER+FRAME_BORDER, .m_bottom=218+72+RECT_BORDER+FRAME_BORDER, .i=2},//10
			{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+152+RECT_BORDER+FRAME_BORDER, .i=3},//21
			{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=202-RECT_BORDER+FRAME_BORDER, .m_bottom=202+88+RECT_BORDER+FRAME_BORDER, .i=4},//24
			{.m_left=750-RECT_BORDER+FRAME_BORDER, .m_right=750+147+RECT_BORDER+FRAME_BORDER, .m_top=346-RECT_BORDER+FRAME_BORDER, .m_bottom=346+120+RECT_BORDER+FRAME_BORDER, .i=5},//25
			{.m_left=273-RECT_BORDER+FRAME_BORDER, .m_right=273+140+RECT_BORDER+FRAME_BORDER, .m_top=154-RECT_BORDER+FRAME_BORDER, .m_bottom=154+120+RECT_BORDER+FRAME_BORDER, .i=6},//26
			{.m_left=542-RECT_BORDER+FRAME_BORDER, .m_right=542+168+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+136+RECT_BORDER+FRAME_BORDER, .i=7},//27
			{.m_left=335-RECT_BORDER+FRAME_BORDER, .m_right=335+168+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+120+RECT_BORDER+FRAME_BORDER, .i=8},//28
			{.m_left=556-RECT_BORDER+FRAME_BORDER, .m_right=556+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+104+RECT_BORDER+FRAME_BORDER, .i=9},//30
			{.m_left=764-RECT_BORDER+FRAME_BORDER, .m_right=764+133+RECT_BORDER+FRAME_BORDER, .m_top=186-RECT_BORDER+FRAME_BORDER, .m_bottom=186+120+RECT_BORDER+FRAME_BORDER, .i=10},//32
			{.m_left=743-RECT_BORDER+FRAME_BORDER, .m_right=743+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+168+RECT_BORDER+FRAME_BORDER, .i=11},//44
			{.m_left=93-RECT_BORDER+FRAME_BORDER, .m_right=93+140+RECT_BORDER+FRAME_BORDER, .m_top=153-RECT_BORDER+FRAME_BORDER, .m_bottom=153+88+RECT_BORDER+FRAME_BORDER, .i=12},//48
			{.m_left=78-RECT_BORDER+FRAME_BORDER, .m_right=78+155+RECT_BORDER+FRAME_BORDER, .m_top=281-RECT_BORDER+FRAME_BORDER, .m_bottom=281+120+RECT_BORDER+FRAME_BORDER, .i=13},//52
			{.m_left=120-RECT_BORDER+FRAME_BORDER, .m_right=120+175+RECT_BORDER+FRAME_BORDER, .m_top=441-RECT_BORDER+FRAME_BORDER, .m_bottom=441+136+RECT_BORDER+FRAME_BORDER, .i=14}//53
		},

		.edges = {
			{.from=0,.to=3},
			{.from=1,.to=7},
			{.from=2,.to=7},
			{.from=3,.to=4},
			{.from=4,.to=7},
			{.from=5,.to=7},
			{.from=6,.to=7},
			{.from=7,.to=11},
			{.from=8,.to=7},
			{.from=9,.to=7},
			{.from=10,.to=7},
			{.from=12,.to=6},
			{.from=13,.to=12},
			{.from=13,.to=14}
		},

		.expected_rectangles = {
			{.m_left=396,.m_right=396+162,.m_top=10,.m_bottom=10+104},//8
			{.m_left=320,.m_right=320+182,.m_top=330,.m_bottom=330+72},//9
			{.m_left=453,.m_right=453+105,.m_top=218,.m_bottom=218+72},//10
			{.m_left=598,.m_right=598+126,.m_top=10,.m_bottom=10+152},//21
			{.m_left=598,.m_right=598+126,.m_top=202,.m_bottom=202+88},//24
			{.m_left=750,.m_right=750+147,.m_top=346,.m_bottom=346+120},//25
			{.m_left=273,.m_right=273+140,.m_top=154,.m_bottom=154+120},//26
			{.m_left=542,.m_right=542+168,.m_top=330,.m_bottom=330+136},//27
			{.m_left=335,.m_right=335+168,.m_top=506,.m_bottom=506+120},//28
			{.m_left=556,.m_right=556+147,.m_top=506,.m_bottom=506+104},//30
			{.m_left=764,.m_right=764+133,.m_top=186,.m_bottom=186+120},//32
			{.m_left=743,.m_right=743+147,.m_top=506,.m_bottom=506+168},//44
			{.m_left=93,.m_right=93+140,.m_top=153,.m_bottom=153+88},//48
			{.m_left=10,.m_right=10+155,.m_top=281,.m_bottom=281+120},//52
			{.m_left=11,.m_right=11+175,.m_top=441,.m_bottom=441+136}//53
		}
	}
	};

	for (const auto& [testid, input_rectangles, edges, expected_rectangles] : test_contexts)
	{
		assert( ranges::is_sorted(edges) );
//if (testid != 1)
//	return 0;
		const MyRect frame = compute_frame(input_rectangles);

		auto compute_holes = [&](const vector<MyRect>& input_rectangles)->vector<RectHole>{

			int n = input_rectangles.size();
			const float k = 1.0f;

			vector<RectHole> holes;

			for (const MyRect& ir : input_rectangles)
			{
				const MyVector directions[4][3]={
						{{.x=-1, .y=-k},{.x=+1, .y=-k},{.x=-1, .y=+k}},
						{{.x=-1, .y=+k},{.x=+1, .y=+k},{.x=-1, .y=-k}},
						{{.x=+1, .y=+k},{.x=+1, .y=-k},{.x=-1, .y=-k}},
						{{.x=-1, .y=+k},{.x=+1, .y=+k},{.x=+1, .y=-k}}
				};

				for (RectCorner rectCorner : RectCorners)
				{
					const MyPoint pt = ir[rectCorner] ;

					for (const MyVector& dir : directions[rectCorner])
					{
						int intervalle[2]={2, INT16_MAX};
						auto& [m, M] = intervalle;
						while (M > 1+m)
						{
							int value = M==INT16_MAX ? 2*m : (m+M)/2 ;
							MyRect rec = rect(pt, pt + value*dir);
							auto rg = input_rectangles | views::filter([&](const MyRect& r){return intersect_strict(rec,r) || is_inside(r, rec);});
							(rg.empty() && is_inside(rec,frame) ? m : M) = value;
							//printf("[%d %d]\n", m, M);
						}

						if (m > 2)
						{
							MyRect rec = rect(pt, pt + m*dir);
							holes.push_back({.ri=-1, .rj=ir.i, .corner=rectCorner, .direction=dir, .value=m, .rec=rec});
						}
					}
				}
			}
  //		printf("holes.size()=%ld\n", holes.size());

			ranges::sort(holes, {}, &RectHole::rec);
			vector<RectHole> holes_dedup;
			ranges::unique_copy(holes, back_inserter(holes_dedup), {}, &RectHole::rec);

			holes.clear();

//cross product
			for (int ri : views::iota(0,n))
			{
				for (const auto& [ri_, rj, corner, direction, value, rec] : holes_dedup)
				{
					if (3*value >= width(input_rectangles[ri]))
					{
						holes.push_back({ri, rj, corner, direction, value, rec});
					}
				}
			}

//			printf("holes_dedup.size()=%ld\n", holes_dedup.size());
			return holes;
		};

		vector<int> stress_line[2];
		compute_stress_line(input_rectangles, stress_line);

		vector<RectHole> holes = compute_holes(input_rectangles);


		auto compute_transformation = [&](const vector<MyRect>& input_rectangles, const RectHole& rh)->vector<MyRect>{

			enum TransformationType {STRETCH_WIDTH, SQUEEZE_WIDTH, STRETCH_HEIGHT, SQUEEZE_HEIGHT};
			struct ST { MyRect initial_tf; MyPoint tf; };
			const ST Transformations[4][2]={
				{
					{.initial_tf = {.m_left=-1, .m_right=0, .m_top=0, .m_bottom=0}, .tf = {.x=-1, .y=0}},
					{.initial_tf = {.m_left=0, .m_right=+1, .m_top=0, .m_bottom=0}, .tf = {.x=+1, .y=0}},
				},
				{
					{.initial_tf = {.m_left=+1, .m_right=0, .m_top=0, .m_bottom=0}, .tf = {.x=+1, .y=0}},
					{.initial_tf = {.m_left=0, .m_right=-1, .m_top=0, .m_bottom=0}, .tf = {.x=-1, .y=0}},
				},
				{
					{.initial_tf = {.m_left=0, .m_right=0, .m_top=-1, .m_bottom=0}, .tf = {.x=0, .y=-1}},
					{.initial_tf = {.m_left=0, .m_right=0, .m_top=0, .m_bottom=+1}, .tf = {.x=0, .y=+1}},
				},
				{
					{.initial_tf = {.m_left=0, .m_right=0, .m_top=+1, .m_bottom=0}, .tf = {.x=0, .y=+1}},
					{.initial_tf = {.m_left=0, .m_right=0, .m_top=0, .m_bottom=-1}, .tf = {.x=0, .y=-1}},
				}
			};

			const auto& [ri, rj, rectCorner, dir, value, hrec] = rh;

		 //printf("ri=%d width(ri)=%d rj=%d corner=%s dir={.x=%.2f, .y=%.2f} value=%d\n", ri, width(input_rectangles[ri]), rj, RectCornerString[rectCorner], dir.x, dir.y, value);

			int n=input_rectangles.size();
			vector<MyRect> accumulated_transformation(n);
			const MyRect dr = hrec - input_rectangles[ri];
			accumulated_transformation[ri] = dr;
			const MyRect zero;

			auto ff=[&](const ST& st)->vector<MyRect> {

				const auto& [initial_tf, tf] = st;

				vector<MyRect> transformation(n);

				transformation[ri] = initial_tf;

				for (bool stop=false; stop==false; )
				{
					stop=true;
					for (int i : views::iota(0,n) | views::filter([&](int i){return transformation[i]==zero;}))
					{
						for (int j : views::iota(0,n) | views::filter([&](int j){return i!=j && transformation[j]!=zero;}))
						{
							if (intersect_strict(input_rectangles[i] + accumulated_transformation[i] + transformation[i],
												input_rectangles[j] + accumulated_transformation[j] + transformation[j]))
							{
								transformation[i] = tf;
								stop=false;
							}
						}
					}
				}
				return transformation;
			};

/*
			vector<MyRect> rectangles = input_rectangles;
			vector<int> is_selected(n,0);
			vector<SweepLineItem> sweep_line;

			for (int ri=0; ri < n; ri++)
			{
				for (RectDim rectdim : {LEFT,RIGHT,TOP,BOTTOM})
				{
					sweep_line2[ RectDimDirection[rectdim] ].push_back({.value=rectangles[ri][rectdim], .rectdim=rectdim, .ri=ri});
				}
			}

			for (Direction direction : {EAST_WEST, NORTH_SOUTH})
			{
		//use the sweep_line that is not impacted by selected translation
				Direction sweep_direction = Direction(1-direction);
				int n = dimensions(-dr)[dimension];

				TransformationType tt;

				switch (direction)
				{
				case EAST_WEST:
					tt = n<0 ? SQUEEZE_WIDTH : STRETCH_WIDTH;
				case NORTH_SOUTH:
					tt = n<0 ? SQUEEZE_HEIGHT : STRETCH_HEIGHT;
				}

				const auto [minCompactRectDim, maxCompactRectDim] : rectDimRanges[direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
				const auto [minSweepRectDim, maxSweepRectDim] = rectDimRanges[sweep_direction];

				ranges::sort(sweep_line);

				auto ff=[&](const ST& st)->vector<MyRect> {

					const auto& [initial_tf, tf] = st;

					ranges::fill(is_selected, 0);
					is_selected[ri]=1;
					rectangles[ri] += initial_tf;

					const MyRect frame = compute_frame(rectangles);

					set<int> active_line;

					for (const SweepLineItem& item : sweep_line)
					{
						const auto& [sweep_value, rectdim, ri] = item;
						switch(rectdim)
						{
						case LEFT:
						case TOP:
							assert(is_selected[ri] == 0);
							for (int rj : active_line | views::filter([](int rj){return is_selected[rj]==1;})
													| views::filter([](int rj){
														return range_intersect_strict(rectangles[ri][minCompactRectDim],
																					rectangles[ri][maxCompactRectDim],
																					rectangles[rj][minCompactRectDim]+1,
																					rectangles[rj][maxCompactRectDim]+1);
																			}
														)
													| views::take(1)
							)
							{
								is_selected[ri]=1;
							}
							active_line.insert(ri);
							break;
						case RIGHT:
						case BOTTOM:
							active_line.erase(ri);
							break;
						}
					}

					for (int i=0; i<n; i++)
					{
						if (i != ri && is_selected[i]==true)
							rectangles[i] += tf;
					}


					return transformation;
				};


				for (TransformationType transformationType : views::iota(0, abs(n)) | views::transform(tt))
				{
						vector<MyRect> transformation = ranges::min(Transformations[transformationType] | views::transform(ff), {},
												[&](const vector<MyRect>& tf){
																const auto [width_, height_] = dimensions(compute_frame(input_rectangles + accumulated_transformation + tf));
																int nb = n - ranges::count(tf, zero);
																return make_tuple(width_, height_, nb);
																 }
											);
						matw(accumulated_transformation) += transformation;
				}
			}
*/

			const auto [n1, n2] = dimensions(-dr);

			auto tt = [&](int i)->TransformationType{
							if (i < abs(n1))
									return n1<0 ? SQUEEZE_WIDTH : STRETCH_WIDTH;
							else
									return n2<0 ? SQUEEZE_HEIGHT : STRETCH_HEIGHT;
			};

			for (TransformationType transformationType : views::iota(0, abs(n1)+abs(n2)) | views::transform(tt))
			{
					vector<MyRect> transformation = ranges::min(Transformations[transformationType] | views::transform(ff), {},
											[&](const vector<MyRect>& tf){
												const auto [width_, height_] = dimensions(compute_frame(input_rectangles + accumulated_transformation + tf));
												int nb = n - ranges::count(tf, zero);
												return make_tuple(width_, height_, nb);
											}
										);
					matw(accumulated_transformation) += transformation;
			}
			return accumulated_transformation;
		};

		auto compute_ranking=[](int n, auto&& proj)->vector<int>{
				vector<int> indices(n), ranking(n);
				for (int ii=0; ii<n; ii++)
						indices[ii]=ii;
				ranges::sort(indices, {}, proj);
				for (int rk=0; rk<n; rk++)
				{
						int ii = indices[rk];
						ranking[ii]=rk;
				}
				return ranking;
		};

		vector<DecisionTreeNode> decision_tree;

		auto build_decision_tree = [&](int parent_index, const vector<MyRect>& input_rectangles, const vector<int>& moved_rectangles, auto&& build_decision_tree)->void{

			vector<RectHole> holes = compute_holes(input_rectangles);
/*
printf("moved_rectangles:");
for (int ri : moved_rectangles)
	printf("%d,", ri);
printf("\n");
*/
			int depth = moved_rectangles.size();

			if (depth >= 5)
				return;

			auto rgh = holes | views::filter([&](const RectHole& rh){return ranges::binary_search(moved_rectangles,rh.ri)==false;});
			vector<RectHole> holes_;
			ranges::copy(rgh, back_inserter(holes_));
			holes.clear();
/*
printf("move candidate rectangles:");
for (const RectHole rh : holes_)
{
        const auto& [m_left, m_right, m_top, m_bottom, i, no_sequence, selected] = rh.rec;
        printf("%d => {%d,%d,%d,%d},", rh.ri, m_left, m_right, m_top, m_bottom);
}
printf("\n");
*/
			auto edge_distance_gain=[&](int ii)->float{
				const auto& [ri, rj, rectCorner, dir, value, hrec] = holes_[ii];
				float gain = 0;
				for (const auto [i, j] : edges)
				{
					int k = -1;
					if (i==ri)
						k = j;
					else if (j==ri)
						k = i;
					if (k == -1)
						continue;
					gain -= rect_distance(input_rectangles[ri], input_rectangles[k]);
					gain += rect_distance(hrec, input_rectangles[k]);
				}
				return gain;
			};

			auto hole_potential=[&](int ii)->float{
				const auto& [ri, rj, rectCorner, dir, value, hrec] = holes_[ii];
				float potential=0;
				for (const auto [i, j] : edges)
				{
						if (i != ri && j != ri)
						{
								const MyRect &r_h=input_rectangles[ri], &r_i=input_rectangles[i],&r_j=input_rectangles[j];
								float d = rect_distance(r_i, r_j);
								float d_ = rect_distance(r_i, r_h) + rect_distance(r_j, r_h);
								if (d_ < d)
										potential += d - d_;
						}
				}
				return potential;
			};

			vector<int> ranking0 = compute_ranking(holes_.size(), edge_distance_gain);
			vector<int> ranking00 = compute_ranking(holes_.size(), hole_potential);
			vector<int> ranking01 = compute_ranking(holes_.size(), [&](int ii){return ranking0[ii]+ranking00[ii];});

			vector<RectHole> keeper_holes;

			int nn = holes_.size();
			int n = input_rectangles.size();

			ranges::copy(views::iota(0,nn) | views::filter([&](int ii){return ranking01[ii] < ranking_cap[n].RC1;})
											| views::transform([&](int ii){return holes_[ii];}),
                                        back_inserter(keeper_holes));

			auto rgr = keeper_holes | views::transform([&](const RectHole& rh)->vector<MyRect>{
				vector<MyRect> rectangles = input_rectangles + compute_transformation(input_rectangles, rh);
				vector<MyPoint> tf = compute_compact_frame_transform(rectangles);
				matw(rectangles) += tf;
				return rectangles;
			});

			vector<vector<MyRect> > node_rectangles;
			ranges::copy(rgr, back_inserter(node_rectangles));

			nn = keeper_holes.size();
			auto rg = views::iota(0, nn) | views::transform([&](int ii)->DecisionTreeNode{

				const vector<MyRect>& rectangles = node_rectangles[ii];
				const RectHole& rh = keeper_holes[ii];
				const auto& [ri, rj, rectCorner, dir, value, hrec] = rh;

				MyRect frame = compute_frame(rectangles);
				MyPoint dim = dimensions(frame);
				float rect_distances=0;
				for (const auto [i, j] : edges)
				{
					rect_distances += rect_distance(rectangles[i], rectangles[j]);
				}

				float potential=0;
				for (const auto [i, j] : edges)
				{
					if (i != ri && j != ri)
					{
						const MyRect &r_h=input_rectangles[ri], &r_i=input_rectangles[i],&r_j=input_rectangles[j];
						float d = rect_distance(r_i, r_j);
						float d_ = rect_distance(r_i, r_h) + rect_distance(r_j, r_h);
						if (d_ < d)
							potential += d - d_;
					}
				}

				return {parent_index, depth, rh, dim, rect_distances, potential};
			});

			vector<DecisionTreeNode> nodes;
			ranges::copy(rg, back_inserter(nodes));

			vector<int> ranking1 = compute_ranking(nodes.size(), [&](int ii){auto [w, h] = nodes[ii].dim; return max(w,h);});
			vector<int> ranking2 = compute_ranking(nodes.size(), [&](int ii){return nodes[ii].rect_distances;});
			vector<int> ranking3 = compute_ranking(nodes.size(), [&](int ii){return nodes[ii].potential;});
			for (int& rk : ranking3)
				rk = nn - rk;
			vector<int> ranking = compute_ranking(nodes.size(), [&](int ii){return ranking1[ii]+ranking2[ii]+ranking3[ii];});

			auto ft=[&](int ii){return ranking[ii] < ranking_cap[n].RC2;};
			size_t index = decision_tree.size();
			ranges::copy(views::iota(0,nn) | views::filter(ft) | views::transform([&](int ii){return nodes[ii];}),
					back_inserter(decision_tree));

			for (int ii : views::iota(0,nn) | views::filter(ft))
			{
				vector<int> v, a{nodes[ii].rh.ri};
				ranges::set_union(moved_rectangles, a, back_inserter(v));
				build_decision_tree(index++, node_rectangles[ii], v, build_decision_tree);
			}
		};

		auto print_html=[&](const vector<MyRect>& rectangles)->string{

			int n = rectangles.size();
			char buffer[10*1000];
			int pos=0;

			pos+= sprintf(buffer+pos, "<html>\n<body>\n");
			pos+= sprintf(buffer+pos, "<svg width=\"%d\" height=\"%d\">\n", width(frame)+100, height(frame));
			for (const MyRect& r : rectangles)
			{
				pos += sprintf(buffer+pos, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:blue;stroke:pink;stroke-width:5;opacity:0.5\" />\n",r.m_left, r.m_top, width(r), height(r));
				pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"red\">r-%d</text>\n", r.m_left, r.m_top, r.i);

				int dy = 0;
//TODO: C++23 introduces views::set_union range adapter. No longer need for vector<int> contacts.
				vector<int> contacts;
				ranges::set_union(
					edges | views::filter([&](const Edge& e){return e.from==r.i;}) | views::transform(&Edge::to),
					edges | views::filter([&](const Edge& e){return e.to==r.i;}) | views::transform(&Edge::from),
					std::back_inserter(contacts)
				);
				for (int ri : contacts)
				{
					dy += 14;
					pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"white\">r-%d</text>\n", r.m_left + 8, r.m_top + dy, ri);
				}

				dy = 0;
				for (int ri : views::iota(0, n) | views::filter([&](int rj){return r.i != rj && edge_overlap(r, /*input_*/rectangles[rj]);}))
				{
					dy += 14;
					pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", r.m_left + 30, r.m_top + dy, ri);
				}
			}
/*
			for (int hi=0; hi < holes.size() && hi < 15; hi++)
			{
					const auto& [ri, rj, RectCorner, direction, value, rec] = holes[hi];

					fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:red;stroke:green;stroke-width:5;opacity:0.5\" />\n",
									rec.m_left, rec.m_top, width(rec), height(rec));
					fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">hole-%d</text>\n", rec.m_left, rec.m_top, hi);

					int dy = 0;
					for (int rj : views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rec, input_rectangles[rj]);}))
					{
									dy += 14;
									fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", rec.m_left + 8, rec.m_top + dy, rj);
					}
					fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">ri=%d</text>\n", rec.m_left + 30, rec.m_top + 1*14, ri);
					fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">rj=%d</text>\n", rec.m_left + 30, rec.m_top + 2*14, rj);
			}
*/
			pos += sprintf(buffer+pos, "</svg>\n</html>");
			buffer[pos]=0;
			return buffer;
		};

		printf("calling build_decision_tree()\n");
		build_decision_tree(-1, input_rectangles, {}, build_decision_tree);

		vector<int> ranking1 = compute_ranking(decision_tree.size(), [&](int ii){return decision_tree[ii].rect_distances;});
		vector<int> ranking2 = compute_ranking(decision_tree.size(), [&](int ii){const auto [w, h] = decision_tree[ii].dim;return max(w, h);});
		vector<int> ranking3 = compute_ranking(decision_tree.size(), [&](int ii){return ranking1[ii]+ranking2[ii];});

		int ndt = decision_tree.size();
		int i_best = ranges::min( views::iota(0, ndt), {}, [&](int ii){return ranking3[ii];});
		int depth = decision_tree[i_best].depth;
		vector<RectHole> chemin(depth+1);
		printf("best ranking result:\n");
		for (int i=i_best, j=depth; i!=-1; i=decision_tree[i].parent_index, j--)
		{
			const auto& [parent_index, depth, rh, dim, rect_distances, potential] = decision_tree[i];
			const auto& [ri, rj, rectCorner, dir, value, hrec] = rh;
			printf("ri=%d depth=%d\n", ri, depth);
			printf("rect_distances=%.2f max(dim)=%d\n", rect_distances, max(dim.x,dim.y));
			chemin[j] = rh;
			printf("chemin[%d] = rh;\n", j);
		}

		vector<MyRect> rectangles = input_rectangles;
		for (int i=0; i < chemin.size(); i++)
		{
			const RectHole& rh = chemin[i];
			vector<MyRect> rectangles2 = rectangles + compute_transformation(rectangles, rh);
			char file_name[60];
			sprintf(file_name, "holes_%d.html", i);
			FILE *f;
			f=fopen(file_name, "w");
			string buffer=print_html(rectangles2);
			fprintf(f, "%s", buffer.c_str());
			fclose(f);
			vector<MyPoint> tf = compute_compact_frame_transform(rectangles2);
			matw(rectangles2) += tf;
			sprintf(file_name, "holes_compact_frame_%d.html", i);
			f=fopen(file_name, "w");
			buffer=print_html(rectangles2);
			fprintf(f, "%s", buffer.c_str());
			fclose(f);
			rectangles = rectangles2;
		}

		{
			FILE *f=fopen("dtstats.csv", "w");
			fprintf(f, "depth;maxdim;rect_distances;potential\n");
			for (const auto& [parent_index, depth, rh, dim, rect_distances, potential] : decision_tree)
			{
				fprintf(f, "%d;%d;%.0f;%.0f\n", depth, max(dim.x, dim.y), rect_distances, potential);
			}
			fclose(f);
		}

//		vector<MyRect> rectangles = input_rectangles + compute_transformation(input_rectangles, holes[5]);
		MyRect frame_ = compute_frame(rectangles);

	//TODO: compute rankings and select best node.
		int n = input_rectangles.size();

		FILE *f=fopen("holes.html", "w");
		string buffer=print_html(rectangles);
		fclose(f);

		f=fopen("rects.html", "w");
		fprintf(f, "<html>\n<body>\n");
		fprintf(f, "<svg width=\"%d\" height=\"%d\">\n", width(frame_)+100, height(frame_));
		for (const MyRect& r : rectangles)
		{
			fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:blue;stroke:pink;stroke-width:5;opacity:0.5\" />\n",
			r.m_left, r.m_top, width(r), height(r));
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"red\">r-%d</text>\n", r.m_left, r.m_top, r.i);

			int dy = 0;
//TODO: C++23 introduces views::set_union range adapter. No longer need for vector<int> contacts.
			vector<int> contacts;
			ranges::set_union(
						edges | views::filter([&](const Edge& e){return e.from==r.i;}) | views::transform(&Edge::to),
						edges | views::filter([&](const Edge& e){return e.to==r.i;}) | views::transform(&Edge::from),
						std::back_inserter(contacts)
							);
			for (int ri : contacts)
			{
				dy += 14;
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"white\">r-%d</text>\n", r.m_left + 8, r.m_top + dy, ri);
			}

			dy = 0;
			for (int ri : views::iota(0, n) | views::filter([&](int rj){return r.i != rj && edge_overlap(r, rectangles[rj]);}))
			{
				dy += 14;
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", r.m_left + 30, r.m_top + dy, ri);
			}
		}
		fprintf(f, "</svg>\n</html>");
		fclose(f);

	}
}
