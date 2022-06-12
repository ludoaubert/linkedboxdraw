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
un des coins de r1 avec le meme coin de r2. On a donc 4X4=16 possibilités.
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
#include "latuile_test_json_output.h"
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

void swap_rectangles(vector<MyRect> &rectangles, const vector<MPD_Arc>& edges, vector<tuple<int, RectCorner, int, RectCorner> >& swaps)
{
        FunctionTimer ft("swap_rectangles");

	int edge_distance = 0 ;
	for (const auto& [i, j] : edges)
	{
		edge_distance += rectangle_distance(rectangles[i], rectangles[j]) ;
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
					for (const auto& [i, j] : edges)
					{
						edge_distance_ += rectangle_distance(rectangles_[i], rectangles_[j]) ;
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
							for (const auto& [i, j] : edges)
							{
								edge_distance_ += rectangle_distance(rectangles_[i], rectangles_[j]) ;
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

	struct TestContext{int testid; vector<string> titles; vector<MyRect> input_rectangles; vector<MPD_Arc> edges; vector<MyRect> expected_rectangles;};

	const vector<TestContext> test_contexts = {

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

		.input_rectangles = {
			{.m_left=369, .m_right=529, .m_top=160, .m_bottom=272}, //0 discussion_topic
			{.m_left=599, .m_right=780, .m_top=416, .m_bottom=560}, //1 external_system
			{.m_left=780, .m_right=1003, .m_top=432, .m_bottom=560},//2 external_system_parameter
			{.m_left=146, .m_right=369, .m_top=256, .m_bottom=560}, //3 folder
			{.m_left=42, .m_right=146, .m_top=256, .m_bottom=336},  //4 category
			{.m_left=216, .m_right=369, .m_top=176, .m_bottom=256}, //5 document_folder
			{.m_left=0,.m_right=146, .m_top=432, .m_bottom=560},   //6 object_type
			{.m_left=91, .m_right=216, .m_top=144, .m_bottom=256},  //7 page
			{.m_left=369, .m_right=536, .m_top=0, .m_bottom=160},   //8 sfcomment
			{.m_left=599, .m_right=759, .m_top=304, .m_bottom=416}, //9 scm_file
			{.m_left=759, .m_right=919, .m_top=240, .m_bottom=416}, //10 scm_file_version
			{.m_left=369, .m_right=599, .m_top=400, .m_bottom=560}, //11 scm_repository
			{.m_left=536, .m_right=710, .m_top=64, .m_bottom=160},  //12 taskmgr_app_folder
			{.m_left=369, .m_right=543, .m_top=272, .m_bottom=400}  //13 taskmgr_task_group
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
			{.m_left=369, .m_right=529, .m_top=160, .m_bottom=272}, //0 discussion_topic
			{.m_left=599, .m_right=780, .m_top=416, .m_bottom=560}, //1 external_system
			{.m_left=780, .m_right=1003, .m_top=432, .m_bottom=560},//2 external_system_parameter
			{.m_left=146, .m_right=369, .m_top=256, .m_bottom=560}, //3 folder
			{.m_left=42, .m_right=146, .m_top=256, .m_bottom=336},  //4 category
			{.m_left=216, .m_right=369, .m_top=176, .m_bottom=256}, //5 document_folder
			{.m_left=0, .m_right=146, .m_top=432, .m_bottom=560},   //6 object_type
			{.m_left=91, .m_right=216, .m_top=144, .m_bottom=256},  //7 page
			{.m_left=369, .m_right=536, .m_top=0, .m_bottom=160},   //8 sfcomment
			{.m_left=599, .m_right=759, .m_top=304, .m_bottom=416}, //9 scm_file
			{.m_left=759, .m_right=919, .m_top=240, .m_bottom=416}, //10 scm_file_version
			{.m_left=369, .m_right=599, .m_top=400, .m_bottom=560}, //11 scm_repository
			{.m_left=536, .m_right=710, .m_top=64, .m_bottom=160},  //12 taskmgr_app_folder
			{.m_left=369, .m_right=543, .m_top=272, .m_bottom=400}  //13 taskmgr_task_group
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

		.input_rectangles = {
			{.m_left=49, .m_right=202, .m_top=448, .m_bottom=576},//frs_package
			{.m_left=571, .m_right=773, .m_top=400, .m_bottom=624},//mntr_template_link
			{.m_left=202, .m_right=369, .m_top=448, .m_bottom=624},//audit_change
			{.m_left=369, .m_right=571, .m_top=0, .m_bottom=256},//mess_template
			{.m_left=201, .m_right=368, .m_top=128, .m_bottom=304},//audit_entry
			{.m_left=571, .m_right=738, .m_top=144, .m_bottom=272},//pending_change
			{.m_left=369, .m_right=571, .m_top=256, .m_bottom=464},//request
			{.m_left=571, .m_right=752, .m_top=272, .m_bottom=400},//request_namedvalues
			{.m_left=-1, .m_right=201, .m_top=208, .m_bottom=448},//change_trx
			{.m_left=201, .m_right=340, .m_top=0, .m_bottom=128}, //role_group
			{.m_left=-1, .m_right=201, .m_top=0, .m_bottom=208},  //sfgroup
			{.m_left=571, .m_right=724, .m_top=32, .m_bottom=144},//groupmembership
			{.m_left=201, .m_right=326, .m_top=304, .m_bottom=384},//transaction
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
			{.m_left=49, .m_right=202, .m_top=448, .m_bottom=576},//frs_package
			{.m_left=571, .m_right=773, .m_top=400, .m_bottom=624},//mntr_template_link
			{.m_left=202, .m_right=369, .m_top=448, .m_bottom=624},//audit_change
			{.m_left=369, .m_right=571, .m_top=0, .m_bottom=256},//mess_template
			{.m_left=201, .m_right=368, .m_top=128, .m_bottom=304},//audit_entry
			{.m_left=571, .m_right=738, .m_top=144, .m_bottom=272},//pending_change
			{.m_left=369, .m_right=571, .m_top=256, .m_bottom=464},//request
			{.m_left=571, .m_right=752, .m_top=272, .m_bottom=400},//request_namedvalues
			{.m_left=-1, .m_right=201, .m_top=208, .m_bottom=448},//change_trx
			{.m_left=201, .m_right=340, .m_top=0, .m_bottom=128}, //role_group
			{.m_left=-1, .m_right=201, .m_top=0, .m_bottom=208},  //sfgroup
			{.m_left=571, .m_right=724, .m_top=32, .m_bottom=144},//groupmembership
			{.m_left=201, .m_right=326, .m_top=304, .m_bottom=384},//transaction
		}
	}
	};

	for (const auto& [testid, titles, input_rectangles, edges, expected_rectangles] : test_contexts)
	{
		vector<MyRect> rectangles = input_rectangles;
		int n = rectangles.size();

		int dm1 = dim_max(compute_frame(rectangles));

		vector<vector<MPD_Arc> > adjacency_list(n) ;
		for (const MPD_Arc &edge : edges)
		{
			adjacency_list[edge._i].push_back(edge) ;
		}
		const vector<vector<MPD_Arc> > adjacency_list_ = adjacency_list ;

		for (int i=0; i < rectangles.size(); i++)
			rectangles[i].i = i ;
		vector<tuple<int, RectCorner, int, RectCorner> > swaps ;

		swap_rectangles(rectangles, edges, swaps) ;
		int i, j ;
		RectCorner rci, rcj ;
		for (int k=0; k < swaps.size(); k++)
		{
			tie(i, rci, j, rcj) = swaps[k] ;
			string swap_names[2] ;
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
