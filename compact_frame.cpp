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
	
	struct TestContext {int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<MyRect> expected_rectangles; };

	const TestContext test_contexts[3]={

	{
		.testid=1,
		.input_rectangles = {
			{.left=209, .right=411, .top=352, .bottom=672},//datamart_metric
			{.left=495, .right=641, .top=0, .bottom=160},//preference
			{.left=0, .right=188, .top=224, .bottom=352},//datamart_report_prop
			{.left=641, .right=843, .top=464, .bottom=672},//destinationblacklist
			{.left=641, .right=829, .top=144, .bottom=272},//user_session
			{.left=-14, .right=209, .top=608, .bottom=720},//datamart_metric_parameter
			{.left=21, .right=209, .top=352, .bottom=480},//datamart_metric_prop
			{.left=188, .right=390, .top=96, .bottom=352},//datamart_report
			{.left=641, .right=836, .top=272, .bottom=464},//folder_preference
			{.left=411, .right=641, .top=160, .bottom=672} //sfuser
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
			{.left=209, .right=411, .top=352, .bottom=672},
			{.left=495, .right=641, .top=0, .bottom=160},
			{.left=0, .right=188, .top=224, .bottom=352},
			{.left=641, .right=843, .top=464, .bottom=672},
			{.left=641, .right=829, .top=144, .bottom=272},
			{.left=-14, .right=209, .top=608, .bottom=720},
			{.left=21, .right=209, .top=352, .bottom=480},
			{.left=188, .right=390, .top=96, .bottom=352},
			{.left=641, .right=836, .top=272, .bottom=464},
			{.left=411, .right=641, .top=160, .bottom=672}
		}
	},
	{
		.testid=2,
		.input_rectangles = {
			{.left=0, .right=10, .top=0, .bottom=10},
			{.left=0, .right=10, .top=20, .bottom=30}
		},

		.edges = {
			{.from=0,.to=1}
		},

		.expected_rectangles = {
			{.left=0, .right=10, .top=10, .bottom=20},
			{.left=0, .right=10, .top=20, .bottom=30}
		}
	},
	{
		.testid=3,
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

		for (int i=0; i < rectangles.size(); i++)
			rectangles[i].i = i ;
		vector<vector<MPD_Arc> > adjacency_list(n) ;
		for (const Edge& e : edges)
		{
			adjacency_list[e.from].push_back({e.from, e.to}) ;
		}
		compact_frame(rectangles, adjacency_list) ;

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
		(bOK ? nbOK : nbKO)++;
	}
}
