#include "compact_frame.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "index_from.h"
#include "FunctionTimer.h"
#include <vector>
#include <ranges>
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

	int n = rectangles.size() ;

	vector<vector<int> > unordered_adjacency_list(n) ;
	for (const auto& [i, j] : adjacency_list | views::join)
	{
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
			printf("RectDim::LEFT\n");
			rake.m_right = frame.m_left ;
			translation.x = 1 ;
			break ;
		case RectDim::RIGHT:
			printf("RectDim::RIGHT\n");
			rake.m_left = frame.m_right ;
			translation.x = -1 ;
			break ;
		case RectDim::TOP:
			printf("RectDim::TOP\n");
			rake.m_bottom = frame.m_top ;
			translation.y = 1 ;
			break ;
		case RectDim::BOTTOM:
			printf("RectDim::BOTTOM\n");
			rake.m_top = frame.m_bottom ;
			translation.y = -1 ;
			break ;
		}
		
		auto [m_left, m_right, m_top, m_bottom] = rake;
		printf("rake=[%d, %d, %d, %d]\n", m_left, m_right, m_top, m_bottom);
		auto [x, y] = translation;
		printf("translation=[%d, %d]\n", x, y);

		stack<MyRect> my_stack ;

		for (const MyRect &r : rectangles)
		{
			if (intersect(rake, r))
			{
				my_stack.push(r) ;
				printf("my_stack.push(r.i=%d) because intersect(rake, r)\n", r.i);
			}
		}

		while (!my_stack.empty())
		{
			const MyRect r = my_stack.top() ;
			my_stack.pop() ;
			printf("my_stack.pop(r.i=%d)\n", r.i);
			if (index2partition[r.i] == 1)
				continue ;
			index2partition[r.i] = 1 ;
			printf("setting index2partition[%d]=1\n", r.i);
			for (int j : unordered_adjacency_list[r.i])
			{
				const MyRect &rj = rectangles[j] ;
				if (index2partition[rj.i] == 1)
					continue ;
				if (intersect_strict(translate(r, translation), rj) || rectangle_distance(rj, translate(r, translation)) > rectangle_distance(rj, r))
				{
					my_stack.push(rj) ;
					printf("my_stack.push(rj.i=%d) because intersect_strict()\n", rj.i);
				}
			}

			for (const MyRect& rr : rectangles)
			{
				if (rr.i == r.i)
					continue ;
				if (index2partition[rr.i] == 1)
					continue ;
				if (edge_overlap(r, translate(rr, translation)) < edge_overlap(r, rr))
				{
					my_stack.push(rr) ;
					printf("my_stack.push(rr.i=%d) because edge_overlap()\n", rr.i);
				}
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
			for (const auto& [i, j] : adjacency_list | views::join)
				_total_distance += rectangle_distance(rectangles[i], rectangles[j]) ;


			for (MyRect& r : rectangles)
			{
				if (index2partition[r.i]==1)
				{
					const auto& [m_left, m_right, m_top, m_bottom] = r ;
					const auto& [x, y] = translation;
					printf("translate [%d, %d, %d, %d] by [%d, %d]\n", m_left, m_right, m_top, m_bottom, x, y);
					translate(r, translation) ;
				}
			}

			float frame_diameter_ = frame_diameter(rectangles) ;
			int total_distance_ = 0 ;
			for (const auto& [i, j] : adjacency_list | views::join)
				total_distance_ += rectangle_distance(rectangles[i], rectangles[j]) ;
			
			printf("_frame_diameter=%d (before translation)\n", _frame_diameter);
			printf("frame_diameter_=%d (after translation)\n", frame_diameter_);
			
			printf("_total_distance=%d (before translation)\n", _total_distance);
			printf("total_distance_=%d (after translation)\n", total_distance_);


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
