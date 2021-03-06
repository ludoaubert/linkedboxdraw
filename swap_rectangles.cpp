/* swap_rectangles.cpp
*
* Copyright (c) 2005-2017 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/

/*
Decision tree vs permutations:
+ One possibility would be to modify the stair_steps algorithm to try all possible permutations in the order processing of a rectangles adjacency
list. There are two big drawbacks: 1) complexity of the source code. 2) there are n! possible permutations and we'd have to set limits
if we don't want the algo to run forever. These 2 drawbacks are overwhelming.

+ An interesting alternative is to use a decision tree of a given depth. The idea is to find all possible permutations of 2 rectangles.
Out of the set of possible permutations, we can grow further the decision tree by applying more such permutations, and cherry pick the most
interesting branches.
The size of the full decision tree of height n would be n!, but the advantage is we have control over the depth. It can be limited to 3 or 4.
1) the complexity is not mixed with the complexity of the stair_steps algorithm.
2) the algo will not run forever if the depth of the decision tree is capped.
*/

/*
diagramme Cap Gemini: dans le premier diagramme, une amelioration possible serait de permuter les positions des rectangles 
'discussion_topic' et 'taskmgr_task_group'.
Meme remarque: si on permutait 'datamart_report' et datamart_metric_parameter'.
    pour ce cas remarquer aussi que 'datamart_metric_prop' devrait etre remonte ce qui diminuerait le diametre du cadre.
Meme remarque: si on permutait 'page' et 'document_folder'.
Meme remarque: si on permutait 'task_alert_pref' et 'ancestor_project'.
                           ou 'role_auto_grant' et 'project_activity'.
						   ou 'project_activity' et 'role_user'.
Proposition d'algo: 
hypothese: on se refuse le droit de deplacer les autres rectangles.
On essaie, pour toutes les paires (r1,r2) possibles, de permuter r1 et r2.
lorsqu'on tente de remplacer r1 par r2, on tente 4 possibilites, celles de faire correspondre
un des coins de r1 avec le meme coin de r2. On a donc 4X4=16 possibilitÚs.
On doit d'abord tester si il y a des collisions avec les n-2 rectangles qui ne bougent pas.
On verifie qu'on ne sort pas du cadre (on s'interdit d'agrandir le cadre). Ou alors on a un coefficient de trade off.
On calcule la somme des distances entre les rectangles qui ont un lien entre eux. On regarde
si il y a une amelioration. On garde le meilleur resultat.
On boucle afin de proceder de maniere iterative.
nom du fichier source : swap_rectangles.cpp 
*/

#include "swap_rectangles.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "FunctionTimer.h"
#include <vector>
#include <tuple>
#include <algorithm>
using namespace std ;

struct DecisionTreeNode
{
	int i, j ;
	int index, parent_index ;
	RectCorner rci, rcj ;
	int edge_distance ;
	int depth ;
} ;

void swap_rectangles(vector<MyRect> &rectangles, vector<const MPD_Arc*> edges, vector<tuple<int, RectCorner, int, RectCorner> >& swaps)
{
        FunctionTimer ft("swap_rectangles");

	int edge_distance = 0 ;
	for (const MPD_Arc* edge : edges)
	{
		edge_distance += rectangle_distance(rectangles[edge->_i], rectangles[edge->_j]) ;
	}

	MyRect frame = compute_frame(rectangles) ;

	vector<DecisionTreeNode> decision_tree ;

	int n = rectangles.size() ;
	vector<MyRect> _rectangles(n), rectangles_(n) ;

//premier etage
	for (int i=0; i < n; i++)
	{
		for (int j=i+1; j < n; j++)
		{

			for (RectCorner rci : RectCorners)
			{
				for (RectCorner rcj : RectCorners)
				{
					std::copy(rectangles.begin(), rectangles.end(), rectangles_.begin()) ;
					MyRect &ri = rectangles_[i], &rj = rectangles_[j] ;

					MyPoint translation_ri = corner(rj, rci) - corner(ri, rci) ;
					MyPoint translation_rj = corner(ri, rcj) - corner(rj, rcj) ;
					translate(ri, translation_ri) ;
					translate(rj, translation_rj) ;

//On doit d'abord tester si il y a des collisions avec les n-2 rectangles qui ne bougent pas.
					bool collision = false ;
					if (intersect_strict(ri,rj))
						collision = true ;
					for (MyRect &r : rectangles_)
					{
						if (r.i==ri.i || r.i==rj.i)
							continue ;
						if (intersect_strict(r, ri) || intersect_strict(r, rj))
							collision = true ;
					}
					if (collision)
						continue ;

					if (rectangle_diameter(compute_frame(rectangles_)) > rectangle_diameter(frame))
						continue ;

					int edge_distance_ = 0 ;
					for (const MPD_Arc* edge : edges)
					{
						edge_distance_ += rectangle_distance(rectangles_[edge->_i], rectangles_[edge->_j]) ;
					}

					int index = decision_tree.size() ;
					int parent_index = -1 ;
					int depth = 1 ;
					DecisionTreeNode node = {i, j, index, parent_index, rci, rcj, edge_distance_, depth} ;
					decision_tree.push_back(node) ;
				}
			}
		}
	}

/*
si n=16:
etage 1: 16*16*16 nodes possible soit 2^12 max
etage 2: etage 1 * 16*16 soit 2^20 = 1 M  max
etage 3: etage 2 * 16*16 soit 2^28 = 256 M  max
*/

	const int DECISION_TREE_MAX_SIZE = 4 * 1024 * 1024 ;
	const int DECISION_TREE_MAX_DEPTH = 2 ;

//etages suivants
	for (int depth=2; depth <= DECISION_TREE_MAX_DEPTH ; depth++)
	{
		vector<DecisionTreeNode*> ancestors(depth-1) ;

		int mysize = decision_tree.size() ;
		for (int i=0; i < mysize && decision_tree.size() < DECISION_TREE_MAX_SIZE; i++)
		{
			DecisionTreeNode parent_node = decision_tree[i] ;

			if (parent_node.depth != depth-1)
				continue ;

//apply swaps leading down to parent_node
			int ind=depth-1 ;
			for (DecisionTreeNode* p=&parent_node; p!=0; p=p->parent_index==-1 ? 0 : &decision_tree[p->parent_index])
				ancestors[--ind]=p ;

			std::copy(rectangles.begin(), rectangles.end(), _rectangles.begin()) ;
			for (DecisionTreeNode *node : ancestors)
			{
				MyRect &ri = _rectangles[node->i], &rj = _rectangles[node->j] ;
				MyPoint translation_ri = corner(rj, node->rci) - corner(ri, node->rci) ;
				MyPoint translation_rj = corner(ri, node->rcj) - corner(rj, node->rcj) ;
				translate(ri, translation_ri) ;
				translate(rj, translation_rj) ;
			}

			for (int k=0; k < n; k++)
			{
				if (k==parent_node.i || k==parent_node.j)
					continue ;

				for (RectCorner rck : RectCorners)
				{
					for (RectCorner rci : RectCorners)
					{
						MyRect &rk = rectangles_[k] ;
						MyRect* r2[2]={&rectangles_[parent_node.i], &rectangles_[parent_node.j]} ;

						for (MyRect* ri : r2)
						{
							std::copy(_rectangles.begin(), _rectangles.end(), rectangles_.begin()) ;
//TODO: appliquer les swap des parents

							MyPoint translate_ri = corner(rk, rci) - corner(*ri, rci) ;
							MyPoint translate_rk = corner(*ri, rck) - corner(rk, rck) ;
							translate(*ri, translate_ri) ;
							translate(rk, translate_rk) ;

	//On doit d'abord tester si il y a des collisions avec les n-2 rectangles qui ne bougent pas.
							bool collision = false ;
							if (intersect_strict(*ri,rk))
								collision = true ;
							for (MyRect &r : rectangles_)
							{
								if (r.i==ri->i || r.i==rk.i)
									continue ;
								if (intersect_strict(r, *ri) || intersect_strict(r, rk))
									collision = true ;
							}
							if (collision)
								continue ;

							if (rectangle_diameter(compute_frame(rectangles_)) > rectangle_diameter(frame))
								continue ;

							int edge_distance_ = 0 ;
							for (const MPD_Arc* edge : edges)
							{
								edge_distance_ += rectangle_distance(rectangles_[edge->_i], rectangles_[edge->_j]) ;
							}

							int index = decision_tree.size() ;
							DecisionTreeNode node = {ri->i, k, index, parent_node.index, rci, rck, edge_distance_, depth} ;
							decision_tree.push_back(node) ;
						}
					}
				}
			}
		}
	}

	auto it = min_element(decision_tree.begin(), decision_tree.end(), [](DecisionTreeNode& node1, DecisionTreeNode& node2){return node1.edge_distance < node2.edge_distance;}) ;

	DecisionTreeNode *node = &* it ;
	if (node->edge_distance >= edge_distance)
		return ;

	vector<DecisionTreeNode*> ancestors(node->depth) ;

//apply swaps leading down to node
	int ind=node->depth ;
	for (DecisionTreeNode* p=node; p!=0; p=p->parent_index==-1 ? 0 : &decision_tree[p->parent_index])
		ancestors[--ind]=p ;

	for (DecisionTreeNode *node : ancestors)
	{
		MyRect &ri = rectangles[node->i], &rj = rectangles[node->j] ;
		MyPoint translation_ri = corner(rj, node->rci) - corner(ri, node->rci) ;
		MyPoint translation_rj = corner(ri, node->rcj) - corner(rj, node->rcj) ;
		translate(ri, translation_ri) ;
		translate(rj, translation_rj) ;
		swaps.push_back(make_tuple(node->i, node->rci, node->j, node->rcj)) ;
	}	
}


void test_swap_rectangles()
{
        TestFunctionTimer ft("test_swap_rectangles");

	{
		const char* titles[14]={
			"discussion_topic",
			"external_system",
			"external_system_parameter",
			"folder",
			"category",
			"document_folder",
			"object_type",
			"page",
			"sfcomment",
			"scm_file",
			"scm_file_version",
			"scm_repository",
			"taskmgr_app_folder",
			"taskmgr_task_group"
		} ;

		vector<MyRect> rectangles = {
			{369,529,160,272}, //0 discussion_topic
			{599,780,416,560}, //1 external_system
			{780,1003,432,560},//2 external_system_parameter
			{146,369,256,560}, //3 folder
			{42,146,256,336},  //4 category
			{216,369,176,256}, //5 document_folder
			{0,146,432,560},   //6 object_type
			{91,216,144,256},  //7 page
			{369,536,0,160},   //8 sfcomment
			{599,759,304,416}, //9 scm_file
			{759,919,240,416}, //10 scm_file_version
			{369,599,400,560}, //11 scm_repository
			{536,710,64,160},  //12 taskmgr_app_folder
			{369,543,272,400}  //13 taskmgr_task_group
		};
		vector<MPD_Arc> edges={
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
		vector<vector<MPD_Arc> > adjacency_list(14) ;
		for (MPD_Arc &edge : edges)
		{
			adjacency_list[edge._i].push_back(edge) ;
		}
		const vector<vector<MPD_Arc> > adjacency_list_ = adjacency_list ;

		for (int i=0; i < rectangles.size(); i++)
			rectangles[i].i = i ;
		vector<tuple<int, RectCorner, int, RectCorner> > swaps ;

                for (tuple<int,RectCorner,int,RectCorner>& t : swaps)
                {
                    printf("{%d,%d,%d,%d},\n", get<0>(t),get<1>(t),get<2>(t),get<3>(t));
                }

		swap_rectangles(rectangles, list_edges(adjacency_list_), swaps) ;
		int i, j ;
		RectCorner rci, rcj ;
		for (int k=0; k < swaps.size(); k++)
		{
			tie(i, rci, j, rcj) = swaps[k] ;
			const char* swap_names[2] ;
			swap_names[0] = titles[i] ;
			swap_names[1] = titles[j] ;
		}
	}

	{
		const char* titles[13]={
			"frs_package",
			"mntr_template_link",
			"audit_change",
			"mess_template",
			"audit_entry",
			"pending_change",
			"request",
			"request_namedvalues",
			"change_trx",
			"role_group",
			"sfgroup",
			"groupmembership",
			"transaction"
		} ;

		vector<MyRect> rectangles = {
			{49,202,448,576},//frs_package
			{571,773,400,624},//mntr_template_link
			{202,369,448,624},//audit_change
			{369,571,0,256},//mess_template
			{201,368,128,304},//audit_entry
			{571,738,144,272},//pending_change
			{369,571,256,464},//request
			{571,752,272,400},//request_namedvalues
			{-1,201,208,448},//change_trx
			{201,340,0,128}, //role_group
			{-1,201,0,208},  //sfgroup
			{571,724,32,144},//groupmembership
			{201,326,304,384},//transaction
		};

		vector<MPD_Arc> edges={
			{1,3},
			{2,4},
			{4,0},
			{4,3},
			{4,6},
			{4,10},
			{4,12},
			{5,8},
			{7,6},
			{8,12},
			{9,10},
			{11,10}
		};

		vector<vector<MPD_Arc> > adjacency_list(13) ;
		for (MPD_Arc &edge : edges)
		{
			adjacency_list[edge._i].push_back(edge) ;
		}
		const vector<vector<MPD_Arc> > adjacency_list_ = adjacency_list ;
		for (int i=0; i < rectangles.size(); i++)
			rectangles[i].i = i ;
		vector<tuple<int, RectCorner, int, RectCorner> > swaps ;
		swap_rectangles(rectangles, list_edges(adjacency_list_), swaps) ;
		int i, j ;
		RectCorner rci, rcj ;
		for (int k=0; k < swaps.size(); k++)
		{
			tie(i, rci, j, rcj) = swaps[k] ;
			const char* swap_names[2] ;
			swap_names[0] = titles[i] ;
			swap_names[1] = titles[j] ;
		}
	}
}
