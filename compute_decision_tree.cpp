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
	vector<vector<int> > logical_edges;
	vector<vector<int> > topological_edges;
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
			{1},//0
			{0}//1
		},

		.topological_edges={
			{2},//0
			{2},//1
			{0,1}//2
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
			{1},//0
			{0,2},//1
			{1}//2
		},
		.topological_edges={
			{3},//0
			{2,3},//1
			{1},//2
			{0,1}//3
		},

		.expected_decision_tree = {
			{.index=0, .parent_index=-1, .i_emplacement_source=0, .i_emplacement_destination=3}
		}
	},
	
	{
		.nr_input_rectangles = 15,
		.nr_emplacements = 15 + 11,
		
		.logical_edges={
			{3},//0
			{7},//1
			{7},//2
			{0,4},//3
			{3,7},//4
			{7},//5
			{7,12},//6
			{1,2,4,5,6,8,9,10,11},//7
			{7},//8
			{7},//9
			{7},//10
			{7},//11
			{6,13},//12
			{12,14},//13
			{13}//14
		},

		.topological_edges={
			{3,6,17,22},//0
			{2,7,14,15,18},//1
			{1,4,6,7,17},//2
			{0,4,10,17,23},//3
			{2,3,7,10,17},//4
			{7,10,11},//5
			{0,2,12,13,17,22},//6
			{1,2,4,5,8,9,11,18},//7
			{7,14,18,19,21},//8
			{7,11,20},//9
			{3,4,5,23},//10
			{5,7,9,20},//11
			{6,13,22,24},//12
			{6,12,14,15,16},//13
			{1,8,13,15,16,18,21},//14
			{1,13,14},//15
			{13,14,25},//16
			{0,2,3,4,6},//17
			{1,7,8,14},//18
			{8,21},//19
			{9,11},//20
			{8,14,19,25},//21
			{0,6,12,24},//22
			{3,10},//23
			{12,22},//24
			{16,21}//25
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


vector<DecisionTreeNode> compute_decision_tree(int nr_input_rectangles, int nr_emplacements, const vector<vector<int> >& logical_edges, const vector<vector<int> >& topological_edges)
{	
	vector<DecisionTreeNode> decision_tree;
	
	int emplacement[100];
	for (int i=0; i<100; i++)
		emplacement[i]=i;
	
	auto build_decision_tree = [&](int parent_index, auto&& build_decision_tree, int depth=0)->void{
		//printf("enter build_decision_tree()\n");

		for (int h : views::iota(nr_input_rectangles, nr_emplacements))
		{
			int eh = emplacement[h];
			//printf("parent_index=%d\n", parent_index);
			//printf("emplacement[h=%d]=%d\n", h, eh);
			
			auto rng = views::iota(0, nr_input_rectangles) |
						views::filter([&](int r){return emplacement[r]==r;}) ;
			
			for (int r : rng)
			{
				const vector<int>& adj_log_r = logical_edges[r];
				const vector<int>& adj_topo_r = topological_edges[r];
				
				const vector<int>& adj_topo_eh = topological_edges[eh];
				
				vector<int> inter;
				ranges::set_intersection(adj_log_r | views::transform([&](int r){return emplacement[r];}), 
				                        adj_topo_eh, 
				                        back_inserter(inter));
				
				if (depth < 6 
					&&
					adj_log_r.size() <= 2
					&&
				   (ranges::binary_search(adj_topo_r, eh) || depth==0)
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
					
					swap(emplacement[r], emplacement[eh]);
					
					build_decision_tree(index, build_decision_tree, depth+1);
					
					swap(emplacement[r], emplacement[eh]);
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
		
		vector<vector<int> > dt_adj(decision_tree.size()+1);
		
		for (const auto [index, parent_index, i_emplacement_source, i_emplacement_destination] : decision_tree)
		{
			dt_adj[parent_index+1].push_back(index);
		}
		
		
		
		vector<Edge> topological_edges_;
		
		for (int from=0; from<nr_emplacements; from++)
		{
			for (int to : topological_edges[from])
			{
				Edge te={from,to};
				topological_edges_.push_back(te);
			}
		}			
		
		int best_idx=-1;
		int best_result=0;
		
		int emplacement[100];
		for (int i=0; i < 100; i++)
			emplacement[i]=i;
		
		auto find_best_result = [&](int parent_index, auto&& find_best_result)->void{
			for (int idx : dt_adj[parent_index+1])
			{
				const auto& [index, parent_index, i_emplacement_source, i_emplacement_destination] = decision_tree[idx];
				swap(emplacement[i_emplacement_source], emplacement[i_emplacement_destination]);
				
				vector<Edge> topo_edges, inter;
				for (int from=0; from<nr_input_rectangles; from++)
				{
					for (int to : logical_edges[from])
					{
						Edge te={.from=emplacement[from],.to=emplacement[to]};
						topo_edges.push_back(te);
					}
				}
				ranges::sort(topo_edges, [](const Edge& e1, const Edge& e2){
							return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
						});
						
				ranges::set_intersection(topo_edges, topological_edges_, back_inserter(inter), [](const Edge& e1, const Edge& e2){
							return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
						});
				
				if (inter.size() > best_result)
				{
					best_idx = idx;
					best_result = inter.size();
				}
				
				find_best_result(index, find_best_result);
				
				swap(emplacement[i_emplacement_source], emplacement[i_emplacement_destination]);
			}
		};
		
		find_best_result(-1, find_best_result);

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
			for (int i=0; i < 100; i++)
				emplacement[i]=i;
		
			for (const auto& [index, parent_index, i_emplacement_source, i_emplacement_destination] : expected_decision_tree)
			{
				swap(emplacement[i_emplacement_source], emplacement[i_emplacement_destination]);
			}
			
			vector<Edge> topo_edges, inter;
			for (int from=0; from<nr_input_rectangles; from++)
			{
				for (int to : logical_edges[from])
				{
					Edge te={.from=emplacement[from], .to=emplacement[to]};
					topo_edges.push_back(te);
				}
			}
			ranges::sort(topo_edges, [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});
					
			ranges::set_intersection(topo_edges, topological_edges_, back_inserter(inter), [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});	
					
			printf("expected result=%ld\n", inter.size());
		}
	}
	return 0;
}