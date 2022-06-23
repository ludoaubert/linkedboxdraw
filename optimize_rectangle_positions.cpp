#include "optimize_rectangle_positions.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "FunctionTimer.h"
#include <vector>
#include <ranges>
#include <algorithm>
#include "latuile_test_json_output.h"
using namespace std ;


struct DecisionTreeNode_
{
	int parent_node_index ;
	int16_t m_left, m_top ; 	//all we need to know about the moved rectangle
	int8_t i ;		//all we need to know about the moved rectangle
	int16_t diameter ;
	int16_t distance ;
	int16_t intersection_penalty ;
} ;


vector<MyPoint> generate_candidate_translations(const vector<MyRect> &rectangles,

                const MyRect& r1)
{
        vector<MyPoint> candidate_translations ;

        for (const MyRect& r : rectangles)
        {
                if (r.i == r1.i)
                        continue ;

                for (RectCorner rc1 : RectCorners)
                {
                        for (RectCorner rc2 : RectCorners)
                        {
                                MyPoint translation = corner(r, rc2) - corner(r1, rc1) ;
                                if (translation == MyPoint{0,0})
                                        continue ;
                                candidate_translations.push_back(translation) ;
                        }
                }
        }

        return candidate_translations ;
}



int measure(const vector<MyRect> &rectangles, const vector<MPD_Arc>& edges, int& diameter, int& distance, int& intersection_penalty)
{
	diameter = frame_diameter_(rectangles) ;

	distance = 0 ;

	for (const MPD_Arc& edge : edges)
	{
		distance += rectangle_distance(rectangles[edge._i], rectangles[edge._j]) ;
	}

	intersection_penalty = 0 ;

	for (const MyRect &r1 : rectangles)
	{
		for (int i=r1.i+1; i < rectangles.size(); i++)
		{
			const MyRect &r2 = rectangles[i];
			if (intersect_strict(r1,r2))
				intersection_penalty += 1 + rectangle_intersection_dimension(r1, r2) ;
		}
	}

	return diameter + distance + intersection_penalty ;
}



void generate_candidate_nodes(int parent_node_index,
				const vector<MyRect> &rectangles,
				MyRect r,
				const vector<vector<MPD_Arc> >& unordered_edges,
				int diameter_,
				int distance_,
				int intersection_penalty_,
				vector<DecisionTreeNode_>& decision_tree,
				vector<int16_t>& measures,
				vector<int>& priority_queue)
{
/*TODO LATER: sort candidates by measure and keep only the 10 best results. */

        MyRect frame = {+INT16_MAX,-INT16_MAX,+INT16_MAX,-INT16_MAX};

        for (const MyRect& rec : rectangles)
        {
                if (rec.i == r.i)
                    continue;
                frame.m_left = min(frame.m_left, rec.m_left) ;
                frame.m_right = max(frame.m_right, rec.m_right) ;
                frame.m_top = min(frame.m_top, rec.m_top) ;
                frame.m_bottom = max(frame.m_bottom, rec.m_bottom) ;
        }

	for (MyPoint& translation : generate_candidate_translations(rectangles, r))
	{
		int16_t _diameter = diameter_ ;
		int16_t _distance = distance_ ;
		int16_t _intersection_penalty = intersection_penalty_ ;

		MyRect rr = r;
		translate(rr, translation) ;

		_diameter -= rectangle_diameter_(enveloppe(frame, r)) ;
		_diameter += rectangle_diameter_(enveloppe(frame, rr)) ;

		for (const MPD_Arc& edge : unordered_edges[r.i])
		{
                        int i=edge._i, j=edge._j;
			_distance -= rectangle_distance(i==r.i ? r : rectangles[i], j==r.i ? r : rectangles[j]) ;
			_distance += rectangle_distance(i==rr.i ? rr : rectangles[i], j==rr.i ? rr : rectangles[j]) ;
		}

		for (int i=0; i < rectangles.size(); i++)
		{
			if (i != r.i)
			{
				if (intersect_strict(rectangles[i],r))
					_intersection_penalty -= 1 + rectangle_intersection_dimension(rectangles[i], r) ;
				if (intersect_strict(rectangles[i],rr))
					_intersection_penalty += 1 + rectangle_intersection_dimension(rectangles[i], rr) ;
			}
		}

		int node_index = decision_tree.size();
                decision_tree.push_back(DecisionTreeNode_{parent_node_index, rr.m_left, rr.m_top, (int8_t)rr.i, _diameter, _distance, _intersection_penalty}) ;
		measures.push_back(_diameter + _distance + _intersection_penalty) ;
		priority_queue.push_back(node_index) ;
		ranges::push_heap(priority_queue, ranges::greater(), [&](int i){return measures[i];});
	}
}

void optimize_rectangle_positions(vector<MyRect> &rectangles, const vector<vector<MPD_Arc> >& adjacency_list)
{
        FunctionTimer ft("optimize_rectangle_positions");

        int n=rectangles.size();
        vector<vector<MPD_Arc> > unordered_edges(n);
        for (const auto& [i, j] : adjacency_list | views::join)
        {
             unordered_edges[i].push_back({i, j});
             unordered_edges[j].push_back({i, j});
        }
	auto r = adjacency_list | views::join ;
	vector<MPD_Arc> edges(begin(r), end(r));

	while (true)
	{
		MyRect& r = *ranges::min_element(rectangles, {}, [&](MyRect& r){return frame_dim_max(rectangles-r);}) ;
		vector<DecisionTreeNode_> decision_tree ;
		vector<int16_t> measures;
		vector<int> priority_queue ;
		int parent_node_index = -1 ;
		int diameter_, distance_, intersection_penalty_ ;
		measure(rectangles, edges, diameter_, distance_, intersection_penalty_) ;
		generate_candidate_nodes(parent_node_index,
					rectangles,
					r,
					unordered_edges,
					diameter_,
					distance_,
					intersection_penalty_,
					decision_tree,
					measures,
					priority_queue) ;

		vector<MyRect> rectangles_ = rectangles ;
                vector<bool> translated_ancestors(n) ;

		int loop_counter = 0 ;

		const unsigned int MAX_TREE_DEPTH = 7 ;
		const unsigned int MAX_LOOP_COUNT = 10 * 1024 ;

		while (!priority_queue.empty() && loop_counter < MAX_LOOP_COUNT)
		{
			loop_counter++ ;

			ranges::pop_heap(priority_queue, ranges::greater(), [&](int i){return measures[i];});
			int node_index = priority_queue.back();
			DecisionTreeNode_ n = decision_tree[node_index];
			priority_queue.pop_back() ;

		//look for intersections: 2 rectangles that intersect. retrieve the list of translated rectangles from the ancestors

                        rectangles_ = rectangles ;
                        std::fill(translated_ancestors.begin(), translated_ancestors.end(), false) ;
			int depth = 0;

                        for (int i=node_index; i!=-1; i=decision_tree[i].parent_node_index)
			{
				depth++;
				DecisionTreeNode_ &p = decision_tree[i] ;
				translated_ancestors[p.i] = true ;
				MyRect& r = rectangles_[p.i];
				r.m_right = int16_t(p.m_left + width(r));
				r.m_bottom = int16_t(p.m_top + height(r));
                                r.m_left = p.m_left ;
                                r.m_top = p.m_top;
			}

			if (depth == MAX_TREE_DEPTH)
				continue ;

//look for intersections and allow intersecting rectangles to move

                	vector<MyRect> intersecting_rectangles;
			for (MyRect &r1 : rectangles_)
			{
				for (int i=r1.i+1; i<rectangles_.size(); i++)
				{
					MyRect &r2 = rectangles_[i];

					if (!intersect_strict(r1, r2))
						continue ;
					for (MyRect *r : {&r1,&r2})
					{
						if (translated_ancestors[r->i])
                                        		continue ;
						intersecting_rectangles.push_back(*r);
						translated_ancestors[r->i] = true;
					}
				}
			}

			for (MyRect& r : intersecting_rectangles)
			{
				generate_candidate_nodes(node_index, rectangles_, r, unordered_edges, n.diameter, n.distance, n.intersection_penalty, decision_tree, measures, priority_queue) ;
			}
		}


		DecisionTreeNode_ &n = *ranges::min_element(
			decision_tree,
			{},
			[&](DecisionTreeNode_& n){
				int i = distance(&decision_tree[0], &n);
				return measures[i] + n.intersection_penalty*1000;
			}
		) ;

		int node_index = distance(&decision_tree[0], &n);

		rectangles_ = rectangles;

                for (int i=node_index; i!=-1; i=decision_tree[i].parent_node_index)
		{
			DecisionTreeNode_ &p= decision_tree[i];
			MyRect& r = rectangles_[p.i];
			r.m_right = int16_t(p.m_left + width(r));
			r.m_bottom = int16_t(p.m_top + height(r));
                        r.m_left = p.m_left;
                        r.m_top = p.m_top;
		}

		measure(rectangles, edges, diameter_, distance_, intersection_penalty_) ;
		int measure_ = diameter_ + distance_ + intersection_penalty_ ;

		if (measures[node_index] < measure_ && !detect_collision(rectangles_) && frame_diameter_(rectangles_) < frame_diameter_(rectangles))
		{
			rectangles = rectangles_ ;
		}
		else
		{
			break ;
		}
	}
}


void test_optimize_rectangle_positions()
{
	TestFunctionTimer ft("test_optimize_rectangle_positions");

	struct TestContext {int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<MyRect> expected_rectangles;};

	const vector<TestContext> test_contexts = {

	{
		.testid=1,
		.input_rectangles = {
			{.m_left=369, .m_right=529, .m_top=272, .m_bottom=384},
			{.m_left=599, .m_right=780, .m_top=400, .m_bottom=544},
			{.m_left=780, .m_right=1003, .m_top=416, .m_bottom=544},
			{.m_left=146, .m_right=369, .m_top=240, .m_bottom=544},
			{.m_left=42, .m_right=146, .m_top=240, .m_bottom=320},
			{.m_left=42, .m_right=195, .m_top=160, .m_bottom=240},
			{.m_left=0, .m_right=146, .m_top=416, .m_bottom=544},
			{.m_left=369, .m_right=494, .m_top=160, .m_bottom=272},
			{.m_left=369, .m_right=536, .m_top=0, .m_bottom=160},
			{.m_left=599, .m_right=759, .m_top=288, .m_bottom=400},
			{.m_left=759, .m_right=919, .m_top=224, .m_bottom=400},
			{.m_left=369, .m_right=599, .m_top=384, .m_bottom=544},
			{.m_left=536, .m_right=710, .m_top=64, .m_bottom=160},
			{.m_left=195, .m_right=369, .m_top=112, .m_bottom=240}
		},
		.edges={
			{.from=2, .to=1},
			{.from=3, .to=0},
			{.from=3, .to=3},
			{.from=3, .to=4},
			{.from=3, .to=5},
			{.from=3, .to=6},
			{.from=3, .to=7},
			{.from=3, .to=11},
			{.from=3, .to=13},
			{.from=8, .to=7},
			{.from=8, .to=12},
			{.from=8, .to=13},
			{.from=9, .to=11},
			{.from=10, .to=9},
			{.from=11, .to=1}
		},

        	.expected_rectangles={
			{.m_left=35, .m_right=195, .m_top=128, .m_bottom=240},
			{.m_left=599, .m_right=780, .m_top=400, .m_bottom=544},
			{.m_left=376, .m_right=599, .m_top=272, .m_bottom=400},
			{.m_left=146, .m_right=369, .m_top=240, .m_bottom=544},
			{.m_left=42, .m_right=146, .m_top=240, .m_bottom=320},
			{.m_left=-7, .m_right=146, .m_top=320, .m_bottom=400},
			{.m_left=0, .m_right=146, .m_top=416, .m_bottom=544},
			{.m_left=369, .m_right=494, .m_top=160, .m_bottom=272},
			{.m_left=369, .m_right=536, .m_top=0, .m_bottom=160},
			{.m_left=599, .m_right=759, .m_top=288, .m_bottom=400},
			{.m_left=599, .m_right=759, .m_top=112, .m_bottom=288},
			{.m_left=369, .m_right=599, .m_top=400, .m_bottom=560},
			{.m_left=536, .m_right=710, .m_top=0, .m_bottom=96},
			{.m_left=195, .m_right=369, .m_top=112, .m_bottom=240}
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
		for (const Edge e : edges)
		{
			adjacency_list[e.from].push_back({e.from, e.to}) ;
		}

		optimize_rectangle_positions(rectangles, adjacency_list) ;

		int dm2 = dim_max(compute_frame(rectangles));

		latuile_test_json_output(input_rectangles,
					rectangles,
					edges,
					expected_rectangles,
					"optimize_rectangle_positions",
					testid);

		for (MyRect &r : rectangles)
			r.i = -1 ;

		bool bOK = rectangles == expected_rectangles;
		printf("optimize_rectangle_positions testid=%d %s\n", testid, bOK ? "OK" : "KO");
		printf("dim_max(frame) : %d => %d\n", dm1, dm2);
		(bOK ? nbOK : nbKO)++;
	}
}
