#include "compact_rectangles.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "index_from.h"
#include "FunctionTimer.h"
#include <vector>
#include <algorithm>
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

	vector<MyRect> rectangles = {
		{202,404,240,544}, //role
		{42,195,544,656}, //role_auto_grant
		{195,397,896,1024},//ancestor_project
		{404,599,320,512},//role_operation
		{244,404,96,240}, //role_path
		{63,202,416,544}, //role_user
		{21,195,672,880}, //task_alert_pref
		{84,244,128,240}, //transition_role
		{28,195,880,992}, //project_membership
		{195,404,544,896}, //project
		{397,592,896,1024},//project_activity
		{404,634,640,896},//project_membership_request
		{404,571,176,320},//operation_cluster
		{84,244,0,128},  //transition
		{404,620,512,640}//role_default_user
	};
	int edges[20][2]={
		{0,9},
		{1,0},
		{1,9},
		{2,9},
		{3,0},
		{3,12},
		{4,0},
		{5,0},
		{5,9},
		{6,0},
		{6,9},
		{7,0},
		{7,13},
		{8,9},
		{9,9},
		{10,9},
		{11,9},
		{12,0},
		{14,0},
		{14,9}
	};

        const vector<MyRect> expected_rectangles={
		{202,404,240,544},
		{42,195,544,656},
		{195,397,896,1024},
		{404,599,320,512},
		{244,404,96,240},
		{63,202,416,544},
		{21,195,672,880},
		{84,244,128,240},
		{28,195,880,992},
		{195,404,544,896},
		{397,592,896,1024},
		{404,634,640,896},
		{404,571,176,320},
		{42,202,240,368},
		{404,620,512,640}
        };

	for (int i=0; i < rectangles.size(); i++)
		rectangles[i].i = i ;
	vector<vector<MPD_Arc> > adjacency_list(15) ;
	for (int (&edge)[2] : edges)
	{
		int i = edge[0], j = edge[1] ;
		adjacency_list[i].push_back(MPD_Arc{i,j}) ;
	}
	compact_rectangles(rectangles, adjacency_list) ;

	for (MyRect& r : rectangles)
		r.i=-1;

        printf("%s\n", rectangles == expected_rectangles ? "OK" : "KO");
}
