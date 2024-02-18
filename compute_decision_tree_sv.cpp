/*
0-take a look at function compute_holes(). There seem to be various versions.
1-first version: do not try to optimize the computation of emplacement.
*/
#include <algorithm>
#include <functional>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <ranges>
#include <initializer_list>
#include <tuple>
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
	vector<Edge> logical_edges;
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


float rectangle_distance(const MyRect& r1, const MyRect& r2)
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
		.logical_edges={
			{0, 1},
			{1, 0}
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
		.logical_edges={
			{0,1},
			{1,0},{1,2},
			{2,1}
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
		.logical_edges={
			{0,3},
			{1,7},
			{2,7},
			{3,0},{3,4},
			{4,3},{4,7},
			{5,7},
			{6,7},{6,12},
			{7,1},{7,2},{7,4},{7,5},{7,6},{7,8},{7,9},{7,10},{7,11},
			{8,7},
			{9,7},
			{10,7},
			{11,7},
			{12,6},{12,13},
			{13,12},{13,14},
			{14,13}
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


vector<DecisionTreeNode> compute_decision_tree(const vector<Edge>& logical_edges, const vector<MyRect>& input_rectangles, const vector<MyRect>& holes)
{
	const int nr_input_rectangles = input_rectangles.size();

	auto il = {input_rectangles, holes};
	const vector<MyRect> rectangles = il | views::join | ranges::to<vector>();
	
	const int nr_emplacements = rectangles.size();
	
	const int N = rectangles.size();
	
	const vector<float> distance_matrix = views::cartesian_product(rectangles, rectangles) |
										views::transform([](auto arg){const auto [r1, r2]=arg;	return rectangle_distance(r1, r2);}) |
										ranges::to<vector>();
	
	vector<DecisionTreeNode> decision_tree;
	
	auto walk_up_from = [&](int parent_index)->generator<int>{
		for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
		{
			co_yield index;
		}		
	};
	
	auto child_nodes = [&](int parent_index, int depth){

		vector<DecisionTreeNode> result;
		
		vector<int> emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		
		for (int ix : walk_up_from(parent_index) | ranges::to<vector>() | views::reverse)
		{
			const DecisionTreeNode& n = decision_tree[ix];
			swap(emplacement[n.i_emplacement_source], emplacement[n.i_emplacement_destination]);
		}
		
		for (auto const [h, r] : views::cartesian_product( views::iota(nr_input_rectangles, nr_emplacements),
															views::iota(0, nr_input_rectangles) |
															views::filter([&](int r){return emplacement[r]==r;}) ))
		{
			vector<int> emplacement_ = emplacement ;
			swap(emplacement_[r], emplacement_[emplacement[h]]);
			
			int sigma_edge_distance = ranges::fold_left(logical_edges | views::transform([&](const Edge& e){return distance_matrix[emplacement_[e.from] * N + emplacement_[e.to]];}), 0, plus<int>()) ;
				
			result.push_back(DecisionTreeNode{
				.index = result.size(),
				.parent_index = parent_index,
				.depth=depth,
				.sigma_edge_distance = sigma_edge_distance,
				.i_emplacement_source = r, 
				.i_emplacement_destination = emplacement[h]
			});
		}

		return result;
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
			
			int floor_size = decision_tree.size() - size;
			
			string hexbuf(10*nr_emplacements*floor_size, ' ');
			int pos=0;
			
			struct Item{
				string_view hex;
				int idx;
			};
			vector<Item> items;
			for (int idx=size; idx<decision_tree.size(); idx++)
			{
				vector<int> emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
				
				for (int ix : walk_up_from(idx) | ranges::to<vector>() | views::reverse)
				{
					const DecisionTreeNode& n = decision_tree[ix];
					swap(emplacement[n.i_emplacement_source], emplacement[n.i_emplacement_destination]);
				}

				int n=0 ;
				for (int i=0; i < nr_emplacements; i++)
				{
					n += sprintf(&hexbuf[pos+n], "%x,", emplacement[i]);
				}

				string_view hex(&hexbuf[pos], n);
				pos += n;
				
				items.push_back(Item{.hex=hex, .idx=idx});
			}
			
			ranges::sort(items, {}, &Item::hex);

			vector<DecisionTreeNode> dedup = items |
											views::chunk_by([](const Item& x, const Item& y){return x.hex==y.hex;}) |
											views::transform([&](auto r){return decision_tree[ r[0].idx ];}) |
											ranges::to<vector>();

			for (int i=0; i<dedup.size(); i++)
			{
				dedup[i].index = size+i;
			}				
			
			printf("floor_size=%d\n", floor_size);
			printf("dedup.size()=%d\n", dedup.size());
			
			decision_tree.resize(size);
			ranges::copy(dedup, back_inserter(decision_tree));
		}
	};
	
	build_decision_tree();
	
	return decision_tree;
}


int main()
{
	for (const auto& [logical_edges, input_rectangles, holes, expected_decision] : test_contexts)
	{
		const int nr_emplacements = input_rectangles.size() + holes.size();
		
		vector<DecisionTreeNode> decision_tree = compute_decision_tree(logical_edges, input_rectangles, holes);
		
		printf("decision_tree.size()=%ld\n", decision_tree.size());
		
		auto walk_up_from = [&](int parent_index)->generator<int>{
			for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
			{
				co_yield index;
			}		
		};
		
		const DecisionTreeNode &bn = * ranges::min_element(decision_tree, {}, &DecisionTreeNode::sigma_edge_distance);
		
		for (int idx : walk_up_from(bn.index) | ranges::to<vector>() | views::reverse)
		{
			const DecisionTreeNode& n = decision_tree[idx];
			printf("{.index=%d, .parent_index=%d, .depth=%d, .sigma_edge_distance=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d},\n", 
					n.index, n.parent_index, n.depth, n.sigma_edge_distance, n.i_emplacement_source, n.i_emplacement_destination);
		}
		
		vector<int> expected_emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		for (const DecisionTreeNode &n : expected_decision)
			swap(expected_emplacement[n.i_emplacement_source], expected_emplacement[n.i_emplacement_destination]);

		vector<int> emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
		for (int idx : walk_up_from(bn.index) | ranges::to<vector>() | views::reverse)
		{
			const DecisionTreeNode& n = decision_tree[idx];
			swap(emplacement[n.i_emplacement_source], emplacement[n.i_emplacement_destination]);
		}
		
		bool bOk1 = emplacement == expected_emplacement;

		printf("bOk=%s\n", bOk1 ? "true" : "false");
	}
	return 0;
}