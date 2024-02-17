#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <ranges>
#include <generator>
#include <execution>
#include <stdio.h>
using namespace std;


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
			{0, 1},
			{1, 0}
		},

		.topological_edges={
			{0, 2},
			{1, 2},
			{2, 0},{2, 1}
		},

		.expected_decision_tree = {
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
		.nr_input_rectangles = 3,
		.nr_emplacements = 4,
		
		.logical_edges={
			{0,1},
			{1,0},{1,2},
			{2,1}
		},
		.topological_edges={
			{0,3},
			{1,2},{1,3},
			{2,1},
			{3,0},{3,1}
		},

		.expected_decision_tree = {
			{.index=0, .parent_index=-1, .depth=0, .i_emplacement_source=0, .i_emplacement_destination=3}
		}
	},
	
	{
		.nr_input_rectangles = 15,
		.nr_emplacements = 15 + 11,
		
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

		.topological_edges={
			{0,3},{0,6},{0,17},{0,22},//0
			{1,2},{1,7},{1,14},{1,15},{1,18},//1
			{2,1},{2,4},{2,6},{2,7},{2,17},//2
			{3,0},{3,4},{3,10},{3,17},{3,23},//3
			{4,2},{4,3},{4,7},{4,10},{4,17},//4
			{5,7},{5,10},{5,11},//5
			{6,0},{6,2},{6,12},{6,13},{6,17},{6,22},//6
			{7,1},{7,2},{7,4},{7,5},{7,8},{7,9},{7,11},{7,18},//7
			{8,7},{8,14},{8,18},{8,19},{8,21},//8
			{9,7},{9,11},{9,20},//9
			{10,3},{10,4},{10,5},{10,23},//10
			{11,5},{11,7},{11,9},{11,20},//11
			{12,6},{12,13},{12,22},{12,24},//12
			{13,6},{13,12},{13,14},{13,15},{13,16},//13
			{14,1},{14,8},{14,13},{14,15},{14,16},{14,18},{14,21},//14
			{15,1},{15,13},{15,14},//15
			{16,13},{16,14},{16,25},//16
			{17,0},{17,2},{17,3},{17,4},{17,6},//17
			{18,1},{18,7},{18,8},{18,14},//18
			{19,8},{19,21},//19
			{20,9},{20,11},//20
			{21,8},{21,14},{21,19},{21,25},//21
			{22,0},{22,6},{22,12},{22,24},//22
			{23,3},{23,10},//23
			{24,12},{24,22},//24
			{25,16},{25,21}//25
		},
		
		.expected_decision_tree = {
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


vector<DecisionTreeNode> compute_decision_tree(int nr_input_rectangles, int nr_emplacements, const vector<Edge>& logical_edges, const vector<Edge>& topological_edges)
{	
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
			const auto adj_log_r = ranges::equal_range(logical_edges, r, ranges::less {}, &Edge::from) ;
			const auto adj_topo_r = ranges::equal_range(topological_edges, r, ranges::less{}, &Edge::from);
			
			const auto adj_topo_eh = ranges::equal_range(topological_edges, emplacement[h], ranges::less{}, &Edge::from);
			
			int inter=0, inter2=0, nb2=0;
			for (const Edge& ar : adj_log_r)
			{
				if (ar.to != emplacement[ar.to])
					nb2++;
				if (ranges::count(adj_topo_eh | views::transform(&Edge::to), emplacement[ar.to]) != 0)
				{
					inter++;
					if (ar.to != emplacement[ar.to])
						inter2++;
				}
			}
									
		// when we are late in the process (depth is high), rectangles in adj_log_r which have already been moved (thus will not move again)
		// we have two make sure we stay close to them. A little like an entropy measure (?)

			
			if ( (depth < 2 || (depth < 7 && inter2==nb2)) 
				&&
				adj_log_r.size() <= 2
				&&
			   (ranges::count(adj_topo_r | views::transform(&Edge::to), emplacement[h])!=0 || depth==0)
				 &&
				inter >= 1
			)
			{
				int index = result.size();
				
				const DecisionTreeNode n = {
					.index = index,
					.parent_index = parent_index,
					.depth=depth,
					.i_emplacement_source = r, 
					.i_emplacement_destination = emplacement[h]
				};

				//printf("{.index=%d, .parent_index=%d, .depth=depth, .i_emplacement_source=%d, .i_emplacement_destination=%d}\n", index, n.parent_index, n.i_emplacement_source, n.i_emplacement_destination);

				result.push_back(n);
			}
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
			
			decision_tree.resize(size);
			ranges::copy(dedup, back_inserter(decision_tree));
		}
	};
	
	build_decision_tree();
	
	return decision_tree;
}


int main()
{
	for (const auto& [nr_input_rectangles, nr_emplacements, logical_edges, topological_edges, expected_decision_tree] : test_contexts)
	{
		vector<DecisionTreeNode> decision_tree = compute_decision_tree(nr_input_rectangles, nr_emplacements, logical_edges, topological_edges);
		
		bool bOk = expected_decision_tree == decision_tree;
		
		printf("decision_tree.size()=%ld\n", decision_tree.size());
		
		auto walk_up_from = [&](int parent_index)->generator<int>{
			for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
			{
				co_yield index;
			}		
		};
		
		auto f=[&](int idx)->int{

			auto emplacement = [&](int i){
				int ei=i;
				for (int idx : walk_up_from(idx) | ranges::to<vector>() | views::reverse)
				{
					const DecisionTreeNode& n = decision_tree[idx];
					if (n.i_emplacement_source==ei)
						ei = n.i_emplacement_destination;
					else if (n.i_emplacement_destination==ei)
						ei = n.i_emplacement_source;
				}
				return ei;
			};

			vector<Edge> topo_edges = logical_edges |
								views::transform([&](const Edge& e){return Edge{emplacement(e.from),emplacement(e.to)};}) |
								ranges::to<vector>(), 
				inter;		

			ranges::sort(topo_edges);
					
			ranges::set_intersection(topo_edges, topological_edges, back_inserter(inter));
			return inter.size();
		};
		
		int n = decision_tree.size();
		vector<int> scores(n), 
			indexes = views::iota(0,n) | ranges::to<vector>() ;
		
		transform(execution::par_unseq, begin(indexes), end(indexes), begin(scores), f);

		auto [min_score, max_score] = ranges::minmax(scores);
		
		printf("max_score=%d\n", max_score);
#if 0		
		for (int best_idx : views::iota(0,n) | views::filter([&](int idx){return scores[idx]==max_score;}))
		{
			printf("\n\nbest_idx=%d\n", best_idx);
			
			for (int idx : walk_up_from(best_idx))
			{
				const DecisionTreeNode& n = decision_tree[idx];
				int depth = ranges::distance(walk_up_from(n.parent_index));
				printf("%.*s{.index=%d, .parent_index=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d},\n", depth, "\t\t\t\t\t\t\t\t\t\t", n.index, n.parent_index, n.i_emplacement_source, n.i_emplacement_destination);
			}
		}
#endif
		printf("\nstatistics on duplication:\n");
		
		unordered_map<string, int> distrib;
		map<int,int> stat;
		
		for (int idx : views::iota(0,n))
		{
			vector<int> emplacement = views::iota(0, nr_emplacements) | ranges::to<vector>();
			
			for (int ix : walk_up_from(idx) | ranges::to<vector>() | views::reverse)
			{
				const DecisionTreeNode& n = decision_tree[ix];
				swap(emplacement[n.i_emplacement_source], emplacement[n.i_emplacement_destination]);
			}
			
			const string hex = emplacement | 
					views::transform([](int i)->string{char buf[10]; sprintf(buf, "%x", i); return buf;}) |
					views::join |
					ranges::to<string>();

			distrib[hex]++;
		}
		
		for (const auto [hex, count] : distrib)
			stat[count]++;
		
		for (const auto [count, freq] : stat)
			printf("count:%d, freq:%d\n", count, freq);

		{
			auto expected_emplacement = [&](int i){
				int ei=i;
				for (const DecisionTreeNode& n : expected_decision_tree)
				{
					if (n.i_emplacement_source==ei)
						ei = n.i_emplacement_destination;
					else if (n.i_emplacement_destination==ei)
						ei = n.i_emplacement_source;
				}
				return ei;
			};
			
			vector<Edge> topo_edges = logical_edges |
					views::transform([&](const Edge& e){return Edge{expected_emplacement(e.from),expected_emplacement(e.to)};}) |
					ranges::to<vector>(), 
				inter;

			ranges::sort(topo_edges);
					
			ranges::set_intersection(topo_edges, topological_edges, back_inserter(inter));	
					
			printf("expected result=%ld\n", inter.size());
		}
		
		int nb = ranges::count(scores, max_score);
		
		printf("\n\nranges::count(scores, %d)=%d\n", max_score, nb);
	}
	return 0;
}