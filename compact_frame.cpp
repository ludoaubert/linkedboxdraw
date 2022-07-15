#include "compact_frame.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "FunctionTimer.h"
#include <vector>
#include <map>
#include <span>
#include <ranges>
#include <cstdint>
#include <assert.h>
#include "latuile_test_json_output.h"
using namespace std ;


struct SweepLineItem
{
	int16_t value;
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

struct RectLink
{
        int i, j;
        auto operator<=>(const RectLink&) const = default;
};

struct TrCandidate{int o, ri, tr;};


vector<MyPoint> compute_compact_frame_transform_(const vector<MyRect>& input_rectangles)
{
	FunctionTimer ft("compute_cft_");

	vector<MyRect> rectangles = input_rectangles;
	int n = rectangles.size();

	vector<SweepLineItem> sweep_line2[2];

	const MyPoint translation2[2]={{.x=1, .y=0}, {.x=0, .y=1}};

	for (Direction compact_direction : {EAST_WEST, NORTH_SOUTH})
	{
		MyRect frame = compute_frame(rectangles);
//use the sweep_line that is not impacted by selected translation
		Direction sweep_direction = Direction(1-compact_direction);

		auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[compact_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
		auto [minSweepRectDim, maxSweepRectDim] = rectDimRanges[sweep_direction];

		sweep_line2[sweep_direction].reserve(2*n);

		for (int ri=0; ri < n; ri++)
		{
			for (RectDim rectdim : {minSweepRectDim, maxSweepRectDim})
			{
				sweep_line2[sweep_direction].push_back({.value=rectangles[ri][rectdim], .rectdim=rectdim, .ri=ri});
			}
		}

		const MyPoint& translation = translation2[compact_direction] ;

		ranges::sort(sweep_line2[sweep_direction]);

		const vector<SweepLineItem>& sweep_line = sweep_line2[sweep_direction];

		int active_line[20];
		int active_line_size=0;

		auto cmp=[&](int i, int j){return rectangles[i][minCompactRectDim]<rectangles[j][minCompactRectDim];};

		auto erase=[&](int i){
			int& lower = *ranges::lower_bound(span(active_line,active_line_size), i, cmp);
			printf("lower = %d\n", lower);
			int pos = distance(active_line, &lower);
			printf("pos = %d\n", pos);
			for (int ii=pos; ii<active_line_size; ii++)
				swap(active_line[ii], active_line[ii+1]);
			active_line_size -= 1;
		};

		auto insert=[&](int i){
			int& upper = *ranges::upper_bound(span(active_line,active_line_size), i, cmp);
			printf("upper = %d\n", upper);
			int pos = distance(active_line, &upper);
			printf("pos = %d\n", pos);
			for (int ii=active_line_size-1; ii>=pos; ii--)
				swap(active_line[ii],active_line[ii+1]);
			active_line_size += 1;
			active_line[pos]=i;
		};

		vector<RectLink> rect_links, forbidden_rect_links, allowed_rect_links;
		rect_links.reserve(256);
		forbidden_rect_links.reserve(256);
		allowed_rect_links.reserve(256);

		auto push_rect_links = [&](){
			for (int i=0; i+1 < active_line_size; i++)
			{
				rect_links.push_back({active_line[i], active_line[i+1]});
			}

			for (int i=0; i+2 < active_line_size; i++)
			{
				forbidden_rect_links.push_back({active_line[i], active_line[i+2]});
			}
		};

		for (const SweepLineItem& item : sweep_line)
		{
			const auto& [value, rectdim, ri] = item;
			switch(rectdim)
			{
			case LEFT:
			case TOP:
				printf("sweep reaching %d %s\n", ri, RectDimString[rectdim]);
				insert(ri);
				push_rect_links();
				break;
			case RIGHT:
			case BOTTOM:
				printf("sweep leaving %d %s\n", ri, RectDimString[rectdim]);
				erase(ri);
				push_rect_links();
				break;
			}
		}

		ranges::sort(rect_links);
		auto ret1 = ranges::unique(rect_links);
		rect_links.erase(ret1.begin(), ret1.end());

		ranges::sort(forbidden_rect_links);
		auto ret2 = ranges::unique(forbidden_rect_links);
		forbidden_rect_links.erase(ret2.begin(), ret2.end());

		ranges::set_difference(rect_links, forbidden_rect_links, back_inserter(allowed_rect_links));

		printf("rect_links:\n");
		for (auto [i, j] : rect_links)
		{
			printf("%d => %d\n", i, j);
		}
		printf("forbidden_rect_links:\n");
		for (auto [i, j] : forbidden_rect_links)
		{
			printf("%d => %d\n", i, j);
		}
		printf("allowed_rect_links:\n");
		for (auto [i, j] : allowed_rect_links)
		{
			printf("%d => %d\n", i, j);
		}

		vector<int> edge_partition={0,1,2,3,3,4,5};

		auto adj_list=[&](int ri)->span<RectLink>{
			int i=edge_partition[ri], j=edge_partition[ri+1];
			return span(&rect_links[i], j-i);
		};

		vector<TrCandidate> translation_candidates;
		translation_candidates.reserve(256);

		auto rec_query_translation=[&](int o, int ri, auto&& rec_query_translation)->int{
			span<RectLink> adj = adj_list(ri);
			if (adj.empty())
			{
				int tr = frame[maxCompactRectDim] - rectangles[ri][maxCompactRectDim];
				translation_candidates.push_back({o, ri, tr});
				return tr;
			}
			int tr = ranges::min(adj | views::transform([&](const RectLink& e){
						return rec_query_translation(o, e.j, rec_query_translation) + rectangles[e.j][minCompactRectDim]-rectangles[ri][maxCompactRectDim];
					}
				)
			);
			translation_candidates.push_back({o, ri, tr});
			return tr;
		};

		for (int o : views::iota(0,n) | views::filter([&](int i){return rectangles[i][minCompactRectDim]==frame[minCompactRectDim];}))
		{
			rec_query_translation(o, o, rec_query_translation);
		}

		for (auto& [o, ri, tr] : translation_candidates)
		{
			printf("o=%d ri=%d tr=%d\n", o, ri, tr);
		}

		int tr_min = ranges::min( translation_candidates | views::filter([&](const TrCandidate& trc){return trc.o==trc.ri;}) | views::transform(&TrCandidate::tr));
		printf("tr_min=%d\n", tr_min);

		vector<int> translations(n,-1);

		for (const auto& [o, ri, tr] : translation_candidates | views::filter([&](const TrCandidate& trc){return trc.o==trc.ri;}))
		{
			translations[o] = tr;
		}

		for (auto& [o, ri, tr] : translation_candidates)
		{
			tr = tr + min(translations[o], tr_min) - translations[o];
		}

		for (auto& [o, ri, tr] : translation_candidates)
		{
			printf("o=%d ri=%d tr=%d\n", o, ri, tr);
		}

		for (auto& [o, ri, tr] : translation_candidates | views::filter([&](const TrCandidate& trc){return rectangles[trc.ri][maxCompactRectDim]==frame[maxCompactRectDim];}))
		{
			tr = 0;
		}

		printf("after setting backline to zero:\n");
		for (auto& [o, ri, tr] : translation_candidates)
		{
			printf("o=%d ri=%d tr=%d\n", o, ri, tr);
		}

		for (auto& [o, ri, tr] : translation_candidates)
		{
			translations[ri]=tr;
		}

		for (int ri=0; ri < n; ri++)
		{
			printf("translations[ri=%d]=%d\n",ri, translations[ri]);
		}

		vector<int> is_selected(n,0);

		while (true)
		{
            printf("looping\n");
			ranges::fill(is_selected,0);

			const MyRect frame = compute_frame(rectangles);
		//rectangles that we want to rake along
			for (int ri : views::iota(0, n) | views::filter([&](int ri){return frame[minCompactRectDim]==rectangles[ri][minCompactRectDim];}))
			{
					is_selected[ri]=1;
			}

			for (bool stop=false; stop==false; )
			{
				stop=true;
				for (auto [ri, rj] : rect_links)
				{
					if (is_selected[ri]==1 && is_selected[rj]==0 &&
						rectangles[ri][maxCompactRectDim]+is_selected[ri] == rectangles[rj][minCompactRectDim]+is_selected[rj]+1)
					{
						printf("rectangles[%d][%s]+is_selected[%d]=%d + %d\n", ri, RectDimString[maxCompactRectDim], ri,
							rectangles[ri][maxCompactRectDim], is_selected[ri]);
						printf("rectangles[%d][%s]+is_selected[%d]=%d + %d\n", rj, RectDimString[minCompactRectDim], rj,
                                                        rectangles[rj][minCompactRectDim], is_selected[rj]);
						is_selected[rj]=1;
						printf("is_selected[%d]=1\n", rj);
						stop=false;
					}
				}
			}

			//rectangles that hit the backline
			auto rg = views::iota(0, n) | views::filter([&](int ri){return frame[maxCompactRectDim]<rectangles[ri][maxCompactRectDim]+is_selected[ri];});

			if ( rg.empty() == false )
			{
				printf("backline is hit. breaking out of loop\n");
				break;
			}

			for (int ri=0; ri < n; ri++)
			{
				if (is_selected[ri]==1)
				{
					printf("rectangles[%d] += {.x=%d, .y=%d}\n", ri, translation.x, translation.y);
					rectangles[ri] += translation;
				}
			}
		}
	}

	vector<MyRect> tf = rectangles - input_rectangles;
	auto rg = tf | views::transform([](const MyRect& r){return MyPoint{r.m_left, r.m_top};});
	vector<MyPoint> tf2(n);
	ranges::copy(rg, &tf2[0]);
	return tf2;
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
				petit_poucet[e._j] = vj;

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
	TestFunctionTimer ft("test_compact_frame");

	struct TestContext {int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<MyRect> expected_rectangles; };

	const vector<TestContext> test_contexts={

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

		.expected_rectangles = {
			{.m_left=209, .m_right=411, .m_top=352, .m_bottom=672},
			{.m_left=495, .m_right=641, .m_top=0, .m_bottom=160},
			{.m_left=0, .m_right=188, .m_top=224, .m_bottom=352},
			{.m_left=641, .m_right=843, .m_top=464, .m_bottom=672},
			{.m_left=641, .m_right=829, .m_top=144, .m_bottom=272},
			{.m_left=-14, .m_right=209, .m_top=608, .m_bottom=720},
			{.m_left=21, .m_right=209, .m_top=352, .m_bottom=480},
			{.m_left=188, .m_right=390, .m_top=96, .m_bottom=352},
			{.m_left=641, .m_right=836, .m_top=272, .m_bottom=464},
			{.m_left=411, .m_right=641, .m_top=160, .m_bottom=672}
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

		.expected_rectangles = {
			{.m_left=0, .m_right=10, .m_top=10, .m_bottom=20},
			{.m_left=0, .m_right=10, .m_top=20, .m_bottom=30}
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
	},
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

		.expected_rectangles = {
			{.m_left=20, .m_right=70, .m_top=0, .m_bottom=50},
			{.m_left=0, .m_right=50, .m_top=50, .m_bottom=100},
			{.m_left=50, .m_right=100, .m_top=50, .m_bottom=100},
			{.m_left=20, .m_right=70, .m_top=100, .m_bottom=150},
			{.m_left=120, .m_right=170, .m_top=0, .m_bottom=50},
			{.m_left=100, .m_right=150, .m_top=50, .m_bottom=100}
		}
	}
	};

	for (const auto& [testid, input_rectangles, edges, expected_rectangles] : test_contexts)
	{
		vector<MyRect> rectangles = input_rectangles;
		int n = rectangles.size();

		int dm1 = dim_max(compute_frame(rectangles));

		for (int i=0; i < rectangles.size(); i++)
			rectangles[i].i = i ;
		vector<vector<MPD_Arc> > adjacency_list(n) ;
		for (const Edge& e : edges)
		{
			adjacency_list[e.from].push_back({e.from, e.to}) ;
		}
		vector<MyPoint> cft = compute_compact_frame_transform_(rectangles) ;
		matw(rectangles) += cft;

		vector<int> stress_line[2];
		compute_stress_line(rectangles, stress_line);

		int dm2 = dim_max(compute_frame(rectangles));

		latuile_test_json_output(input_rectangles,
					rectangles,
					edges,
					expected_rectangles,
					"compact_frame",
					testid);

		for (MyRect& r : rectangles)
			r.i=-1;

		bool bOK = rectangles == expected_rectangles;
        	printf("compact_frame testid=%d : %s\n", testid, bOK ? "OK" : "KO");
		printf("dim_max(frame) : %d => %d\n", dm1, dm2);
		(bOK ? nbOK : nbKO)++;
	}
}
