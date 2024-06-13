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
	int parent_index=-1;
	int depth;
	int sigma_edge_distance;
	int i_emplacement_source, i_emplacement_destination;
	
	friend bool operator==(const DecisionTreeNode&, const DecisionTreeNode&) = default;
};

struct TestContext{
	vector<Edge> edges;
	vector<MyRect> input_rectangles, holes ;
	vector<DecisionTreeNode> expected_decision;
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
			{.index=0, .parent_index=-1, .depth=0, .i_emplacement_source=0, .i_emplacement_destination=2}
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
			{.index=0, .parent_index=-1, .depth=0, .i_emplacement_source=0, .i_emplacement_destination=3}
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
			{.index=0, .parent_index=-1, .depth=0, .i_emplacement_source=1, .i_emplacement_destination=15+3},
			{.index=1, .parent_index=0, .depth=1, .i_emplacement_source=2, .i_emplacement_destination=1},
			{.index=2, .parent_index=1, .depth=2, .i_emplacement_source=0, .i_emplacement_destination=15+8},
			{.index=3, .parent_index=2, .depth=3, .i_emplacement_source=6, .i_emplacement_destination=2},
			{.index=4, .parent_index=3, .depth=4, .i_emplacement_source=12, .i_emplacement_destination=6},
			{.index=5, .parent_index=4, .depth=5, .i_emplacement_source=13, .i_emplacement_destination=15+7},
			{.index=6, .parent_index=5, .depth=6, .i_emplacement_source=14, .i_emplacement_destination=0}
		}
	}

};


vector<DecisionTreeNode> compute_decision_tree(const vector<Edge>& edges, const vector<MyRect>& input_rectangles, const vector<MyRect>& holes)
{
	const int nr_input_rectangles = input_rectangles.size();

	auto il = {input_rectangles, holes};
	const vector<MyRect> rectangles = il | views::join | ranges::to<vector>();
//TODO:
//	const vector<MyRect> rectangles = views::concat(input_rectangles, holes) | ranges::to<vector>();
	
	const int nr_emplacements = rectangles.size();
	
	const int N = rectangles.size();
	
	const vector<int> distance_matrix = views::cartesian_product(rectangles, rectangles) |
										views::transform([](auto arg){const auto [r1, r2]=arg;	return rectangle_distance(r1, r2);}) |
										ranges::to<vector>();
										
//TODO: use mdspan
	
	vector<DecisionTreeNode> decision_tree;
	
	auto walk_up_from = [&](int parent_index)->generator<int>{
		for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
		{
			co_yield index;
		}		
	};

    auto index2emplacement = [&](int idx){
		vector<int> emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		for (int ix : walk_up_from(idx) | ranges::to<vector>() | views::reverse)
		{
			const DecisionTreeNode& n = decision_tree[ix];
			swap(emplacement[n.i_emplacement_source], emplacement[n.i_emplacement_destination]);
		}
        return emplacement;
    };
	
	auto child_nodes = [&](int parent_index, int depth){

		vector<int> emplacement = index2emplacement(parent_index);

		return views::cartesian_product( views::iota(nr_input_rectangles, nr_emplacements),
									  views::iota(0, nr_input_rectangles) |
											views::filter([&](int r){return emplacement[r]==r;}) 
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
			
				int sigma_edge_distance = ranges::fold_left(edges | views::transform([&](const Edge& e){return distance_matrix[emplacement[swap_r_h(e.from)] * N + emplacement[swap_r_h(e.to)]];}),
					0, plus<int>()) ;
				
				return DecisionTreeNode{
					.index = -1,
					.parent_index = parent_index,
					.depth=depth,
					.sigma_edge_distance = sigma_edge_distance,
					.i_emplacement_source = r, 
					.i_emplacement_destination = h
				};
			}) |
			ranges::to<vector>();
	};
	
	auto build_decision_tree = [&](){
		printf("enter build_decision_tree()\n");
		
		for (int depth=0; depth<=7; depth++)
		{	
			int size = decision_tree.size();
			
			vector<int> indexes = decision_tree 
						| views::filter([&](const DecisionTreeNode& n){return n.depth==depth-1;})
						| views::transform(&DecisionTreeNode::index)
						| ranges::to<vector>() ;
						
			ranges::sort(indexes, {}, [&](int idx){return decision_tree[idx].sigma_edge_distance;});
			if (indexes.size() > 2000)
				indexes.resize(2000);

			if (depth==0)
				indexes.push_back(-1);

			vector<vector<DecisionTreeNode> > vv(indexes.size());
			transform(execution::par_unseq, begin(indexes), end(indexes), begin(vv), [&](int parent_index){return child_nodes(parent_index, depth);});
			
			ranges::copy( vv | views::join, back_inserter(decision_tree));
			
			for (int i=size; i<decision_tree.size(); i++)
				decision_tree[i].index = i;
		}
		
		printf("exit build_decision_tree()\n");
	};
	
	build_decision_tree();
	
	return decision_tree;
}


vector<DecisionTreeNode> compute_decision_subtree(const vector<DecisionTreeNode>& decision_tree, int count)
{
	auto walk_up_from = [&](int parent_index)->generator<int>{
		for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
		{
			co_yield index;
		}		
	};
	
    //const vector<int> v = {6,9,9,6};
    //const auto dense_rank = views::zip(v | ranges::to<set>(), views::iota(0)) | ranges::to<unordered_map>() ;
	
	const int n = decision_tree.size();
	vector<int> index = views::iota(0, n) | ranges::to<vector>();
	ranges::sort(index, {}, [&](int id){return decision_tree[id].sigma_edge_distance;});
	
	vector<int> subtree_index = index | 
					views::take(count) |
					views::transform(walk_up_from) |
					views::join |
					ranges::to<vector>() |	//doesn't compile without this
					ranges::to<set>() |
					ranges::to<vector>() ;

	ranges::sort(subtree_index, {}, [&](int id){return decision_tree[id].sigma_edge_distance;});
	
	map<int, long int> subtree_index_map = subtree_index |
					views::enumerate |
					views::transform([](auto arg){const auto [i, id]=arg; return make_pair(id,i);}) |
					ranges::to<map>();
	
	vector<DecisionTreeNode> dst =  subtree_index |
		views::transform([&](int id){
			DecisionTreeNode node = decision_tree[id];
			assert(node.index == id);
			node.index = subtree_index_map[node.index];
			node.parent_index = subtree_index_map[node.parent_index];
			return node;}
		) |
		ranges::to<vector>();
	
	auto walk_up_from_ = [&](int parent_index)->generator<int>{
		for (int index=parent_index; index != -1; index = dst[index].parent_index)
		{
			co_yield index;
		}		
	};
		
	string buffer = views::iota(0, count) |
		views::transform([&](int id){
			return walk_up_from_(id) | 
				ranges::to<vector>() | 
				views::reverse |
				views::transform([&](int id){return dst[id];}) |
				views::transform([](const DecisionTreeNode& n){
					return format(R"({{"depth":{},"sigma_edge_distance":{},"i_emplacement_source":{},"i_emplacement_destination":{}}})",
						n.depth, n.sigma_edge_distance, n.i_emplacement_source, n.i_emplacement_destination);
				}) |
				views::join_with(",\n"s) ;
		}) |
		views::join_with("},\n{"s) |
		ranges::to<string>();
		
//	FILE* f=fopen("decision_tree.json", "w");
//	fprintf(f, "{%s}", buffer.c_str());
	printf("{%s}", buffer.c_str());
//	fclose(f);
	
	return dst;
}



int main()
{
	for (const auto& [edges, input_rectangles, holes, expected_decision] : test_contexts)
	{
		const int nr_emplacements = input_rectangles.size() + holes.size();
		
		vector<DecisionTreeNode> decision_tree = compute_decision_tree(edges, input_rectangles, holes);

		int count=20;
		vector<DecisionTreeNode> decision_subtree = compute_decision_subtree(decision_tree, count) ;
		
		printf("decision_tree.size()=%ld\n", decision_tree.size());
		
		auto walk_up_from = [&](int parent_index)->generator<int>{
			for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
			{
				co_yield index;
			}		
		};
		
		const DecisionTreeNode &bn = * ranges::min_element(decision_tree, {}, &DecisionTreeNode::sigma_edge_distance);
		
		vector<int> emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		
		for (int idx : walk_up_from(bn.index) | ranges::to<vector>() | views::reverse)
		{
			const DecisionTreeNode& n = decision_tree[idx];
			printf("{.index=%d, .parent_index=%d, .depth=%d, .sigma_edge_distance=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d},\n", 
					n.index, n.parent_index, n.depth, n.sigma_edge_distance, emplacement[n.i_emplacement_source], emplacement[n.i_emplacement_destination]);
			swap(emplacement[n.i_emplacement_source], emplacement[n.i_emplacement_destination]);
		}
		
		vector<int> expected_emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		for (const DecisionTreeNode &n : expected_decision)
			swap(expected_emplacement[n.i_emplacement_source], expected_emplacement[n.i_emplacement_destination]);
		
		bool bOk1 = emplacement == expected_emplacement;

		printf("bOk=%s\n", bOk1 ? "true" : "false");
	}
	return 0;
}