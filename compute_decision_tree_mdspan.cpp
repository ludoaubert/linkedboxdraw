/*
0-take a look at function compute_holes(). There seem to be various versions.
1-first version: do not try to optimize the computation of emplacement.
*/
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <mdspan>
#include <limits.h>
#include <string>
#include <ranges>
#include <initializer_list>
#include <generator>
#include <execution>
#include <format>
#include <stdio.h>
using namespace std;

struct MyRect
{
	int m_left, m_right, m_top, m_bottom ;
};


struct Edge {
	int from;
	int to;
	auto operator<=>(const Edge&) const = default;
};


struct DecisionTreeNode
{
	int index=0;
	DecisionTreeNode *parent_node=0;
	int depth;
	int sigma_edge_distance = INT_MAX;	// initialize sigma_edge_distance to infinity.
	int i_emplacement_source, i_emplacement_destination;
	
	friend bool operator==(const DecisionTreeNode&, const DecisionTreeNode&) = default;
};

struct Decision
{
	int i_emplacement_source, i_emplacement_destination;
};

struct TestContext{
	vector<Edge> edges;
	vector<MyRect> input_rectangles, holes ;
	vector<Decision> expected_decision;
};


int distance_between_ranges(int left1, int right1, int left2, int right2)
{
	if (left2 > right1)
		return left2 - right1 ;
	else if (left1 > right2)
		return left1 - right2 ;
	else
		return 0 ;
}


int rectangle_distance(const MyRect& r1, const MyRect& r2)
{
	if (r1.m_left > r2.m_right)
	{
		return r1.m_left - r2.m_right + distance_between_ranges(r1.m_top, r1.m_bottom, r2.m_top, r2.m_bottom) ;
	}
	else if (r1.m_right < r2.m_left)
	{
		return r2.m_left - r1.m_right + distance_between_ranges(r1.m_top, r1.m_bottom, r2.m_top, r2.m_bottom) ;
	}
	else if (r1.m_top > r2.m_bottom)
	{
		return r1.m_top - r2.m_bottom + distance_between_ranges(r1.m_left, r1.m_right, r2.m_left, r2.m_right) ;
	}
	else if (r1.m_bottom < r2.m_top)
	{
		return r2.m_top - r1.m_bottom + distance_between_ranges(r1.m_left, r1.m_right, r2.m_left, r2.m_right) ;
	}
	else
	{
//the two rectangles intersect.
		return 0 ;
	}
}


const vector<TestContext> test_contexts = {
/*
	+---------+
	|   r0    | 0
	+---------+
	|   h0    | 2
	+---------+
	|   r1    | 1
	+---------+

	emplacements={r0, r1, h0};
				  0,  1,  2
*/
	{
		.edges={
			{.from=0, .to=1},
			{.from=1, .to=0}
		},
		
		.input_rectangles={
			{.m_left=0, .m_right=100, .m_top=0, .m_bottom=50},
			{.m_left=0, .m_right=100, .m_top=100, .m_bottom=150}
		},
		
		.holes={
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=100}
		},

		.expected_decision = {
			{.i_emplacement_source=0, .i_emplacement_destination=2}
		}
	},

/*
	+---------+
	|   r0    | 0
	+---------+
	|   h0    | 3
	+---------+
	|   r1    | 1
	+---------+
	|   r2    | 2
	+---------+

	emplacements={r0, r1, r2, h0};
				  0,  1,  2,  3
*/
	{
		.edges={
			{.from=0,.to=1},
			{.from=1,.to=0},{.from=1,.to=2},
			{.from=2,.to=1}
		},
		
		.input_rectangles={
			{.m_left=0, .m_right=100, .m_top=0, .m_bottom=50},
			{.m_left=0, .m_right=100, .m_top=100, .m_bottom=150},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=200}
		},
		
		.holes={
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=100}
		},

		.expected_decision = {
			{.i_emplacement_source=0, .i_emplacement_destination=3}
		}
	},
	
	{
		.edges={
			{.from=0,.to=3},
			{.from=1,.to=7},
			{.from=2,.to=7},
			{.from=3,.to=0},{.from=3,.to=4},
			{.from=4,.to=3},{.from=4,.to=7},
			{.from=5,.to=7},
			{.from=6,.to=7},{.from=6,.to=12},
			{.from=7,.to=1},{.from=7,.to=2},{.from=7,.to=4},{.from=7,.to=5},{.from=7,.to=6},{.from=7,.to=8},{.from=7,.to=9},{.from=7,.to=10},{.from=7,.to=11},
			{.from=8,.to=7},
			{.from=9,.to=7},
			{.from=10,.to=7},
			{.from=11,.to=7},
			{.from=12,.to=6},{.from=12,.to=13},
			{.from=13,.to=12},{.from=13,.to=14},
			{.from=14,.to=13}
		},
		
		.input_rectangles={
			{.m_left=406, .m_right=608, .m_top=20, .m_bottom=164},
			{.m_left=330, .m_right=552, .m_top=340, .m_bottom=451},
			{.m_left=463, .m_right=608, .m_top=228, .m_bottom=340},
			{.m_left=608, .m_right=774, .m_top=20, .m_bottom=212},
			{.m_left=608, .m_right=774, .m_top=212, .m_bottom=340},
			{.m_left=760, .m_right=947, .m_top=356, .m_bottom=516},
			{.m_left=283, .m_right=463, .m_top=164, .m_bottom=324},
			{.m_left=552, .m_right=760, .m_top=340, .m_bottom=516},
			{.m_left=345, .m_right=553, .m_top=516, .m_bottom=676},
			{.m_left=566, .m_right=753, .m_top=516, .m_bottom=660},
			{.m_left=774, .m_right=947, .m_top=196, .m_bottom=356},
			{.m_left=753, .m_right=940, .m_top=516, .m_bottom=724},
			{.m_left=103, .m_right=283, .m_top=163, .m_bottom=291},
			{.m_left=88, .m_right=283, .m_top=291, .m_bottom=451},
			{.m_left=130, .m_right=345, .m_top=451, .m_bottom=627}
		},
		.holes={
			{.m_left=283,.m_right=330,.m_top=340,.m_bottom=451},
			{.m_left=88,.m_right=130,.m_top=451,.m_bottom=627},
			{.m_left=463,.m_right=608,.m_top=164,.m_bottom=228},
			{.m_left=345,.m_right=552,.m_top=451,.m_bottom=516},
			{.m_left=345,.m_right=553,.m_top=676,.m_bottom=724},
			{.m_left=566,.m_right=753,.m_top=660,.m_bottom=724},
			{.m_left=130,.m_right=345,.m_top=627,.m_bottom=724},
			{.m_left=283,.m_right=406,.m_top=20,.m_bottom=164},
			{.m_left=774,.m_right=947,.m_top=20,.m_bottom=196},
			{.m_left=103,.m_right=283,.m_top=20,.m_bottom=163},
			{.m_left=88,.m_right=130,.m_top=627,.m_bottom=724}
		},

		.expected_decision = {
			{.i_emplacement_source=1, .i_emplacement_destination=15+3},
			{.i_emplacement_source=2, .i_emplacement_destination=1},
			{.i_emplacement_source=0, .i_emplacement_destination=15+8},
			{.i_emplacement_source=6, .i_emplacement_destination=2},
			{.i_emplacement_source=12, .i_emplacement_destination=6},
			{.i_emplacement_source=13, .i_emplacement_destination=15+7},
			{.i_emplacement_source=14, .i_emplacement_destination=0}
		}
	}

};


vector<DecisionTreeNode> compute_decision_tree(const vector<Edge>& edges, const vector<MyRect>& input_rectangles, const vector<MyRect>& holes)
{
	const int MAX_FLOOR_COUNT=8;
	const int FLOOR_MAX_SIZE=2000;
	
	const int nr_input_rectangles = input_rectangles.size();

	const vector<MyRect> rectangles = views::concat(input_rectangles, holes) | ranges::to<vector>();
	
	const int nr_emplacements = rectangles.size();
	
	const int N = rectangles.size();
	
	const vector<int> distance_matrix = views::cartesian_product(rectangles, rectangles) |
										views::transform([](auto arg){const auto [r1, r2]=arg;	return rectangle_distance(r1, r2);}) |
										ranges::to<vector>();

	vector<int> emplacement_data(MAX_FLOOR_COUNT*FLOOR_MAX_SIZE*rectangles.size()) ;
	auto emp = mdspan(emplacement_data.data(), MAX_FLOOR_COUNT, FLOOR_MAX_SIZE, rectangles.size());

	vector<DecisionTreeNode> decision_tree_data(MAX_FLOOR_COUNT*FLOOR_MAX_SIZE);
	auto decision_tree = mdspan(decision_tree_data.data(), MAX_FLOOR_COUNT, FLOOR_MAX_SIZE);
	
	auto child_nodes = [&](DecisionTreeNode *parent_node){

		return views::cartesian_product( views::iota(nr_input_rectangles, nr_emplacements),
									  views::iota(0, nr_input_rectangles) |
											views::filter([&](int r){return parent_node==0 || emp[parent_node->depth, parent_node->index, r]==r;}) 
									) |
			views::transform([&](const auto arg) {
				auto const [h, r] = arg;
				auto swap_r_h = [&](int u){
					if (u==r)
						return h;
					if (u==h)
						return r;
					else
						return u;
				};
			
				auto f = [&](int u){return parent_node==0 ? swap_r_h(u) : emp[parent_node->depth, parent_node->index, swap_r_h(u)];};
				int sigma_edge_distance = ranges::fold_left(edges | views::transform([&](const Edge& e){return distance_matrix[f(e.from) * N + f(e.to)];}),
					0, plus<int>()) ;
				
				return DecisionTreeNode{
					.index = -1,
					.parent_node = parent_node,
					.depth = parent_node==0 ? 0 : parent_node->depth+1,
					.sigma_edge_distance = sigma_edge_distance,
					.i_emplacement_source = r, 
					.i_emplacement_destination = h
				};
			}) |
			ranges::to<vector>();
	};
	
	auto build_decision_tree = [&](){
		printf("enter build_decision_tree()\n");
		
		for (const auto [index, r] : views::cartesian_product(views::iota(0, FLOOR_MAX_SIZE), views::iota(0, (int)rectangles.size())))
		{
			const int depth = 0;
			emp[depth, index, r] = r;
		}
		
		for (int depth=0; depth<MAX_FLOOR_COUNT; depth++)
		{
			const vector<DecisionTreeNode*> parent_nodes = (depth > 0) ?
					views::iota(0, FLOOR_MAX_SIZE) |
					views::transform([&](int index){return &decision_tree[depth-1, index];}) |
					views::filter([](DecisionTreeNode* node){return node->sigma_edge_distance != INT_MAX;}) |
					ranges::to<vector>() :
				vector{ (DecisionTreeNode*) 0 };

			vector<vector<DecisionTreeNode> > vv(parent_nodes.size());
			transform(execution::par_unseq, begin(parent_nodes), end(parent_nodes), begin(vv), [&](DecisionTreeNode* parent_node){return child_nodes(parent_node);});

			vector<DecisionTreeNode> floor = vv | views::join | ranges::to<vector>() ;
						
			ranges::sort(floor, {}, [&](const DecisionTreeNode& n){return n.sigma_edge_distance;});
			
			for (int i=0; i<floor.size(); i++)
				floor[i].index = i;
			
			floor = floor | views::take(FLOOR_MAX_SIZE) | ranges::to<vector>() ;
			
			ranges::copy( floor, views::iota(0, FLOOR_MAX_SIZE) |
					views::transform([&](int index){return &decision_tree[depth, index];}));
					
			for (const auto [index, r] : views::cartesian_product(views::iota(0, FLOOR_MAX_SIZE), views::iota(0, (int)rectangles.size())))
			{
				if (depth > 0)
				{
					emp[depth, index, r] = emp[depth-1, index, r];
				}
			}
			
			for (const DecisionTreeNode& n : floor)
			{
				swap(emp[n.depth, n.index, n.i_emplacement_source], emp[n.depth, n.index, n.i_emplacement_destination]);
			}
		}
		
		printf("exit build_decision_tree()\n");
	};
	
	build_decision_tree();
	
	return decision_tree;
}


generator<const DecisionTreeNode*> walk_up_from(const DecisionTreeNode* parent_node)
{
	for (const DecisionTreeNode *node=parent_node; node != 0; node = node.parent_node)
	{
		co_yield node;
	}		
};


vector<vector<Decision> > compute_decisions(const vector<DecisionTreeNode>& decision_tree, int count)
{	
	const int n = decision_tree.size();
	vector<int> index = views::iota(0, n) | ranges::to<vector>();
	ranges::sort(index, {}, [&](int position){return decision_tree[position].sigma_edge_distance;});
		
	const vector<vector<Decision> > decision_lists = index |
		views::iota(0, count) |
		views::transform([&](int position){
			return walk_up_from(&decision_tree[position]) | 
						views::reverse |
						views::transform([](const DecisionTreeNode* node){
							return Decision{.i_emplacement_source=node->i_emplacement_source, .i_emplacement_destination=node->i_emplacement_destination};
						}) |
						ranges::to<vector>();
		}) |
		ranges::to<vector>();
		
	const buffer = decision_lists |
		views::transform([](const vector<Decision>& decision){
			return decision |
				views::transform([](const Decision& d){
					return format(R"({{"i_emplacement_source":{},"i_emplacement_destination":{}}})",
						d.i_emplacement_source, d.i_emplacement_destination);
				}) |
				views::join_with(",\n"s) ;
		}) | views::join_with("},\n{"s) |
		ranges::to<string>() ;
		
//	FILE* f=fopen("decision_tree.json", "w");
//	fprintf(f, "{%s}", buffer.c_str());
	printf("{%s}", buffer.c_str());
//	fclose(f);
	
	return decision_lists;
}



int main()
{
	for (const auto& [edges, input_rectangles, holes, expected_decision] : test_contexts)
	{
		const int nr_emplacements = input_rectangles.size() + holes.size();
		
		vector<DecisionTreeNode> decision_tree = compute_decision_tree(edges, input_rectangles, holes);

		const int count=20;
		const vector<vector<Decision> > decision_lists = compute_decisions(decision_tree, count) ;
		
		
		const DecisionTreeNode &bn = * ranges::min_element(decision_tree, {}, &DecisionTreeNode::sigma_edge_distance);
		
		vector<int> emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		
		for (int idx : walk_up_from(&bn) | ranges::to<vector>() | views::reverse)
		{
			const DecisionTreeNode* node = decision_tree[idx];
			printf("{.i_emplacement_source=%d, .i_emplacement_destination=%d},\n", 
					emplacement[node->i_emplacement_source], emplacement[node->i_emplacement_destination]);
			swap(emplacement[node->i_emplacement_source], emplacement[node->i_emplacement_destination]);
		}
		
		vector<int> expected_emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		for (const Decision &d : expected_decision)
			swap(expected_emplacement[d.i_emplacement_source], expected_emplacement[d.i_emplacement_destination]);
		
		bool bOk1 = emplacement == expected_emplacement;

		printf("bOk=%s\n", bOk1 ? "true" : "false");
	}
	return 0;
}