#include <algorithm>
#include <vector>
#include <string>
#include <ranges>
#include <stdio.h>
using namespace std;


struct Edge {
	int from;
	int to;

    // note: to is ignored by these comparison operators
    friend bool operator== (const Edge e1, const Edge e2) { return e1.from == e2.from; }
    friend auto operator<=>(const Edge e1, const Edge e2) { return e1.from <=> e2.from; }
};


struct DecisionTreeNode
{
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
	|   r0    |
	+---------+
	|   h0    |
	+---------+
	|   r1    |
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
			{.parent_index=-1, .i_emplacement_source=0, .i_emplacement_destination=2},
			{.parent_index=-1, .i_emplacement_source=1, .i_emplacement_destination=2}
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

		.expected_decision_tree = {}
	}
};


vector<DecisionTreeNode> compute_decision_tree(int nr_input_rectangles, int nr_emplacements, const vector<Edge>& logical_edges, const vector<Edge>& topological_edges)
{
	vector<DecisionTreeNode> decision_tree;
	
	auto build_decision_tree = [&](int parent_index, auto&& build_decision_tree)->void{
		vector<int> chemin;
		for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
		{
			chemin.push_back(index);
		}

		auto emplacement = [&](int i){
			int ei=i;
			for (int index : chemin | views::reverse)
			{
				const auto& [parent_index, i_emplacement_source, i_emplacement_destination] = decision_tree[index];
				if (i_emplacement_source==ei)
					ei = i_emplacement_destination;
				if (i_emplacement_destination==ei)
					ei = i_emplacement_source;
			}
			return ei;
		};
	/*	
	Description of the algorithm:
	loop on all holes h:
	-----figure out emplacement(h)
	-----loop on all rectangles r that have not already been moved before :
	----------if at least one logical edge of r becomes a topological edge by moving r from emplacement(r) to emplacement(h):
	-----------------move r to h.
	-----------------for all remaining logical links of r, do the same recursively
	*/
		for (int h : views::iota(nr_input_rectangles, nr_emplacements))
		{
			int eh = emplacement(h);
			printf("parent_index=%d\n", parent_index);
			printf("emplacement(h=%d)=%d\n", h, eh);
			for (int r : views::iota(0, nr_input_rectangles) | views::filter([&](int r){return emplacement(r)==r;}))
			{
				printf("emplacement(r=%d)=%d\n", r, r);
				
				auto rg = ranges::equal_range(logical_edges, Edge{.from=r,.to=-1});
				for (Edge const& le : rg)
				{
					printf("le={.from=%d, .to=%d}\n", le.from, le.to);
					Edge te = {.from=emplacement(le.from), .to=emplacement(le.to)};
					Edge moved_te = {.from=emplacement(h), .to=emplacement(le.to)};
					printf("te={.from=%d, .to=%d}\n", te.from, te.to);
					printf("moved_te={.from=%d, .to=%d}\n", moved_te.from, moved_te.to);				
					bool test = ranges::binary_search(topological_edges, moved_te, [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});
					printf(test ? "true\n" : "false\n");
					if (test)
					{
						const DecisionTreeNode n = {
							.parent_index = parent_index,
							.i_emplacement_source = r, 
							.i_emplacement_destination = moved_te.from
						};
						int index = decision_tree.size();

						printf("decision_tree[%d]={.parent_index=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d}\n", index, n.parent_index, n.i_emplacement_source, n.i_emplacement_destination);

						decision_tree.push_back(n);
						
						build_decision_tree(index, build_decision_tree);
					}
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
		
		for (const DecisionTreeNode& n : decision_tree)
		{
			printf("{.parent_index=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d},\n", n.parent_index, n.i_emplacement_source, n.i_emplacement_destination);
		}
	}
	return 0;
}