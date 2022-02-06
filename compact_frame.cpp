#include "compact_frame.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "index_from.h"
#include "FunctionTimer.h"
#include <vector>
#include <stack>
#include <cstdint>
#include "latuile_test_json_output.h"
using namespace std ;

/*
snowball effect.
on cree un rectangle = une ligne correspondant a un cote du cadre actuel.
on la fait avancer de facon iterative 1 pixel par 1 pixel. on prend les rectangles r qui l'intersectent. 
Pour chaque r, on prend les rectangles r' qui sont connectés avec r ou qui sont en contact avec r.
r' connectes: si la distance de la connection diminue, on ne prend pas r'.
r' en contact: si le overlap de contact diminue, on prend. si elle augmente, on ne prend pas.
*/



void compact_frame(vector<MyRect>& rectangles, const vector<vector<MPD_Arc> > &adjacency_list)
{
        FunctionTimer ft("compact_frame");

	vector<const MPD_Arc*> edges = list_edges(adjacency_list) ;

	int n = rectangles.size() ;

	vector<vector<int> > unordered_adjacency_list(n) ;
	for (const MPD_Arc* edge : list_edges(adjacency_list))
	{
		int i = edge->_i, j = edge->_j ;
		if (i == j)
			continue ;
		unordered_adjacency_list[i].push_back(j) ;
		unordered_adjacency_list[j].push_back(i) ;
	}

	for (RectDim rect_dim : RectDims)
	{
		MyRect frame = compute_frame(rectangles) ;
		vector<int> index2partition(n,0) ;
		MyRect rake = {-INT16_MAX, INT16_MAX, -INT16_MAX, INT16_MAX} ;
		MyPoint translation = {0,0} ;

		switch(rect_dim)
		{
		case RectDim::LEFT:
			rake.m_right = frame.m_left ;
			translation.x = 1 ;
			break ;
		case RectDim::RIGHT:
			rake.m_left = frame.m_right ;
			translation.x = -1 ;
			break ;
		case RectDim::TOP:
			rake.m_bottom = frame.m_top ;
			translation.y = 1 ;
			break ;
		case RectDim::BOTTOM:
			rake.m_top = frame.m_bottom ;
			translation.y = -1 ;
			break ;
		}

		stack<MyRect> my_stack ;

		for (const MyRect &r : rectangles)
		{
			if (intersect(rake, r))
			{
				my_stack.push(r) ;
			}
		}

		while (!my_stack.empty())
		{
			const MyRect r = my_stack.top() ;
			my_stack.pop() ;
			if (index2partition[r.i] == 1)
				continue ;
			index2partition[r.i] = 1 ;
			for (int j : unordered_adjacency_list[r.i])
			{
				const MyRect &rj = rectangles[j] ;
				if (index2partition[rj.i] == 1)
					continue ;
				if (intersect_strict(translate(r, translation), rj) || rectangle_distance(rj, translate(r, translation)) > rectangle_distance(rj, r))
					my_stack.push(rj) ;
			}

			for (const MyRect& rr : rectangles)
			{
				if (rr.i == r.i)
					continue ;
				if (index2partition[rr.i] == 1)
					continue ;
				if (edge_overlap(r, translate(rr, translation)) < edge_overlap(r, rr))
					my_stack.push(rr) ;
			}
		}//while (!my_stack.empty())

		if (index_from(index2partition, 0)==-1 || index_from(index2partition, 1)==-1)
			continue ;

		bool collision = false ;

		while (collision == false)
		{
			for (MyRect& ri : rectangles)
			{
				 for (const MyRect& rj: rectangles)
				 {
					 if (index2partition[ri.i] < index2partition[rj.i] && intersect_strict(ri, translate(rj, translation)))
						 collision = true ;
				 }
			}

			if (collision)
				continue ;

			float _frame_diameter = frame_diameter(rectangles) ;
			int _total_distance = 0 ;
			for (const MPD_Arc* edge : list_edges(adjacency_list))
				_total_distance += rectangle_distance(rectangles[edge->_i], rectangles[edge->_j]) ;


			for (MyRect& r : rectangles)
			{
				if (index2partition[r.i]==1)
					translate(r, translation) ;
			}

			float frame_diameter_ = frame_diameter(rectangles) ;
			int total_distance_ = 0 ;
			for (const MPD_Arc* edge : list_edges(adjacency_list))
				total_distance_ += rectangle_distance(rectangles[edge->_i], rectangles[edge->_j]) ;

			if (frame_diameter_ >= _frame_diameter && !(total_distance_ < _total_distance))
				break ;
		}
	}
}


void test_compact_frame()
{
	TestFunctionTimer ft("test_compact_frame");

	{
		vector<MyRect> rectangles = {
			{209,411,352,672},//datamart_metric
			{495,641,0,160},//preference
			{0,188,224,352},//datamart_report_prop
			{641,843,464,672},//destinationblacklist
			{641,829,144,272},//user_session
			{-14,209,608,720},//datamart_metric_parameter
			{21,209,352,480},//datamart_metric_prop
			{188,390,96,352},//datamart_report
			{641,836,272,464},//folder_preference
			{411,641,160,672} //sfuser
		};

		const int edges[11][2]={
			{0,7},
			{0,9},
			{1,9},
			{2,7},
			{3,9},
			{4,9},
			{5,0},
			{6,0},
			{7,9},
			{8,9},
			{9,9}
		};

		const vector<MyRect> expected_rectangles = {
			{209,411,352,672},
			{495,641,0,160},
			{0,188,224,352},
			{641,843,464,672},
			{641,829,144,272},
			{-14,209,608,720},
			{21,209,352,480},
			{188,390,96,352},
			{641,836,272,464},
			{411,641,160,672}
		};

		latuile_test_json_output(rectangles,
					//edges,
					expected_rectangles,
					"compact_frame",
					1);

		for (int i=0; i < rectangles.size(); i++)
			rectangles[i].i = i ;
		vector<vector<MPD_Arc> > adjacency_list(10) ;
		for (const int (&edge)[2] : edges)
		{
			int i = edge[0], j = edge[1] ;
			adjacency_list[i].push_back(MPD_Arc{i,j}) ;
		}
		compact_frame(rectangles, adjacency_list) ;

		for (MyRect& r : rectangles)
			r.i=-1;

        	printf("%s\n", rectangles == expected_rectangles ? "OK" : "KO");
	}

	{
		vector<MyRect> rectangles = {
			{0, 10, 0, 10},
			{0, 10, 20, 30}
		} ;

		const int edges[1][2]={
			{0,1}
		} ;

		const vector<MyRect> expected_rectangles = {
			{0,10,10,20},
			{0,10,20,30}
		};

                latuile_test_json_output(rectangles,
					//edges,
					expected_rectangles,
					"compact_frame",
					2);

		for (int i=0; i < rectangles.size(); i++)
			rectangles[i].i = i ;
		vector<vector<MPD_Arc> > adjacency_list(2) ;
		for (const int (&edge)[2] : edges)
		{
			int i = edge[0], j = edge[1] ;
			adjacency_list[i].push_back(MPD_Arc{i,j}) ;
		}
		compact_frame(rectangles, adjacency_list) ;

		for (MyRect& r : rectangles)
			r.i=-1;

        	printf("%s\n", rectangles == expected_rectangles ? "OK" : "KO");
	}
}
