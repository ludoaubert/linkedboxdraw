#include "compact_frame.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "FunctionTimer.h"
#include <thread>
#include <vector>
#include <map>
#include <span>
#include <ranges>
#include <cstdint>
#include <assert.h>
#include "latuile_test_json_output.h"
#include "thread_pool.h"
using namespace std ;

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

	bool operator==(const SweepLineItem&) const = default;
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
        int i, j;
        auto operator<=>(const RectLink&) const = default;
};


vector<RectTranslation> compute_compact_frame_transform_(const vector<MyRect>& input_rectangles)
{
	FunctionTimer ft("compute_cft_");

	vector<RectTranslation> rect_translations;

	const int N=20;
        int n = input_rectangles.size();

	MyRect rectangles_buffer[N];
	span rectangles(rectangles_buffer, n);
	ranges::copy(input_rectangles, rectangles_buffer);

	SweepLineItem sweep_line_buffer[2*N];
	span sweep_line(sweep_line_buffer, 2*n);

	int active_line[N];
	RectLink rect_links_buffer[256];
	RectLink in_rect_links_buffer[N];
	int in_edge_count[N];
	int edge_partition[N+1];

	MyPoint translations[N];

	for (Direction compact_direction : {EAST_WEST, NORTH_SOUTH})
	{
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

{
        FunctionTimer ft("cft_sort_sweepline");
		ranges::sort(sweep_line, CustomLess());
}

		int active_line_size=0;
		int rect_links_size=0;

		auto cmp=[&](int i, int j){return rectangles[i][minCompactRectDim]<rectangles[j][minCompactRectDim];};

		auto erase=[&](int i){
			int& lower = *lower_bound(active_line,active_line+active_line_size, i, cmp);
                        int pos = distance(active_line, &lower);
			D(printf("lower = %d\n", lower));
			D(printf("pos = %d\n", pos));

			for (int ii=pos; ii<active_line_size; ii++)
				swap(active_line[ii], active_line[ii+1]);
			active_line_size -= 1;
		};

		auto insert=[&](int i){
			int& upper = *upper_bound(active_line,active_line+active_line_size, i, cmp);
			D(printf("upper = %d\n", upper));
			int pos = distance(active_line, &upper);
			D(printf("pos = %d\n", pos));

			for (int ii=active_line_size-1; ii>=pos; ii--)
				swap(active_line[ii],active_line[ii+1]);
			active_line_size += 1;
			active_line[pos]=i;

			if (pos > 0)
                        	rect_links_buffer[rect_links_size++] = {active_line[pos-1], active_line[pos]};
			if (pos+1 < active_line_size)
                                rect_links_buffer[rect_links_size++] = {active_line[pos], active_line[pos+1]};
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
				insert(ri);
				break;
			case RIGHT:
			case BOTTOM:
				D(printf("sweep leaving %d %s\n", ri, RectDimString[rectdim]));
				erase(ri);
				break;
			}
		}
}
{
        FunctionTimer ft("cft_rectlinks");
		sort(rect_links_buffer, rect_links_buffer + rect_links_size);

		auto end = unique(rect_links_buffer, rect_links_buffer + rect_links_size);
		rect_links_size = distance(rect_links_buffer, end);
}

#ifdef _TRACE_
		D(printf("rect_links:"));
		for (auto [i, j] : span(rect_links_buffer, rect_links_size))
		{
			D(printf("{%d => %d},", i, j));
		}
		D(printf("\n"));
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
		for (const auto& [i, j] : span(rect_links_buffer, rect_links_size))
			in_edge_count[j] += 1;
		for (int ri : views::iota(0, n) | views::filter([&](int ri){return in_edge_count[ri]==0;}))
			in_rect_links_buffer[in_rect_links_size++] = {-INT16_MAX, ri};
#ifdef _TRACE_
		D(printf("in_edges: "));
		for (const auto [i, j] : span(in_rect_links_buffer, in_rect_links_size))
			D(printf("%d, ", j));
		D(printf("\n"));
#endif
}

		auto adj_list=[&](int ri)->span<RectLink>{
			int i=edge_partition[ri];
			int j=edge_partition[ri+1];
			return span(&rect_links_buffer[i], j-i);
		};
		int compact_dimension=0;
		int tr;
{
        FunctionTimer ft("cft_query_compact_dim");
		auto rec_query_compact_dimension=[&](int ri, auto&& rec_query_compact_dimension)->int{
			span<RectLink> adj = adj_list(ri);
			if (adj.empty())
			{
				return dimensions(rectangles[ri])[compact_direction];
			}
			int tr = ranges::max(adj | views::transform([&](const RectLink& e){return rec_query_compact_dimension(e.j, rec_query_compact_dimension);}));
			return tr + dimensions(rectangles[ri])[compact_direction];
		};

		auto query_compact_dimension=[&]()->int{
			span adj(in_rect_links_buffer, in_rect_links_size);
			return ranges::max(adj | views::transform([&](const RectLink& e){return rec_query_compact_dimension(e.j, rec_query_compact_dimension);}));
		};

		compact_dimension = query_compact_dimension();
		tr = dimensions(frame)[compact_direction] - compact_dimension;
		D(printf("compact_dimension=%d tr=%d\n", compact_dimension, tr));
		rect_translations.push_back({QUERY_COMPACT_DIMENSION, compact_direction, -1, tr});
}
{
        FunctionTimer ft("cft_push");
		auto rec_push=[&](int ri, int tri, auto&& rec_push)->void{
			span<RectLink> adj = adj_list(ri);
			translations[ri][compact_direction] = tri;
			for (const RectLink& e : adj)
			{
				int trj = tri - (rectangles[e.j][minCompactRectDim] - rectangles[ri][maxCompactRectDim]);
				if (trj > 0)
					rec_push(e.j, trj, rec_push);
			}
		};

		auto push=[&](int tr){
			span adj(in_rect_links_buffer, in_rect_links_size);
			for (const RectLink& e : adj)
			{
				int trj = tr - (rectangles[e.j][minCompactRectDim] - frame[minCompactRectDim]);
				if (trj > 0)
					rec_push(e.j, trj, rec_push);
			}
		};
		push(tr);
}
{
#ifdef _TRACE_
		D(printf("translations={"));
		for (int ri=0; ri < n; ri++)
		{
			int tr = translations[ri][compact_direction];
			if (tr != 0)
				D(printf("{ri=%d, tr=%d},",ri, tr));
		}
		D(printf("}\n"));
#endif
                for (int ri=0; ri < n; ri++)
                {
			int value = translations[ri][compact_direction];
			if (value != 0)
				rect_translations.push_back({COMPACT_FRAME, compact_direction, ri, value});
//			rectangles[ri] += translations[FORWARD_LINKS][ri];
		}
}
	}//for (Direction compact_direction : {EAST_WEST, NORTH_SOUTH})
{
        FunctionTimer ft("cft_return_result");

#ifdef _TRACE_
	D(printf("rect_translations={\n"));
	for (const auto [algorithm, compact_direction, ri, value] : rect_translations)
	{
		printf("{.algorithm=%s, .compact_direction=%s, .ri=%d, .value=%d},\n", AlgorithmString[algorithm],
			DirectionString[compact_direction], ri, value);
	}
	D(printf("}\n"));
#endif
	return rect_translations;
}
}


vector<MyPoint> compute_compact_frame_transform(const vector<MyRect>& rectangles)
{
//	FunctionTimer ft("compute_compact_frame_transform");

	int n = rectangles.size();

	vector<MyPoint> accumulated_transform(n);

	const MyPoint zero;

	const MyPoint translation4[4]={{.x=1, .y=0},
				 {.x=-1, .y=0},
				 {.x=0, .y=1},
				 {.x=0, .y=-1}};

	for (RectDim rect_dim : RectDims)//{LEFT, RIGHT, TOP, BOTTOM}
	{
		const MyPoint& translation = translation4[rect_dim] ;

		while (true)
		{
			vector<MyPoint> transform(n) ;

			const MyRect frame = compute_frame(rectangles + accumulated_transform);

			const MyRect rake4[4] = {
						{.m_left=-INT16_MAX, .m_right=frame.m_left, .m_top=-INT16_MAX, .m_bottom=INT16_MAX},
						{.m_left=frame.m_right, .m_right=INT16_MAX, .m_top=-INT16_MAX, .m_bottom=INT16_MAX},
						{.m_left=-INT16_MAX, .m_right=INT16_MAX, .m_top=-INT16_MAX, .m_bottom=frame.m_top},
						{.m_left=-INT16_MAX, .m_right=INT16_MAX, .m_top=frame.m_bottom, .m_bottom=INT16_MAX}
					};

			int sens = rect_dim % 2;
			int dimension = rect_dim / 2;
			assert(rect_dim == 2*dimension + sens);

			const MyRect rake = rake4[rect_dim];
			const MyRect baseline = rake4[2*dimension + (1-sens)];

			//rectangles that we want to rake along
			for (int ri : views::iota(0, n) | views::filter([&](int ri){return intersect(rake, rectangles[ri] + accumulated_transform[ri]);}))
			{
				transform[ri] = translation;
			}

			for  (bool stop = false; stop == false;)
			{
				stop=true;

				for (int ri=0; ri<n; ri++)
				{
					for (int rj=0; rj<n; rj++)
					{
						if (transform[ri]!=zero && transform[rj]==zero &&
							intersect_strict(rectangles[ri]+accumulated_transform[ri]+transform[ri], rectangles[rj]+accumulated_transform[rj]+transform[rj])
						)
						{
							stop = false;
							transform[rj] = translation;
						}
					}
				}
			}

			//rectangles that hit the baseline
			auto rg = views::iota(0, n) | views::filter([&](int ri){return intersect_strict(baseline, rectangles[ri] + accumulated_transform[ri] + transform[ri]);});

			if ( rg.empty() == false )
			{
				break;
			}

			matw(accumulated_transform) += transform;
		}
	}

	return accumulated_transform;
}


void compact_frame(vector<MyRect>& rectangles, const vector<vector<MPD_Arc> > &adjacency_list)
{
	FunctionTimer ft("compact_frame");

	for (int i=0; i < rectangles.size(); i++)
		rectangles[i].i = i;

	int n = rectangles.size();

	MyPoint translation4[4]={{.x=1, .y=0},
				 {.x=-1, .y=0},
				 {.x=0, .y=1},
				 {.x=0, .y=-1}};

	for (RectDim rect_dim : RectDims)//{LEFT, RIGHT, TOP, BOTTOM}
	{
		MyPoint translation = translation4[rect_dim] ;

		const vector<MyRect> rects = rectangles;

		while (true)
		{
			const MyRect frame = compute_frame(rectangles);

			const MyRect rake4[4] = {
						{.m_left=-INT16_MAX, .m_right=frame.m_left, .m_top=-INT16_MAX, .m_bottom=INT16_MAX},
						{.m_left=frame.m_right, .m_right=INT16_MAX, .m_top=-INT16_MAX, .m_bottom=INT16_MAX},
						{.m_left=-INT16_MAX, .m_right=INT16_MAX, .m_top=-INT16_MAX, .m_bottom=frame.m_top},
						{.m_left=-INT16_MAX, .m_right=INT16_MAX, .m_top=frame.m_bottom, .m_bottom=INT16_MAX}
					};

			int sens = rect_dim % 2;
			int dimension = rect_dim / 2;
			assert(rect_dim == 2*dimension + sens);

			const MyRect rake = rake4[rect_dim];
			const MyRect baseline = rake4[2*dimension + (1-sens)];

			for (MyRect& r : rectangles)
				r.selected = false;

			//rectangles that we want to rake along
			for (MyRect& r : rectangles | views::filter([&](const MyRect& r){return intersect(rake, r);}))
			{
				r.selected = true;
				translate(r, translation);
			}

			for  (bool stop = false; stop == false;)
			{
				stop=true;

				for (MyRect& r : rectangles | views::filter([](const MyRect& r){return r.selected;}))
				{
					for (MyRect& rc : rectangles | views::filter([](const MyRect& r){return !r.selected;}))
					{
						if (intersect_strict(r, rc))
						{
							rc.selected = true;
							stop = false;
							translate(rc, translation);
						}
					}
				}
			}

			//rectangles that hit the baseline
			auto rg = rectangles | views::filter([&](const MyRect& r){return intersect_strict(baseline, r);});

			if ( rg.empty() == false )
			{
				for (MyRect& r : rectangles)
				{
					if (r.selected)
					{
						r.selected = false;
						translate(r, - translation);
					}
				}

				break;
			}
		}

		for (int i=0; i < n; i++)
		{
			const MyRect &r2 = rectangles[i], &r1 = rects[i];
			int xx = r2.m_left - r1.m_left;
			int yy = r2.m_top - r1.m_top;
			if (xx != 0 || yy != 0)
				printf("translate(r.i=%d, {x=%d, y=%d}\n", i, xx, yy);
		}
	}
}


void compute_stress_line(const vector<MyRect>& rectangles, vector<int> (&stress_line)[2])
{
	const int n = rectangles.size();
	const MyRect frame = compute_frame(rectangles);

	for (Direction direction : directions)
	{
		const auto [minRectDim, maxRectDim] = rectDimRanges[direction];

		vector<MPD_Arc> contacts;

	//rectangles that hit the baseline
		for (const MyRect& r : rectangles | views::filter([&](const MyRect& r){return frame[minRectDim] == r[minRectDim];}))
		{
			contacts.push_back({-INT16_MAX, r.i});
		}

		for (const MyRect& r1 : rectangles)
		{
			for (const MyRect& r2 : rectangles)
			{
				if (r1.i == r2.i)
					continue;

				if (
					(direction == EAST_WEST && r1[RIGHT]==r2[LEFT] && range_overlap(r1.m_top, r1.m_bottom, r2.m_top, r2.m_bottom)!=0) ||
					(direction == NORTH_SOUTH && r1[BOTTOM]==r2[TOP] && range_overlap(r1.m_left, r1.m_right, r2.m_left, r2.m_right)!=0)
				)
				{
					contacts.push_back({r1.i, r2.i});
				}
			}
		}


	//rectangles that hit the baseline
		for (const MyRect& r : rectangles | views::filter([&](const MyRect& r){return r[maxRectDim] == frame[maxRectDim];}))
		{
			contacts.push_back({r.i, INT16_MAX});
		}

		printf("contacts:\n");
		for (const auto& [i, j] : contacts)
		{
			printf("%d -> %d\n", i, j);
		}

// As explained here : https://www.geeksforgeeks.org/recursive-lambda-expressions-in-cpp/

		map<int, vector<int> > petit_poucet;	//remembers all rectangles visited via all possible roads

		for (int i : views::iota(0, n))
			petit_poucet[i] = {i};
		for (int i : {-INT16_MAX, INT16_MAX})
			petit_poucet[i] = {i};

//TODO: use https://en.cppreference.com/w/cpp/language/member_functions#Explicit_object_parameter
//C++23 feature called "deducing `this`"
/*
		auto list_stress_line = [&](this auto self, int source)->void {

			for (const MPD_Arc& e : contacts | views::filter([&](const MPD_Arc& e){return e._i == source;}))
			{
				vector<int> vj;
				ranges::set_union(petit_poucet[e._i], petit_poucet[e._j], std::back_inserter(vj));
B				petit_poucet[e._j] = vj;

				self(e._j);
			}

		};
*/

		auto list_stress_line = [&](int source, auto&& list_stress_line)->void {

			for (const MPD_Arc& e : contacts | views::filter([&](const MPD_Arc& e){return e._i == source;}))
			{
				vector<int> vj;
				ranges::set_union(petit_poucet[e._i], petit_poucet[e._j], std::back_inserter(vj));
				petit_poucet[e._j] = vj;

				list_stress_line(e._j, list_stress_line);
			}

		};

		// Function as an argument
		list_stress_line(-INT16_MAX, list_stress_line);

		for (int ri : petit_poucet[INT16_MAX])
		{
			if (ri != INT16_MAX && ri != -INT16_MAX)
				stress_line[direction].push_back(ri);
		}

		printf("pressure line:\n");
		for (int i: petit_poucet[INT16_MAX])
		{
			printf("%d\n", i);
		}

	}

}



void test_compact_frame()
{
        FunctionTimer::MAX_NESTING=2;
//	const int TEST_LOOP_REPEAT=1000000;
	const int TEST_LOOP_REPEAT=1;
	D(static_assert(TEST_LOOP_REPEAT==1));

FunctionTimer ft("lulu");

	struct TestContext {int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<RectTranslation> expected_translations; };

	const vector<TestContext> test_contexts={
/*
           +-------+
           |       |
+------+   |   1   |   +------+
|      |   |       |   |      |
|  0   |   +-------+   |  2   |   +------+
|      |               |      |   |      |
+------+               +------+   |  3   |
+------+   +-------+              |      |
|      |   |       |              +------+
|  4   |   |   5   |
|      |   |       |
+------+   +-------+
*/
        {
                .testid=0,
                .input_rectangles = {
                        {.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
                        {.m_left=150, .m_right=250, .m_top=0, .m_bottom=100},
                        {.m_left=300, .m_right=400, .m_top=50, .m_bottom=150},
                        {.m_left=450, .m_right=550, .m_top=100, .m_bottom=200},
                        {.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
                        {.m_left=150, .m_right=250, .m_top=150, .m_bottom=250}
                },
                .edges = {},
                .expected_translations = {
			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=EAST_WEST, .ri=-1, .value=150},

			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=0, .value=150},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=1, .value=100},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=2, .value=50},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=4, .value=150},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=5, .value=100},

			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=NORTH_SOUTH, .ri=-1, .value=50},
			{.algorithm=COMPACT_FRAME, .compact_direction=NORTH_SOUTH, .ri=1, .value=50}
                }
        },

	{
		.testid=1,
		.input_rectangles = {
			{.m_left=209, .m_right=411, .m_top=352, .m_bottom=672},//datamart_metric
			{.m_left=495, .m_right=641, .m_top=0, .m_bottom=160},//preference
			{.m_left=0, .m_right=188, .m_top=224, .m_bottom=352},//datamart_report_prop
			{.m_left=641, .m_right=843, .m_top=464, .m_bottom=672},//destinationblacklist
			{.m_left=641, .m_right=829, .m_top=144, .m_bottom=272},//user_session
			{.m_left=-14, .m_right=209, .m_top=608, .m_bottom=720},//datamart_metric_parameter
			{.m_left=21, .m_right=209, .m_top=352, .m_bottom=480},//datamart_metric_prop
			{.m_left=188, .m_right=390, .m_top=96, .m_bottom=352},//datamart_report
			{.m_left=641, .m_right=836, .m_top=272, .m_bottom=464},//folder_preference
			{.m_left=411, .m_right=641, .m_top=160, .m_bottom=672} //sfuser
		},

		.edges = {
			{ .from=0, .to=7},
			{ .from=0, .to=9},
			{ .from=1, .to=9},
			{ .from=2, .to=7},
			{ .from=3, .to=9},
			{ .from=4, .to=9},
			{ .from=5, .to=0},
			{ .from=6, .to=0},
			{ .from=7, .to=9},
			{ .from=8, .to=9},
			{ .from=9, .to=9}
		},

		.expected_translations = {
			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=EAST_WEST, .ri=-1, .value=0},

			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=NORTH_SOUTH, .ri=-1, .value=48},

			{.algorithm=COMPACT_FRAME, .compact_direction=NORTH_SOUTH, .ri=1, .value=48},
			{.algorithm=COMPACT_FRAME, .compact_direction=NORTH_SOUTH, .ri=9, .value=48}
		}
	},
	{
		.testid=2,
		.input_rectangles = {
			{.m_left=0, .m_right=10, .m_top=0, .m_bottom=10},
			{.m_left=0, .m_right=10, .m_top=20, .m_bottom=30}
		},

		.edges = {
			{.from=0,.to=1}
		},

		.expected_translations = {
			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=EAST_WEST, .ri=-1, .value=0},
			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=NORTH_SOUTH, .ri=-1, .value=10},

			{.algorithm=COMPACT_FRAME, .compact_direction=NORTH_SOUTH, .ri=0, .value=10}
		}
	},
	{
		.testid=3,
		.input_rectangles = {
			{.m_left=396-RECT_BORDER+FRAME_BORDER, .m_right=396+162+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+104+RECT_BORDER+FRAME_BORDER},//8
			{.m_left=320-RECT_BORDER+FRAME_BORDER, .m_right=320+182+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+72+RECT_BORDER+FRAME_BORDER},//9
			{.m_left=453-RECT_BORDER+FRAME_BORDER, .m_right=453+105+RECT_BORDER+FRAME_BORDER, .m_top=218-RECT_BORDER+FRAME_BORDER, .m_bottom=218+72+RECT_BORDER+FRAME_BORDER},//10
			{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+152+RECT_BORDER+FRAME_BORDER},//21
			{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=202-RECT_BORDER+FRAME_BORDER, .m_bottom=202+88+RECT_BORDER+FRAME_BORDER},//24
			{.m_left=750-RECT_BORDER+FRAME_BORDER, .m_right=750+147+RECT_BORDER+FRAME_BORDER, .m_top=346-RECT_BORDER+FRAME_BORDER, .m_bottom=346+120+RECT_BORDER+FRAME_BORDER},//25
			{.m_left=273-RECT_BORDER+FRAME_BORDER, .m_right=273+140+RECT_BORDER+FRAME_BORDER, .m_top=154-RECT_BORDER+FRAME_BORDER, .m_bottom=154+120+RECT_BORDER+FRAME_BORDER},//26
			{.m_left=542-RECT_BORDER+FRAME_BORDER, .m_right=542+168+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+136+RECT_BORDER+FRAME_BORDER},//27
			{.m_left=335-RECT_BORDER+FRAME_BORDER, .m_right=335+168+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+120+RECT_BORDER+FRAME_BORDER},//28
			{.m_left=556-RECT_BORDER+FRAME_BORDER, .m_right=556+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+104+RECT_BORDER+FRAME_BORDER},//30
			{.m_left=764-RECT_BORDER+FRAME_BORDER, .m_right=764+133+RECT_BORDER+FRAME_BORDER, .m_top=186-RECT_BORDER+FRAME_BORDER, .m_bottom=186+120+RECT_BORDER+FRAME_BORDER},//32
			{.m_left=743-RECT_BORDER+FRAME_BORDER, .m_right=743+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+168+RECT_BORDER+FRAME_BORDER},//44
			{.m_left=93-RECT_BORDER+FRAME_BORDER, .m_right=93+140+RECT_BORDER+FRAME_BORDER, .m_top=153-RECT_BORDER+FRAME_BORDER, .m_bottom=153+88+RECT_BORDER+FRAME_BORDER},//48
			{.m_left=20-RECT_BORDER+FRAME_BORDER, .m_right=10+155+RECT_BORDER+FRAME_BORDER, .m_top=281-RECT_BORDER+FRAME_BORDER, .m_bottom=281+120+RECT_BORDER+FRAME_BORDER},//52
			{.m_left=21-RECT_BORDER+FRAME_BORDER, .m_right=21+175+RECT_BORDER+FRAME_BORDER, .m_top=441-RECT_BORDER+FRAME_BORDER, .m_bottom=441+136+RECT_BORDER+FRAME_BORDER}//53
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

		.expected_translations = {
			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=EAST_WEST, .ri=-1, .value=68},

			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=13, .value=68},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=14, .value=67},

			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=NORTH_SOUTH, .ri=-1, .value=0}
		}
	},
/*
      +-----------+          +----------+
      |           |          |          |
      |           |          |          |
      |     0     |          |     4    |
      |           |          |          |
      |           |          |          |
+-----+-----+-----+----+-----+----+-----+
|           |          |          |
|           |          |          |
|     1     |    2     |     5    |
|           |          |          |
|           |          |          |
+-----+-----+-----+----+----------+
      |           |
      |           |
      |     3     |
      |           |
      |           |
      +-----------+
*/
	{
		.testid=4,
		.input_rectangles = {
			{.m_left=20, .m_right=70, .m_top=0, .m_bottom=50},
			{.m_left=0, .m_right=50, .m_top=50, .m_bottom=100},
			{.m_left=50, .m_right=100, .m_top=50, .m_bottom=100},
			{.m_left=20, .m_right=70, .m_top=100, .m_bottom=150},
			{.m_left=120, .m_right=170, .m_top=0, .m_bottom=50},
			{.m_left=100, .m_right=150, .m_top=50, .m_bottom=100}
		},

		.edges = {
			{.from=0,.to=1}
		},

		.expected_translations = {
			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=EAST_WEST, .ri=-1, .value=20},

			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=1, .value=20},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=2, .value=20},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=5, .value=20},

			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=NORTH_SOUTH, .ri=-1, .value=0}
		}
	},
/*
           +-------+
           |       |
+------+   |   1   |   +------+
|      |   |       |   |      |
|  0   |   +-------+   |  2   |   +------+
|      |               |      |   |      |
+------+               +------+   |  3   |
  +------+ +-------+              |      |
  |      | |       |              +------+
  |  4   | |   5   |
  |      | |       |
  +------+ +-------+
*/
        {
                .testid=5,
                .input_rectangles = {
                        {.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
                        {.m_left=150, .m_right=250, .m_top=0, .m_bottom=100},
                        {.m_left=300, .m_right=400, .m_top=50, .m_bottom=150},
                        {.m_left=450, .m_right=550, .m_top=100, .m_bottom=200},
                        {.m_left=20, .m_right=120, .m_top=170, .m_bottom=270},
                        {.m_left=150, .m_right=250, .m_top=170, .m_bottom=270}
                },
                .edges = {},
                .expected_translations = {
			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=EAST_WEST, .ri=-1, .value=150},

			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=0, .value=150},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=1, .value=100},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=2, .value=50},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=4, .value=130},
			{.algorithm=COMPACT_FRAME, .compact_direction=EAST_WEST, .ri=5, .value=100},

			{.algorithm=QUERY_COMPACT_DIMENSION, .compact_direction=NORTH_SOUTH, .ri=-1, .value=70},

			{.algorithm=COMPACT_FRAME, .compact_direction=NORTH_SOUTH, .ri=0, .value=20},
			{.algorithm=COMPACT_FRAME, .compact_direction=NORTH_SOUTH, .ri=1, .value=70},
			{.algorithm=COMPACT_FRAME, .compact_direction=NORTH_SOUTH, .ri=2, .value=20}
                }
        }
	};

	unsigned hc = 1;// thread::hardware_concurrency();
	printf("hardware_concurrency=%d\n", hc);

auto job=[&](){

for(int loop=0; loop * hc < TEST_LOOP_REPEAT; loop++)
{
	for (const auto& [testid, input_rectangles, edges, expected_translations] : test_contexts)
	{
		int n = input_rectangles.size();
		vector<RectTranslation> translations = compute_compact_frame_transform_(input_rectangles) ;
                bool bOK = translations == expected_translations;
if constexpr (TEST_LOOP_REPEAT==1)
{
                int dm1 = dim_max(compute_frame(input_rectangles));

                vector<vector<MPD_Arc> > adjacency_list(n) ;
                for (const Edge& e : edges)
                {
                        adjacency_list[e.from].push_back({e.from, e.to}) ;
                }
		vector<int> stress_line[2];
		compute_stress_line(input_rectangles, stress_line);
/*
		int dm2 = dim_max(compute_frame(input_rectangles + translations));

		latuile_test_json_output(input_rectangles,
					input_rectangles + translations,
					edges,
					input_rectangles + expected_translations,
					"compact_frame",
					testid);

        	D(printf("compact_frame testid=%d : %s\n", testid, bOK ? "OK" : "KO"));
		D(printf("dim_max(frame) : %d => %d\n", dm1, dm2));
*/
}//if constexpr (TEST_LOOP_REPEAT==1)
		(bOK ? nbOK : nbKO)++;
	}//for (const auto& [testid, input_rectangles, edges, expected_translations] : test_contexts)
}//for(int loop=0; loop<TEST_LOOP_REPEAT; loop++)
};//auto job=[&](int id){

    ThreadPool tp;

    // queue work tasks
    for (int i=0; i<hc; ++i)
        tp.enqueue(job);

    tp.waitFinished();
    printf("tp.getProcessed(): %d\n", tp.getProcessed());

    // destructor will close down thread pool
    //return EXIT_SUCCESS;
}
