#include <vector>
#include <ranges>
#include <stdio.h>
using namespace std;


struct Edge {
	int from;
	int to;

	friend auto operator<=>(const Edge&, const Edge&) = default;
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
		for (int index : chemin | views::reverse())
		{
			const auto& [parent_index, i_emplacement_source, i_emplacement_destination] = decision_tree[index];
			if (i_emplacement_source==ei)
				ei = i_emplacement_destination;
			if (i_emplacement_destination==ei)
				ei = i_emplacement_source;
		}
		return ei;
	};
	
	for (int i : views::iota(nr_input_rectangles, nr_emplacements))
		printf("%d\n", emplacement(i));
	return 0;
}