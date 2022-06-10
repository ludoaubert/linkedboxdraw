#include "optimize_rectangle_positions.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "FunctionTimer.h"
#include <vector>
#include <algorithm>
#include "latuile_test_json_output.h"
using namespace std ;


struct DecisionTreeNode_
{
	int parent_node_index ;
	int16_t left, top ; 	//all we need to know about the moved rectangle
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
        for (MPD_Arc edge : list_edges_(adjacency_list))
        {
             unordered_edges[edge._i].push_back(edge);
             unordered_edges[edge._j].push_back(edge);
        }
	vector<MPD_Arc> edges = list_edges_(adjacency_list) ;

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
				r.m_right = int16_t(p.left + width(r));
				r.m_bottom = int16_t(p.top + height(r));
                                r.m_left = p.left ;
                                r.m_top = p.top;
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
			r.m_right = int16_t(p.left + width(r));
			r.m_bottom = int16_t(p.top + height(r));
                        r.m_left = p.left;
                        r.m_top = p.top;
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
	
	const TestContext text_contexts[]= {
		
	{
		.testid=1,
		.input_rectangles = {
			{.left=369, .right=529, .top=272, .bottom=384},
			{.left=599, .right=780, .top=400, .bottom=544},
			{.left=780, .right=1003, .top=416, .bottom=544},
			{.left=146, .right=369, .top=240, .bottom=544},
			{.left=42, .right=146, .top=240, .bottom=320},
			{.left=42, .right=195, .top=160, .bottom=240},
			{.left=0, .right=146, .top=416, .bottom=544},
			{.left=369, .right=494, .top=160, .bottom=272},
			{.left=369, .right=536, .top=0, .bottom=160},
			{.left=599, .right=759, .top=288, .bottom=400},
			{.left=759, .right=919, .top=224, .bottom=400},
			{.left=369, .right=599, .top=384, .bottom=544},
			{.left=536, .right=710, .top=64, .bottom=160},
			{.left=195, .right=369, .top=112, .bottom=240}
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
			{.left=35, .right=195, .top=128, .bottom=240},
			{.left=599, .right=780, .top=400, .bottom=544},
			{.left=376, .right=599, .top=272, .bottom=400},
			{.left=146, .right=369, .top=240, .bottom=544},
			{.left=42, .right=146, .top=240, .bottom=320},
			{.left=-7, .right=146, .top=320, .bottom=400},
			{.left=0, .right=146, .top=416, .bottom=544},
			{.left=369, .right=494, .top=160, .bottom=272},
			{.left=369, .right=536, .top=0, .bottom=160},
			{.left=599, .right=759, .top=288, .bottom=400},
			{.left=599, .right=759, .top=112, .bottom=288},
			{.left=369, .right=599, .top=400, .bottom=560},
			{.left=536, .right=710, .top=0, .bottom=96},
			{.left=195, .right=369, .top=112, .bottom=240}
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
		
		.expected_rectangles = {
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
