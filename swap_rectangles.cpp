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
#include <string>
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
					ranges::copy(rectangles, rectangles_.begin()) ;
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

			ranges::copy(rectangles, _rectangles.begin()) ;
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
							ranges::copy(_rectangles, rectangles_.begin()) ;
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

	auto it = ranges::min_element(decision_tree, {}, [](DecisionTreeNode& node){return node.edge_distance;}) ;

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
	
	struct TestContext{int testid; vector<string> titles; vector<MyRect> rectangles; vector<MPD_Arc> edges; vector<MyRect> expected_rectangles;};

	const TestContext test_contexts[2] = {

	{
		.testid=1,
		.titles={
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
		},

		.rectangles = {
			{.left=369, .right=529, .top=160, .bottom=272}, //0 discussion_topic
			{.left=599, .right=780, .top=416, .bottom=560}, //1 external_system
			{.left=780, .right=1003, .top=432, .bottom=560},//2 external_system_parameter
			{.left=146, .right=369, .top=256, .bottom=560}, //3 folder
			{.left=42, .right=146, .top=256, .bottom=336},  //4 category
			{.left=216, .right=369, .top=176, .bottom=256}, //5 document_folder
			{.left=0,.right=146, .top=432, .bottom=560},   //6 object_type
			{.left=91, .right=216, .top=144, .bottom=256},  //7 page
			{.left=369, .right=536, .top=0, .bottom=160},   //8 sfcomment
			{.left=599, .right=759, .top=304, .bottom=416}, //9 scm_file
			{.left=759, .right=919, .top=240, .bottom=416}, //10 scm_file_version
			{.left=369, .right=599, .top=400, .bottom=560}, //11 scm_repository
			{.left=536, .right=710, .top=64, .bottom=160},  //12 taskmgr_app_folder
			{.left=369, .right=543, .top=272, .bottom=400}  //13 taskmgr_task_group
		},
		.edges={
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
		},
		.expected_rectangles = {
			{.left=369, .right=529, .top=160, .bottom=272}, //0 discussion_topic
			{.left=599, .right=780, .top=416, .bottom=560}, //1 external_system
			{.left=780, .right=1003, .top=432, .bottom=560},//2 external_system_parameter
			{.left=146, .right=369, .top=256, .bottom=560}, //3 folder
			{.left=42, .right=146, .top=256, .bottom=336},  //4 category
			{.left=216, .right=369, .top=176, .bottom=256}, //5 document_folder
			{.left=0, .right=146, .top=432, .bottom=560},   //6 object_type
			{.left=91, .right=216, .top=144, .bottom=256},  //7 page
			{.left=369, .right=536, .top=0, .bottom=160},   //8 sfcomment
			{.left=599, .right=759, .top=304, .bottom=416}, //9 scm_file
			{.left=759, .right=919, .top=240, .bottom=416}, //10 scm_file_version
			{.left=369, .right=599, .top=400, .bottom=560}, //11 scm_repository
			{.left=536, .right=710, .top=64, .bottom=160},  //12 taskmgr_app_folder
			{.left=369, .right=543, .top=272, .bottom=400}  //13 taskmgr_task_group
		}
	},
	{
		.testid=2,
		.titles={
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
		},

		.rectangles = {
			{.left=49, .right=202, .top=448, .bottom=576},//frs_package
			{.left=571, .right=773, .top=400, .bottom=624},//mntr_template_link
			{.left=202, .right=369, .top=448, .bottom=624},//audit_change
			{.left=369, .right=571, .top=0, .bottom=256},//mess_template
			{.left=201, .right=368, .top=128, .bottom=304},//audit_entry
			{.left=571, .right=738, .top=144, .bottom=272},//pending_change
			{.left=369, .right=571, .top=256, .bottom=464},//request
			{.left=571, .right=752, .top=272, .bottom=400},//request_namedvalues
			{.left=-1, .right=201, .top=208, .bottom=448},//change_trx
			{.left=201, .right=340, .top=0, .bottom=128}, //role_group
			{.left=-1, .right=201, .top=0, .bottom=208},  //sfgroup
			{.left=571, .right=724, .top=32, .bottom=144},//groupmembership
			{.left=201, .right=326, .top=304, .bottom=384},//transaction
		},

		.edges={
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
		},
		.expected_rectangles = {
			{.left=49, .right=202, .top=448, .bottom=576},//frs_package
			{.left=571, .right=773, .top=400, .bottom=624},//mntr_template_link
			{.left=202, .right=369, .top=448, .bottom=624},//audit_change
			{.left=369, .right=571, .top=0, .bottom=256},//mess_template
			{.left=201, .right=368, .top=128, .bottom=304},//audit_entry
			{.left=571, .right=738, .top=144, .bottom=272},//pending_change
			{.left=369, .right=571, .top=256, .bottom=464},//request
			{.left=571, .right=752, .top=272, .bottom=400},//request_namedvalues
			{.left=-1, .right=201, .top=208, .bottom=448},//change_trx
			{.left=201, .right=340, .top=0, .bottom=128}, //role_group
			{.left=-1, .right=201, .top=0, .bottom=208},  //sfgroup
			{.left=571, .right=724, .top=32, .bottom=144},//groupmembership
			{.left=201, .right=326, .top=304, .bottom=384},//transaction
		}
	}
	};
	
	for (const auto& [testid, titles, rectangles, edges, expected_rectangles] : test_contexts)
	{
		int n = rectangles.size();
		
		int dm1 = dim_max(compute_frame(rectangles));
		
		vector<vector<MPD_Arc> > adjacency_list(n) ;
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
		
		int dm2 = dim_max(compute_frame(rectangles));
		
		bool bOK = rectangles == expected_rectangles;
		printf("swap_rectangles testid=%d %s\n", testid, bOK ? "OK" : "KO");
		printf("dim_max(frame) : %d => %d\n", dm1, dm2);
		(bOK ? nbOK : nbKO)++;
	}
}
