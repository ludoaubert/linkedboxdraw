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

	const vector<MyRect> input_rectangles = {
		{369,529,272,384},
		{599,780,400,544},
		{780,1003,416,544},
		{146,369,240,544},
		{42,146,240,320},
		{42,195,160,240},
		{0,146,416,544},
		{369,494,160,272},
		{369,536,0,160},
		{599,759,288,400},
		{759,919,224,400},
		{369,599,384,544},
		{536,710,64,160},
		{195,369,112,240}
	};

	const vector<Edge> edges={
		{2,1},
		{3,0},
		{3,3},
		{3,4},
		{3,5},
		{3,6},
		{3,7},
		{3,11},
		{3,13},
		{8,7},
		{8,12},
		{8,13},
		{9,11},
		{10,9},
		{11,1}
	};

        const vector<MyRect> expected_rectangles={
		{35,195,128,240},
		{599,780,400,544},
		{376,599,272,400},
		{146,369,240,544},
		{42,146,240,320},
		{-7,146,320,400},
		{0,146,416,544},
		{369,494,160,272},
		{369,536,0,160},
		{599,759,288,400},
		{599,759,112,288},
		{369,599,400,560},
		{536,710,0,96},
		{195,369,112,240}
        };

	vector<MyRect> rectangles = input_rectangles;

	for (int i=0; i < rectangles.size(); i++)
		rectangles[i].i = i ;
	vector<vector<MPD_Arc> > adjacency_list(14) ;
	for (const Edge e : edges)
	{
		adjacency_list[e.from].push_back({e.from, e.to}) ;
	}

	optimize_rectangle_positions(rectangles, adjacency_list) ;

	latuile_test_json_output(input_rectangles,
				rectangles,
				edges,
				expected_rectangles,
				"optimize_rectangle_positions",
				1);

	for (MyRect &r : rectangles)
		r.i = -1 ;

	bool bOK = rectangles == expected_rectangles;
        printf("%s\n", bOK ? "OK" : "KO");
	(bOK ? nbOK : nbKO)++;
}
