#include "compact_frame.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "FunctionTimer.h"
#include <vector>
#include <ranges>
#include <cstdint>
#include <assert.h>
#include "latuile_test_json_output.h"
using namespace std ;


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

		vector<MPD_Arc> contacts;

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

			contacts.clear();

			for (MyRect& r : rectangles)
				r.selected = false;

			//rectangles that we want to rake along
			for (MyRect& r : rectangles | views::filter([&](const MyRect& r){return intersect(rake, r);}))
			{
				r.selected = true;
				contacts.push_back({-INT16_MAX, r.i});
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
							contacts.push_back({r.i, rc.i});
							stop = false;
							translate(rc, translation);
						}
					}
				}
			}

			//rectangles that hit the baseline
			auto rg = rectangles | views::filter([&](const MyRect& r){return intersect_strict(baseline, r);});
			for (MyRect& r : rg)
			{
				contacts.push_back({r.i, INT16_MAX});
			}

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

		for (auto [_i, _j] : contacts)
		{
			printf("contact trace: {i=%d, j=%d}\n", _i, _j);
		}

		vector<int> stress_line;

// As explained here : https://www.geeksforgeeks.org/recursive-lambda-expressions-in-cpp/

		auto list_stress_line = [&](int target, auto&& list_stress_line) {
			if (target == -INT16_MAX)
				return;
			if (target != INT16_MAX)
				stress_line.push_back(target);
			for (int i : contacts | views::filter([&](const MPD_Arc& e){return e._j == target;}) | views::transform(&MPD_Arc::_i))
				list_stress_line(i, list_stress_line);
		};

		// Function as an argument
		list_stress_line(INT16_MAX, list_stress_line);

		printf("pressure line:\n");
		for (int i: stress_line)
		{
			printf("%d\n", i);
		}
	}

}



void compute_stress_line(const vector<MyRect>& rectangles, vector<int> (&stress_line)[2])
{
	MyRect frame = compute_frame(rectangles);
	
	for (Direction direction : directions)
	{
		const auto [minRectDim, maxRectDim] = rectDimRanges[direction];
		
		vector<MPD_Arc> contacts;
		
	//rectangles that hit the baseline
		for (const MyRect& r : rectangles | views::filter([&](const MyRect& r){return r[minRectDim] == frame[minRectDim];}))
		{
			contacts.push_back({-INT16_MAX, r.i});
		}

		for (const MyRect& r1 : rectangles)
		{
			for (const MyRect& r2 : rectangles)
			{
				if (r1.i != r2.i && edge_overlap(r1, r2, directions[1-direction]))
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

// As explained here : https://www.geeksforgeeks.org/recursive-lambda-expressions-in-cpp/

		auto list_stress_line = [&](int target, auto&& list_stress_line) {
			if (target == -INT16_MAX)
				return;
			if (target != INT16_MAX)
				stress_line[direction].push_back(target);
			for (int i : contacts | views::filter([&](const MPD_Arc& e){return e._j == target;}) | views::transform(&MPD_Arc::_i))
				list_stress_line(i, list_stress_line);
		};

		// Function as an argument
		list_stress_line(INT16_MAX, list_stress_line);

	}
	
	for (Direction direction : directions)
	{
		printf("pressure line:\n");
		for (int i: stress_line[direction])
		{
			printf("%d\n", i);
		}
	}
}



void test_compact_frame()
{
	TestFunctionTimer ft("test_compact_frame");

	struct TestContext {int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<MyRect> expected_rectangles; };

	const TestContext test_contexts[3]={

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
		compact_frame(rectangles, adjacency_list) ;
		
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
