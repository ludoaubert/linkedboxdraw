#include "compact_rectangles.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "index_from.h"
#include "FunctionTimer.h"
#include <vector>
#include <ranges>
#include <algorithm>
#include "latuile_test_json_output.h"
using namespace std ;



bool compact_rectangles(vector<MyRect> &rectangles, const vector<vector<MPD_Arc> >& adjacency_list)
{
        FunctionTimer ft("compact_rectangles");

	auto it = ranges::min_element(rectangles, {}, [&](const MyRect& r){
		return frame_diameter(rectangles - r) ;
		}
	) ;

	MyRect *rr = &*it ;

	int n = rectangles.size() ;
	vector<vector<MyRect*> > unordered_adjacency_list(n) ;
	for (const auto& [i, j] : adjacency_list | views::join)
	{
		if (i == j)
			continue ;
		unordered_adjacency_list[i].push_back(&rectangles[j]) ;
		unordered_adjacency_list[j].push_back(&rectangles[i]) ;
	}

	vector<MyPoint> candidate_translations ;

	for (MyRect* r1 : unordered_adjacency_list[rr->i])
	{
		vector<MyPoint> saillant_points ;
		for (RectCorner rect_corner : RectCorners)
		{
			saillant_points.push_back(corner(*r1, rect_corner)) ;
		}

		for (MyRect& r2 : rectangles)
		{
			if (r2==*r1)
				continue ;
			for (RectCorner rect_corner : RectCorners)
			{
				MyPoint p = corner(r2, rect_corner) ;
				if (is_on_rect_border(*r1,p))
					saillant_points.push_back(p) ;
			}
		}

		for (MyPoint& p : saillant_points)
		{
			for (RectCorner rect_corner : RectCorners)
			{
				MyPoint translation = p - corner(*rr, rect_corner) ;
				candidate_translations.push_back(translation) ;
			}
		}
	}

	int best_frame_diameter = frame_diameter(rectangles) ;
	MyPoint best_translation = {0,0} ;
	MyRect rr_snapshot = *rr ;
	for (MyPoint &translation : candidate_translations)
	{
		*rr = rr_snapshot ;
		translate(*rr, translation) ;
		if (index_from_if(rectangles, [&](const MyRect& r){return r.i != rr->i && intersect_strict(r,*rr);}) != -1)
			continue ;
		int my_frame_diameter = frame_diameter(rectangles) ;
		if (my_frame_diameter < best_frame_diameter)
		{
			best_frame_diameter = my_frame_diameter ;
			best_translation = translation ;
		}
	}

	*rr = rr_snapshot ;
	translate(*rr, best_translation) ;

	return best_translation != MyPoint{0,0} ;
}

void test_compact_rectangles()
{
	TestFunctionTimer ft("test_compact_rectangles");

	struct TestContext{int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<MyRect> expected_rectangles;};

	const vector<TestContext> test_contexts = {

	{
		.testid=1,
		.input_rectangles = {
			{.m_left=202, .m_right=404, .m_top=240, .m_bottom=544}, //role
			{.m_left=42, .m_right=195, .m_top=544, .m_bottom=656}, //role_auto_grant
			{.m_left=195, .m_right=397, .m_top=896, .m_bottom=1024},//ancestor_project
			{.m_left=404, .m_right=599, .m_top=320, .m_bottom=512},//role_operation
			{.m_left=244, .m_right=404, .m_top=96, .m_bottom=240}, //role_path
			{.m_left=63, .m_right=202, .m_top=416, .m_bottom=544}, //role_user
			{.m_left=21, .m_right=195, .m_top=672, .m_bottom=880}, //task_alert_pref
			{.m_left=84, .m_right=244, .m_top=128, .m_bottom=240}, //transition_role
			{.m_left=28, .m_right=195, .m_top=880, .m_bottom=992}, //project_membership
			{.m_left=195, .m_right=404, .m_top=544, .m_bottom=896}, //project
			{.m_left=397, .m_right=592, .m_top=896, .m_bottom=1024},//project_activity
			{.m_left=404, .m_right=634, .m_top=640, .m_bottom=896},//project_membership_request
			{.m_left=404, .m_right=571, .m_top=176, .m_bottom=320},//operation_cluster
			{.m_left=84, .m_right=244, .m_top=0, .m_bottom=128},  //transition
			{.m_left=404, .m_right=620, .m_top=512, .m_bottom=640}//role_default_user
		},

		.edges = {
			{.from=0, .to=9},
			{.from=1, .to=0},
			{.from=1, .to=9},
			{.from=2, .to=9},
			{.from=3, .to=0},
			{.from=3, .to=12},
			{.from=4, .to=0},
			{.from=5, .to=0},
			{.from=5, .to=9},
			{.from=6, .to=0},
			{.from=6, .to=9},
			{.from=7, .to=0},
			{.from=7, .to=13},
			{.from=8, .to=9},
			{.from=9, .to=9},
			{.from=10, .to=9},
			{.from=11, .to=9},
			{.from=12, .to=0},
			{.from=14, .to=0},
			{.from=14, .to=9}
		},

        .expected_rectangles={
			{.m_left=202, .m_right=404, .m_top=240, .m_bottom=544},
			{.m_left=42, .m_right=195, .m_top=544, .m_bottom=656},
			{.m_left=195, .m_right=397, .m_top=896, .m_bottom=1024},
			{.m_left=404, .m_right=599, .m_top=320, .m_bottom=512},
			{.m_left=244, .m_right=404, .m_top=96, .m_bottom=240},
			{.m_left=63, .m_right=202, .m_top=416, .m_bottom=544},
			{.m_left=21, .m_right=195, .m_top=672, .m_bottom=880},
			{.m_left=84, .m_right=244, .m_top=128, .m_bottom=240},
			{.m_left=28, .m_right=195, .m_top=880, .m_bottom=992},
			{.m_left=195, .m_right=404, .m_top=544, .m_bottom=896},
			{.m_left=397, .m_right=592, .m_top=896, .m_bottom=1024},
			{.m_left=404, .m_right=634, .m_top=640, .m_bottom=896},
			{.m_left=404, .m_right=571, .m_top=176, .m_bottom=320},
			{.m_left=42, .m_right=202, .m_top=240, .m_bottom=368},
			{.m_left=404, .m_right=620, .m_top=512, .m_bottom=640}
        }
	},

	{
		.testid=2,

		.input_rectangles = {
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
		},

		.edges = {
			{.from=13,.to=12},
			{.from=13,.to=14},
			{.from=4,.to=7},
			{.from=3,.to=4},
			{.from=1,.to=7},
			{.from=5,.to=7},
			{.from=7,.to=11},
			{.from=6,.to=7},
			{.from=10,.to=7},
			{.from=9,.to=7},
			{.from=2,.to=7},
			{.from=8,.to=7},
			{.from=0,.to=3},
			{.from=12,.to=6}
		},

        .expected_rectangles={
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
		for (const Edge &e : edges)
		{
			adjacency_list[e.from].push_back({e.from, e.to}) ;
		}
		compact_rectangles(rectangles, adjacency_list) ;

		int dm2 = dim_max(compute_frame(rectangles));

		latuile_test_json_output(input_rectangles,
					rectangles,
					edges,
					expected_rectangles,
					"compact_rectangles",
					testid);

		for (MyRect& r : rectangles)
			r.i=-1;

		bool bOK = rectangles == expected_rectangles;
		printf("compact_rectangles testid=%d %s\n", testid, bOK ? "OK" : "KO");
		printf("dim_max(frame) : %d => %d\n", dm1, dm2);
		(bOK ? nbOK : nbKO)++;
	}
}
