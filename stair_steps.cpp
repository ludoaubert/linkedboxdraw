#include "stair_steps.h"
#include "MyRect.h"
#include "index_from.h"
#include "MPD_Arc.h"
#include "WidgetContext.h"
#include "swap_rectangles.h"
#include "compact_rectangles.h"
#include "compact_frame.h"
#include "binpack.h"
#include "optimize_rectangle_positions.h"
#include "fit_together.h"
#include "permutation.h"
#include "KMeansRexCore.h"
#include "FunctionTimer.h"
#include <vector>
#include <stack>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <ranges>
#include <numeric>
#include <random>
#include <fstream>
#include <assert.h>
#include <chrono>
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include "latuile_test_json_output.h"
//#include <omp.h>
using namespace std ;
using namespace std::chrono;
using namespace Eigen ;

typedef Matrix<WidgetContext,Dynamic,Dynamic> MatrixXw ;
typedef Matrix<MyRect,Dynamic,Dynamic> MatrixXr ;

//implemented in this file. called in this file before the implementation.
bool minimum_cut(const MatrixXd& W,
                 PermutationMatrix<Dynamic>& perm2,
                 vector<int> &component_distribution);

vector<vector<MPD_Arc> > compute_adjacency_list_(const Matrix<int8_t,-1,-1>& OW) ;
vector<vector<MPD_Arc> > compute_adjacency_list(const MatrixXd& OW) ;
void connected_components(const vector<vector<MPD_Arc> >& adjacency_list,
                          vector<int>& connected_component) ;


bool stair_steps(vector<MyRect> &rectangles, MyRect& rr, vector<vector<MPD_Arc> > &adjacency_list)
{
        FunctionTimer ft("stair_steps");
	int n = rectangles.size() ;

	vector<vector<MyRect*> > unordered_adjacency_list(n) ;
	for (const auto& [i, j] : adjacency_list | views::join)
	{
		if (i == j)
			continue ;
		unordered_adjacency_list[i].push_back(&rectangles[j]) ;
		unordered_adjacency_list[j].push_back(&rectangles[i]) ;
	}
//ORDER each adjacency list BY rectangle width DESC. This is to make sure that lower steps of the stairway are larger.
	for (vector<MyRect*>& adj : unordered_adjacency_list)
		ranges::sort(adj, ranges::greater(), [&](MyRect* r){return width(*r);}) ;

	for (MyRect& r : rectangles)
		r.selected = false ;
	rr.selected = true ;

	stack<MyRect*> pending_stack ;
	pending_stack.push(&rr) ;


	struct NormalDirection {Direction direction;	Sens sens;	} ;
	const NormalDirection normal4[4] = {
		{EAST_WEST, INCREASE},
		{NORTH_SOUTH, DECREASE},
		{EAST_WEST, DECREASE},
		{NORTH_SOUTH, INCREASE}
	} ;


	while (!pending_stack.empty())
	{
		MyRect *rr = pending_stack.top() ;
		pending_stack.pop() ;

		const NormalDirection *normal = &normal4[0] ;
		MyRect *prec = 0 ;
		for (MyRect* r : unordered_adjacency_list[rr->i])
		{
			if (r->selected)
				continue ;

			pending_stack.push(r) ;

			translate(*r, {rr->m_left - r->m_left, rr->m_top - r->m_top}) ;

			const NormalDirection * next_normal = normal+1 == normal4+4 ? normal4 : normal+1 ;

			if (prec == 0)
			{
				MyPoint translation ;
				value(translation, normal->direction) = value(*rr, normal->direction, normal->sens) -
															value(*r, normal->direction, reverse(normal->sens)) ;
				value(translation, transpose(normal->direction)) = value(*rr, transpose(normal->direction), normal->sens) -
															value(*r, transpose(normal->direction), normal->sens) ;
				translate(*r, translation) ;
			}
			else if (value(*rr, next_normal->direction, next_normal->sens)*next_normal->sens >
				value(*prec, next_normal->direction, next_normal->sens)*next_normal->sens)
			{
			//there is room for another step
				MyPoint translation ;
				value(translation, normal->direction) = value(*prec, normal->direction, reverse(normal->sens)) -
															value(*r, normal->direction, reverse(normal->sens)) ;
				value(translation, next_normal->direction) = value(*prec, next_normal->direction, next_normal->sens) -
															value(*r, next_normal->direction, reverse(next_normal->sens)) ;
				translate(*r, translation) ;
			}
			else
			{
			//no more room for another step. start using the next side
				MyPoint translation ;
			//use the yet to be replaced normal to compute the transaction coordinate in this direction
				value(translation, normal->direction) = value(*rr, normal->direction, normal->sens) -
															value(*r, normal->direction, normal->sens) ;
				normal++ ;
				if (distance(normal4, normal) >= 4)
					break ;

				prec = 0 ;
				value(translation, normal->direction) = value(*rr, normal->direction, normal->sens) -
															value(*r, normal->direction, reverse(normal->sens)) ;
				translate(*r, translation) ;
			}

			int index = index_from_if(rectangles, [&](const MyRect& rec){return rec.i!=r->i && rec.selected && intersect_strict(rec, *r) ;}) ;

			if (index == -1)
			{
				r->selected = true ;
				prec = r ;
			}

			if (index != -1 && prec==0)
			{

		//rr is not the first one. so some space has already been reserved. We try the start position for every normal.
				normal = normal4 - 1 ;
				while (normal+1 < normal4+4 &&
					index_from_if(rectangles, [&](const MyRect& rec){return rec.i!=r->i && rec.selected && intersect_strict(rec, *r) ;}) != -1)
				{
					translate(*r, {rr->m_left - r->m_left, rr->m_top - r->m_top}) ;
			// same code as 'no more room for another step. start using the next side'
					MyPoint translation ;
				//use the yet to be replaced normal to compute the translation coordinate in this direction
					const NormalDirection *normal_ = normal == normal4 - 1 ? normal + 4 : normal ;
					value(translation, normal_->direction) = value(*rr, normal_->direction, normal_->sens) -
																value(*r, normal_->direction, normal_->sens) ;
					normal++ ;
					value(translation, normal->direction) = value(*rr, normal->direction, normal->sens) -
																value(*r, normal->direction, reverse(normal->sens)) ;
					translate(*r, translation) ;

					int index = index_from_if(rectangles, [&](const MyRect& rec){return rec.i!=r->i && rec.selected && intersect_strict(rec, *r) ;}) ;

					if (index != -1)
					{
						MyRect *prec = &rectangles[index] ;
				//now try to use the hit rectangle as predecessor along this direction. So we reuse the code from 'case we have a pred'

						translate(*r, {rr->m_left - r->m_left, rr->m_top - r->m_top}) ;

						const NormalDirection * next_normal = normal+1 == normal4+4 ? normal4 : normal+1 ;

						if (value(*rr, next_normal->direction, next_normal->sens)*next_normal->sens >
								value(*prec, next_normal->direction, next_normal->sens)*next_normal->sens)
						{
					//there is room for another step
							MyPoint translation ;
							value(translation, normal->direction) = value(*prec, normal->direction, reverse(normal->sens)) -
																		value(*r, normal->direction, reverse(normal->sens)) ;
							value(translation, next_normal->direction) = value(*prec, next_normal->direction, next_normal->sens) -
															value(*r, next_normal->direction, reverse(next_normal->sens)) ;
							translate(*r, translation) ;
						}
					}
				}/*while (normal+1 < normal4+4 &&
					index_from_if(rectangles, [&](const MyRect& rec){return rec.i!=r->i && rec.selected && intersect_strict(rec, *r) ;}) != -1)*/

				if (index_from_if(rectangles, [&](const MyRect& rec){return rec.i!=r->i && rec.selected && intersect_strict(rec, *r) ;}) == -1)
				{
					r->selected = true ;
					prec = r ;
				}
				else
				{
					return false ;
				}
			}//if (index != -1 && prec==0)

		}//for (MyRect* r : unordered_adjacency_list[rr->i])

	}//while (!pending_stack.empty())

	MyRect frame = compute_frame(rectangles) ;
	for (MyRect &r : rectangles)
	{
		translate(r, {- frame.m_left,- frame.m_top}) ;
	}

	return index_from_if(rectangles, [](const MyRect& r){return r.selected == false;}) == -1 ;
}


void composite_from_selected_rectangles(vector<WidgetContext> &rects, vector<vector<MPD_Arc> > &adjacency_list)
{
	int n = rects.size() ;
	int not_selected = ranges::count_if(rects, [](const WidgetContext& widget){return widget.r.selected==false ;}) ;
	int selected = rects.size() - not_selected ;

//1) calcule de la matrice de permutation
	vector<int> permutation(n) ;
	ranges::copy(views::iota(0, n), begin(permutation)) ;
//on met les selectionnes a droite.
	ranges::sort(permutation, {}, [&](int i){return rects[i].r.selected; }) ;
	permutation = compute_reverse_permutation(permutation) ;
	PermutationMatrix<Dynamic> perm(n) ;
	for (int i=0; i < n; i++)
		perm.indices().data()[i] = permutation[i] ;
//2) mise a jour des rectangles composites
	vector<WidgetContext> my_rectangles(n) ;
	Map<MatrixXw>(my_rectangles.data(), n, 1) = (perm * Map<MatrixXw>(rects.data(), n, 1)) ;
	vector<WidgetContext> selected_rectangles(my_rectangles.begin() + not_selected, my_rectangles.end()),
						not_selected_rectangles(my_rectangles.begin(), my_rectangles.begin() + not_selected) ;
	my_rectangles = not_selected_rectangles ;
	WidgetContext widget ;
	widget.type = WidgetType::COMPOSITE_WIDGET ;
	widget.widgets = selected_rectangles ;
	MyRect frame = compute_frame(vector<MyRect>(selected_rectangles.begin(), selected_rectangles.end())) ;
	for (WidgetContext& child_widget : widget.widgets)
		translate(child_widget.r, {-frame.m_left, -frame.m_top}) ;
	widget.r = frame ;
	translate(widget.r, {-frame.m_left, -frame.m_top}) ;
	my_rectangles.push_back(widget) ;
	rects = my_rectangles ;
//3) mise a jour de la liste d'adjacence
	MatrixXd OW = MatrixXd::Zero(n,n) ;	//Oriented Weights
	for (auto [i, j] : adjacency_list | std::views::join)
	{
		OW(i, j) = 1 ;
	}
	OW = perm * OW * perm.transpose() ;
								// (startRow, startCol, blockRows, blockCols)
	OW.col(not_selected) = OW.block(0, not_selected, n, selected).rowwise().sum() ;	//rowwise().sum() returns a vector of the sums in each row
	OW.row(not_selected) = OW.block(not_selected, 0, selected, n).colwise().sum() ;
	adjacency_list = compute_adjacency_list(OW.block(0,0,not_selected+1, not_selected+1)) ;
}


bool stair_steps_(vector<MyRect> &rectangles, vector<vector<MPD_Arc> > &adj_list)
{
        FunctionTimer ft("stair_steps_");
	int n = rectangles.size() ;

	vector<vector<MyRect> > solutions ;

	for (int i=0; i < n; i++)
	{
		vector<vector<MPD_Arc> > adjacency_list = adj_list ;

		vector<WidgetContext> rects(n) ;
		for (int ii=0; ii < n; ii++)
		{
			WidgetContext &widget = rects[ii] ;
			MyRect& r = rectangles[ii] ;
			widget.type = WidgetType::RECTANGLE ;
			widget.r = r ;
			widget.r.selected = false ;
		}

		while(int not_selected = ranges::count_if(rects, [](const WidgetContext& widget){return widget.r.selected==false ;}))
		{
			int selected = rects.size() - not_selected ;
			int n = rects.size() ;
			MatrixXd W = MatrixXd::Zero(n,n) ;
			PermutationMatrix<Dynamic> perm(n) ;
			for (const auto& [i, j] : adjacency_list | views::join)
			{
				W(i,j) = W(j,i) = 1 ;
			}
                        vector<int> v(n);
			if (selected != 0 && minimum_cut(W, perm, v)==false)
			{
				composite_from_selected_rectangles(rects, adjacency_list) ;
				if (rects.size() == n)
					break ;	// composite hat nichts gebracht
			}
			if (selected != 0 && minimum_cut(W, perm, v)==true)
			{
				break ;
			}

			n = rects.size() ;
			vector<MyRect> rectangles_(n) ;
			for (int i=0; i < n; i++)
			{
				MyRect& r = rectangles_[i] ;
				WidgetContext& widget = rects[i] ;
				r = widget.r ;
				r.i = i ;
			}

			bool result = stair_steps(rectangles_, rectangles_[i < n ? i : n-1], adjacency_list) ;

			for (int i=0; i < n; i++)
			{
				MyRect& r = rectangles_[i] ;
				WidgetContext& widget = rects[i] ;
				int ri = widget.r.i ;
				widget.r = r ;
				widget.r.i = ri ;
			}
		}

		rects = collapse_composite(rects) ;
		vector<MyRect> rects2(rects.size()) ;
		for (WidgetContext &widget : rects)
		{
			assert(widget.type == WidgetType::RECTANGLE) ;
			MyRect &r = widget.r ;
			rects2[r.i] = r ;
		}
		if (index_from_if(rects,[](const WidgetContext& widget){return widget.r.selected==false;}) == -1)
			solutions.push_back(rects2) ;
	}

	if (solutions.empty())
		return false ;

	rectangles = * ranges::min_element(solutions, {}, [](const vector<MyRect>& rects){
		return dim_max(compute_frame(rects));
	}) ;

	MyRect frame = compute_frame(rectangles) ;
	for (MyRect &r : rectangles)
		translate(r, {-frame.m_left, -frame.m_top}) ;
	return true ;
}


bool stair_steps(vector<MyRect> &rectangles, vector<vector<MPD_Arc> > adjacency_list)
{
        FunctionTimer ft("stair_steps");

	vector<vector<MyRect> > solutions ; 
	vector<MyRect> rectangles_ = rectangles ;
	bool result = stair_steps_(rectangles_, adjacency_list) ;
	if (result)
		solutions.push_back(rectangles_) ;

/*
Cas des chaines:
+---+
|   |
|   |
|   +---+---+
|   |   |   |
| A | B | C |
+---+---+---+
si un element C n'est connecte que par un lien et que celui a qui il est connecte (B) l'est par deux liens, alors on essaye de connecter A avec C, car en s'enroulant
autour de A, B et C vont bien se retrouver l'un a cote de l'autre.

Detection des chaines : # liens == # rectangles - 1 (en retirant les liens self) et max(cardinality) == 2
*/
	int n = rectangles.size() ;
	vector<vector<int> > unoriented_adjacency_list(n) ;
	for (const auto& [i, j] : adjacency_list | views::join)
	{
		if (i == j)
			continue ;
		unoriented_adjacency_list[i].push_back(j) ;
		unoriented_adjacency_list[j].push_back(i) ;
	}

	int edge_count = 0 ;
	for (vector<int>& adj : unoriented_adjacency_list)
		edge_count += adj.size() ;
	edge_count /= 2 ;
	int max_cardinality = 0 ;
	for (vector<int>& adj : unoriented_adjacency_list)
		max_cardinality = max<int>(max_cardinality, adj.size()) ;

	if (edge_count == n - 1 && max_cardinality == 2)
	{
	// on a une chaine.
		for (int i=0; i<n; i++)
		{
			vector<int>& adj = unoriented_adjacency_list[i] ;
			if (adj.size() == 2)
			{
				adjacency_list[ adj[0] ].push_back( MPD_Arc{adj[0], adj[1]} ) ;
			}
		}

		vector<MyRect> rectangles_ = rectangles ;
		bool result = stair_steps(rectangles_, adjacency_list) ;
		if (result)
			solutions.push_back(rectangles_) ;
	}

	if (solutions.empty())
		return false ;

	rectangles = * ranges::min_element(solutions, {}, [](const vector<MyRect>& rectangles){return dim_max(compute_frame(rectangles)) ;}) ;
	MyRect frame = compute_frame(rectangles) ;
	for (MyRect &r : rectangles)
		translate(r, {-frame.m_left, -frame.m_top}) ;
	return true ;
}


void write_json(const vector<MyRect>& rectangles, const vector<Context>& contexts, char (&buffer)[100000])
{
	int pos=0;
	pos += sprintf(buffer + pos, "{\"contexts\":[\n");

	for (const Context& ctx : contexts)
	{
		pos += sprintf(buffer + pos, "{\"title\":\"%s\",\n", ctx.title.c_str());
		pos += sprintf(buffer + pos, "\"frame\":{\"left\":%d,\"right\":%d,\"top\":%d,\"bottom\":%d},\n", 0, width(ctx.frame), 0, height(ctx.frame));

		pos += sprintf(buffer + pos, "\"translatedBoxes\":[\n");

		for (const MyRect& r : ctx.rectangles)
		{
			assert(r.m_left < r.m_right);
			assert(r.m_top < r.m_bottom);
			pos += sprintf(buffer + pos, "{\"id\":%d, \"translation\":{\"x\":%d,\"y\":%d}}%c\n", r.no_sequence, r.m_left, r.m_top,
                              &r == &ctx.rectangles.back() ? ' ' : ',');
		}

		pos += sprintf(buffer + pos, "],\n");
		pos += sprintf(buffer + pos, "\"links\":[]\n");
		pos += sprintf(buffer + pos, "}%c\n", &ctx == &contexts.back() ? ' ' : ',');
	}

	pos += sprintf(buffer + pos, "],\n");

	pos += sprintf(buffer + pos, "\"rectangles\":[\n");
	for (const MyRect& r : rectangles)
	{
		pos += sprintf(buffer + pos, "\t{\"left\":%hu,\"right\":%hu,\"top\":%hu,\"bottom\":%hu}%c\n", 0, width(r), 0, height(r), &r == &rectangles.back() ? ' ' : ',');
	}
	pos += sprintf(buffer + pos, "]\n");

	pos += sprintf(buffer + pos, "}\n");
	buffer[pos]=0;
	assert(pos < 100000);
}

void test_stair_steps(int rect_border)
{
        TestFunctionTimer ft("test_stair_steps");

        struct DataContext{int testid; string title; vector<MyRect> input_rectangles; vector<Edge> edges; vector<TranslatedBox> expected_translations; MyRect frame;};

        const vector<DataContext> vdctx = {
                {
		    1,"My first SVG",
                    {
                        {0, 50, 0, 60},
                        {0, 50, 0, 120},
                        {0, 40, 0, 40},
                        {0, 60, 0, 50},
                        {0, 30, 0, 40}
                    },
                    {
                        {.from=0, .to=1},
                        {.from=0, .to=2},
                        {.from=1, .to=3},
                        {.from=1, .to=4}
                    },
                    {
                        {0,{90,50}},
                        {1,{0,80}},
                        {2,{180,70}},
                        {3,{90,150}},
                        {4,{20,0}}
                    },
                    /*frame*/{0,220,0,200}
                },
                {
                    2,"RANELITEG/1",
	            {
			{0, 121, 0, 104},
			{0, 137, 0, 248},
			{0, 95, 0, 56},
			{0, 109, 0, 56},
			{0, 79, 0, 40},
			{0, 89, 0, 72},
			{0, 85, 0, 72},
			{0, 120, 0, 72},
			{0, 96, 0, 72},
		    },
	            {
			{.from=0, .to=1},
			{.from=1, .to=2},
			{.from=1, .to=3},
			{.from=1, .to=4},
			{.from=5, .to=1},
			{.from=6, .to=1},
			{.from=7, .to=0},
			{.from=8, .to=1}
		    },
                    {
                        {0,{0,256}},
                        {1,{161,112}},
                        {2,{338,96}},
                        {3,{338,304}},
                        {4,{161,400}},
                        {5,{209,0}},
                        {6,{84,0}},
                        {7,{1,144}},
                        {8,{338,192}}
                    },
                    /*frame*/{0,447,0,440}
                },
                {
		    3,"RANELITEG/2",
		    {
			{0, 135, 0, 152},
			{0, 105, 0, 88},
			{0, 105, 0, 88},
			{0, 55, 0, 56},
			{0, 84, 0, 104},
			{0, 136, 0, 40},
			{0, 100, 0, 56}
		    },
		    {
			{.from=0, .to=3},
			{.from=0, .to=4},
			{.from=0, .to=5},
			{.from=1, .to=0},
			{.from=1, .to=6},
			{.from=2, .to=0}
		    },
                    {
                        {0,{95,144}},
                        {1,{270,128}},
                        {2,{125,16}},
                        {3,{0,144}},
                        {4,{1,0}},
                        {5,{270,256}},
                        {6,{415,160}}
                    },
                    /*frame*/{0,515,0,296}
                },
                {
		    4,"SINITAX/1",
		    {
			{0, 77, 0, 104},
			{0, 79, 0, 88},
			{0, 51, 0, 88},
			{0, 105, 0, 232},
			{0, 79, 0, 104},
			{0, 77, 0, 88},
			{0, 95, 0, 120},
			{0, 50, 0, 72}
		    },
		    {
			{.from=0, .to=2},
			{.from=0, .to=4},
			{.from=1, .to=0},
			{.from=3, .to=0},
			{.from=3, .to=1},
			{.from=3, .to=2},
			{.from=3, .to=4},
			{.from=3, .to=7},
			{.from=4, .to=2},
			{.from=6, .to=3},
			{.from=6, .to=5}
		    },
                    {
                        {0,{0,0}},
                        {1,{236,128}},
                        {2,{0,144}},
                        {3,{91,144}},
                        {4,{117,0}},
                        {5,{371,288}},
                        {6,{236,256}},
                        {7,{1,272}}
                    },
                    /*frame*/{0,448,0,376}
                },
                {
		    5,"SINITAX/2",
		    {
			{0, 102, 0, 104},
			{0, 88, 0, 104},
			{0, 69, 0, 72},
			{0, 78, 0, 88},
			{0, 88, 0, 88},
			{0, 69, 0, 72}
		    },
		    {
			{.from=0, .to=5},
			{.from=1, .to=1},
			{.from=1, .to=3},
			{.from=2, .to=0},
			{.from=2, .to=1},
			{.from=4, .to=0},
			{.from=4, .to=3}
		    },
                    {
                        {0,{128,128}},
                        {1,{0,16}},
                        {2,{19,160}},
                        {3,{128,0}},
                        {4,{246,0}},
                        {5,{270,160}}
                    },
                    /*frame*/{0,339,0,232}
                },
                {
		    6,"SINITAX/3",
		    {
			{0, 58, 0, 72},
			{0, 78, 0, 88},
			{0, 77, 0, 136}
		    },
		    {
			{.from=0, .to=2},
			{.from=1, .to=0}
		    },
                    {
                        {0,{20,0}},
                        {1,{0,112}},
                        {2,{118,64}}
                    },
                    /*frame*/{0,195,0,200}
                },
                {
		    7,"SAKILA/1",
		    {
			{0, 92, 0, 168},
			{0, 93, 0, 88},
			{0, 93, 0, 136},
			{0, 92, 0, 136},
			{0, 83, 0, 200}
		    },
		    {
			{.from=2, .to=0},
			{.from=2, .to=3},
			{.from=2, .to=4},
			{.from=3, .to=0},
			{.from=3, .to=1},
			{.from=3, .to=4}
		    },
                    {
                        {0,{255,208}},
                        {1,{255,80}},
                        {2,{122,240}},
                        {3,{123,64}},
                        {4,{0,0}}
                    },
                    /*frame*/{0,348,0,376}
                },
                {
		    8,"SAKILA/2",
                    {
			{0, 84, 0, 152},
			{0, 89, 0, 168},
			{0, 111, 0, 312},
			{0, 109, 0, 88}
		    },
		    {
			{.from=0, .to=1},
			{.from=1, .to=2},
			{.from=3, .to=0}
		    },
                    {
                        {0,{129,160}},
                        {1,{0,160}},
                        {2,{253,0}},
                        {3,{104,32}}
                    },
                    /*frame*/{0,364,0,328}
                },
                {
                    9,"SAKILA/3",
                    {
			{0, 80, 0, 88},
			{0, 89, 0, 72},
			{0, 136, 0, 232},
			{0, 89, 0, 72},
			{0, 105, 0, 72},
			{0, 91, 0, 72}
		    },
                    {
			{.from=2, .to=5},
			{.from=3, .to=0},
			{.from=3, .to=2},
			{.from=4, .to=1},
			{.from=4, .to=2}
		    },
                    {
                        {0,{305,0}},
                        {1,{321,240}},
                        {2,{0,80}},
                        {3,{176,16}},
                        {4,{176,240}},
                        {5,{176,128}}
                    },
                    /*frame*/{0,410,0,312}
                },
                {
                    10,"COCOGIRL/1",
                    {
			{0, 100, 0, 136},
			{0, 100, 0, 136},
			{0, 124, 0, 216},
			{0, 109, 0, 88},
			{0, 147, 0, 88},
			{0, 80, 0, 72},
			{0, 107, 0, 88},
			{0, 124, 0, 168},
			{0, 128, 0, 88},
			{0, 69, 0, 40}
		    },
                    {
			{.from=0, .to=1},
			{.from=1, .to=2},
			{.from=1, .to=3},
			{.from=1, .to=5},
			{.from=1, .to=7},
			{.from=1, .to=9},
			{.from=4, .to=3},
			{.from=6, .to=5},
			{.from=8, .to=7}
		    },
                    {
                        {0,{71,336}},
                        {1,{211,208}},
                        {2,{351,128}},
                        {3,{62,208}},
                        {4,{0,80}},
                        {5,{211,384}},
                        {6,{211,496}},
                        {7,{187,0}},
                        {8,{351,0}},
                        {9,{331,384}}
                    },
                    /*frame*/{0,479,0,584}
                },
                {
                    11,"COCOGIRL/2",
                    {
			{0, 127, 0, 120},
			{0, 142, 0, 232},
			{0, 114, 0, 168},
			{0, 63, 0, 72},
			{0, 89, 0, 104},
			{0, 146, 0, 248}
		    },
                    {
			{.from=0, .to=2},
			{.from=1, .to=2},
			{.from=4, .to=0},
			{.from=4, .to=3},
			{.from=5, .to=0},
			{.from=5, .to=3}
		    },
                    {
                        {0,{116,400}},
                        {1,{283,0}},
                        {2,{129,192}},
                        {3,{26,144}},
                        {4,{0,256}},
                        {5,{283,272}}
                    },
                    /*frame*/{0,429,0,520}
                },

                {
                    .testid=12, .title="LOLA",
                    .input_rectangles={
			{.m_left=0,.m_right=162,.m_top=0,.m_bottom=104},//8-0
			{.m_left=0,.m_right=182,.m_top=0,.m_bottom=72},//9-1
			{.m_left=0,.m_right=105,.m_top=0,.m_bottom=72},//10-2
			{.m_left=0,.m_right=126,.m_top=0,.m_bottom=152},//21-3
			{.m_left=0,.m_right=126,.m_top=0,.m_bottom=88},//24-4
			{.m_left=0,.m_right=147,.m_top=0,.m_bottom=120},//25-5
			{.m_left=0,.m_right=140,.m_top=0,.m_bottom=120},//26-6
			{.m_left=0,.m_right=168,.m_top=0,.m_bottom=136},//27-7
			{.m_left=0,.m_right=168,.m_top=0,.m_bottom=120},//28-8
			{.m_left=0,.m_right=147,.m_top=0,.m_bottom=104},//30-9
			{.m_left=0,.m_right=133,.m_top=0,.m_bottom=120},//32-10
			{.m_left=0,.m_right=147,.m_top=0,.m_bottom=168},//44-11
			{.m_left=0,.m_right=140,.m_top=0,.m_bottom=88},//48-12
			{.m_left=0,.m_right=155,.m_top=0,.m_bottom=120},//52-13
			{.m_left=0,.m_right=175,.m_top=0,.m_bottom=136}//53-14
		    },
                    .edges={
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
                    .expected_translations={
			{.id=0,.translation={.x=396,.y=10}},
			{.id=1,.translation={.x=320,.y=330}},
			{.id=2,.translation={.x=453,.y=218}},
			{.id=3,.translation={.x=598,.y=10}},
			{.id=4,.translation={.x=598,.y=202}},
			{.id=5,.translation={.x=750,.y=346}},
			{.id=6,.translation={.x=273,.y=154}},
			{.id=7,.translation={.x=542,.y=330}},
			{.id=8,.translation={.x=335,.y=506}},
			{.id=9,.translation={.x=556,.y=506}},
			{.id=10,.translation={.x=764,.y=186}},
			{.id=11,.translation={.x=743,.y=506}},
			{.id=12,.translation={.x=93,.y=153}},
			{.id=13,.translation={.x=10,.y=281}},
			{.id=14,.translation={.x=11,.y=441}}
                    },
                    .frame={.m_left=0,.m_right=884,.m_top=0,.m_bottom=552}
                }
	};

        for (const auto& [testid, title, input_rectangles, edges, expected_translations, frame] : vdctx)
        {
            	vector<MyRect> rectangles = input_rectangles;
            	int n = rectangles.size();
            	vector<vector<MPD_Arc> > adjacency_list(n) ;

            	for (const Edge& e : edges)
            	{
                	adjacency_list[e.from].push_back({e.from, e.to}) ;
            	}

                high_resolution_clock::time_point t1 = high_resolution_clock::now();

                int i=0;
		for (MyRect& r : rectangles)
                {
                    r.i = r.no_sequence = i++;
	        }

		for (MyRect &r : rectangles)
		{
			r.m_right += 2*rect_border ;
			r.m_bottom += 2*rect_border ;
		}
		stair_steps(rectangles, adjacency_list) ;
		for (MyRect &r : rectangles)
		{
			expand_by(r, - rect_border) ;
		}

		MyRect frame_ = compute_frame(rectangles) ;
		for (MyRect &r : rectangles)
		{
			translate(r, {- frame_.m_left,- frame_.m_top}) ;
		}
		frame_ = compute_frame(rectangles) ;

                ranges::sort(rectangles, {}, &MyRect::no_sequence);
                vector<TranslatedBox> translations;
                for (MyRect &r : rectangles)
                    translations.push_back({r.no_sequence,{r.m_left, r.m_top}});

                duration<double> time_span = high_resolution_clock::now() - t1;
		bool bOK = expected_translations == translations;
                printf("%s [%d] %20s %f seconds elapsed\n", bOK ? "OK": "KO",
					testid, title.c_str(), time_span.count());
		(bOK ? nbOK : nbKO)++;

                vector<MyRect> expected_rectangles = input_rectangles;

                for (int i=0; i<n; i++)
                {
                        expected_rectangles[i] = translate(input_rectangles[i], expected_translations[i].translation);
                }

                latuile_test_json_output(input_rectangles,
                                        rectangles,
                                        edges,
                                        expected_rectangles,
                                        "stair_steps",
                                        testid);
	}
}



vector<WidgetContext> composite_stair_steps_layout(vector<WidgetContext>& rectangles, const vector<vector<MPD_Arc> >& adjacency_list)
{
        FunctionTimer ft("composite_stair_steps_layout");
	int n = rectangles.size() ;
	vector<MyRect> vec(n) ;
	for (int i=0; i < n ; i++)
	{
		MyRect &r = vec[i] ;
		const WidgetContext &widget = rectangles[i] ;
		r = widget.r ;
		r.i = i ;
	}

	bool result = stair_steps(vec, adjacency_list) ;
	if (result)
	{
		vector<WidgetContext> rectangles_ = rectangles ;
		for (int i=0; i < vec.size(); i++)
		{
			MyRect &r = vec[i] ;
			WidgetContext &widget = rectangles_[i] ;
			int ri = widget.r.i ;
			widget.r = r ;
			widget.r.i = ri ;
		}
		return rectangles_ ;
	}
	else
	{
		MatrixXd W = MatrixXd::Zero(n,n) ;
		PermutationMatrix<Dynamic> perm(n) ;
		for (const auto [i, j] : adjacency_list | views::join)
		{
			W(i,j) = W(j,i) = 1 ;
		}
		Matrix<int8_t,-1,-1> OW = Matrix<int8_t,-1,-1>::Zero(n, n) ;	//Oriented Weights
		for (const auto [i, j] : adjacency_list | views::join)
		{
			OW(i, j) = 1 ;
		}
		vector<int> component_distribution ;
		minimum_cut(W,
					perm,
					component_distribution) ;
		int nc = component_distribution.size() ;
		int n_acc = 0 ;

		vector<WidgetContext> composite_widgets(nc) ;

		MatrixXd P = MatrixXd::Zero(n,nc) ;

		for (int i=0; i < nc; i++)
		{
			int np = component_distribution[i] ;
	//block(Index startRow, Index startCol, Index blockRows, Index blockCols)
			P.block(n_acc, i, np, 1) = MatrixXd::Constant(np, 1, 1) ;
			vector<vector<MPD_Arc> > my_adjacency_list = compute_adjacency_list_( (perm * OW * perm.transpose()).block(n_acc, n_acc, np, np) ) ;
			vector<WidgetContext> my_rectangles(np) ;
			Map<MatrixXw>(my_rectangles.data(), np,1) = (perm * Map<MatrixXw>(rectangles.data(), n,1)).block(n_acc, 0, np, 1) ;
			WidgetContext &widget = composite_widgets[i] ;
			widget.type = WidgetType::COMPOSITE_WIDGET ;
			widget.widgets = my_rectangles ;
			widget.widgets = composite_stair_steps_layout(my_rectangles, my_adjacency_list) ;
			vector<WidgetContext> &widgets = widget.widgets ;
			widget.r = compute_frame(vector<MyRect>(widgets.begin(), widgets.end())) ;
			widget.r.i = i ;
			n_acc += np ;
		}

		composite_widgets = composite_stair_steps_layout(composite_widgets, compute_adjacency_list(P.transpose() * (perm*W*perm.transpose()) * P)) ;

		return composite_widgets ;
	}
}


void rotate_composite(vector<WidgetContext>& rectangles, int rotation_bitmap)
{
        FunctionTimer ft("rotate_composite");

	vector<WidgetContext*> composites ;
	walk_composite(rectangles, [&](WidgetContext& widget){if (widget.type==WidgetType::COMPOSITE_WIDGET && widget.widgets.size()>1)composites.push_back(&widget);}) ;
	for (int index=0; index < composites.size(); index++)
	{
		int rotations = (rotation_bitmap >> (index * 2)) & 0x03 ;
		WidgetContext &widget = * composites[index] ;
		vector<WidgetContext> &widgets = widget.widgets ;
		MyRect frame = compute_frame(vector<MyRect>(widgets.begin(), widgets.end())) ;
		for (WidgetContext &child_widget : widgets)
		{
			if (rotations & 0x01)
			{
				child_widget.r = symmetric(child_widget.r, EAST_WEST, middle(frame, NORTH_SOUTH)) ;
			}
			if (rotations & 0x02)
			{
				child_widget.r = symmetric(child_widget.r, NORTH_SOUTH, middle(frame, EAST_WEST)) ;
			}
		}
	}
}


void translate_composite(vector<WidgetContext> &rectangles, MyPoint translation = {0,0})
{
	for (WidgetContext& widget : rectangles)
	{
		switch (widget.type)
		{
		case COMPOSITE_WIDGET:
			translate_composite(widget.widgets, translation + MyPoint{widget.r.m_left, widget.r.m_top}) ;
			break ;
		case RECTANGLE:
			translate(widget.r, translation) ;
			break ;
		} 
	}
}


void fit_together_composite(vector<WidgetContext>& rectangles)
{
        FunctionTimer ft("fit_together_composite");

	translate_composite(rectangles) ;

	vector<WidgetContext*> composites ;
	walk_composite(rectangles, [&](WidgetContext& widget){if (widget.type==WidgetType::COMPOSITE_WIDGET && widget.widgets.size()>1)composites.push_back(&widget);}) ;

	while (true)
	{
		unsigned int best_m ;
		int best_diameter = INT_MAX ;
		vector<MyRect> best_B ;
		MyPoint best_translation_B ;
		Direction best_direction ;

		int n = composites.size() ;
		for (unsigned int m=0; m < pow(2, n); m++)
		{
			vector<WidgetContext*> composite_partition[2] ;
			for (int i=0; i < composites.size(); i++)
			{
				composite_partition[m & (1 << i) ? 0 : 1].push_back(composites[i]) ;
			}

			for (Direction direction : directions)
			{
				vector<MyRect> B ;
				MyPoint translation_B ;
				int diameter ;
				fit_together(composite_partition,
							direction,
							B,
							translation_B,
							diameter) ;

				if (diameter < best_diameter)
				{
					best_m = m ;
					best_diameter = diameter ;
					best_B = B ;
					best_translation_B = translation_B ;
					best_direction = direction ;
				}
			}
		}

		unordered_set<MyRect> BB(best_B.begin(), best_B.end()) ;
		walk_composite(rectangles, [&](WidgetContext& widget){if (widget.type==WidgetType::RECTANGLE && BB.count(widget.r)) translate(widget.r, best_translation_B);}) ;
		if (best_translation_B==MyPoint{0,0} || best_B.empty())
			break ;
	}
}


void stair_steps_layout(vector<MyRect> &vect, const vector<vector<MPD_Arc> > &adjacency_list, int rect_border)
{
        FunctionTimer ft("stair_steps_layout");
	int n = vect.size() ;
	vector<WidgetContext> _rectangles(n) ;
	for (int i=0; i < n; i++)
	{
		WidgetContext &widget = _rectangles[i] ;
		MyRect &r = vect[i] ;
		widget.r = r ;
		widget.type = WidgetType::RECTANGLE ;
		widget.r.m_right += 2*rect_border ;
		widget.r.m_bottom += 2*rect_border ;
	}

	_rectangles = composite_stair_steps_layout(_rectangles, adjacency_list) ;

//TODO: essayer les symmetries autour les axes EAST_WEST et NORTH_SOUTH pour trouver la meilleure disposition cad celle qui minimize
// la longueur totale des liens. Ceci sans deformer les composites puisqu'on ne fait que des symmetries.
	int count = 0 ;
	walk_composite(_rectangles, [&](WidgetContext& widget){if (widget.type==WidgetType::COMPOSITE_WIDGET && widget.widgets.size()>1)count++ ;}) ;
//cap count otherwise the possibilities might become overwhelming.
	count = min(count, 3) ;
	int best_rotation_bitmap = -1 ;
	int min_total_distance = INT_MAX ;
	for (int rotation_bitmap=0; rotation_bitmap < pow(2, 2*NR_DIRECTIONS*count) ; rotation_bitmap++)
	{
		vector<WidgetContext> rects = _rectangles ;
		rotate_composite(rects, rotation_bitmap) ;
		rects = collapse_composite(rects) ;
		vector<MyRect> vec(n) ;
		for (WidgetContext& widget : rects)
		{
			MyRect &r = widget.r ;
			vec[r.i] = r ;
		}
		for (MyRect &r : vec)
		{
			expand_by(r, - rect_border) ;
		}

		MyRect frame = compute_frame(vec) ;
		for (MyRect &r : vec)
		{
			translate(r, {- frame.m_left,- frame.m_top}) ;
		}

//on verifie que les rectangles n'ont pas été permutés.
		assert(ranges::equal(vec, vect, {}, [](MyRect& r){return dimensions(r);})) ;
		int total_distance = 0 ;
		for (const auto& [i, j] : adjacency_list | views::join)
		{
			total_distance += rectangle_distance(vec[i], vec[j]) ;
		}
		if (total_distance < min_total_distance)
		{
			min_total_distance = total_distance ;
			best_rotation_bitmap = rotation_bitmap ;
		}
	}

	rotate_composite(_rectangles, best_rotation_bitmap) ;
	fit_together_composite(_rectangles) ;
	vector<WidgetContext> rectangles_ ;
	walk_composite(_rectangles, [&](WidgetContext& widget){if (widget.type==WidgetType::RECTANGLE)rectangles_.push_back(widget.r);}) ;
	_rectangles = rectangles_ ;
//	_rectangles = collapse_composite(_rectangles) ;
	vector<MyRect> vec(n) ;
	for (WidgetContext& widget : _rectangles)
	{
		MyRect &r = widget.r ;
		vec[r.i] = r ;
	}

        auto rg = adjacency_list | views::join ;
        vector<MPD_Arc> edges(begin(rg), end(rg));

	vector<tuple<int, RectCorner, int, RectCorner> > swaps ;
	do
	{
		swaps.clear() ;
		swap_rectangles(vec, edges, swaps) ;
	}
	while (!swaps.empty()) ;

	while (compact_rectangles(vec, adjacency_list)) ;

//call again after the calls to compact_rectangles()
	do
	{
		swaps.clear() ;
		swap_rectangles(vec, edges, swaps) ;
	}
	while (!swaps.empty()) ;

	compact_frame(vec, adjacency_list) ;
	optimize_rectangle_positions(vec, adjacency_list) ;
	compact_frame(vec, adjacency_list) ;

	for (MyRect &r : vec)
	{
		expand_by(r, - rect_border) ;
	}

	MyRect frame = compute_frame(vec) ;
	for (MyRect &r : vec)
	{
		translate(r, {- frame.m_left,- frame.m_top}) ;
	}

//on verifie que les rectangles n'ont pas été permutés.
	assert(ranges::equal(vec, vect, {}, [](MyRect& r){return dimensions(r);})) ;
	vect = vec ;
}


void test_stair_steps_layout()
{
	TestFunctionTimer ft("test_stair_steps_layout");

        struct DataContext{int testid; string title; vector<MyRect> input_rectangles; vector<Edge> edges; vector<TranslatedBox> expected_translations; MyRect frame;};

        const vector<DataContext> vdctx = {
	        {
		    .testid=1,
			.title="RANELITEG",
            .input_rectangles={
					{0, 120, 0, 104},
					{0, 137, 0, 248},
					{0, 135, 0, 152},
					{0, 105, 0, 88},
					{0, 95, 0, 56},
					{0, 109, 0, 56},
					{0, 105, 0, 88},
					{0, 79, 0, 40},
					{0, 55, 0, 56},
					{0, 84, 0, 104},
					{0, 89, 0, 72},
					{0, 85, 0, 72},
					{0, 120, 0, 72},
					{0, 96, 0, 72},
					{0, 136, 0, 40},
					{0, 100, 0, 56}
		    },
            .edges={
					{.from=0, .to=1},
					{.from=0, .to=15},
					{.from=1, .to=4},
					{.from=1, .to=5},
					{.from=1, .to=7},
					{.from=2, .to=8},
					{.from=2, .to=9},
					{.from=2, .to=14},
					{.from=3, .to=2},
					{.from=3, .to=15},
					{.from=6, .to=1},
					{.from=6, .to=2},
					{.from=10, .to=1},
					{.from=11, .to=1},
					{.from=12, .to=0},
					{.from=13, .to=1}
		    },
            .expected_translations={
					{0,{145,480}},
					{1,{465,112}},
					{2,{145,288}},
					{3,{0,400}},
					{4,{642,96}},
					{5,{496,400}},
					{6,{320,272}},
					{7,{346,112}},
					{8,{101,192}},
					{9,{196,144}},
					{10,{513,0}},
					{11,{388,0}},
					{12,{305,512}},
					{13,{642,192}},
					{14,{320,400}},
					{15,{5,528}}
                    },
                    .frame={0,738,0,584}
            },
			{
				.testid=2,
				.title="SAKILA",
				.input_rectangles={
					{0, 80, 0, 88},
					{0, 84, 0, 152},
					{0, 89, 0, 72},
					{0, 89, 0, 168},
					{0, 111, 0, 312},
					{0, 92, 0, 168},
					{0, 136, 0, 232},
					{0, 89, 0, 72},
					{0, 105, 0, 72},
					{0, 93, 0, 88},
					{0, 91, 0, 72},
					{0, 93, 0, 136},
					{0, 92, 0, 136},
					{0, 83, 0, 200},
					{0, 109, 0, 88}
				},
				.edges={
					{.from=1, .to=3},
					{.from=3, .to=4},
					{.from=5, .to=1},
					{.from=5, .to=14},
					{.from=6, .to=10},
					{.from=7, .to=0},
					{.from=7, .to=6},
					{.from=8, .to=2},
					{.from=8, .to=6},
					{.from=9, .to=6},
					{.from=9, .to=14},
					{.from=11, .to=5},
					{.from=11, .to=12},
					{.from=11, .to=13},
					{.from=12, .to=5},
					{.from=12, .to=9},
					{.from=12, .to=13},
					{.from=13, .to=1},
					{.from=13, .to=14},
					{.from=14, .to=1}
				},
				.expected_translations={
					{0,{450,1}},
					{1,{444,576}},
					{2,{16,129}},
					{3,{568,560}},
					{4,{602,208}},
					{5,{454,224}},
					{6,{145,80}},
					{7,{321,0}},
					{8,{0,241}},
					{9,{321,224}},
					{10,{14,17}},
					{11,{188,352}},
					{12,{321,352}},
					{13,{321,528}},
					{14,{453,432}}
				},
				.frame={0,713,0,728}
			},
			{
				.testid=3,
				.title="SINITAX",
				.input_rectangles={
					{0, 103, 0, 104},
					{0, 58, 0, 72},
					{0, 88, 0, 104},
					{0, 69, 0, 72},
					{0, 77, 0, 104},
					{0, 79, 0, 88},
					{0, 51, 0, 88},
					{0, 78, 0, 88},
					{0, 105, 0, 232},
					{0, 88, 0, 88},
					{0, 78, 0, 88},
					{0, 69, 0, 72},
					{0, 79, 0, 104},
					{0, 77, 0, 136},
					{0, 77, 0, 88},
					{0, 95, 0, 120},
					{0, 50, 0, 72}
				},
				.edges={
					{.from=0, .to=11},
					{.from=0, .to=15},
					{.from=1, .to=13},
					{.from=2, .to=7},
					{.from=3, .to=0},
					{.from=3, .to=2},
					{.from=4, .to=6},
					{.from=4, .to=12},
					{.from=5, .to=4},
					{.from=7, .to=13},
					{.from=8, .to=4},
					{.from=8, .to=5},
					{.from=8, .to=6},
					{.from=8, .to=12},
					{.from=8, .to=16},
					{.from=9, .to=0},
					{.from=9, .to=7},
					{.from=10, .to=0},
					{.from=10, .to=1},
					{.from=12, .to=4},
					{.from=12, .to=6},
					{.from=13, .to=6},
					{.from=15, .to=8},
					{.from=15, .to=11},
					{.from=15, .to=13},
					{.from=15, .to=14}
				},
				.expected_translations={
					{0,{0,272}},
					{1,{254,416}},
					{2,{261,0}},
					{3,{152,128}},
					{4,{261,144}},
					{5,{378,288}},
					{6,{261,288}},
					{7,{34,16}},
					{8,{352,416}},
					{9,{0,144}},
					{10,{135,416}},
					{11,{26,416}},
					{12,{378,144}},
					{13,{144,240}},
					{14,{235,544}},
					{15,{100,544}},
					{16,{497,528}}
				},
				.frame={0,547,0,664}
			},
			{
				.testid=4,
				.title="COCOGIRL/1",
				.input_rectangles={
					{0, 131, 0, 184},
					{0, 56, 0, 40},
					{0, 82, 0, 56},
					{0, 101, 0, 40},
					{0, 140, 0, 216},
					{0, 77, 0, 88},
					{0, 108, 0, 88},
					{0, 147, 0, 88},
					{0, 118, 0, 88}
				},
				.edges={
					{.from=0, .to=4},
					{.from=0, .to=6},
					{.from=0, .to=8},
					{.from=4, .to=1},
					{.from=4, .to=2},
					{.from=4, .to=3},
					{.from=4, .to=5},
					{.from=4, .to=6},
					{.from=4, .to=8},
					{.from=7, .to=6}
				},
				.expected_translations={
					{0,{335,128}},
					{1,{59,352}},
					{2,{33,128}},
					{3,{335,352}},
					{4,{155,128}},
					{5,{38,224}},
					{6,{187,0}},
					{7,{0,0}},
					{8,{335,0}}
				},
				.frame={0,466,0,392}
			},
			{
				.testid=5,
				.title="COCOGIRL/2",
				.input_rectangles={
					{0, 110, 0, 136},
					{0, 135, 0, 104},
					{0, 114, 0, 168},
					{0, 122, 0, 152},
					{0, 83, 0, 120},
					{0, 146, 0, 248},
					{0, 93, 0, 104},
					{0, 125, 0, 120},
					{0, 75, 0, 120},
					{0, 158, 0, 184},
					{0, 116, 0, 56},
					{0, 123, 0, 72},
					{0, 114, 0, 136}
				},
				.edges={
					{.from=0, .to=9},
					{.from=1, .to=9},
					{.from=2, .to=8},
					{.from=3, .to=9},
					{.from=4, .to=9},
					{.from=5, .to=9},
					{.from=5, .to=10},
					{.from=6, .to=9},
					{.from=7, .to=6},
					{.from=10, .to=12},
					{.from=11, .to=10},
					{.from=12, .to=8},
					{.from=12, .to=10}
				},
				.expected_translations={
					{0,{139,480}},
					{1,{162,112}},
					{2,{681,0}},
					{3,{0,64}},
					{4,{16,321}},
					{5,{337,192}},
					{6,{289,480}},
					{7,{422,480}},
					{8,{679,208}},
					{9,{139,256}},
					{10,{523,384}},
					{11,{679,368}},
					{12,{525,208}}
				},
				.frame={0,802,0,616}
			},
			{
				.testid=6,
				.title="COCOGIRL/3",
				.input_rectangles={
					{0, 131, 0, 280},
					{0, 154, 0, 72},
					{0, 125, 0, 88},
					{0, 131, 0, 216},
					{0, 124, 0, 88},
					{0, 131, 0, 168},
					{0, 128, 0, 72},
					{0, 106, 0, 72},
					{0, 131, 0, 184},
					{0, 147, 0, 472},
					{0, 112, 0, 88}
				},
				.edges={
					{.from=0, .to=3},
					{.from=0, .to=9},
					{.from=1, .to=0},
					{.from=2, .to=0},
					{.from=3, .to=9},
					{.from=4, .to=3},
					{.from=5, .to=9},
					{.from=6, .to=9},
					{.from=7, .to=9},
					{.from=8, .to=9},
					{.from=9, .to=9},
					{.from=10, .to=9}
				},
				.expected_translations={
					{0,{358,400}},
					{1,{529,608}},
					{2,{529,480}},
					{3,{358,144}},
					{4,{529,273}},
					{5,{187,0}},
					{6,{19,96}},
					{7,{25,336}},
					{8,{0,496}},
					{9,{171,208}},
					{10,{19,208}}
				},
				.frame={0,683,0,680}
			},
			{
				.testid=7,
				.title="COCOGIRL/4",
				.input_rectangles={
					{0, 134, 0, 88},
					{0, 149, 0, 392},
					{0, 106, 0, 120},
					{0, 135, 0, 312},
					{0, 119, 0, 88},
					{0, 168, 0, 216},
					{0, 114, 0, 72},
					{0, 89, 0, 104},
					{0, 93, 0, 72},
					{0, 135, 0, 88},
					{0, 80, 0, 88},
					{0, 80, 0, 88},
					{0, 106, 0, 168}
				},

//arbre centre sur le noeud 3 et ayant 12 enfants

				.edges={
					{.from=0, .to=3},
					{.from=1, .to=3},
					{.from=2, .to=3},
					{.from=4, .to=3},
					{.from=5, .to=3},
					{.from=6, .to=3},
					{.from=7, .to=3},
					{.from=8, .to=3},
					{.from=9, .to=3},
					{.from=10, .to=3},
					{.from=11, .to=3},
					{.from=12, .to=3}
				},
				.expected_translations={
					{0,{159,560}},
					{1,{334,0}},
					{2,{542,528}},
					{3,{159,208}},
					{4,{0,0}},
					{5,{334,432}},
					{6,{5,336}},
					{7,{523,0}},
					{8,{26,448}},
					{9,{159,80}},
					{10,{542,400}},
					{11,{39,560}},
					{12,{13,128}}
				},
				.frame={0,648,0,648}
			},
			{
		.testid=8, .title="COCOGIRL/5",
				.input_rectangles={
					{0, 106, 0, 248},
					{0, 112, 0, 104},
					{0, 109, 0, 296},
					{0, 128, 0, 152},
					{0, 97, 0, 152},
					{0, 101, 0, 72},
					{0, 112, 0, 104},
					{0, 156, 0, 88},
					{0, 106, 0, 136},
					{0, 141, 0, 72},
					{0, 140, 0, 264},
					{0, 86, 0, 88},
					{0, 131, 0, 232},
					{0, 63, 0, 72},
					{0, 149, 0, 184},
					{0, 106, 0, 120},
					{0, 149, 0, 200},
					{0, 95, 0, 104},
					{0, 147, 0, 120},
					{0, 131, 0, 216}
				},
				.edges={
					{.from=0, .to=8},
					{.from=1, .to=0},
					{.from=1, .to=8},
					{.from=3, .to=4},
					{.from=5, .to=4},
					{.from=7, .to=6},
					{.from=9, .to=8},
					{.from=10, .to=2},
					{.from=10, .to=5},
					{.from=10, .to=11},
					{.from=10, .to=13},
					{.from=10, .to=15},
					{.from=10, .to=18},
					{.from=12, .to=0},
					{.from=12, .to=4},
					{.from=12, .to=5},
					{.from=12, .to=18},
					{.from=14, .to=15},
					{.from=15, .to=8},
					{.from=15, .to=19},
					{.from=16, .to=15},
					{.from=17, .to=18},
					{.from=18, .to=6}
				},
				.expected_translations={
					{.id=0,.translation={.x=152,.y=258}},
					{.id=1,.translation={.x=0,.y=338}},
					{.id=2,.translation={.x=649,.y=112}},
					{.id=3,.translation={.x=313,.y=0}},
					{.id=4,.translation={.x=481,.y=0}},
					{.id=5,.translation={.x=508,.y=192}},
					{.id=6,.translation={.x=820,.y=304}},
					{.id=7,.translation={.x=821,.y=176}},
					{.id=8,.translation={.x=181,.y=546}},
					{.id=9,.translation={.x=0,.y=546}},
					{.id=10,.translation={.x=469,.y=304}},
					{.id=11,.translation={.x=523,.y=608}},
					{.id=12,.translation={.x=298,.y=192}},
					{.id=13,.translation={.x=420,.y=608}},
					{.id=14,.translation={.x=1017,.y=225}},
					{.id=15,.translation={.x=871,.y=448}},
					{.id=16,.translation={.x=828,.y=608}},
					{.id=17,.translation={.x=649,.y=608}},
					{.id=18,.translation={.x=649,.y=448}},
					{.id=19,.translation={.x=1035,.y=449}}
				},
				.frame={0,1166,0,808}
			},
			{
				.testid=9,
				.title="COCOGIRL/6",
				.input_rectangles={
					{0, 131, 0, 376},
					{0, 90, 0, 216},
					{0, 104, 0, 104},
					{0, 131, 0, 264},
					{0, 123, 0, 152},
					{0, 100, 0, 88},
					{0, 141, 0, 72},
					{0, 91, 0, 72}
				},
				.edges={
					{.from=1, .to=0},
					{.from=2, .to=3},
					{.from=4, .to=2},
					{.from=4, .to=3},
					{.from=5, .to=0},
					{.from=6, .to=0},
					{.from=6, .to=5},
					{.from=7, .to=3},
					{.from=7, .to=5}
				},
				.expected_translations={
					{0,{0,0}},
					{1,{171,0}},
					{2,{482,48}},
					{3,{311,80}},
					{4,{482,192}},
					{5,{171,256}},
					{6,{171,384}},
					{7,{352,384}}
				},
				.frame={0,605,0,456}
			},
			{
				.testid=10,
				.title="COCOGIRL/7",
				.input_rectangles={
					{0, 130, 0, 200},
					{0, 127, 0, 120},
					{0, 142, 0, 232},
					{0, 101, 0, 88},
					{0, 101, 0, 120},
					{0, 115, 0, 56},
					{0, 121, 0, 248},
					{0, 69, 0, 40}
				},
				.edges={
					{.from=0, .to=6},
					{.from=0, .to=7},
					{.from=3, .to=0},
					{.from=4, .to=1},
					{.from=4, .to=2},
					{.from=4, .to=5},
					{.from=4, .to=6},
					{.from=4, .to=7}
				},
				.expected_translations={
					{0,{161,320}},
					{1,{135,0}},
					{2,{302,48}},
					{3,{331,432}},
					{4,{161,160}},
					{5,{6,160}},
					{6,{0,272}},
					{7,{331,352}}
				},
				.frame={0,444,0,520}
			},
			{
				.testid=11,
				.title="COCOGIRL/8",
				.input_rectangles={
					{0, 101, 0, 136},
					{0, 100, 0, 136},
					{0, 83, 0, 88},
					{0, 84, 0, 88},
					{0, 106, 0, 72},
					{0, 143, 0, 120},
					{0, 131, 0, 216},
					{0, 67, 0, 56},
					{0, 80, 0, 72},
					{0, 107, 0, 88},
					{0, 131, 0, 168},
					{0, 128, 0, 88},
					{0, 110, 0, 56},
					{0, 95, 0, 72},
					{0, 100, 0, 136},
					{0, 131, 0, 168}
				},
				.edges={
					{.from=0, .to=1},
					{.from=1, .to=2},
					{.from=1, .to=3},
					{.from=1, .to=5},
					{.from=1, .to=6},
					{.from=1, .to=7},
					{.from=1, .to=8},
					{.from=1, .to=10},
					{.from=1, .to=12},
					{.from=1, .to=15},
					{.from=4, .to=15},
					{.from=9, .to=8},
					{.from=11, .to=10},
					{.from=14, .to=12},
					{.from=14, .to=13}
				},
				.expected_translations={
					{.id=0,.translation={.x=578,.y=336}},
					{.id=1,.translation={.x=438,.y=160}},
					{.id=2,.translation={.x=578,.y=208}},
					{.id=3,.translation={.x=207,.y=178}},
					{.id=4,.translation={.x=78,.y=335}},
					{.id=5,.translation={.x=290,.y=0}},
					{.id=6,.translation={.x=395,.y=336}},
					{.id=7,.translation={.x=331,.y=160}},
					{.id=8,.translation={.x=753,.y=256}},
					{.id=9,.translation={.x=777,.y=128}},
					{.id=10,.translation={.x=578,.y=0}},
					{.id=11,.translation={.x=749,.y=0}},
					{.id=12,.translation={.x=140,.y=82}},
					{.id=13,.translation={.x=0,.y=177}},
					{.id=14,.translation={.x=0,.y=1}},
					{.id=15,.translation={.x=224,.y=335}}
				},
				.frame={0,884,0,552}
			},
			{
				.testid=12,
				.title="LOLA",
				.input_rectangles={
					{.m_left=0,.m_right=162,.m_top=0,.m_bottom=104},//8-0
					{.m_left=0,.m_right=182,.m_top=0,.m_bottom=72},//9-1
					{.m_left=0,.m_right=105,.m_top=0,.m_bottom=72},//10-2
					{.m_left=0,.m_right=126,.m_top=0,.m_bottom=152},//21-3
					{.m_left=0,.m_right=126,.m_top=0,.m_bottom=88},//24-4
					{.m_left=0,.m_right=147,.m_top=0,.m_bottom=120},//25-5
					{.m_left=0,.m_right=140,.m_top=0,.m_bottom=120},//26-6
					{.m_left=0,.m_right=168,.m_top=0,.m_bottom=136},//27-7
					{.m_left=0,.m_right=168,.m_top=0,.m_bottom=120},//28-8
					{.m_left=0,.m_right=147,.m_top=0,.m_bottom=104},//30-9
					{.m_left=0,.m_right=133,.m_top=0,.m_bottom=120},//32-10
					{.m_left=0,.m_right=147,.m_top=0,.m_bottom=168},//44-11
					{.m_left=0,.m_right=140,.m_top=0,.m_bottom=88},//48-12
					{.m_left=0,.m_right=155,.m_top=0,.m_bottom=120},//52-13
					{.m_left=0,.m_right=175,.m_top=0,.m_bottom=136}//53-14
				},
				.edges={
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
				.expected_translations={
					{.id=0,.translation={.x=396,.y=10}},
					{.id=1,.translation={.x=320,.y=330}},
					{.id=2,.translation={.x=453,.y=218}},
					{.id=3,.translation={.x=598,.y=10}},
					{.id=4,.translation={.x=598,.y=202}},
					{.id=5,.translation={.x=750,.y=346}},
					{.id=6,.translation={.x=273,.y=154}},
					{.id=7,.translation={.x=542,.y=330}},
					{.id=8,.translation={.x=335,.y=506}},
					{.id=9,.translation={.x=556,.y=506}},
					{.id=10,.translation={.x=764,.y=186}},
					{.id=11,.translation={.x=743,.y=506}},
					{.id=12,.translation={.x=93,.y=153}},
					{.id=13,.translation={.x=10,.y=281}},
					{.id=14,.translation={.x=11,.y=441}}
				},
				.frame={.m_left=0,.m_right=884,.m_top=0,.m_bottom=552}
			}
        } ;

        for (const auto& [testid, title, input_rectangles, edges, expected_translations, frame] : vdctx)
        {
            	vector<MyRect> rectangles = input_rectangles;
            	int n = rectangles.size();
            	vector<vector<MPD_Arc> > adjacency_list(n) ;
            	for (const Edge& e : edges)
            	{
                	adjacency_list[e.from].push_back({e.from, e.to}) ;
            	}

            	high_resolution_clock::time_point t1 = high_resolution_clock::now();

            	int i=0;
		for (MyRect &r : rectangles)
	            	r.i = r.no_sequence = i++ ;

		stair_steps_layout(rectangles, adjacency_list, RECT_BORDER) ;

                ranges::sort(rectangles, {}, &MyRect::no_sequence);
                vector<TranslatedBox> translations;
                for (MyRect &r : rectangles)
                    translations.push_back({r.no_sequence,{r.m_left, r.m_top}});

                duration<double> time_span = high_resolution_clock::now() - t1;

		bool bOK = expected_translations == translations;
                printf("%s [%d] %20s %f seconds elapsed\n", bOK ? "OK": "KO",
                       testid, title.c_str(), time_span.count());
		(bOK ? nbOK : nbKO)++;

                vector<MyRect> expected_rectangles = input_rectangles;

                for (int i=0; i<n; i++)
                {
                        expected_rectangles[i] = translate(input_rectangles[i], expected_translations[i].translation);
                }

		latuile_test_json_output(input_rectangles,
					rectangles,
                               		edges,
                                	expected_rectangles,
                                	"test_stair_steps_layout",
                                	testid);
	}
}


MyRect expand(const MyRect& r, float factor)
{
	assert(factor >=1.0) ;
	assert(r.m_left == 0) ;
	assert(r.m_top == 0) ;
	MyRect result = r ;
	result.m_right = r.m_right > factor * r.m_bottom ? r.m_right : factor * min(r.m_right, r.m_bottom) ;
	result.m_bottom = r.m_bottom > factor * r.m_right ? r.m_bottom : factor * min(r.m_right, r.m_bottom) ;
	return result ;
}

void test_expand_rectangles()
{
	MyRect r ;
	r.m_left = r.m_top = 0 ;

	r.m_right = 10 ;
	r.m_bottom = 60 ;
//check that only m_right has expanded
	assert(expand(r, 2.5) == MyRect({0, 25, 0, 60}));

//check that result has become a square
	assert(expand(r, 10.0) == MyRect({ 0, 100, 0, 100 }));

	r.m_right = 60 ;
	r.m_bottom = 10 ;
//check that only m_right has expanded
	assert(expand(r, 2.5) == MyRect({ 0, 60, 0, 25 }));

//check that result has become a square
	assert(expand(r, 10.0) == MyRect({ 0, 100, 0, 100 }));
}


//[pr(lower_bound), pr(upper_bound) = [false, true]
template <typename Pr>
void dichotomy(float &lower_bound, float &upper_bound, float spread, Pr& pr)
{
	upper_bound = lower_bound ;

//explode
	while (!pr(upper_bound))
		upper_bound *= 2 ;

//converge
	while (upper_bound - lower_bound > spread)
	{
		float middle = (lower_bound + upper_bound) / 2 ;
		if (pr(middle))
			upper_bound = middle ;
		else
			lower_bound = middle ;
	}
}


void compute_contexts(vector<MyRect> &rectangles,
			const vector<vector<MPD_Arc> > &adjacency_list,
			int max_nb_boxes_per_diagram,
			vector<Context> &contexts)
{
        FunctionTimer ft("compute_contexts");
	int n = rectangles.size() ;
	if (n==0)
		return ;
	MatrixXd W = MatrixXd::Zero(n, n) ;
	for (const auto& [i, j] : adjacency_list | views::join)
	{
		W(i, j) = W(j, i) = 1.0f ;
	}

//must be computed from unoriented graph
	vector<int> connected_component(n, -1) ;
	connected_components(compute_adjacency_list(W), connected_component) ;

	int nr_comp = 1 + *ranges::max_element(connected_component) ;
	vector<int> component_distribution(nr_comp, 0) ;
	for (int comp : connected_component)
		component_distribution[comp]++;

/*
Matrix P1 * OW * P1.transpose() or P1 * W * P1.transpose()
    n1      n2       n3
  +-----+------------------+
  |     |                   |
n1| cc1 |                   |
  +-----+-------+----------+    
  |     |       |           |
n2|     |  cc2  |           |
  |     +-------+----------+
  |     |       |           |
n3|     |       |  cc3      |
  |     |       |           |
  +-----+-------+----------+
*/
//to create a permutation matrix, permute the columns of the identity matrix.
	vector<int> permutation1(n) ;
	ranges::copy(views::iota(0,n), begin(permutation1)) ;
	ranges::sort(permutation1, {}, [&](int i){return connected_component[i];}) ;
	permutation1 = compute_reverse_permutation(permutation1) ;
	PermutationMatrix<Dynamic> perm1(n) ;
	ranges::copy(permutation1, perm1.indices().data()) ;

	Matrix<int8_t,-1,-1> OW = Matrix<int8_t,-1,-1>::Zero(n, n) ;	//Oriented Weights

	for (const auto& [i, j] : adjacency_list | views::join)
	{
		OW(i, j) = 1 ;
	}

	vector<int> fan_in(n, 0) ;
	for (const auto& [i, j] : adjacency_list | views::join)
	{
		fan_in[j] ++ ;
	}
	MatrixXd WW = MatrixXd::Zero(n, n) ;
	for (const auto& [i, j] : adjacency_list | views::join)
	{
		double value = 1.0f / fan_in[j] ;
		WW(i, j) = WW(j, i) = value ;
	}

	int n_acc = 0 ;

	for (int i=0; i < component_distribution.size(); i++)
	{
		int np = component_distribution[i] ;

		if (np > max_nb_boxes_per_diagram)
		{
//keep on cutting
			PermutationMatrix<Dynamic> perm2(n), perm3(np) ;
			perm2.setIdentity() ;
			vector<int> sub_component_distribution ;

			bool b = minimum_cut(
					(perm1 * WW * perm1.transpose()).block(n_acc, n_acc, np, np),
					perm3,
					sub_component_distribution
				) ;

			if (b)
			{
				transform(
					perm3.indices().data(),
					perm3.indices().data()+np,
					perm2.indices().data()+n_acc,
					[&](int pi){return pi+n_acc;}
				) ;
	//			perm2.block(n_acc, n_acc, np, np) = perm3 ;
	// if we want to apply P2 on P1*W*tP1 : 
	// P2*(P1* W* tP1)* tP2  or  (P2 * P1) * W * t(P2 * P1) so the new permutation is P2*P1
				perm1 = perm2 * perm1 ;
				component_distribution.erase(component_distribution.begin()+i) ;
				component_distribution.insert(component_distribution.begin()+i, 
											sub_component_distribution.begin(), 
											sub_component_distribution.end()) ;
	//re-initialize the loop : force it to make as many cuts as necessary
				i = -1 ;
				n_acc = - np ;
			}
		}

		n_acc += np ;
	}

	vector<MyRect> single_tables ;

	n_acc=0 ;
	for (int np : component_distribution)
	{
		vector<vector<MPD_Arc> > my_adjacency_list = compute_adjacency_list_( (perm1 * OW * perm1.transpose()).block(n_acc, n_acc, np, np)) ;
		vector<MyRect> my_rectangles(np) ;
/*
A * perm : permute columns
perm * A : permute rows
*/
		Map<MatrixXr>(my_rectangles.data(), np,1) = (perm1 * Map<MatrixXr>(rectangles.data(), n,1)).block(n_acc, 0, np, 1) ;

		if (np != 1)
		{
			Context ctx ;
			ctx.rectangles = my_rectangles ;
			ctx.adjacency_list = my_adjacency_list ;
			contexts.push_back(ctx) ;
		}
		else
		{
			MyRect &rect = my_rectangles[0] ;
			single_tables.push_back(rect) ;
		}

		n_acc += np ;
	}

	if (!single_tables.empty())
	{
		Context ctx ;
		ctx.rectangles = single_tables ;
		ctx.adjacency_list.resize(ctx.rectangles.size()) ;
		contexts.push_back(ctx) ;
	}

//C++11 range based for loop not compatible with openMP so far.
        #pragma omp parallel for
	for (int index=0; index < contexts.size(); index++)
        {
                Context &ctx = contexts[index];

		for (int i=0; i < ctx.rectangles.size(); i++)
			ctx.rectangles[i].i = i ;

		if ((ctx.adjacency_list | views::join).empty())
		{
			int w=0/*800*/, h=0/*600*/ ;
			binpack(ctx.rectangles, w, h) ;
		}
		else
		{
			stair_steps_layout(ctx.rectangles, ctx.adjacency_list, RECT_BORDER) ;
		}

		int rect_border = RECT_BORDER/2 ;
		int frame_border = RECT_BORDER*2 ;

		MyRect frame = compute_frame(ctx.rectangles) ;

		const int FRAME_MARGIN = 20; // consistent with diagload.js head of file.

// consistent with function encode_bounding_rectangle(context) in diagload.js bottom of file.

		expand_by(frame, FRAME_MARGIN) ;
		MyPoint translation = {-frame.m_left , -frame.m_top} ;
		translate(frame, translation) ;
		assert(frame.m_left == 0) ;
		assert(frame.m_top == 0) ;

		translation.x = FRAME_MARGIN/2 ;
		translation.y = FRAME_MARGIN/2 ;

		for (MyRect &rec : ctx.rectangles)
			translate(rec, translation) ;

		ctx.frame = frame ;
	}
}


void test_stair_steps_layout_from_111_boxes()
{
	TestFunctionTimer ft("test_stair_steps_layout_from_111_boxes");

        struct Result{MyRect frame; vector<TranslatedBox> translatedBoxes; };
        struct DataContext{string title; vector<MyRect> input_rectangles; vector<Edge> edges; vector<Result> expected_contexts;};
        const DataContext dctx = {
            .title="Coco",
            .input_rectangles={
		{0,134,0,88},
		{0,106,0,248},
		{0,110,0,136},
		{0,149,0,392},
		{0,112,0,104},
		{0,95,0,136},
		{0,131,0,184},
		{0,101,0,136},
		{0,101,0,136},
		{0,135,0,104},
		{0,56,0,40},
		{0,82,0,56},
		{0,116,0,104},
		{0,131,0,200},
		{0,131,0,280},
		{0,154,0,72},
		{0,125,0,88},
		{0,131,0,216},
		{0,124,0,88},
		{0,131,0,168},
		{0,109,0,296},
		{0,128,0,152},
		{0,128,0,72},
		{0,97,0,152},
		{0,101,0,72},
		{0,127,0,120},
		{0,101,0,40},
		{0,142,0,232},
		{0,112,0,120},
		{0,114,0,168},
		{0,112,0,104},
		{0,156,0,88},
		{0,131,0,376},
		{0,106,0,72},
		{0,106,0,136},
		{0,141,0,72},
		{0,140,0,264},
		{0,90,0,216},
		{0,122,0,152},
		{0,83,0,88},
		{0,86,0,88},
		{0,84,0,88},
		{0,84,0,120},
		{0,106,0,72},
		{0,164,0,56},
		{0,167,0,104},
		{0,140,0,216},
		{0,106,0,120},
		{0,143,0,120},
		{0,131,0,216},
		{0,97,0,120},
		{0,131,0,232},
		{0,131,0,184},
		{0,67,0,56},
		{0,77,0,88},
		{0,104,0,104},
		{0,63,0,72},
		{0,109,0,88},
		{0,147,0,88},
		{0,103,0,72},
		{0,101,0,88},
		{0,149,0,184},
		{0,106,0,120},
		{0,149,0,200},
		{0,83,0,120},
		{0,135,0,312},
		{0,119,0,88},
		{0,168,0,216},
		{0,114,0,72},
		{0,89,0,104},
		{0,75,0,72},
		{0,107,0,88},
		{0,146,0,248},
		{0,80,0,72},
		{0,107,0,88},
		{0,131,0,168},
		{0,128,0,88},
		{0,131,0,264},
		{0,93,0,72},
		{0,135,0,88},
		{0,80,0,88},
		{0,123,0,152},
		{0,95,0,104},
		{0,80,0,88},
		{0,125,0,72},
		{0,110,0,56},
		{0,95,0,72},
		{0,100,0,136},
		{0,147,0,120},
		{0,93,0,104},
		{0,125,0,120},
		{0,96,0,56},
		{0,101,0,120},
		{0,131,0,168},
		{0,147,0,472},
		{0,75,0,120},
		{0,97,0,88},
		{0,106,0,168},
		{0,115,0,56},
		{0,121,0,248},
		{0,118,0,88},
		{0,158,0,184},
		{0,131,0,216},
		{0,69,0,40},
		{0,100,0,88},
		{0,141,0,72},
		{0,91,0,72},
		{0,112,0,88},
		{0,117,0,56},
		{0,123,0,72},
		{0,114,0,136}
            },
	    .edges={
		{0,65},
		{1,34},
		{2,101},
		{3,1},
		{3,34},
		{3,41},
		{3,62},
		{3,65},
		{3,94},
		{3,101},
		{4,1},
		{4,32},
		{4,34},
		{6,1},
		{6,20},
		{6,23},
		{6,24},
		{6,25},
		{6,27},
		{6,46},
		{6,57},
		{6,65},
		{6,94},
		{6,95},
		{6,100},
		{6,101},
		{6,108},
		{7,8},
		{8,1},
		{8,6},
		{8,10},
		{8,20},
		{8,23},
		{8,24},
		{8,25},
		{8,30},
		{8,39},
		{8,40},
		{8,41},
		{8,48},
		{8,49},
		{8,53},
		{8,56},
		{8,57},
		{8,62},
		{8,65},
		{8,72},
		{8,73},
		{8,75},
		{8,77},
		{8,85},
		{8,88},
		{8,93},
		{8,94},
		{8,99},
		{8,100},
		{8,101},
		{8,102},
		{8,103},
		{8,108},
		{9,34},
		{9,94},
		{9,101},
		{11,65},
		{13,94},
		{13,99},
		{13,103},
		{14,17},
		{14,94},
		{15,14},
		{16,14},
		{17,94},
		{18,17},
		{19,94},
		{20,94},
		{21,21},
		{21,23},
		{21,95},
		{22,20},
		{22,94},
		{23,23},
		{23,94},
		{24,23},
		{25,29},
		{25,94},
		{27,29},
		{27,94},
		{29,25},
		{29,94},
		{29,95},
		{31,30},
		{32,32},
		{32,94},
		{32,101},
		{33,32},
		{33,94},
		{34,32},
		{35,34},
		{36,10},
		{36,20},
		{36,24},
		{36,26},
		{36,36},
		{36,40},
		{36,54},
		{36,56},
		{36,62},
		{36,65},
		{36,88},
		{36,94},
		{36,100},
		{36,101},
		{37,32},
		{37,101},
		{38,94},
		{38,101},
		{39,95},
		{41,34},
		{43,93},
		{43,94},
		{45,1},
		{45,44},
		{45,65},
		{45,99},
		{46,1},
		{46,10},
		{46,11},
		{46,23},
		{46,24},
		{46,25},
		{46,26},
		{46,39},
		{46,41},
		{46,48},
		{46,53},
		{46,54},
		{46,56},
		{46,57},
		{46,62},
		{46,73},
		{46,85},
		{46,88},
		{46,94},
		{46,99},
		{46,100},
		{46,101},
		{46,108},
		{47,1},
		{47,62},
		{47,65},
		{48,95},
		{49,94},
		{51,1},
		{51,23},
		{51,24},
		{51,25},
		{51,88},
		{51,94},
		{51,99},
		{51,101},
		{51,108},
		{52,49},
		{52,94},
		{55,77},
		{58,57},
		{60,13},
		{61,62},
		{62,34},
		{62,102},
		{63,62},
		{64,94},
		{64,101},
		{65,36},
		{65,65},
		{65,94},
		{66,65},
		{67,65},
		{67,94},
		{68,65},
		{68,94},
		{69,1},
		{69,23},
		{69,25},
		{69,56},
		{69,65},
		{69,85},
		{69,94},
		{69,99},
		{69,108},
		{70,70},
		{71,70},
		{72,1},
		{72,23},
		{72,24},
		{72,25},
		{72,39},
		{72,56},
		{72,62},
		{72,85},
		{72,94},
		{72,99},
		{72,101},
		{72,108},
		{74,73},
		{75,94},
		{76,75},
		{77,65},
		{77,94},
		{78,65},
		{78,77},
		{79,65},
		{79,77},
		{80,65},
		{80,77},
		{80,93},
		{81,55},
		{81,77},
		{82,77},
		{82,88},
		{83,65},
		{83,77},
		{83,94},
		{86,88},
		{87,85},
		{87,86},
		{88,30},
		{89,94},
		{89,101},
		{90,89},
		{92,1},
		{92,20},
		{92,25},
		{92,27},
		{92,56},
		{92,94},
		{92,98},
		{92,99},
		{92,100},
		{92,101},
		{92,103},
		{92,108},
		{93,94},
		{94,94},
		{94,95},
		{97,65},
		{97,77},
		{97,94},
		{101,102},
		{102,65},
		{102,94},
		{104,32},
		{104,34},
		{105,32},
		{105,104},
		{106,77},
		{106,104},
		{107,94},
		{108,110},
		{109,108},
		{110,94},
		{110,95},
		{110,108}
            },
            .expected_contexts={
		{
			.frame={.m_left=0,.m_right=986,.m_top=0,.m_bottom=992},
			.translatedBoxes={
			{.id=94, .translation={.x=608,.y=218}},
			{.id=90, .translation={.x=649,.y=842}},
			{.id=33, .translation={.x=649,.y=730}},
			{.id=29, .translation={.x=795,.y=521}},
			{.id=49, .translation={.x=437,.y=250}},
			{.id=107, .translation={.x=794,.y=57}},
			{.id=75, .translation={.x=437,.y=42}},
			{.id=110, .translation={.x=795,.y=185}},
			{.id=76, .translation={.x=269,.y=123}},
			{.id=14, .translation={.x=10,.y=650}},
			{.id=27, .translation={.x=814,.y=729}},
			{.id=15, .translation={.x=181,.y=858}},
			{.id=89, .translation={.x=516,.y=730}},
			{.id=95, .translation={.x=795,.y=361}},
			{.id=19, .translation={.x=608,.y=10}},
			{.id=18, .translation={.x=352,.y=730}},
			{.id=17, .translation={.x=181,.y=602}},
			{.id=16, .translation={.x=16,.y=522}},
			{.id=52, .translation={.x=437,.y=506}}
			}
		},
		{
			.frame={.m_left=0,.m_right=638,.m_top=0,.m_bottom=640},
			.translatedBoxes={
			{.id=86, .translation={.x=221,.y=10}},
			{.id=108, .translation={.x=221,.y=122}},
			{.id=85, .translation={.x=99,.y=218}},
			{.id=25, .translation={.x=481,.y=458}},
			{.id=72, .translation={.x=378,.y=170}},
			{.id=69, .translation={.x=249,.y=218}},
			{.id=56, .translation={.x=378,.y=458}},
			{.id=44, .translation={.x=10,.y=473}},
			{.id=109, .translation={.x=378,.y=58}},
			{.id=99, .translation={.x=217,.y=362}},
			{.id=87, .translation={.x=81,.y=10}},
			{.id=45, .translation={.x=10,.y=329}}
			}
		},
		{
			.frame={.m_left=0,.m_right=730,.m_top=0,.m_bottom=704},
			.translatedBoxes={
			{.id=60, .translation={.x=282,.y=10}},
			{.id=13, .translation={.x=81,.y=138}},
			{.id=80, .translation={.x=10,.y=586}},
			{.id=22, .translation={.x=572,.y=202}},
			{.id=98, .translation={.x=572,.y=314}},
			{.id=8, .translation={.x=290,.y=378}},
			{.id=7, .translation={.x=431,.y=538}},
			{.id=6, .translation={.x=252,.y=154}},
			{.id=103, .translation={.x=181,.y=378}},
			{.id=43, .translation={.x=130,.y=586}},
			{.id=100, .translation={.x=431,.y=410}},
			{.id=20, .translation={.x=423,.y=42}},
			{.id=93, .translation={.x=10,.y=378}},
			{.id=92, .translation={.x=589,.y=410}}
			}
		},
		{
			.frame={.m_left=0,.m_right=662,.m_top=0,.m_bottom=752},
			.translatedBoxes={
			{.id=41, .translation={.x=520,.y=314}},
			{.id=3, .translation={.x=331,.y=10}},
			{.id=1, .translation={.x=526,.y=442}},
			{.id=38, .translation={.x=10,.y=26}},
			{.id=64, .translation={.x=10,.y=218}},
			{.id=37, .translation={.x=11,.y=442}},
			{.id=4, .translation={.x=374,.y=618}},
			{.id=101, .translation={.x=133,.y=218}},
			{.id=34, .translation={.x=374,.y=442}},
			{.id=35, .translation={.x=193,.y=586}},
			{.id=9, .translation={.x=191,.y=442}},
			{.id=2, .translation={.x=181,.y=42}}
			}
		},
		{
			.frame={.m_left=0,.m_right=718,.m_top=0,.m_bottom=640},
			.translatedBoxes={
			{.id=65, .translation={.x=345,.y=266}},
			{.id=67, .translation={.x=520,.y=394}},
			{.id=68, .translation={.x=370,.y=154}},
			{.id=47, .translation={.x=199,.y=266}},
			{.id=62, .translation={.x=53,.y=234}},
			{.id=61, .translation={.x=10,.y=10}},
			{.id=102, .translation={.x=199,.y=10}},
			{.id=66, .translation={.x=520,.y=266}},
			{.id=0, .translation={.x=524,.y=138}},
			{.id=63, .translation={.x=10,.y=394}}
			}
		},
		{
			.frame={.m_left=0,.m_right=536,.m_top=0,.m_bottom=720},
			.translatedBoxes={
			{.id=54, .translation={.x=222,.y=474}},
			{.id=10, .translation={.x=107,.y=346}},
			{.id=11, .translation={.x=81,.y=250}},
			{.id=40, .translation={.x=222,.y=602}},
			{.id=53, .translation={.x=383,.y=266}},
			{.id=58, .translation={.x=345,.y=10}},
			{.id=48, .translation={.x=363,.y=394}},
			{.id=57, .translation={.x=383,.y=138}},
			{.id=74, .translation={.x=10,.y=10}},
			{.id=73, .translation={.x=83,.y=138}},
			{.id=46, .translation={.x=203,.y=138}},
			{.id=39, .translation={.x=222,.y=10}},
			{.id=36, .translation={.x=42,.y=426}},
			{.id=26, .translation={.x=222,.y=394}}
			}
		},
		{
			.frame={.m_left=0,.m_right=510,.m_top=0,.m_bottom=480},
			.translatedBoxes={
			{.id=51, .translation={.x=10,.y=202}},
			{.id=24, .translation={.x=10,.y=90}},
			{.id=21, .translation={.x=288,.y=10}},
			{.id=88, .translation={.x=181,.y=330}},
			{.id=31, .translation={.x=181,.y=202}},
			{.id=82, .translation={.x=377,.y=202}},
			{.id=30, .translation={.x=368,.y=346}},
			{.id=23, .translation={.x=151,.y=10}}
			}
		},
		{
			.frame={.m_left=0,.m_right=515,.m_top=0,.m_bottom=688},
			.translatedBoxes={
			{.id=79, .translation={.x=344,.y=42}},
			{.id=83, .translation={.x=53,.y=330}},
			{.id=97, .translation={.x=27,.y=458}},
			{.id=55, .translation={.x=173,.y=10}},
			{.id=78, .translation={.x=40,.y=218}},
			{.id=106, .translation={.x=213,.y=458}},
			{.id=32, .translation={.x=344,.y=170}},
			{.id=104, .translation={.x=204,.y=570}},
			{.id=77, .translation={.x=173,.y=154}},
			{.id=81, .translation={.x=10,.y=10}},
			{.id=105, .translation={.x=344,.y=586}}
			}
		},
		{
			.frame={.m_left=0,.m_right=147,.m_top=0,.m_bottom=240},
			.translatedBoxes={
			{.id=71, .translation={.x=10,.y=10}},
			{.id=70, .translation={.x=10,.y=138}}
			}
		},
		{
			.frame={.m_left=0,.m_right=344,.m_top=0,.m_bottom=440},
			.translatedBoxes={
			{.id=5, .translation={.x=122,.y=10}},
			{.id=12, .translation={.x=10,.y=146}},
			{.id=28, .translation={.x=10,.y=10}},
			{.id=42, .translation={.x=217,.y=130}},
			{.id=50, .translation={.x=217,.y=10}},
			{.id=59, .translation={.x=135,.y=250}},
			{.id=84, .translation={.x=10,.y=250}},
			{.id=91, .translation={.x=107,.y=322}},
			{.id=96, .translation={.x=10,.y=322}}
			}
		}
}
	} ;

	const auto& [title, input_rectangles, edges, expected_contexts] = dctx;
	vector<MyRect> rectangles = input_rectangles;

        int i=0;
        for (MyRect& r : rectangles)
        {
            r.i = r.no_sequence = i++;
        }

        int n = rectangles.size();
        vector<vector<MPD_Arc> > adjacency_list(n);

        for (const Edge& e : edges)
        {
            adjacency_list[e.from].push_back({e.from, e.to}) ;
        }

        vector<Context> contexts ;
        compute_contexts(rectangles, adjacency_list, max_nb_boxes_per_diagram, contexts) ;

	char buffer[100000];
	write_json(rectangles, contexts, buffer);
        FILE *f = fopen("test-latuile-101boxes-output-contexts.json", "w");
        fprintf(f, "%s", buffer);
        fclose(f);
	int testid=1;
	json_diagdata_output(n, edges, "test-latuile-101boxes-diagdata.json", testid);

        int c=0;
        for (Context &ctx : contexts)
        {
            vector<TranslatedBox> translatedBoxes;
            for (MyRect &r : ctx.rectangles)
                translatedBoxes.push_back({r.no_sequence,{r.m_left, r.m_top}});
	    bool bOK = translatedBoxes == expected_contexts[c++].translatedBoxes;
            printf("%s\n", bOK ? "OK" : "KO");
	    (bOK ? nbOK : nbKO)++;
        }
}



vector<vector<MPD_Arc> > compute_adjacency_list_(const Matrix<int8_t,-1,-1>& OW)
{
	assert(OW.rows() == OW.cols()) ;

	int n = OW.rows() ;

	vector<vector<MPD_Arc> > adjacency_list(n) ;

	for (int i=0; i < n; i++)
	{
		for (int j=0; j < n; j++)
		{
                        if (OW(i,j) == 0)
				continue ;
			adjacency_list[i].push_back(MPD_Arc{i,j}) ;
		}
	}

	return adjacency_list ;
}


vector<vector<MPD_Arc> > compute_adjacency_list(const MatrixXd& OW)
{
	assert(OW.rows() == OW.cols()) ;

	int n = OW.rows() ;

	vector<vector<MPD_Arc> > adjacency_list(n) ;

	for (int i=0; i < n; i++)
	{
		for (int j=0; j < n; j++)
		{
			double val = OW(i,j) ;
			if (OW(i,j) == 0.0f)
				continue ;
			MPD_Arc a ;
			a._i = i ;
			a._j = j ;
			adjacency_list[a._i].push_back(a) ;
		}
	}

	return adjacency_list ;
}


//input: (P1 * W * P1.transpose()).block(0, 0, np, np)
//output: P2, component_distribution
bool minimum_cut(const MatrixXd& W, 
				 PermutationMatrix<Dynamic>& perm2, 
				 vector<int> &component_distribution)
{
#ifdef _DEBUG
	ostringstream buffer ;
	buffer << W ;
#endif

	int n = W.rows() ;

	MatrixXd D = W.rowwise().sum().asDiagonal() ;
// Ulrike von Luxburg : we thus advocate for using Lrw (Laplacien randow walk).
	MatrixXd Lrw = D.inverse() * (D - W) ;

	EigenSolver<MatrixXd> es(Lrw) ;
	VectorXd ev = es.eigenvalues().real() ;
	MatrixXd V = es.eigenvectors().real() ;

	std::vector<double*> evp(n) ;
	transform(ev.data(), ev.data()+n, evp.data(), [](double& val){return &val;}) ; 
	ranges::sort(evp, {}, [](double *p){return *p;}) ;
/*
non null eigenvalues => each corresponds to a cut.
*/
	const static double epsilon = pow(10,-6) ;
	vector<int> cut_indexes = index_if(evp, [=](double *p){return *p > epsilon;}) ;
	cut_indexes.push_back(0) ;
	double min_Ncut = INT_MAX ;
	int n1, n2 ;

	for (int pos : cut_indexes)
	{
		int column = evp[pos] - &ev[0] ;
		VectorXd fiedler_vector = V.col(column) ;
		std::vector<double> fv(fiedler_vector.data(), fiedler_vector.data()+n) ;
		int DD=1, K=2, Niter = 100, seed = 14567437496 ;
		const char *initname = "random" ;//either "random" or "plusplus"
		vector<double> Mu_OUT(K), Z_OUT(n) ;
		RunKMeans(fiedler_vector.data(), n, DD, K, Niter, seed, initname, &Mu_OUT[0], &Z_OUT[0]);
		n1 = ranges::count(Z_OUT, 1.0) ;
		n2 = ranges::count(Z_OUT, 0.0) ;

		if (n1 == 0 || n2 == 0)
			return false ;

//if there are small connected components as side effect of the cut, move them to the other side where they
//might be connected.
		vector<int> cc(n) ;
		vector<vector<MPD_Arc> > adj(n), adj_ = compute_adjacency_list(W) ;
		for (const auto& [i, j] : adj_ | views::join)
		{
			if (Z_OUT[i] != Z_OUT[j])
				continue ;
			adj[i].push_back({i, j}) ;
		}
		connected_components(adj, cc) ;
		int nr_comp = 1 + *ranges::max_element(cc) ;
		vector<int> distribution(nr_comp, 0) ;
		for (int comp : cc)
			distribution[comp]++ ;
		vector<int> component(nr_comp) ;
		for (int comp=0; comp < nr_comp; comp++)
			component[comp] = comp ;
		ranges::sort(component, {}, [&](int comp){return distribution[comp]; }) ;
		if (component.size() < 2)
			return false ;
		component.pop_back() ;
		component.pop_back() ;
		for (int comp : component)
		{
			for (int i=0 ; i < n ; i++)
			{
				if (cc[i] != comp)
					continue ;
				Z_OUT[i] = 1 - Z_OUT[i] ;
			}
		}
		n1 = ranges::count(Z_OUT, 1.0) ;
		n2 = ranges::count(Z_OUT, 0.0) ;

		adj = vector<vector<MPD_Arc> >(n) ;
		cc = vector<int>(n,0) ;
		for (const auto& [i, j] : adj_ | views::join)
		{
			if (Z_OUT[i] != Z_OUT[j])
				continue ;
			adj[i].push_back({i, j}) ;
		}
		connected_components(adj, cc) ;
		nr_comp = 1 + *ranges::max_element(cc) ;

//to create a permutation matrix, permute the columns of the identity matrix
		vector<int> permutation2(n) ;
		ranges::copy(views::iota(0,n), begin(permutation2)) ;
		ranges::sort(permutation2, ranges::greater(), [&](int i){return Z_OUT[i];}) ;
		permutation2 = compute_reverse_permutation(permutation2) ;
		ranges::copy(permutation2, perm2.indices().data()) ;
/*
		P2 = MatrixXd::Zero(n,n) ;
		for (int i=0 ; i < n; i++)
		{
			P2(permutation2[i], i) = 1.0f ;
		}
*/
//a partir de P1, P2 et Z_OUT2 on peut retrouver les 2 ensembles dans le referentiel de depart.

		double intra2[2] ;
/*
     n1           n2
  +-----+------------------+
  |     |                   |
n1|  A  |        B          |
  +-----+------------------+    
  |     |                   |
  |     |                   |
n2|  C  |        D          |
  |     |                   |
  |     |                   |
  |     |                   |
  +-----+------------------+
*/
		intra2[0] = (perm2 * W * perm2.transpose()).block(0, 0, n1, n1).sum() ;//A
		intra2[1] = (perm2 * W * perm2.transpose()).block(n1, n1, n2, n2).sum() ;//D
		//cut = B + C
		double cut = (perm2 * W * perm2.transpose()).block(0, n1, n1, n2).sum() + (perm2 * W * perm2.transpose()).block(n1, 0, n2, n1).sum() ;

//critere de qualité pour choisir la meilleure cut - Cf Ulrike von Luxburg paragraph 5
		double Ncut = cut / intra2[0] + cut / intra2[1] ; 
//penalty to make a small n1 (resp. n2) be taken into account as being added to nr_comp.
//goal is to make small asymmetric cut less attractive.
		int penalty = 0 ;
		if (n1*4 <= n)
			penalty+= abs(n-2*n1) ;
		if (n2*4 <= n)
			penalty+= abs(n-2*n2) ;
		Ncut = 1.0/(1.0+n1) + 1.0/(1.0+n2) + 1.0*(nr_comp+penalty)/(1.0+n)  ;
		if (Ncut < min_Ncut)
		{
			min_Ncut = Ncut ;
//play again the best at loop end.
			cut_indexes.back() = pos ;
		}
	}

	if (n1==0 || n2==0)
		return false ;

	vector<int> cc1(n1), cc2(n2) ;
	connected_components(compute_adjacency_list( (perm2 * W * perm2.transpose()).block(0, 0, n1, n1) ),
						  cc1) ;
	connected_components(compute_adjacency_list( (perm2 * W * perm2.transpose()).block(n1, n1, n2, n2) ),
						  cc2) ;
	for (int &comp : cc2)
		comp += 1 + *ranges::max_element(cc1) ;
	vector<int> connected_component ;
	ranges::copy(cc1, back_inserter(connected_component)) ;
	ranges::copy(cc2, back_inserter(connected_component)) ;
	int nr_comp = 1 + *ranges::max_element(connected_component) ;
	component_distribution = vector<int>(nr_comp, 0) ;
	for (int comp : connected_component)
		component_distribution[comp]++;

	vector<int> permutation(n) ;
	ranges::copy(views::iota(0,n), begin(permutation)) ;
	ranges::sort(permutation, {}, [&](int i){return connected_component[i];}) ;
	permutation = compute_reverse_permutation(permutation) ;
	PermutationMatrix<Dynamic> perm1(n) ;
	ranges::copy(permutation, perm1.indices().data()) ;
/*
//output: P1, component_distribution
	MatrixXd P1 = MatrixXd::Zero(n,n) ;
	for (int i=0 ; i < n; i++)
	{
		P1(permutation[i], i) = 1.0f ;
	}
*/
// if we want to apply P1 on P2*W*tP1 : 
// P1*(P2* W* tP2)* tP1  or  (P1 * P2) * W * t(P1 * P2) so the new permutation is P1*P2
	perm2 = perm1 * perm2 ;

	return true ;
}


//must be computed from unoriented graph
void connected_components(const vector<vector<MPD_Arc> >& adjacency_list,
						  vector<int>& connected_component)
{
	int n = adjacency_list.size() ;
	vector<bool> visited(n, false) ;
	int comp=0 ;
	for (int i=0; i < adjacency_list.size(); i++)
	{
		if (visited[i])
			continue ;
		stack<int> Q ;
		Q.push(i) ;
		while (!Q.empty())
		{
			int ii = Q.top() ;
			Q.pop() ;
			if (visited[ii])
				continue ;
			connected_component[ii] = comp ;
			visited[ii] = true ;

			for (int k=0; k < adjacency_list[ii].size(); k++)
			{
				int j=adjacency_list[ii][k]._j ;
				if (!visited[j])
					Q.push(j) ;
			}
		}
		comp++ ;
	}
}
