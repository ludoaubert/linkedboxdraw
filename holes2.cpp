#include <vector>
#include <string>
#include <deque>
#include <map>
#include <algorithm>
#include <ranges>
#include <span>
#include <stdio.h>
//#include <fmt/ranges.h>
//#include <format>
#include "MyRect.h"
#include "fit.h"
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
	int match;
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
const vector<LogicalEdge> logical_edges = {
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



bool filter(const LogicalEdge& e){

	int dist = rect_distance(input_rectangles[e.from], input_rectangles[e.to]);
	return dist <= 20;
};


vector<int> compute_connected_components(size_t n,
					const vector<LogicalEdge>& logical_edges)
{
	assert(ranges::is_sorted(logical_edges));
	vector<int> connected_component(n, -1);

	auto rec_compute_connected_components = [&](int i, int c, auto&& rec_compute_connected_components)->void{
		connected_component[i] = c;

		span adj_list = ranges::equal_range(logical_edges, i, {}, &LogicalEdge::from);

		for (const LogicalEdge& e : adj_list)
		{
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

{
/* TODO
	auto jv = input_rectangles |
		views::transform([&](const MyRect& r)->string{
			char buffer[200];
			sprintf(buffer, "\t{\"m_left\":%d, \"m_right\":%d, \"m_top\":%d, \"m_bottom\":%d},\n",r.m_left, r.m_right, r.m_top, r.m_bottom);
			return buffer;}) |
		views ::join;
*/
	char buffer[10*1000];
	int pos=0;
	pos += sprintf(buffer + pos,"{\n\"input_rectangles\":[");
	for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : input_rectangles)
        {
                pos += sprintf(buffer + pos, "\n\t{\"m_left\":%d, \"m_right\":%d, \"m_top\":%d, \"m_bottom\":%d},", m_left, m_right, m_top, m_bottom);
        }
	pos += sprintf(buffer + --pos,"\n],\n\"logical_edges\":[");
	for (const auto& [from, to] : logical_edges)
	{
		pos += sprintf(buffer + pos, "\n\t{\"from\":%d, \"to\":%d},", from, to);
	}
	pos += sprintf(buffer + --pos,"\n],\n\"topological_edges\":[");

        for (int ri=0; ri < n; ri++)
        {
                for (int rj : views::iota(0, n) | views::filter([&](int rj){return ri != rj && edge_overlap(input_rectangles[ri], input_rectangles[rj]);}))
                {
                        pos += sprintf(buffer + pos, "\n\t{\"from\":%d, \"to\":%d},", ri, rj);
                }
	}
	pos += sprintf(buffer + --pos,"\n]\n}");

        FILE *f=fopen("logical_graph.json", "w");
	fprintf(f, "%s", buffer);
	fclose(f);
}

	for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : input_rectangles)
	{
		printf("{.m_left=%d, .m_right=%d, .m_top=%d, .m_bottom=%d}\n", m_left, m_right, m_top, m_bottom);
	}
	vector<RectHole> holes = compute_holes(input_rectangles);

{
	char buffer[100*1000];
	int pos=0;
        pos += sprintf(buffer + pos,"{\n\"holes\":[");
	for (int hi=0; hi < holes.size(); hi++)
        {
                const auto& [ri, RectCorner, direction, value, rec] = holes[hi];
		pos += sprintf(buffer+pos, "\n\t{\"hi\":%d, \"ri\":%d, \"RectCorner\":%d, \"RectCornerString\":\"%s\", \"direction\":{\"x\":%.0f,\"y\":%.0f},\"value\":%d, \"rec\":{\"m_left\":%d,\"m_right\":%d,\"m_top\":%d,\"m_bottom\":%d}},",
			hi, ri, RectCorner, RectCornerString[RectCorner], direction.x, direction.y, value, rec.m_left, rec.m_right, rec.m_top, rec.m_bottom);
	}
	pos += sprintf(buffer + --pos,"\n],\n\"topological_contact\":[");

	for (int hi=0; hi < holes.size(); hi++)
	{
		RectHole& rh = holes[hi];
                for (int rj : views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rh.rec, input_rectangles[rj]);}))
                {
                        pos += sprintf(buffer+pos, "\n\t{\"hi\":%d, \"rj\":%d},", hi, rj);
                }
	}
	pos += sprintf(buffer + --pos, "\n]\n}");

        FILE *f=fopen("holes.json", "w");
        fprintf(f, "%s", buffer);
        fclose(f);
}

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

	vector<int> connected_component = compute_connected_components(input_rectangles.size(), logical_edges);

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

                        if (connected_component[i] == cmax && depth <= 2)
			{
				printf("connected_component[%d] == cmax && depth=%d <= 2\n", i, depth);
			}
			else if (connected_component[i] != cmax && depth > 2)
			{
                                printf("connected_component[%d] != cmax && depth=%d > 2\n", i, depth);
			}
			else
                        {
                                printf("connected_component[%d] == %d. depth=%d. skipping %d\n", i, cmax, depth, i);
                                continue;
                        }

			if (depth > 6)
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

			span le_adj_list = ranges::equal_range(logical_edges, i, {}, &LogicalEdge::from);

		//si tous les liens de i {e sont des liens dont e.j n'a pas ete mappé et e.j que l'on peut deplacer}, alors i n'est pas
		// stable
			if (ranges::all_of(le_adj_list,
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
				auto rg1 = le_adj_list |
// si connected_component[i]==cmax alors i ne doit pas etre deplacé.
					views::filter([&](const LogicalEdge& e){return connected_component[e.to] == cmax;}) |
					views::transform([](const LogicalEdge& e){return e.to;});

				printf("rg1={");
				for (int i : rg1)
					printf(" %d,", i);
				printf("}\n");


				span te_adj_list = ranges::equal_range(topological_edges, j, {}, &TopologicalEdge::from);

			//les rectangles auxquels j est topologiquement lié:
				auto rg2 = te_adj_list |
					views::filter([&](const TopologicalEdge& e){return e.to < input_rectangles.size();}) |
					views::filter([&](const TopologicalEdge& e){return mapping[e.to]==e.to /*connected_component[e.to] == cmax*/;}) |
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
				auto rg3 = le_adj_list |
					views::filter([&](const LogicalEdge& e){return mapping[e.to]!=e.to || connected_component[e.to] == cmax;}) |
					views::transform([&](const LogicalEdge& e){return TopologicalEdge{.from=mapping[e.from], .to=mapping[e.to], .distance=0};});

				assert(ranges::is_sorted(te_adj_list));
			//rg4 est triée, mais pas rg3 because mapping shuffles the ordering
				auto it = ranges::find_if(rg3, [&](const TopologicalEdge& e){return ranges::count(te_adj_list, e)==0;});
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

	int max_value=0;
	const int known_max_value=26;

        map<unsigned int, int> subset_distrib;

	for (int i=0; i < decision_tree.size(); i++)
	{
		deque<RectMap> chemin;

		for (int pos=i; pos != -1; pos = decision_tree[pos].parent_index)
		{
			chemin.push_front( decision_tree[pos].recmap );
		}
		for (int ii=0; ii<input_rectangles.size(); ii++)
			mapping[ii]=ii;
		for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
		{
			mapping[i_emplacement_source] = i_emplacement_destination;
		}

		auto rg = logical_edges |
			views::transform([&](const LogicalEdge& e)->TopologicalEdge{return {
				.from=mapping[e.from],
				.to=mapping[e.to],
				.distance=0
			};});
		vector<TopologicalEdge> v(logical_edges.size()), inter;
		ranges::copy(rg, &v[0]);
		ranges::sort(v);
		ranges::set_intersection(v, topological_edges, back_inserter(inter));
                decision_tree[i].match = inter.size();
		printf("i=%d inter.size()=%ld\n", i, inter.size());
		max_value = max<int>(max_value, inter.size());

		if (max_value == known_max_value)
		{
			unsigned int subset = 0;
			for (int ii=0; ii < input_rectangles.size(); ii++)
			{
				subset += mapping[ii] == ii ? 1 << ii : 0;
			}
			subset_distrib[subset]++;
		}
	}

{
        FILE *f=fopen("decision_tree.json", "w");
        fprintf(f, "[\n");
        for (int i=0; i < decision_tree.size(); i++)
        {
                const auto& [parent_index, depth, recmap, match] = decision_tree[i];
                const auto& [i_emplacement_source, i_emplacement_destination] = recmap;
                fprintf(f, "{\"i\":%d, \"parent_index\":%d, \"depth\":%d, \"i_emplacement_source\":%d, \"i_emplacement_destination\":%d, \"match\":%d}%s\n",
                        i, parent_index, depth, i_emplacement_source, i_emplacement_destination, match,
                        i+1 == decision_tree.size() ? "": ",");
        }
        fprintf(f, "]\n");
        fclose(f);
}

	printf("max_value=%d\n", max_value);
	printf("subset_distrib.size()=%ld\n", subset_distrib.size());
	printf("subset_distrib={\n");
	for (const auto& [subset, count] : subset_distrib)
	{
		printf("{.subset=%u, .count=%d}\n", subset, count);
	}
	printf("}\n");

	test_fit();

	return 0;
}
