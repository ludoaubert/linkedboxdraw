#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <ranges>
#include <execution>
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
	int depth;
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
			{.index=0, .parent_index=-1, .depth=0, .i_emplacement_source=0, .i_emplacement_destination=3}
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


vector<DecisionTreeNode> compute_decision_tree(int nr_input_rectangles, int nr_emplacements, const vector<vector<int> >& logical_edges, const vector<vector<int> >& topological_edges)
{	
	vector<DecisionTreeNode> decision_tree;
	
	auto child_nodes = [&](int parent_index, int depth){

		vector<DecisionTreeNode> result;
		
		vector<int> chemin;
		for (int index=parent_index; index != -1; index = decision_tree[index].parent_index)
		{
			chemin.push_back(index);
		}
		
		int emplacement[100];
		for (int i=0; i<nr_emplacements; i++)
			emplacement[i]=i;
		
		for (int ix : chemin | views::reverse)
		{
			const auto& [index, parent_index, depth, i_emplacement_source, i_emplacement_destination] = decision_tree[ix];
			swap(emplacement[i_emplacement_source], emplacement[i_emplacement_destination]);
		}

		for (int h : views::iota(nr_input_rectangles, nr_emplacements))
		{
			//printf("parent_index=%d\n", parent_index);
			//printf("emplacement[h=%d]=%d\n", h, eh);
			
			auto rng = views::iota(0, nr_input_rectangles) |
						views::filter([&](int r){return emplacement[r]==r;}) ;
			
			for (int r : rng)
			{
				const vector<int>& adj_log_r = logical_edges[r];
				const vector<int>& adj_topo_r = topological_edges[r];
				
				const vector<int>& adj_topo_eh = topological_edges[emplacement[h]];
				
				int inter=0, inter2=0, nb2=0;
				for (int ar : adj_log_r)
				{
					if (ar != emplacement[ar])
						nb2++;
					if (ranges::count(adj_topo_eh, emplacement[ar]) != 0)
					{
						inter++;
						if (ar != emplacement[ar])
							inter2++;
					}
				}
										
			// when we are late in the process (depth is high), rectangles in adj_log_r which have already been moved (thus will not move again)
			// we have two make sure we stay close to them. A little like an entropy measure (?)

				
				if ( (depth < 2 || (depth < 7 && inter2==nb2)) 
					&&
					adj_log_r.size() <= 2
					&&
				   (ranges::count(adj_topo_r, emplacement[h])!=0 || depth==0)
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
		}

		return result;
	};
	
	auto build_decision_tree = [&]()->void{
		//printf("enter build_decision_tree()\n");
		
		for (int depth=0; depth<=7; depth++)
		{
			int size = decision_tree.size();
			
			auto rg = decision_tree 
						| views::filter([&](const DecisionTreeNode& n){return n.depth==depth-1;})
						| views::transform(&DecisionTreeNode::index) ;
			vector<int> indexes ;
			for (int ix : rg)
				indexes.push_back(ix);
			if (depth==0)
				indexes.push_back(-1);
			vector<vector<DecisionTreeNode> > vv(indexes.size());
			transform(execution::par_unseq, begin(indexes), end(indexes), begin(vv), [&](int parent_index){return child_nodes(parent_index, depth);});
			
			for (const DecisionTreeNode& n : vv | views::join)
				decision_tree.push_back(n);
			
			auto hex=[&](int idx)->string
			{
				vector<int> chemin;
				for (int index=idx; index != -1; index = decision_tree[index].parent_index)
				{
					chemin.push_back(index);
				}

				int emplacement[100];
				for (int i=0; i<nr_emplacements; i++)
					emplacement[i]=i;
				
				for (int ix : chemin | views::reverse)
				{
					const auto& [index, parent_index, depth, i_emplacement_source, i_emplacement_destination] = decision_tree[ix];
					swap(emplacement[i_emplacement_source], emplacement[i_emplacement_destination]);
				}

//TODO: use views::join_with(',') instead of "%x,".
				auto rg = views::counted(emplacement, nr_emplacements) | 
						views::transform([](int i)->string{char buf[10]; sprintf(buf, "%x,", i); return buf;}) |
						views::join;
						
				string result;
				for (char c : rg)
					result.push_back(c);

				return result;
			};

			struct Item{
				string hex;
				int i;
			};
			vector<Item> items;
			for (int i=0; i+size<decision_tree.size(); i++)
			{
				items.push_back(Item{.hex=hex(i+size), .i=i+size});
			}
			
			ranges::sort(items, {}, &Item::hex);
/*
//science fiction
			vector<DecisionTreeNode> dedup = items | 
										views::chunk_by_key(&Item::hex) |
										views::transform([](auto r){return r[0];}) |
										views::transform(&Item::i) |
										views::transform([&](int i){return decision_tree[i];}) |
										views::to<vector> ;
//which is not so much better than the version using loops. It may be easier to understand.
// chunk_by_key or chunk_on ?
			auto view = items | views::chunk_by([](const Item& x, const Item& y){return x.hex==y.hex;});
			auto rg = view | views::transform([](auto const subrange){return views::iota(0, subrange.size());})
				| views::join;
*/
			vector<DecisionTreeNode> dedup;
			
			for (int j=0; j<items.size(); j++)
			{
				if (j==0 || items[j].hex != items[j-1].hex)
				{
					dedup.push_back(decision_tree[items[j].i]);
				}
			}

			for (int i=0; i<dedup.size(); i++)
			{
				dedup[i].index = size+i;
			}				
			
			decision_tree.resize(size);
			for (const DecisionTreeNode& n : dedup)
				decision_tree.push_back(n);
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
		
		vector<Edge> topological_edges_; 

		for (int from : views::iota(0, nr_emplacements))
		{
			for (int to : topological_edges[from])
			{
				topological_edges_.push_back(Edge{from,to});
			}
		}			
		
		auto f=[&](int idx)->int{
			vector<int> chemin;
			for (int index=idx; index != -1; index = decision_tree[index].parent_index)
			{
				chemin.push_back(index);
			}

			auto emplacement = [&](int i){
				int ei=i;
				for (int idx : chemin | views::reverse)
				{
					const auto& [index, parent_index, depth, i_emplacement_source, i_emplacement_destination] = decision_tree[idx];
					if (i_emplacement_source==ei)
						ei = i_emplacement_destination;
					else if (i_emplacement_destination==ei)
						ei = i_emplacement_source;
				}
				return ei;
			};

			vector<Edge> topo_edges, inter;		

			for (int from=0; from<nr_input_rectangles; from++)
			{
				for (int to : logical_edges[from])
				{
					Edge te={emplacement(from),emplacement(to)};
					topo_edges.push_back(te);
				}
			}
			ranges::sort(topo_edges, [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});
					
			ranges::set_intersection(topo_edges, topological_edges_, back_inserter(inter), [](const Edge& e1, const Edge& e2){
						return (e1.from != e2.from) ? e1.from < e2.from : e1.to < e2.to;
					});
			return inter.size();
		};
		
		int n = decision_tree.size();
		vector<int> scores(n), indexes(n);
		
		for (int idx : views::iota(0,n))
			indexes[idx]=idx;
		
//		vector indexes = views::iota(0,n) | views::to<vector>;
		
		transform(execution::par_unseq, begin(indexes), end(indexes), begin(scores), f);

		auto [min_score, max_score] = ranges::minmax(scores);
		
		printf("max_score=%d\n", max_score);
#if 0		
		for (int best_idx : views::iota(0,n) | views::filter([&](int idx){return scores[idx]==max_score;}))
		{
			printf("\n\nbest_idx=%d\n", best_idx);
			
			for (int idx = best_idx; idx != -1; idx = decision_tree[idx].parent_index)
			{
				const DecisionTreeNode& n = decision_tree[idx];
				int depth=0;
				for (int index=n.parent_index; index!=-1; index=decision_tree[index].parent_index)
					depth++;
				printf("%.*s{.index=%d, .parent_index=%d, .i_emplacement_source=%d, .i_emplacement_destination=%d},\n", depth, "\t\t\t\t\t\t\t\t\t\t", n.index, n.parent_index, n.i_emplacement_source, n.i_emplacement_destination);
			}
		}
#endif
		printf("\nstatistics on duplication:\n");
		
		unordered_map<string, int> distrib;
		map<int,int> stat;
		
		for (int idx : views::iota(0,n))
		{
			vector<int> chemin;
			for (int index=idx; index != -1; index = decision_tree[index].parent_index)
			{
				chemin.push_back(index);
			}

			int emplacement[100];
			for (int i=0; i<nr_emplacements; i++)
				emplacement[i]=i;
			
			for (int ix : chemin | views::reverse)
			{
				const auto& [index, parent_index, depth, i_emplacement_source, i_emplacement_destination] = decision_tree[ix];
				swap(emplacement[i_emplacement_source], emplacement[i_emplacement_destination]);
			}
			
			auto rg = views::counted(emplacement, nr_emplacements) | 
					views::transform([](int i)->string{char buf[10]; sprintf(buf, "%x", i); return buf;}) |
					views::join;
					
			string hex;
			for (char c : rg)
				hex.push_back(c);

			distrib[hex]++;
		}
		
		for (const auto [hex, count] : distrib)
			stat[count]++;
		
		for (const auto [count, freq] : stat)
			printf("count:%d, freq:%d\n", count, freq);

		{
			auto expected_emplacement = [&](int i){
				int ei=i;
				for (const auto& [index, parent_index, depth, i_emplacement_source, i_emplacement_destination] : expected_decision_tree)
				{
					if (i_emplacement_source==ei)
						ei = i_emplacement_destination;
					else if (i_emplacement_destination==ei)
						ei = i_emplacement_source;
				}
				return ei;
			};
			
			vector<Edge> topo_edges, inter;
			for (int from=0; from<nr_input_rectangles; from++)
			{
				for (int to : logical_edges[from])
				{
					Edge te={expected_emplacement(from),expected_emplacement(to)};
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
		
		int nb = ranges::count(scores, max_score);
		
		printf("\n\nranges::count(scores, %d)=%d\n", max_score, nb);
	}
	return 0;
}