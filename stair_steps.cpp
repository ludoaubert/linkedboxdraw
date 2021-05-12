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
#include <numeric>
#include <random>
#include <fstream>
#include <assert.h>
#include <chrono>
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <omp.h>
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


template <class Container>
std::string implode(Container c, std::string e)
{
	std::ostringstream os;
	typename Container::iterator it = c.begin();
	if (it != c.end())
		os << *it++;
	for (; it != c.end(); it++) {
		os << e << *it;
	}
	return os.str();
}



bool stair_steps(vector<MyRect> &rectangles, MyRect& rr, vector<vector<MPD_Arc> > &adjacency_list)
{
        FunctionTimer ft("stair_steps");		
	int n = rectangles.size() ;

	vector<vector<MyRect*> > unordered_adjacency_list(n) ;
	for (const MPD_Arc *edge : list_edges(adjacency_list))
	{
		if (edge->_i == edge->_j)
			continue ;
		unordered_adjacency_list[edge->_i].push_back(&rectangles[edge->_j]) ;
		unordered_adjacency_list[edge->_j].push_back(&rectangles[edge->_i]) ;
	}
//ORDER each adjacency list BY rectangle width DESC. This is to make sure that lower steps of the stairway are larger.
	for (vector<MyRect*>& adj : unordered_adjacency_list)
		sort(adj.begin(), adj.end(), [&](MyRect* ri, MyRect* rj){return width(*ri) > width(*rj) ;}) ;

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
	int not_selected = count_if(rects.begin(), rects.end(), [](const WidgetContext& widget){return widget.r.selected==false ;}) ;
	int selected = rects.size() - not_selected ;

//1) calcule de la matrice de permutation
	vector<int> permutation(n) ;
	iota(permutation.begin(), permutation.end(), 0) ;
//on met les selectionnes a droite.
	sort(permutation.begin(), permutation.end(), [&](int i,int j){return rects[i].r.selected < rects[j].r.selected; }) ;
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
	for (MPD_Arc* arc : list_edges(adjacency_list))
	{
		int i = arc->_i, j = arc->_j ;
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

		while(int not_selected = count_if(rects.begin(), rects.end(), [](const WidgetContext& widget){return widget.r.selected==false ;}))
		{
			int selected = rects.size() - not_selected ;
			int n = rects.size() ;
			MatrixXd W = MatrixXd::Zero(n,n) ;
			PermutationMatrix<Dynamic> perm(n) ;
			for (const MPD_Arc* arc : list_edges(adjacency_list))
			{
				int i = arc->_i ;
				int j = arc->_j ;
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

	auto it = min_element(solutions.begin(), solutions.end(), [](vector<MyRect>& rectangles1, vector<MyRect>& rectangles2){
		return dim_max(compute_frame(rectangles1)) < dim_max(compute_frame(rectangles2)); 
	}) ;
	int index = distance(solutions.begin(), it) ;
	rectangles = * it ;
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
	vector<MPD_Arc*> edges = list_edges(adjacency_list) ;
	vector<vector<int> > unoriented_adjacency_list(n) ;
	for (MPD_Arc* edge : edges)
	{
		if (edge->_i == edge->_j)
			continue ;
		unoriented_adjacency_list[edge->_i].push_back(edge->_j) ;
		unoriented_adjacency_list[edge->_j].push_back(edge->_i) ;
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

	auto it = min_element(solutions.begin(),
						solutions.end(), 
						[](const vector<MyRect>& rectangles1, const vector<MyRect>& rectangles2){
							return dim_max(compute_frame(rectangles1)) < dim_max(compute_frame(rectangles2)) ;                                                                                                      
						}
	) ;
	int index = distance(solutions.begin(), it) ;
	rectangles = *it ;
	MyRect frame = compute_frame(rectangles) ;
	for (MyRect &r : rectangles)
		translate(r, {-frame.m_left, -frame.m_top}) ;
	return true ;
}


template <unsigned N>
void make_adjacency_list(int (&edges)[N][2], vector<vector<MPD_Arc> >& adjacency_list)
{
	for (int (&edge)[2] : edges)
	{
		adjacency_list[edge[0]].push_back(MPD_Arc(edge[0], edge[1])) ;
	}
}


void write_json(const vector<Context>& contexts)
{
	printf("{\"contexts\":[");

	for (const Context& ctx : contexts)
	{
		printf("{\"title\":\"%s\",", ctx.title.c_str());
		printf("\"frame\":{\"left\":%d,\"right\":%d,\"top\":%d,\"bottom\":%d},", 0, width(ctx.frame), 0, height(ctx.frame));

		printf("\"translatedBoxes\":[");

		for (const MyRect& r : ctx.rectangles)
		{
			assert(r.m_left < r.m_right);
			assert(r.m_top < r.m_bottom);
			printf("{\"id\":%d, \"translation\":{\"x\":%d,\"y\":%d}}%c", r.no_sequence, r.m_left, r.m_top,
                              &r == &ctx.rectangles.back() ? ' ' : ',');
		}

		printf("]");
		printf("}%c", &ctx == &contexts.back() ? ' ' : ',');
	}

	printf("]}");
}

void test_stair_steps(int rect_border)
{
        TestFunctionTimer ft("test_stair_steps");

        struct Edge{int from,to;};
        struct DataContext{string title; vector<MyRect> rectangles; vector<Edge> edges; vector<TranslatedBox> expected_translations; MyRect frame;};
        const vector<DataContext> vdctx = {
                {
                    "My first SVG",
                    {
                        {0, 50, 0, 60},
                        {0, 50, 0, 120},
                        {0, 40, 0, 40},
                        {0, 60, 0, 50},
                        {0, 30, 0, 40}
                    },
                    {
                        {0, 2},
                        {0, 1},
                        {1, 4},
                        {1, 3}
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
                    "RANELITEG/1",
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
			{0, 1},
			{1, 2},
			{1, 3},
			{1, 4},
			{5, 1},
			{6, 1},
			{7, 0},
			{8, 1}
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
		    "RANELITEG/2",
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
			{0, 3},
			{0, 4},
			{0, 5},
			{1, 0},
			{1, 6},
			{2, 0}
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
		    "SINITAX/1",
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
			{0, 2},
			{0, 4},
			{1, 0},
			{3, 0},
			{3, 1},
			{3, 2},
			{3, 4},
			{3, 7},
			{4, 2},
			{6, 3},
			{6, 5}
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
		    "SINITAX/2",
		    {
			{0, 102, 0, 104},
			{0, 88, 0, 104},
			{0, 69, 0, 72},
			{0, 78, 0, 88},
			{0, 88, 0, 88},
			{0, 69, 0, 72}
		    },
		    {
			{0, 5},
			{1, 1},
			{1, 3},
			{2, 0},
			{2, 1},
			{4, 0},
			{4, 3}
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
		    "SINITAX/3",
		    {
			{0, 58, 0, 72},
			{0, 78, 0, 88},
			{0, 77, 0, 136}
		    },
		    {
			{0, 2},
			{1, 0}
		    },
                    {
                        {0,{20,0}},
                        {1,{0,112}},
                        {2,{118,64}}
                    },
                    /*frame*/{0,195,0,200}
                },
                {
		    "SAKILA/1",
		    {
			{0, 92, 0, 168},
			{0, 93, 0, 88},
			{0, 93, 0, 136},
			{0, 92, 0, 136},
			{0, 83, 0, 200}
		    },
		    {
			{2, 0},
			{2, 3},
			{2, 4},
			{3, 0},
			{3, 1},
			{3, 4}
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
		    "SAKILA/2",
                    {
			{0, 84, 0, 152},
			{0, 89, 0, 168},
			{0, 111, 0, 312},
			{0, 109, 0, 88}
		    },
		    {
			{0, 1},
			{1, 2},
			{3, 0}
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
                    "SAKILA/3",
                    {
			{0, 80, 0, 88},
			{0, 89, 0, 72},
			{0, 136, 0, 232},
			{0, 89, 0, 72},
			{0, 105, 0, 72},
			{0, 91, 0, 72}
		    },
                    {
			{2, 5},
			{3, 0},
			{3, 2},
			{4, 1},
			{4, 2}
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
                    "COCOGIRL/1",
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
			{0, 1},
			{1, 2},
			{1, 3},
			{1, 5},
			{1, 7},
			{1, 9},
			{4, 3},
			{6, 5},
			{8, 7}
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
                    "COCOGIRL/2",
                    {
			{0, 127, 0, 120},
			{0, 142, 0, 232},
			{0, 114, 0, 168},
			{0, 63, 0, 72},
			{0, 89, 0, 104},
			{0, 146, 0, 248}
		    },
                    {
			{0, 2},
			{1, 2},
			{4, 0},
			{4, 3},
			{5, 0},
			{5, 3}
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
                }
	};

        vector<Context> contexts;
        for (const DataContext &dctx : vdctx)
        {
            Context ctx ;
            ctx.title = dctx.title;
            ctx.rectangles = dctx.rectangles;
            int n = ctx.rectangles.size();
            ctx.adjacency_list.resize(n) ;

            for (const Edge& e : dctx.edges)
            {
                MPD_Arc edge;
                edge._i = e.from;
                edge._j = e.to;
                assert(edge._i < n);
                assert(edge._j < n);
                ctx.adjacency_list[edge._i].push_back(edge) ;
            }

            contexts.push_back(ctx);
        } 

        int c=0;
	for (Context &ctx : contexts)
	{
                high_resolution_clock::time_point t1 = high_resolution_clock::now();

                int i=0;
		for (MyRect& r : ctx.rectangles)
                {
                    r.i = r.no_sequence = i++;
	        }
	
		for (MyRect &r : ctx.rectangles)
		{
			r.m_right += 2*rect_border ;
			r.m_bottom += 2*rect_border ;
		}
		stair_steps(ctx.rectangles, ctx.adjacency_list) ;
		for (MyRect &r : ctx.rectangles)
		{
			expand_by(r, - rect_border) ;
		}

		MyRect frame = compute_frame(ctx.rectangles) ;
		for (MyRect &r : ctx.rectangles)
		{
			translate(r, {- frame.m_left,- frame.m_top}) ;
		}
		ctx.frame = compute_frame(ctx.rectangles) ;

                sort(begin(ctx.rectangles), end(ctx.rectangles), [](MyRect &r1, MyRect &r2){return r1.no_sequence < r2.no_sequence;});
                vector<TranslatedBox> translations;
                for (MyRect &r : ctx.rectangles)
                    translations.push_back({r.no_sequence,{r.m_left, r.m_top}});
                const DataContext& dctx = vdctx[c++];
                duration<double> time_span = high_resolution_clock::now() - t1;
                printf("%s %20s %f seconds elapsed\n", dctx.expected_translations == translations ? "OK": "KO", dctx.title.c_str(), time_span.count());
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
		for (const MPD_Arc* arc : list_edges(adjacency_list))
		{
			int i = arc->_i ;
			int j = arc->_j ;
			W(i,j) = W(j,i) = 1 ;
		}
		Matrix<int8_t,-1,-1> OW = Matrix<int8_t,-1,-1>::Zero(n, n) ;	//Oriented Weights
		for (const MPD_Arc* arc : list_edges(adjacency_list))
		{
			int i = arc->_i, j = arc->_j ;
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
		assert(equal(vec.begin(), vec.end(), vect.begin(), [](MyRect& r1, MyRect& r2){return dimensions(r1)==dimensions(r2);})) ;
		int total_distance = 0 ;
		for (const MPD_Arc *edge : list_edges(adjacency_list))
		{
			total_distance += rectangle_distance(vec[edge->_i], vec[edge->_j]) ;
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

	vector<tuple<int, RectCorner, int, RectCorner> > swaps ;
	do
	{
		swaps.clear() ;
		swap_rectangles(vec, list_edges(adjacency_list), swaps) ;
	}
	while (!swaps.empty()) ;

	while (compact_rectangles(vec, adjacency_list)) ;

//call again after the calls to compact_rectangles()
	do
	{
		swaps.clear() ;
		swap_rectangles(vec, list_edges(adjacency_list), swaps) ;
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
	assert(equal(vec.begin(), vec.end(), vect.begin(), [](MyRect& r1, MyRect& r2){return dimensions(r1)==dimensions(r2);})) ;
	vect = vec ;
}


void test_stair_steps_layout()
{
	TestFunctionTimer ft("test_stair_steps_layout");

        vector<Context> contexts ;
        
        struct Edge{int from,to;};
        struct DataContext{string title; vector<MyRect> rectangles; vector<Edge> edges; vector<TranslatedBox> expected_translations; MyRect frame;};
        const vector<DataContext> vdctx = {
	        {
		    "RANELITEG",
                    {
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
                    {
			{0, 1},
			{0, 15},
			{1, 4},
			{1, 5},
			{1, 7},
			{2, 8},
			{2, 9},
			{2, 14},
			{3, 2},
			{3, 15},
			{6, 1},
			{6, 2},
			{10, 1},
			{11, 1},
			{12, 0},
			{13, 1}
		    },
                    {
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
                    /*frame*/{0,738,0,584}
                },
                {
		    "SAKILA",
                    {
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
                    {
			{1, 3},
			{3, 4},
			{5, 1},
			{5, 14},
			{6, 10},
			{7, 0},
			{7, 6},
			{8, 2},
			{8, 6},
			{9, 6},
			{9, 14},
			{11, 5},
			{11, 12},
			{11, 13},
			{12, 5},
			{12, 9},
			{12, 13},
			{13, 1},
			{13, 14},
			{14, 1}
		    },
                      {
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
                    /*frame*/{0,713,0,728}  
                },
                {
		    "SINITAX",
                    {
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
                    {
			{0, 11},
			{0, 15},
			{1, 13},
			{2, 7},
			{3, 0},
			{3, 2},
			{4, 6},
			{4, 12},
			{5, 4},
			{7, 13},
			{8, 4},
			{8, 5},
			{8, 6},
			{8, 12},
			{8, 16},
			{9, 0},
			{9, 7},
			{10, 0},
			{10, 1},
			{12, 4},
			{12, 6},
			{13, 6},
			{15, 8},
			{15, 11},
			{15, 13},
			{15, 14}
		    },
                    {
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
                    /*frame*/{0,547,0,664}
                },
                {
                    "COCOGIRL/1",
                    {
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
                    {
			{0, 4},
			{0, 6},
			{0, 8},
			{4, 1},
			{4, 2},
			{4, 3},
			{4, 5},
			{4, 6},
			{4, 8},
			{7, 6}
		    },
                    {
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
                    /*frame*/{0,466,0,392}
                },
                {
		    "COCOGIRL/2",
                    {
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
                    {
			{0, 9},
			{1, 9},
			{2, 8},
			{3, 9},
			{4, 9},
			{5, 9},
			{5, 10},
			{6, 9},
			{7, 6},
			{10, 12},
			{11, 10},
			{12, 8},
			{12, 10}
		    },
                    {
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
                    /*frame*/{0,802,0,616}
                },
                {
                    "COCOGIRL/3",
                    {
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
                    {
			{0, 3},
			{0, 9},
			{1, 0},
			{2, 0},
			{3, 9},
			{4, 3},
			{5, 9},
			{6, 9},
			{7, 9},
			{8, 9},
			{9, 9},
			{10, 9}
		    },
                    {
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
                    /*frame*/{0,683,0,680}
                },
                {
                    "COCOGIRL/4",
                    {
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

		    {
			{0, 3},
			{1, 3},
			{2, 3},
			{4, 3},
			{5, 3},
			{6, 3},
			{7, 3},
			{8, 3},
			{9, 3},
			{10, 3},
			{11, 3},
			{12, 3}
		    },
                    {
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
                    /*frame*/{0,648,0,648}
                },
                {
		    "COCOGIRL/5",
                    {
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
                    {
			{0, 8},
			{1, 0},
			{1, 8},
			{3, 4},
			{5, 4},
			{7, 6},
			{9, 8},
			{10, 2},
			{10, 5},
			{10, 11},
			{10, 13},
			{10, 15},
			{10, 18},
			{12, 0},
			{12, 4},
			{12, 5},
			{12, 18},
			{14, 15},
			{15, 8},
			{15, 19},
			{16, 15},
			{17, 18},
			{18, 6}
		    },
                    {
                        {0,{1158,208}},
                        {1,{1158,496}},
                        {2,{0,272}},
                        {3,{24,0}},
                        {4,{192,0}},
                        {5,{188,192}},
                        {6,{500,304}},
                        {7,{500,176}},
                        {8,{1004,464}},
                        {9,{823,464}},
                        {10,{149,304}},
                        {11,{203,608}},
                        {12,{329,176}},
                        {13,{100,608}},
                        {14,{780,80}},
                        {15,{823,304}},
                        {16,{969,224}},
                        {17,{516,448}},
                        {18,{329,448}},
                        {19,{652,304}}
                    },
                    /*frame*/{0,1270,0,696}
                },
                {
                    "COCOGIRL/6",
                    {
			{0, 131, 0, 376},
			{0, 90, 0, 216},
			{0, 104, 0, 104},
			{0, 131, 0, 264},
			{0, 123, 0, 152},
			{0, 100, 0, 88},
			{0, 141, 0, 72},
			{0, 91, 0, 72}
		    },
                    {
			{1, 0},
			{2, 3},
			{4, 2},
			{4, 3},
			{5, 0},
			{6, 0},
			{6, 5},
			{7, 3},
			{7, 5}
		    },
                    {
                        {0,{0,0}},
                        {1,{171,0}},
                        {2,{482,48}},
                        {3,{311,80}},
                        {4,{482,192}},
                        {5,{171,256}},
                        {6,{171,384}},
                        {7,{352,384}}
                    },
                    /*frame*/{0,605,0,456}
                },
                {
                    "COCOGIRL/7",
                    {
			{0, 130, 0, 200},
			{0, 127, 0, 120},
			{0, 142, 0, 232},
			{0, 101, 0, 88},
			{0, 101, 0, 120},
			{0, 115, 0, 56},
			{0, 121, 0, 248},
			{0, 69, 0, 40}
		    },
                    {
			{0, 6},
			{0, 7},
			{3, 0},
			{4, 1},
			{4, 2},
			{4, 5},
			{4, 6},
			{4, 7}
		    },
                    {
                        {0,{161,320}},
                        {1,{135,0}},
                        {2,{302,48}},
                        {3,{331,432}},
                        {4,{161,160}},
                        {5,{6,160}},
                        {6,{0,272}},
                        {7,{331,352}}
                    },
                    /*frame*/{0,444,0,520}
                },
                {
                    "COCOGIRL/8",
                    {
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
                    {
			{0, 1},
			{1, 2},
			{1, 3},
			{1, 5},
			{1, 6},
			{1, 7},
			{1, 8},
			{1, 10},
			{1, 12},
			{1, 15},
			{4, 15},
			{9, 8},
			{11, 10},
			{14, 12},
			{14, 13}
		    },
                    {
                        {0,{676,128}},
                        {1,{677,304}},
                        {2,{817,176}},
                        {3,{834,304}},
                        {4,{166,336}},
                        {5,{494,337}},
                        {6,{505,48}},
                        {7,{817,432}},
                        {8,{940,176}},
                        {9,{892,32}},
                        {10,{311,128}},
                        {11,{315,0}},
                        {12,{344,336}},
                        {13,{0,416}},
                        {14,{0,240}},
                        {15,{140,128}}
                    },
                    /*frame*/{0,1020,0,488}
                }
        } ;

        for (const DataContext& dctx : vdctx)
        {
            Context ctx ;
            ctx.title = dctx.title;
            ctx.rectangles = dctx.rectangles;
            int n = ctx.rectangles.size();
            ctx.adjacency_list.resize(n) ;
            for (const Edge& e : dctx.edges)
            {
                MPD_Arc edge;
                edge._i = e.from;
                edge._j = e.to;
                assert(edge._i < n);
                assert(edge._j < n);
                ctx.adjacency_list[edge._i].push_back(edge) ;
            }

            contexts.push_back(ctx);
        } 

        int c=0;
	for (Context &ctx : contexts)
	{
                high_resolution_clock::time_point t1 = high_resolution_clock::now();

                int i=0;
		for (MyRect &r : ctx.rectangles)
	            r.i = r.no_sequence = i++ ;

		stair_steps_layout(ctx.rectangles, ctx.adjacency_list, RECT_BORDER) ;
		ctx.frame = compute_frame(ctx.rectangles) ;
                sort(begin(ctx.rectangles), end(ctx.rectangles), [](MyRect &r1, MyRect &r2){return r1.no_sequence < r2.no_sequence;});
                vector<TranslatedBox> translations;
                for (MyRect &r : ctx.rectangles)
                    translations.push_back({r.no_sequence,{r.m_left, r.m_top}});
                const DataContext &dctx = vdctx[c++];
                duration<double> time_span = high_resolution_clock::now() - t1;

                printf("%s %20s %f seconds elapsed\n", dctx.expected_translations == translations ? "OK": "KO",
                       dctx.title.c_str(), time_span.count());
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
					  int no_sequence_from_center,
					  vector<Context> &contexts)
{
        FunctionTimer ft("compute_contexts");
	int n = rectangles.size() ;
	if (n==0)
		return ;
	MatrixXd W = MatrixXd::Zero(n, n) ;
	for (const MPD_Arc* arc : list_edges(adjacency_list))
	{
		int i = arc->_i, j = arc->_j ;
		W(i, j) = W(j, i) = 1.0f ;
	}

//must be computed from unoriented graph
	vector<int> connected_component(n, -1) ;
	connected_components(compute_adjacency_list(W), connected_component) ;

	int nr_comp = 1 + *max_element(connected_component.begin(), connected_component.end()) ;
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
	iota(permutation1.begin(), permutation1.end(), 0) ;
	sort(permutation1.begin(), permutation1.end(), [&](int i,int j){return connected_component[i] < connected_component[j];}) ;
	permutation1 = compute_reverse_permutation(permutation1) ;
	PermutationMatrix<Dynamic> perm1(n) ;
	copy(permutation1.begin(), permutation1.end(), perm1.indices().data()) ;

	Matrix<int8_t,-1,-1> OW = Matrix<int8_t,-1,-1>::Zero(n, n) ;	//Oriented Weights

	for (const MPD_Arc* arc : list_edges(adjacency_list))
	{
		int i = arc->_i, j = arc->_j ;
		OW(i, j) = 1 ;
	}

	vector<int> fan_in(n, 0) ;
	for (const MPD_Arc* arc : list_edges(adjacency_list))
	{
		fan_in[arc->_j] ++ ;
	}
	MatrixXd WW = MatrixXd::Zero(n, n) ;
	for (const MPD_Arc* arc : list_edges(adjacency_list))
	{
		int i = arc->_i, j = arc->_j ;
		double value = 1.0f / fan_in[j] ;
		WW(i, j) = WW(j, i) = value ;
	}

	int n_acc = 0 ;

//	while (int& np = *find_if(begin(component_distribution), end(component_distribution), [=](int& np) { return np > max_nb_boxes_per_diagram; }))

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

//	sort(single_tables.begin(), single_tables.end(), [](WidgetContext& widg1, WidgetContext& widg2){return widg1.label<widg2.label;}) ;

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

		if (list_edges(ctx.adjacency_list).empty())
		{
			int w=0/*800*/, h=0/*600*/ ;
			binpack(ctx.rectangles, w, h) ;
		}
		else
		{
			Context ctx2 = ctx ;
			stair_steps_layout(ctx.rectangles, ctx.adjacency_list, RECT_BORDER) ;
			
			auto it = find_if(ctx2.rectangles.begin(), ctx2.rectangles.end(), [&](MyRect& r){ return r.no_sequence==no_sequence_from_center;}) ;
			if (it != ctx2.rectangles.end())
			{
				for (MyRect &r : ctx2.rectangles)
				{
					r.m_right += 2*RECT_BORDER ;
					r.m_bottom += 2*RECT_BORDER ;
				}
				vector<MyRect> rects = ctx2.rectangles ;
				MyRect& rr = *it ;

				float lower_bound = 1.0, upper_bound, spread = 0.001 ;
				
				auto tree_layout = [&](float factor){
					copy(rects.begin(), rects.end(), ctx2.rectangles.begin()) ;
					rr = expand(rr, factor) ;
					return stair_steps(ctx2.rectangles, rr, ctx2.adjacency_list) ;
				} ;

				dichotomy(lower_bound, upper_bound, spread, tree_layout) ;
				bool result = tree_layout(upper_bound) ;
				assert(result) ;

				rr = translate((const MyRect&)rects[rr.i], center(rr) - center(rects[rr.i])) ;

				for (MyRect &r : ctx2.rectangles)
				{
					expand_by(r, - RECT_BORDER) ;
				}

				if (dim_max(compute_frame(ctx2.rectangles)) < dim_max(compute_frame(ctx.rectangles)))
					ctx = ctx2 ;
			}
		}

		int rect_border = RECT_BORDER/2 ;
		int frame_border = RECT_BORDER*2 ;

		MyRect frame = compute_frame(ctx.rectangles) ;
		
		const int FRAME_MARGIN = 20 // consistent with diagload.js head of file.

// consistent with function encode_bounding_rectangle(context) in diagload.js bottom of file.

		expand_by(frame, FRAME_MARGIN) ;
		MyPoint translation = {-frame.m_left , -frame.m_top} ;
		translate(frame, translation) ;
		assert(frame.m_left == 0) ;
		assert(frame.m_top == 0) ;

		translation.x = -frame.m_left + FRAME_MARGIN/2 ;
		translation.y = -frame.m_top + FRAME_MARGIN/2 ;

		for (MyRect &rec : ctx.rectangles)
			translate(rec, translation) ;

		ctx.frame = frame ;
	}
}


void test_stair_steps_layout_from_111_boxes()
{
	TestFunctionTimer ft("test_stair_steps_layout_from_111_boxes");

        struct Edge{int from,to;};
        struct Result{vector<TranslatedBox> expected_translations; MyRect frame;};
        struct DataContext{string title; vector<MyRect> rectangles; vector<Edge> edges; vector<Result> contexts;};
        DataContext dctx = {
            "Coco",
            {
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
	    {
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
            /*contexts*/{
                    {
                        {
                            {14,{131,280}},
                            {15,{154,72}},
                            {16,{125,88}},
                            {17,{131,216}},
                            {18,{124,88}},
                            {19,{131,168}},
                            {27,{142,232}},
                            {29,{114,168}},
                            {33,{106,72}},
                            {49,{131,216}},
                            {52,{131,184}},
                            {75,{131,168}},
                            {76,{128,88}},
                            {89,{93,104}},
                            {90,{125,120}},
                            {94,{147,472}},
                            {95,{75,120}},
                            {107,{112,88}},
                            {110,{114,136}},
                        },
                        /*frame*/{0,978,0,984}
                    },
                    {
                        {
                            {25,{127,120}},
                            {44,{164,56}},
                            {45,{167,104}},
                            {56,{63,72}},
                            {69,{89,104}},
                            {72,{146,248}},
                            {85,{110,56}},
                            {86,{95,72}},
                            {87,{100,136}},
                            {99,{121,248}},
                            {108,{117,56}},
                            {109,{123,72}},
                        },
                        /*frame*/{0,630,0,632}
                    },
                    {
                        {
                            {6,{131,184}},
                            {7,{101,136}},
                            {8,{101,136}},
                            {13,{131,200}},
                            {20,{109,296}},
                            {22,{128,72}},
                            {43,{106,72}},
                            {60,{101,88}},
                            {80,{80,88}},
                            {92,{101,120}},
                            {93,{131,168}},
                            {98,{115,56}},
                            {100,{118,88}},
                            {103,{69,40}},
                        },
                        /*frame*/{0,722,0,696}
                    },
                    {
                        {
                            {1,{106,248}},
                            {2,{110,136}},
                            {3,{149,392}},
                            {4,{112,104}},
                            {9,{135,104}},
                            {34,{106,136}},
                            {35,{141,72}},
                            {37,{90,216}},
                            {38,{122,152}},
                            {41,{84,88}},
                            {64,{83,120}},
                            {101,{158,184}},
                        },
                        /*frame*/{0,654,0,744}
                    },
                    {
                        {
                            {0,{134,88}},
                            {47,{106,120}},
                            {61,{149,184}},
                            {62,{106,120}},
                            {63,{149,200}},
                            {65,{135,312}},
                            {66,{119,88}},
                            {67,{168,216}},
                            {68,{114,72}},
                            {102,{131,216}},
                        },
                        /*frame*/{0,710,0,632}
                    },
                    {
                        {
                            {10,{56,40}},
                            {11,{82,56}},
                            {26,{101,40}},
                            {36,{140,264}},
                            {39,{83,88}},
                            {40,{86,88}},
                            {46,{140,216}},
                            {48,{143,120}},
                            {53,{67,56}},
                            {54,{77,88}},
                            {57,{109,88}},
                            {58,{147,88}},
                            {73,{80,72}},
                            {74,{107,88}},
                        },
                        /*frame*/{0,533,0,712}
                    },
                    {
                        {
                            {21,{128,152}},
                            {23,{97,152}},
                            {24,{101,72}},
                            {30,{112,104}},
                            {31,{156,88}},
                            {51,{131,232}},
                            {82,{95,104}},
                            {88,{147,120}},
                        },
                        /*frame*/{0,502,0,472}
                    },
                    {
                        {
                            {32,{131,376}},
                            {55,{104,104}},
                            {77,{131,264}},
                            {78,{93,72}},
                            {79,{135,88}},
                            {81,{123,152}},
                            {83,{80,88}},
                            {97,{106,168}},
                            {104,{100,88}},
                            {105,{141,72}},
                            {106,{91,72}},
                        },
                        /*frame*/{0,507,0,680}
                    },
                    {
                        {
                            {70,{75,72}},
                            {71,{107,88}},
                        },
                        /*frame*/{0,139,0,232}
                    },
                    {
                        {
                            {5,{95,136}},
                            {12,{116,104}},
                            {28,{112,120}},
                            {42,{84,120}},
                            {50,{97,120}},
                            {59,{103,72}},
                            {84,{125,72}},
                            {91,{96,56}},
                            {96,{97,88}},
                        },
                        /*frame*/{0,336,0,432}
                    }
            }
	} ;

        int i=0;
        for (MyRect& r : dctx.rectangles)
        {
            r.i = r.no_sequence = i++;
        }

        int n = dctx.rectangles.size();
        vector<vector<MPD_Arc> > adjacency_list(n);

        for (Edge& e : dctx.edges)
        {
            MPD_Arc edge;
            edge._i = e.from;
            edge._j = e.to;
            assert(edge._i < n);
            assert(edge._j < n);
            adjacency_list[edge._i].push_back(edge) ;
        }

        int no_sequence_from_center = -1 ;

        vector<Context> contexts ;
        compute_contexts(dctx.rectangles, adjacency_list, max_nb_boxes_per_diagram, no_sequence_from_center,contexts) ;
        int c=0;
        for (Context &ctx : contexts)
        {
            sort(begin(ctx.rectangles), end(ctx.rectangles), [](MyRect &r1, MyRect &r2){return r1.no_sequence < r2.no_sequence;});
            vector<TranslatedBox> translations;
            for (MyRect &r : ctx.rectangles)
                translations.push_back({r.no_sequence,{width(r), height(r)}});
            printf("%s\n", translations == dctx.contexts[c++].expected_translations ? "OK" : "KO");
        }
}



void select_neighbours(const vector<MPD_Arc>& edges, vector<bool>& filter)
{
	int n = filter.size() ;

	MatrixXi W = MatrixXi::Zero(n, n) ;
	for (const MPD_Arc& edge : edges)
	{
		int i = edge._i, j = edge._j ;
		W(i, j) = W(j, i) = 1 ;
	}

	VectorXi V = VectorXi::Zero(n) ;
	for (int i=0; i < n; i++)
	{
		if (filter[i])
			V(i) = 1 ;
	}

	V += W * V ;

	for (int i=0; i < n; i++)
	{
		if (V(i))
		{
			filter[i] = true ;
		}
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
	sort(evp.begin(), evp.end(), [](double *p1, double *p2){return *p1 < *p2;}) ;
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
		n1 = count(Z_OUT.begin(), Z_OUT.end(), 1.0) ;
		n2 = count(Z_OUT.begin(), Z_OUT.end(), 0.0) ;

		if (n1 == 0 || n2 == 0)
			return false ;

//if there are small connected components as side effect of the cut, move them to the other side where they
//might be connected.
		vector<int> cc(n) ;
		vector<vector<MPD_Arc> > adj(n), adj_ = compute_adjacency_list(W) ;
		for (const MPD_Arc *arc : list_edges(adj_))
		{
			if (Z_OUT[arc->_i] != Z_OUT[arc->_j])
				continue ;
			adj[arc->_i].push_back(*arc) ;
		}
		connected_components(adj, cc) ;
		int nr_comp = 1 + *max_element(cc.begin(), cc.end()) ;
		vector<int> distribution(nr_comp, 0) ;
		for (int comp : cc)
			distribution[comp]++ ;
		vector<int> component(nr_comp) ;
		for (int comp=0; comp < nr_comp; comp++)
			component[comp] = comp ;
		std::sort(component.begin(), component.end(), [&](int comp1, int comp2){return distribution[comp1] < distribution[comp2] ;}) ;
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
		n1 = count(Z_OUT.begin(), Z_OUT.end(), 1.0) ;
		n2 = count(Z_OUT.begin(), Z_OUT.end(), 0.0) ;

		adj = vector<vector<MPD_Arc> >(n) ;
		cc = vector<int>(n,0) ;
		for (const MPD_Arc *arc : list_edges(adj_))
		{
			if (Z_OUT[arc->_i] != Z_OUT[arc->_j])
				continue ;
			adj[arc->_i].push_back(*arc) ;
		}
		connected_components(adj, cc) ;
		nr_comp = 1 + *max_element(cc.begin(), cc.end()) ;

//to create a permutation matrix, permute the columns of the identity matrix
		vector<int> permutation2(n) ;
		iota(permutation2.begin(), permutation2.end(), 0) ;
		std::sort(permutation2.begin(), permutation2.end(), [&](int i,int j){return Z_OUT[i] > Z_OUT[j];}) ;
		permutation2 = compute_reverse_permutation(permutation2) ;
		copy(permutation2.begin(), permutation2.end(), perm2.indices().data()) ;
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
		comp += 1 + *max_element(cc1.begin(), cc1.end()) ;
	vector<int> connected_component ;
	copy(cc1.begin(), cc1.end(), back_inserter(connected_component)) ;
	copy(cc2.begin(), cc2.end(), back_inserter(connected_component)) ;
	int nr_comp = 1 + *max_element(connected_component.begin(), connected_component.end()) ;
	component_distribution = vector<int>(nr_comp, 0) ;
	for (int comp : connected_component)
		component_distribution[comp]++;

	vector<int> permutation(n) ;
	iota(permutation.begin(), permutation.end(), 0) ;
	sort(permutation.begin(), permutation.end(), [&](int i,int j){return connected_component[i] < connected_component[j];}) ;
	permutation = compute_reverse_permutation(permutation) ;
	PermutationMatrix<Dynamic> perm1(n) ;
	copy(permutation.begin(), permutation.end(), perm1.indices().data()) ;
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
