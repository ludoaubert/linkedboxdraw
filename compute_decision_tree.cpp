#include <algorithm>
#include <vector>
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
};

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

const int nr_input_rectangles = 2;
const int nr_emplacements = 3;

const vector<Edge> logical_edges={
	{.from=0, .to=1},
	{.from=1, .to=0}
};

const vector<Edge> topological_edges={
	{.from=0, .to=2},
	{.from=1, .to=2},
	{.from=2, .to=0},
	{.from=2, .to=1},	
};

vector<DecisionTreeNode> decision_tree;

int main()
{
	int parent_index = -1;
	
	vector<int> chemin;
	while (parent_index != -1)
	{
		chemin.push_back(parent_index);
		parent_index = decision_tree[parent_index].parent_index;
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
					decision_tree.push_back({
						.parent_index = parent_index,
						.i_emplacement_source = r, 
						.i_emplacement_destination = moved_te.from
					});
				}
			}
		}
	}
	return 0;
}