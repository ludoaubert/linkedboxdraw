#include <algorithm>
#include <vector>
#include <string>
#include <ranges>
#include <stdio.h>
using namespace std;


struct Edge {
	int from;
	int to;
};


struct DecisionTreeNode
{
	int index=0;
	int parent_index=-1;
	int i_emplacement_source, i_emplacement_destination;
	
	friend bool operator==(const DecisionTreeNode&, const DecisionTreeNode&) = default;
};

struct TestContext{
	int nr_input_rectangles;
	int nr_emplacements;
	vector<Edge> logical_edges;
	vector<Edge> topological_edges;
	vector<DecisionTreeNode> expected_decision_tree;
};


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
		.nr_input_rectangles = 2,
		.nr_emplacements = 3,

		.logical_edges={
			{.from=0, .to=1},
			{.from=1, .to=0}
		},

		.topological_edges={
			{.from=0, .to=2},
			{.from=1, .to=2},
			{.from=2, .to=0},
			{.from=2, .to=1},	
		},

		.expected_decision_tree = {
			{.index=0, .parent_index=-1, .i_emplacement_source=0, .i_emplacement_destination=2}
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
		.nr_input_rectangles = 3,
		.nr_emplacements = 4,

		.logical_edges={
			{.from=0, .to=1},
			{.from=1, .to=0},
			{.from=1, .to=2},
			{.from=2, .to=1}
		},

		.topological_edges={
			{.from=0, .to=3},
			{.from=1, .to=2},
			{.from=1, .to=3},
			{.from=2, .to=1},
			{.from=3, .to=0},
			{.from=3, .to=1}			
		},

		.expected_decision_tree = {
			{.index=0, .parent_index=-1, .i_emplacement_source=0, .i_emplacement_destination=3}
		}
	},
	
	{
		.nr_input_rectangles = 15,
		.nr_emplacements = 15 + 11,
		
		.logical_edges={
			{.from=0, .to=3},
			{.from=1, .to=7},
			{.from=2, .to=7},
			{.from=3, .to=0},
			{.from=3, .to=4},
			{.from=4, .to=3},
			{.from=4, .to=7},
			{.from=5, .to=7},
			{.from=6, .to=7},
			{.from=6, .to=12},
			{.from=7, .to=1},
			{.from=7, .to=2},
			{.from=7, .to=4},
			{.from=7, .to=5},
			{.from=7, .to=6},
			{.from=7, .to=8},
			{.from=7, .to=9},
			{.from=7, .to=10},
			{.from=7, .to=11},
			{.from=8, .to=7},
			{.from=9, .to=7},
			{.from=10, .to=7},
			{.from=11, .to=7},
			{.from=12, .to=6},
			{.from=12, .to=13},
			{.from=13, .to=12},
			{.from=13, .to=14},
			{.from=14, .to=13}
		},
		
		.topological_edges={
			{.from=0, .to=3},
			{.from=0, .to=6},
			{.from=0, .to=15+2},
			{.from=0, .to=15+7},
			{.from=1, .to=2},
			{.from=1, .to=7},
			{.from=1, .to=14},
			{.from=1, .to=15+0},
			{.from=1, .to=15+3},
			{.from=2, .to=1},
			{.from=2, .to=4},
			{.from=2, .to=6},
			{.from=2, .to=7},
			{.from=2, .to=15+2},
			{.from=3, .to=0},
			{.from=3, .to=4},
			{.from=3, .to=10},
			{.from=3, .to=15+2},
			{.from=3, .to=15+8},
			{.from=4, .to=2},
			{.from=4, .to=3},
			{.from=4, .to=7},
			{.from=4, .to=10},
			{.from=4, .to=15+2},
			{.from=5, .to=7},
			{.from=5, .to=10},
			{.from=5, .to=11},
			{.from=6, .to=0},
			{.from=6, .to=2},
			{.from=6, .to=12},
			{.from=6, .to=13},
			{.from=6, .to=15+2},
			{.from=6, .to=15+7},
			{.from=7, .to=1},
			{.from=7, .to=2},
			{.from=7, .to=4},
			{.from=7, .to=5},
			{.from=7, .to=8},
			{.from=7, .to=9},
			{.from=7, .to=11},
			{.from=7, .to=15+3},
			{.from=8, .to=7},
			{.from=8, .to=14},
			{.from=8, .to=15+3},
			{.from=8, .to=15+4},
			{.from=8, .to=15+6},
			{.from=9, .to=7},
			{.from=9, .to=11},
			{.from=9, .to=15+5},
			{.from=10, .to=3},
			{.from=10, .to=4},
			{.from=10, .to=5},
			{.from=10, .to=15+8},
			{.from=11, .to=5},
			{.from=11, .to=7},
			{.from=11, .to=9},
			{.from=11, .to=15+5},
			{.from=12, .to=6},
			{.from=12, .to=13},
			{.from=12, .to=15+7},
			{.from=12, .to=15+9},
			{.from=13, .to=6},
			{.from=13, .to=12},
			{.from=13, .to=14},
			{.from=13, .to=15+0},
			{.from=13, .to=15+1},
			{.from=14, .to=1},
			{.from=14, .to=8},
			{.from=14, .to=13},
			{.from=14, .to=15+0},
			{.from=14, .to=15+1},
			{.from=14, .to=15+3},
			{.from=14, .to=15+6},
			{.from=15+0, .to=1},
			{.from=15+0, .to=13},
			{.from=15+0, .to=14},
			{.from=15+1, .to=13},
			{.from=15+1, .to=14},
			{.from=15+1, .to=15+10},
			{.from=15+2, .to=0},
			{.from=15+2, .to=2},
			{.from=15+2, .to=3},
			{.from=15+2, .to=4},
			{.from=15+2, .to=6},
			{.from=15+3, .to=1},
			{.from=15+3, .to=7},
			{.from=15+3, .to=8},
			{.from=15+3, .to=14},
			{.from=15+4, .to=8},
			{.from=15+4, .to=15+6},
			{.from=15+5, .to=9},
			{.from=15+5, .to=11},
			{.from=15+6, .to=8},
			{.from=15+6, .to=14},
			{.from=15+6, .to=15+4},
			{.from=15+6, .to=15+10},
			{.from=15+7, .to=0},
			{.from=15+7, .to=6},
			{.from=15+7, .to=12},
			{.from=15+7, .to=15+9},
			{.from=15+8, .to=3},
			{.from=15+8, .to=10},
			{.from=15+9, .to=12},
			{.from=15+9, .to=15+7},
			{.from=15+10, .to=15+1},
			{.from=15+10, .to=15+6}
		},
		
		.expected_decision_tree = {
			{.index=0, .parent_index=-1, .i_emplacement_source=1, .i_emplacement_destination=15+3},
			{.index=1, .parent_index=0, .i_emplacement_source=2, .i_emplacement_destination=1},
			{.index=2, .parent_index=1, .i_emplacement_source=0, .i_emplacement_destination=15+8},
			{.index=3, .parent_index=2, .i_emplacement_source=6, .i_emplacement_destination=2},
			{.index=4, .parent_index=3, .i_emplacement_source=12, .i_emplacement_destination=6},
			{.index=5, .parent_index=4, .i_emplacement_source=13, .i_emplacement_destination=15+7},
			{.index=6, .parent_index=5, .i_emplacement_source=14, .i_emplacement_destination=0}
		}
	}

};


vector<DecisionTreeNode> compute_decision_tree(int nr_input_rectangles, int nr_emplacements, const vector<Edge>& logical_edges, const vector<Edge>& topological_edges)
{	
	vector<DecisionTreeNode> decision_tree;
	
	auto build_decision_tree = [&](int parent_index, auto&& build_decision_tree, int depth=0)->void{
		//printf("enter build_decision_tree()\n");
		vector<int> chemin;
		for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
		{
			chemin.push_back(index);
		}

		auto emplacement = [&](int i){
			int ei=i;
			for (int idx : chemin | views::reverse)
			{
				const auto& [index, parent_index, i_emplacement_source, i_emplacement_destination] = decision_tree[idx];
				if (i_emplacement_source==ei)
					ei = i_emplacement_destination;
				else if (i_emplacement_destination==ei)
					ei = i_emplacement_source;
			}
			return ei;
		};

		for (int h : views::iota(nr_input_rectangles, nr_emplacements))
		{
			int eh = emplacement(h);
			//printf("parent_index=%d\n", parent_index);
			//printf("emplacement(h=%d)=%d\n", h, eh);
			
			auto rng = views::iota(0, nr_input_rectangles) |
						views::filter([&](int r){return ranges::none_of(chemin, [&](int idx){return decision_tree[idx].i_emplacement_source==r;});}) ;
			
			for (int r : rng)
			{
				auto adj_log_r = ranges::equal_range(logical_edges, r, ranges::less {}, &Edge::from);
				auto adj_topo_r = ranges::equal_range(topological_edges, r, ranges::less {}, &Edge::from);
				
				auto adj_topo_eh = ranges::equal_range(topological_edges, eh, ranges::less {}, &Edge::from);
				
				vector<int> inter;
				ranges::set_intersection(adj_log_r | views::transform(&Edge::to), 
				                        adj_topo_eh | views::transform(&Edge::to), 
				                        back_inserter(inter));
				
				if (depth < 6 
					&&
					adj_log_r.size() <= 2
					&&
				   (ranges::binary_search(adj_topo_r, eh, {}, &Edge::to) || depth==0)
					 &&
					inter.size() >= 1
				)
				{
					int index = decision_tree.size();
					
					const DecisionTreeNode n = {
						.index = index,
						.parent_index = parent_index,
						.i_emplacement_source = r, 
						.i_emplacement_destination = eh
					};

					//printf("{.index=%d, .parent_index=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d}\n", index, n.parent_index, n.i_emplacement_source, n.i_emplacement_destination);

					decision_tree.push_back(n);
					
					build_decision_tree(index, build_decision_tree, depth+1);
				}

			}
		}
	};
	
	int parent_index = -1;
	build_decision_tree(parent_index, build_decision_tree);
	
	return decision_tree;
}


int main()
{
	for (const auto& [nr_input_rectangles, nr_emplacements, logical_edges, topological_edges, expected_decision_tree] : test_contexts)
	{	
		vector<DecisionTreeNode> decision_tree = compute_decision_tree(nr_input_rectangles, nr_emplacements, logical_edges, topological_edges);
		
		bool bOk = expected_decision_tree == decision_tree;
		
		printf("decision_tree.size()=%ld\n", decision_tree.size());
		
		int best_idx=-1;
		int best_result=0;
		
		for (int idx=-1; idx < (int)decision_tree.size(); idx++)
		{
			vector<int> chemin;
			for (int index=idx; index != -1; index = decision_tree[index].parent_index)
			{
				chemin.push_back(index);
			}

			auto emplacement = [&](int i){
				int ei=i;
				for (int idx : chemin | views::reverse)
				{
					const auto& [index, parent_index, i_emplacement_source, i_emplacement_destination] = decision_tree[idx];
					if (i_emplacement_source==ei)
						ei = i_emplacement_destination;
					else if (i_emplacement_destination==ei)
						ei = i_emplacement_source;
				}
				return ei;
			};

			vector<Edge> topo_edges, inter;
			for (const Edge& le : logical_edges)
			{
				Edge te={emplacement(le.from),emplacement(le.to)};
				topo_edges.push_back(te);
			}
			ranges::sort(topo_edges, [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});
					
			ranges::set_intersection(topo_edges, topological_edges, back_inserter(inter), [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});
/*					
			printf("idx=%d\n", idx);
			printf("topo_edges:\n");
			for (const Edge& te : topo_edges)
				printf("{.fom=%d, .to=%d}\n", te.from, te.to);
			printf("inter:\n");
			for (const Edge& te : inter)
				printf("{.fom=%d, .to=%d}\n", te.from, te.to);			
*/					
			if (inter.size() > best_result)
			{
				best_idx = idx;
				best_result = inter.size();
			}
		}

		printf("\nbest_idx=%d\n", best_idx);
		printf("best_result=%d\n", best_result);

		for (int idx = best_idx; idx != -1; idx = decision_tree[idx].parent_index)
		{
			const DecisionTreeNode& n = decision_tree[idx];
			int depth=0;
			for (int index=n.parent_index; index!=-1; index=decision_tree[index].parent_index)
				depth++;
			printf("%.*s{.index=%d, .parent_index=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d},\n", depth, "\t\t\t\t\t\t\t\t\t\t", n.index, n.parent_index, n.i_emplacement_source, n.i_emplacement_destination);
		}
/*		
		for (const DecisionTreeNode& n : decision_tree)
		{
			int depth=0;
			for (int index=n.parent_index; index!=-1; index=decision_tree[index].parent_index)
				depth++;
			printf("%.*s{.index=%d, .parent_index=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d},\n", depth, "\t\t\t\t\t\t\t\t\t\t", n.index, n.parent_index, n.i_emplacement_source, n.i_emplacement_destination);
		}
*/
		{
			auto expected_emplacement = [&](int i){
				int ei=i;
				for (const auto& [index, parent_index, i_emplacement_source, i_emplacement_destination] : expected_decision_tree)
				{
					if (i_emplacement_source==ei)
						ei = i_emplacement_destination;
					else if (i_emplacement_destination==ei)
						ei = i_emplacement_source;
				}
				return ei;
			};
			
			vector<Edge> topo_edges, inter;
			for (const Edge& le : logical_edges)
			{
				Edge te={expected_emplacement(le.from),expected_emplacement(le.to)};
				topo_edges.push_back(te);
			}
			ranges::sort(topo_edges, [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});
					
			ranges::set_intersection(topo_edges, topological_edges, back_inserter(inter), [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});	
					
			printf("expected result=%ld\n", inter.size());
		}
	}
	return 0;
}