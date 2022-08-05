#include <vector>
#include <string>
#include <deque>
#include <algorithm>
#include <ranges>
#include <span>
#include <stdio.h>
//#include <fmt/ranges.h>
//#include <format>
#include "MyRect.h"
using namespace std;


//Cf compute_box_rectangles.js
const int RECTANGLE_BOTTOM_CAP=200;


struct LogicalEdge {
	int from;
	int to;

	friend auto operator<=>(const LogicalEdge&, const LogicalEdge&) = default;
};


struct TopologicalEdge {
	int from;
	int to;
	int distance;

	friend auto operator<=>(const TopologicalEdge&, const TopologicalEdge&) = default;
};


struct MyVector
{
	float x=0.0f;
	float y=0.0f;

	operator MyPoint() const {
		return {x, y};
	}
};

inline MyVector operator*(int16_t value, const MyVector& vec)
{
	const auto& [x, y] = vec;
	return {value*x, value*y};
}


struct RectHole {int ri; RectCorner corner; MyVector direction; int value; MyRect rec;};

//	lightweight node

struct RectMap
{
	int i_emplacement_source, i_emplacement_destination;
};

struct DecisionTreeNode
{
	int parent_index=-1;
	int depth;
	RectMap recmap;
};

enum EtatEmplacement
{
	LIBRE,
	OCCUPE
};


const vector<MyRect> input_rectangles = {
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
};

//bi directional edges
vector<LogicalEdge> logical_edges = {
	{.from=0,.to=3},
	{.from=1,.to=7},
	{.from=2,.to=7},
	{.from=3,.to=0},
	{.from=3,.to=4},
	{.from=4,.to=3},
	{.from=4,.to=7},
	{.from=5,.to=7},
	{.from=6,.to=7},
	{.from=6,.to=12},
	{.from=7,.to=1},
	{.from=7,.to=2},
	{.from=7,.to=4},
	{.from=7,.to=5},
	{.from=7,.to=6},
	{.from=7,.to=8},
	{.from=7,.to=9},
	{.from=7,.to=10},
	{.from=7,.to=11},
	{.from=8,.to=7},
	{.from=9,.to=7},
	{.from=10,.to=7},
	{.from=11,.to=7},
	{.from=12,.to=6},
	{.from=12,.to=13},
	{.from=13,.to=12},
	{.from=13,.to=14},
	{.from=14,.to=13}
};


vector<RectHole> compute_holes(const vector<MyRect>& input_rectangles)
{
	MyRect frame={
		.m_left=ranges::min(input_rectangles | views::transform(&MyRect::m_left)),
		.m_right=ranges::max(input_rectangles | views::transform(&MyRect::m_right)),
		.m_top=ranges::min(input_rectangles | views::transform(&MyRect::m_top)),
		.m_bottom=ranges::max(input_rectangles | views::transform(&MyRect::m_bottom))
	};

	int n = input_rectangles.size();

	vector<RectHole> holes;

	for (int i=0; i<n; i++)
	{
		const MyRect& r = input_rectangles[i];
/*
	TOP_LEFT=0,
	BOTTOM_LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT
*/
		const MyPoint rect_corners[4]={
			{.x=r.m_left, .y=r.m_top},
			{.x=r.m_left, .y=r.m_bottom},
			{.x=r.m_right, .y=r.m_top},
			{.x=r.m_right, .y=r.m_bottom}
		};

		const MyVector directions[4][3]={
				{{.x=-1, .y=-1},{.x=+1, .y=-1},{.x=-1, .y=+1}},
				{{.x=-1, .y=+1},{.x=+1, .y=+1},{.x=-1, .y=-1}},
				{{.x=+1, .y=+1},{.x=+1, .y=-1},{.x=-1, .y=-1}},
				{{.x=-1, .y=+1},{.x=+1, .y=+1},{.x=+1, .y=-1}}
		};

		for (int rc : views::iota(0,4))
		{
			const MyPoint& pt = rect_corners[rc] ;

			for (const MyVector& dir : directions[rc])
			{
				int intervalle[2]={2, INT16_MAX};
				auto& [m, M] = intervalle;
				while (M > 1+m)
				{
					int value = M==INT16_MAX ? 2*m : (m+M)/2 ;
					MyRect rec = rect(pt, pt + value*dir);
					auto rg = input_rectangles | views::filter([&](const MyRect& r){return intersect_strict(rec,r) /*|| is_inside(r, rec)*/;});
					(rg.empty() && is_inside(rec,frame) ? m : M) = value;
					printf("{.i=%d, .RectCorner=%s, .direction={.x=%.0f, .y=%.0f} [.m=%d .M=%d]}\n", i, RectCornerString[rc], dir.x, dir.y, m, M);
				}

				if (m > 2)
				{
 					MyRect rec = rect(pt, pt + m*dir);
					holes.push_back({.ri=i, .corner=(RectCorner)rc, .direction=dir, .value=m, .rec=rec});
				}
			}
		}
	}

	ranges::sort(holes, {}, &RectHole::rec);
	vector<RectHole> holes_dedup;
	ranges::unique_copy(holes, back_inserter(holes_dedup), {}, &RectHole::rec);

	holes.clear();

	for (const auto& [ri, corner, direction, value, rec] : holes_dedup)
	{
		if (5*value >= RECTANGLE_BOTTOM_CAP)
		{
			holes.push_back({ri, corner, direction, value, rec});
		}
	}

	return holes;
};


template <typename EdgeType>
vector<int> compute_edge_partition(int n, vector<EdgeType>& edges)
{
	vector<int> edge_partition(n+1);
	edge_partition[0]=0;
	for (int pos=0, ii=0; ii<n; ii++)
	{
//C++23: use views::adjacent or views::slide
		int &start_pos = edge_partition[ii];
		int &end_pos = edge_partition[ii+1];
		end_pos = start_pos;
		for ( ; pos < edges.size() && edges[pos].from==ii; pos++)
		{
			end_pos = max(end_pos, pos+1);
		}
	}
	return edge_partition;
}


string print_html(const vector<MyRect>& input_rectangles, const vector<RectHole>& holes)
{
	int n = input_rectangles.size();
	char buffer[100*1000];
	int pos=0;

	MyRect frame={
		.m_left=ranges::min(input_rectangles | views::transform(&MyRect::m_left)),
 		.m_right=ranges::max(input_rectangles | views::transform(&MyRect::m_right)),
		.m_top=ranges::min(input_rectangles | views::transform(&MyRect::m_top)),
 		.m_bottom=ranges::max(input_rectangles | views::transform(&MyRect::m_bottom))
	};

	pos+= sprintf(buffer+pos, "<html>\n<body>\n");
	pos+= sprintf(buffer+pos, "<svg width=\"%d\" height=\"%d\">\n", width(frame)+100, height(frame));
	for (int ri=0; ri < input_rectangles.size(); ri++)
	{
		const MyRect& r = input_rectangles[ri];
		const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] = r;
		pos += sprintf(buffer+pos, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:blue;stroke:pink;stroke-width:5;opacity:0.5\" />\n",m_left, m_top, width(r), height(r));
		pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"red\">r-%d</text>\n", m_left, m_top, ri);

		int dy = 0;

		auto contacts = logical_edges |
			views::filter([&](const LogicalEdge& e){return e.from==ri;}) |
			views::transform(&LogicalEdge::to);

		for (int rj : contacts)
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"white\">r-%d</text>\n", m_left + 8, m_top + dy, rj);
		}

		dy = 0;
		for (int rj : views::iota(0, n) | views::filter([&](int rj){return ri != rj && edge_overlap(input_rectangles[ri], input_rectangles[rj]);}))
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", r.m_left + 30, r.m_top + dy, rj);
		}
	}

	for (int hi=0; hi < holes.size(); hi++)
	{
		const auto& [ri, RectCorner, direction, value, rec] = holes[hi];

		pos += sprintf(buffer+pos, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:red;stroke:green;stroke-width:5;opacity:0.5\" />\n",
					rec.m_left, rec.m_top, width(rec), height(rec));
		pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">hole-%d</text>\n", rec.m_left, rec.m_top, hi);

		int dy = 0;
		for (int rj : views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rec, input_rectangles[rj]);}))
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", rec.m_left + 8, rec.m_top + dy, rj);
		}
		pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">ri=%d</text>\n", rec.m_left + 30, rec.m_top + 1*14, ri);
	}

	pos += sprintf(buffer+pos, "</svg>\n</html>");
	buffer[pos]=0;
	return buffer;
}


bool filter(const LogicalEdge& e){

	int dist = rect_distance(input_rectangles[e.from], input_rectangles[e.to]);
	return dist <= 20;
};


vector<int> compute_connected_components(const vector<MyRect>& input_rectangles,
					const vector<LogicalEdge>& logical_edges,
					const vector<int>& logical_edge_partition)
{
	vector<int> connected_component(input_rectangles.size(), -1);

	auto rec_compute_connected_components = [&](int i, int c, auto&& rec_compute_connected_components)->void{
		connected_component[i] = c;
//TODO: C++23. auto [start_pos, end_pos] = (logical_edge_partition | view::slide(2))[i];
		int start_pos = logical_edge_partition[i];
		int end_pos = logical_edge_partition[i+1];
		for (int pos=start_pos; pos < end_pos; pos++)
		{
			const LogicalEdge& e = logical_edges[pos];
			if (connected_component[e.to] == -1 && filter(e))
				rec_compute_connected_components(e.to, c, rec_compute_connected_components);
		}
	};

	int c=0;
	while (true)
	{
		auto it=ranges::find(connected_component, -1);
		if (it == end(connected_component))
			break;
		int i = ranges::distance(begin(connected_component), it);
		rec_compute_connected_components(i, c++, rec_compute_connected_components);
	}
	return connected_component;
}

int main()
{
	const int n = input_rectangles.size();

	for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : input_rectangles)
	{
		printf("{.m_left=%d, .m_right=%d, .m_top=%d, .m_bottom=%d}\n", m_left, m_right, m_top, m_bottom);
	}
	vector<RectHole> holes = compute_holes(input_rectangles);

	FILE *f;
	f=fopen("topo_space.html", "w");
	string buffer=print_html(input_rectangles, holes);
	fprintf(f, "%s", buffer.c_str());
	fclose(f);

//La liste des rectangles et des trous devient une liste d'emplacements, et un graphe topologique.

	vector<MyRect> emplacements;
	for (const MyRect &r : input_rectangles)
		emplacements.push_back(r);
	for (const RectHole &rh : holes)
		emplacements.push_back(rh.rec);

	vector<TopologicalEdge> topological_edges;
	for (int i=0; i < emplacements.size(); i++)
	{
		for (int j=0; j < emplacements.size(); j++)
		{
			int dist = rect_distance(emplacements[i], emplacements[j]);
			if (dist < 20)
				topological_edges.push_back({.from=i, .to=j, .distance=dist});
		}
	}

	ranges::sort(topological_edges);

	vector<int> topological_edge_partition = compute_edge_partition(emplacements.size(), topological_edges);

//	fmt::print("topological_edge_partition: {}\n", topological_edge_partition);

	printf("topological_edge_partition: ");
	for (int pos : topological_edge_partition)
		printf("%d, ", pos);
	printf("\n");

	vector<int> logical_edge_partition = compute_edge_partition(input_rectangles.size(), logical_edges);

//	fmt::print("logical_edge_partition: {}\n", logical_edge_partition);

	printf("logical_edge_partition: ");
	for (int pos : logical_edge_partition)
		printf("%d, ", pos);
	printf("\n");

	vector<int> connected_component = compute_connected_components(input_rectangles, logical_edges, logical_edge_partition);

//	fmt::print("connected_component: {}\n", connected_component);

	printf("connected_component: ");
	for (int c : connected_component)
		printf("%d, ", c);
	printf("\n");

	int nb = ranges::max(connected_component);
	vector<int> cc_size(nb, 0);
	for (int c : connected_component)
		cc_size[c]++;
	auto it = ranges::max_element(cc_size);
	int cmax = ranges::distance(begin(cc_size), it);
// si connected_component[i]==cmax alors i ne doit pas etre deplacé.

	vector<int> recmap(input_rectangles.size());
	for (int i=0; i < recmap.size(); i++)
		recmap[i] = i;

	vector<EtatEmplacement> etat_emplacement(emplacements.size());
	vector<int> mapping(input_rectangles.size());

	vector<DecisionTreeNode> decision_tree;

	auto build_decision_tree = [&](int parent_index, auto&& build_decision_tree)->void{

		for (int i=0; i < input_rectangles.size(); i++)
		{
			printf("i=%d\n", i);

			if (connected_component[i] == cmax && parent_index != -1)
			{
				printf("connected_component[%d] == %d. skipping %d\n", i, cmax, i);
				continue;
			}


// par default, les intput_rectangles sont des emplacements non libres, les autres emplacements etant libres
			ranges::fill(etat_emplacement, LIBRE);
			for (int ii=0; ii < input_rectangles.size(); ii++)
				etat_emplacement[ii] = OCCUPE;

			deque<RectMap> chemin;

			for (int pos=parent_index; pos != -1; pos = decision_tree[pos].parent_index)
			{
				chemin.push_front( decision_tree[pos].recmap );
			}

			printf("chemin=");
			for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
				printf("{.i_emplacement_source=%d, i_emplacement_destination=%d},", i_emplacement_source, i_emplacement_destination);
			printf("\n");

			int depth = chemin.size();
			if (depth > 4)
				continue;

			for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
			{
				swap(etat_emplacement[i_emplacement_source], etat_emplacement[i_emplacement_destination]);
			}

			for (int i=0; i<input_rectangles.size(); i++)
				mapping[i]=i;
			for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
			{
				mapping[i_emplacement_source] = i_emplacement_destination;
			}

			if (mapping[i] != i)
			{
				printf("on ne mappe pas 2 fois un meme emplacement.\n");
				printf("mapping[%d] != %d\n", i, i);
				continue;
			}

//TODO: C++23. auto [start_pos1, end_pos1] = (logical_edge_partition | view::slide(2))[i];
			int start_pos1 = logical_edge_partition[i];
			int end_pos1 = logical_edge_partition[i+1];

		//si tous les liens de i {e sont des liens dont e.j n'a pas ete mappé et e.j que l'on peut deplacer}, alors i n'est pas
		// stable
			if (ranges::all_of(span(&logical_edges[start_pos1], end_pos1 - start_pos1),
					[&](const LogicalEdge& e){return mapping[e.to]==e.to && connected_component[e.to] != cmax;}
					)
			)
			{
				printf("tous les liens de %d {e sont des liens dont e.j n'a pas ete mappé et e.j que l'on peut deplacer}\n", i);
				printf("%d is not stable\n", i);
				continue;
			}

			for (int j=0; j < emplacements.size(); j++)
			{
				printf("i=%d j=%d h=%d\n", i, j, j-n);

				if (j == i)
					continue;

				if (etat_emplacement[j] == OCCUPE)
				{
					printf("etat_emplacement[%d] == OCCUPE\n", j);
					continue;
				}

			//on regarde si emplacements[j] intersecte un emplacement deja occupé
				if (ranges::any_of(views::iota(input_rectangles.size()) |
									views::take(emplacements.size() - input_rectangles.size()) |
									views::filter([&](int i){return i!=j;}) |
									views::filter([&](int i){return etat_emplacement[i]==OCCUPE;}),
									[&](int i){return intersect_strict(emplacements[i], emplacements[j]);}
									)
					)
				{
					printf("emplacements[%d] intersecte un emplacement deja occupé\n", j);
					continue;
				}

// l'emplacement j est-il topologiquement lié aux rectangles auxquels i est logiquement lié et que l'on ne peut pas deplacer ?
			//les rectangles auxquels i est logiquement lié et que l'on ne peut pas déplacer:
				auto rg1 = span(&logical_edges[start_pos1], end_pos1 - start_pos1) |
// si connected_component[i]==cmax alors i ne doit pas etre deplacé.
					views::filter([&](const LogicalEdge& e){return connected_component[e.to] == cmax;}) |
					views::transform([](const LogicalEdge& e){return e.to;});

				printf("rg1={");
				for (int i : rg1)
					printf(" %d,", i);
				printf("}\n");

//TODO: C++23. auto [start_pos2, end_pos2] = (topological_edge_partition | view::slide(2))[j];
				int start_pos2 = topological_edge_partition[j];
				int end_pos2 = topological_edge_partition[j+1];
			//les rectangles auxquels j est topologiquement lié:
				auto rg2 = span(&topological_edges[start_pos2], end_pos2 - start_pos2) |
					views::filter([&](const TopologicalEdge& e){return e.to < input_rectangles.size();}) |
					views::filter([&](const TopologicalEdge& e){return connected_component[e.to] == cmax;}) |
					views::transform([](const TopologicalEdge& e){return e.to;});

                                printf("rg2={");
                                for (int i : rg2)
                                        printf(" %d,", i);
                                printf("}\n");

				assert(ranges::is_sorted(rg1));
				assert(ranges::is_sorted(rg2));

				if (ranges::includes(rg2, rg1)==false)
				{
					printf("l'emplacement %d(h=%d) est-il topologiquement lié aux rectangles auxquels %d est logiquement lié et que l'on ne peut pas deplacer ?\n", j, j-n, i);
					printf("ranges::includes(rg2, rg1)==false\n");
					continue;
				}

			//ensuite on mappe les liens de i et on regarde si ils figurent bien dans les liens de j
			// en ne gardant que les liens de i {e dont e.j a deja ete mappé ou e.j que l'on ne peut deplacer}
				mapping[i]=j;
				auto rg3 = span(&logical_edges[start_pos1], end_pos1 - start_pos1) |
					views::filter([&](const LogicalEdge& e){return mapping[e.to]!=e.to || connected_component[e.to] == cmax;}) |
					views::transform([&](const LogicalEdge& e){return TopologicalEdge{.from=mapping[e.from], .to=mapping[e.to], .distance=0};});

				auto rg4 = span(&topological_edges[start_pos2], end_pos2 - start_pos2);
				assert(ranges::is_sorted(rg4));
			//rg4 est triée, mais pas rg3 because mapping shuffles the ordering
				auto it = ranges::find_if(rg3, [&](const TopologicalEdge& e){return ranges::count(rg4, e)==0;});
				if (it != ranges::end(rg3))
				{
					const TopologicalEdge& e = *it;
					printf("ensuite on mappe les liens de %d et on regarde si ils figurent bien dans les liens de %d\n", i, j);
					printf("it != ranges::end(rg3)\n");
					printf("mapped TopologicalEdge={.from=%d, .to=%d} ne figure pas parmi les liens topologiques de %d\n", e.from, e.to, j);
					continue;
				}

				decision_tree.push_back({
					.parent_index=parent_index,
					.depth=depth,
					.recmap={
						.i_emplacement_source=i,
						.i_emplacement_destination=j
					}
				});

				build_decision_tree(decision_tree.size()-1, build_decision_tree);
			}
		}
	};

	build_decision_tree(-1, build_decision_tree);

	printf("decision_tree.size()=%ld\n", decision_tree.size());

	printf("decision_tree={\n");
	for (const auto& [parent_index, depth, recmap] : decision_tree)
	{
		const auto& [i_emplacement_source, i_emplacement_destination] = recmap;
		printf("{.parent_index=%d, .depth=%d, .recmap={.i_emplacement_source=%d, i_emplacement_destination=%d}}\n",
			parent_index, depth, i_emplacement_source, i_emplacement_destination);
	}
	printf("}\n");

	return 0;
}
