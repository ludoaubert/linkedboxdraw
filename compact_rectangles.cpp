#include "compact_rectangles.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "index_from.h"
#include "FunctionTimer.h"
#include <vector>
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
	for (const MPD_Arc *edge : list_edges(adjacency_list))
	{
		if (edge->_i == edge->_j)
			continue ;
		unordered_adjacency_list[edge->_i].push_back(&rectangles[edge->_j]) ;
		unordered_adjacency_list[edge->_j].push_back(&rectangles[edge->_i]) ;
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
	
	const TestContext test_contexts = {

	{
		testid=1,
		.input_rectangles = {
			{.left=202, .right=404, .top=240, .bottom=544}, //role
			{.left=42, .right=195, .top=544, .bottom=656}, //role_auto_grant
			{.left=195, .right=397, .top=896, .bottom=1024},//ancestor_project
			{.left=404, .right=599, .top=320, .bottom=512},//role_operation
			{.left=244, .right=404, .top=96, .bottom=240}, //role_path
			{.left=63, .right=202, .top=416, .bottom=544}, //role_user
			{.left=21, .right=195, .top=672, .bottom=880}, //task_alert_pref
			{.left=84, .right=244, .top=128, .bottom=240}, //transition_role
			{.left=28, .right=195, .top=880, .bottom=992}, //project_membership
			{.left=195, .right=404, .top=544, .bottom=896}, //project
			{.left=397, .right=592, .top=896, .bottom=1024},//project_activity
			{.left=404, .right=634, .top=640, .bottom=896},//project_membership_request
			{.left=404, .right=571, .top=176, .bottom=320},//operation_cluster
			{.left=84, .right=244, .top=0, .bottom=128},  //transition
			{.left=404, .right=620, .top=512, .bottom=640}//role_default_user
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
			{.left=202, .right=404, .top=240, .bottom=544},
			{.left=42, .right=195, .top=544, .bottom=656},
			{.left=195, .right=397, .top=896, .bottom=1024},
			{.left=404, .right=599, .top=320, .bottom=512},
			{.left=244, .right=404, .top=96, .bottom=240},
			{.left=63, .right=202, .top=416, .bottom=544},
			{.left=21, .right=195, .top=672, .bottom=880},
			{.left=84, .right=244, .top=128, .bottom=240},
			{.left=28, .right=195, .top=880, .bottom=992},
			{.left=195, .right=404, .top=544, .bottom=896},
			{.left=397, .right=592, .top=896, .bottom=1024},
			{.left=404, .right=634, .top=640, .bottom=896},
			{.left=404, .right=571, .top=176, .bottom=320},
			{.left=42, .right=202, .top=240, .bottom=368},
			{.left=404, .right=620, .top=512, .bottom=640}
        }
	},
	
	{
		.testid=2,
		
		.input_rectangles = {
			{.left=396,.right=396+162,.top=10,.bottom=10+104},//8
			{.left=320,.right=320+182,.top=330,.bottom=330+72},//9
			{.left=453,.right=453+105,.top=218,.bottom=218+72},//10
			{.left=598,.right=598+126,.top=10,.bottom=10+152},//21
			{.left=598,.right=598+126,.top=202,.bottom=202+88},//24
			{.left=750,.right=750+147,.top=346,.bottom=346+120},//25
			{.left=273,.right=273+140,.top=154,.bottom=154+120},//26
			{.left=542,.right=542+168,.top=330,.bottom=330+136},//27
			{.left=335,.right=335+168,.top=506,.bottom=506+120},//28
			{.left=556,.right=556+147,.top=506,.bottom=506+104},//30
			{.left=764,.right=764+133,.top=186,.bottom=186+120},//32
			{.left=743,.right=743+147,.top=506,.bottom=506+168},//44
			{.left=93,.right=93+140,.top=153,.bottom=153+88},//48
			{.left=10,.right=10+155,.top=281,.bottom=281+120},//52
			{.left=11,.right=11+175,.top=441,.bottom=441+136}//53			
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
			{.left=396,.right=396+162,.top=10,.bottom=10+104},//8
			{.left=320,.right=320+182,.top=330,.bottom=330+72},//9
			{.left=453,.right=453+105,.top=218,.bottom=218+72},//10
			{.left=598,.right=598+126,.top=10,.bottom=10+152},//21
			{.left=598,.right=598+126,.top=202,.bottom=202+88},//24
			{.left=750,.right=750+147,.top=346,.bottom=346+120},//25
			{.left=273,.right=273+140,.top=154,.bottom=154+120},//26
			{.left=542,.right=542+168,.top=330,.bottom=330+136},//27
			{.left=335,.right=335+168,.top=506,.bottom=506+120},//28
			{.left=556,.right=556+147,.top=506,.bottom=506+104},//30
			{.left=764,.right=764+133,.top=186,.bottom=186+120},//32
			{.left=743,.right=743+147,.top=506,.bottom=506+168},//44
			{.left=93,.right=93+140,.top=153,.bottom=153+88},//48
			{.left=10,.right=10+155,.top=281,.bottom=281+120},//52
			{.left=11,.right=11+175,.top=441,.bottom=441+136}//53
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
