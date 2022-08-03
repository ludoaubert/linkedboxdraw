#include <vector>
#include <string>
#include <deque>
#include <algorithm>
#include <ranges>
#include <span>
#include <stdio.h>
#include "MyRect.h"
using namespace std;


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


struct RectHole {int ri; int rj; RectCorner corner; MyVector direction; int value; MyRect rec;};

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
	{.m_left=396-RECT_BORDER+FRAME_BORDER, .m_right=396+162+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+104+RECT_BORDER+FRAME_BORDER},//8
	{.m_left=320-RECT_BORDER+FRAME_BORDER, .m_right=320+182+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+72+RECT_BORDER+FRAME_BORDER},//9
	{.m_left=453-RECT_BORDER+FRAME_BORDER, .m_right=453+105+RECT_BORDER+FRAME_BORDER, .m_top=218-RECT_BORDER+FRAME_BORDER, .m_bottom=218+72+RECT_BORDER+FRAME_BORDER},//10
	{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+152+RECT_BORDER+FRAME_BORDER},//21
	{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=202-RECT_BORDER+FRAME_BORDER, .m_bottom=202+88+RECT_BORDER+FRAME_BORDER},//24
	{.m_left=750-RECT_BORDER+FRAME_BORDER, .m_right=750+147+RECT_BORDER+FRAME_BORDER, .m_top=346-RECT_BORDER+FRAME_BORDER, .m_bottom=346+120+RECT_BORDER+FRAME_BORDER},//25
	{.m_left=273-RECT_BORDER+FRAME_BORDER, .m_right=273+140+RECT_BORDER+FRAME_BORDER, .m_top=154-RECT_BORDER+FRAME_BORDER, .m_bottom=154+120+RECT_BORDER+FRAME_BORDER},//26
	{.m_left=542-RECT_BORDER+FRAME_BORDER, .m_right=542+168+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+136+RECT_BORDER+FRAME_BORDER},//27
	{.m_left=335-RECT_BORDER+FRAME_BORDER, .m_right=335+168+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+120+RECT_BORDER+FRAME_BORDER},//28
	{.m_left=556-RECT_BORDER+FRAME_BORDER, .m_right=556+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+104+RECT_BORDER+FRAME_BORDER},//30
	{.m_left=764-RECT_BORDER+FRAME_BORDER, .m_right=764+133+RECT_BORDER+FRAME_BORDER, .m_top=186-RECT_BORDER+FRAME_BORDER, .m_bottom=186+120+RECT_BORDER+FRAME_BORDER},//32
	{.m_left=743-RECT_BORDER+FRAME_BORDER, .m_right=743+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+168+RECT_BORDER+FRAME_BORDER},//44
	{.m_left=93-RECT_BORDER+FRAME_BORDER, .m_right=93+140+RECT_BORDER+FRAME_BORDER, .m_top=153-RECT_BORDER+FRAME_BORDER, .m_bottom=153+88+RECT_BORDER+FRAME_BORDER},//48
	{.m_left=78-RECT_BORDER+FRAME_BORDER, .m_right=78+155+RECT_BORDER+FRAME_BORDER, .m_top=281-RECT_BORDER+FRAME_BORDER, .m_bottom=281+120+RECT_BORDER+FRAME_BORDER},//52
	{.m_left=120-RECT_BORDER+FRAME_BORDER, .m_right=120+175+RECT_BORDER+FRAME_BORDER, .m_top=441-RECT_BORDER+FRAME_BORDER, .m_bottom=441+136+RECT_BORDER+FRAME_BORDER}//53
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
	const float k = 1.0f;

	vector<RectHole> holes;

	for (const MyRect& ir : input_rectangles)
	{
		const MyVector directions[4][3]={
				{{.x=-1, .y=-k},{.x=+1, .y=-k},{.x=-1, .y=+k}},
				{{.x=-1, .y=+k},{.x=+1, .y=+k},{.x=-1, .y=-k}},
				{{.x=+1, .y=+k},{.x=+1, .y=-k},{.x=-1, .y=-k}},
				{{.x=-1, .y=+k},{.x=+1, .y=+k},{.x=+1, .y=-k}}
		};

		for (RectCorner rectCorner : RectCorners)
		{
			const MyPoint pt = ir[rectCorner] ;

			for (const MyVector& dir : directions[rectCorner])
			{
				int intervalle[2]={2, INT16_MAX};
				auto& [m, M] = intervalle;
				while (M > 1+m)
				{
					int value = M==INT16_MAX ? 2*m : (m+M)/2 ;
					MyRect rec = rect(pt, pt + value*dir);
					auto rg = input_rectangles | views::filter([&](const MyRect& r){return intersect_strict(rec,r) || is_inside(r, rec);});
					(rg.empty() && is_inside(rec,frame) ? m : M) = value;
					//printf("[%d %d]\n", m, M);
				}

				if (m > 2)
				{
					MyRect rec = rect(pt, pt + m*dir);
					holes.push_back({.ri=-1, .rj=ir.i, .corner=rectCorner, .direction=dir, .value=m, .rec=rec});
				}
			}
		}
	}

	ranges::sort(holes, {}, &RectHole::rec);
	vector<RectHole> holes_dedup;
	ranges::unique_copy(holes, back_inserter(holes_dedup), {}, &RectHole::rec);

	holes.clear();

//cross product
	for (int ri : views::iota(0,n))
	{
		for (const auto& [ri_, rj, corner, direction, value, rec] : holes_dedup)
		{
			if (3*value >= width(input_rectangles[ri]))
			{
				holes.push_back({ri, rj, corner, direction, value, rec});
			}
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
	char buffer[10*1000];
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
//TODO: C++23 introduces views::set_union range adapter. No longer need for vector<int> contacts.
		auto contacts = logical_edges |
			views::filter([&](const LogicalEdge& e){return e.from==ri;}) |
			views::transform(&LogicalEdge::to);

		for (int rj : contacts)
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"white\">r-%d</text>\n", m_left + 8, m_top + dy, rj);
		}

		dy = 0;
		for (int rj : views::iota(0, n) | views::filter([&](int rj){return r.i != rj && edge_overlap(r, input_rectangles[rj]);}))
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", r.m_left + 30, r.m_top + dy, ri);
		}
	}

	for (int hi=0; hi < holes.size() && hi < 15; hi++)
	{
		const auto& [ri, rj, RectCorner, direction, value, rec] = holes[hi];

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
		pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">rj=%d</text>\n", rec.m_left + 30, rec.m_top + 2*14, rj);
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

	printf("topological_edge_partition: ");
	for (int pos : topological_edge_partition)
		printf("%d, ", pos);
	printf("\n");

	vector<int> logical_edge_partition = compute_edge_partition(input_rectangles.size(), logical_edges);

	printf("logical_edge_partition: ");
	for (int pos : logical_edge_partition)
		printf("%d, ", pos);
	printf("\n");

	vector<int> connected_component = compute_connected_components(input_rectangles, logical_edges, logical_edge_partition);

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
// par default, les intput_rectangles sont des emplacements non libres, les autres emplacements etant libres
			ranges::fill(etat_emplacement, LIBRE);
			for (int ii=0; ii < input_rectangles.size(); ii++)
				etat_emplacement[ii] = OCCUPE;

			deque<RectMap> chemin;

			for (int pos=parent_index; pos != -1; pos = decision_tree[pos].parent_index)
			{
				chemin.push_front( decision_tree[pos].recmap );
			}

			int depth = chemin.size();
			if (depth > 5)
				continue;

			for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
			{
				etat_emplacement[i_emplacement_source] = LIBRE;
				etat_emplacement[i_emplacement_destination] = OCCUPE;
			}

			for (int i=0; i<input_rectangles.size(); i++)
				mapping[i]=i;
			for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
			{
				mapping[i_emplacement_source] = i_emplacement_destination;
			}

//on ne mappe pas 2 fois un meme emplacement
			if (mapping[i] != i)
				continue;

			for (int j=0; j < emplacements.size(); j++)
			{
				if (j == i)
					continue;

				if (etat_emplacement[j] == OCCUPE)
					continue;

// l'emplacement j est-il topologiquement lié au rectangles auxquels i est logiquement lié et que l'on ne peut pas deplacer ?
				int start_pos1 = logical_edge_partition[i];
				int end_pos1 = logical_edge_partition[i+1];
			//les rectangles auxquels i est logiquement lié et que l'on ne peut pas déplacer:
				auto rg1 = span(&logical_edges[start_pos1], end_pos1 - start_pos1) |
// si connected_component[i]==cmax alors i ne doit pas etre deplacé.
					views::filter([&](const LogicalEdge& e){return connected_component[e.to] == cmax;}) |
					views::transform([](const LogicalEdge& e){return e.to;});

				int start_pos2 = topological_edge_partition[j];
				int end_pos2 = topological_edge_partition[j+1];
			//les rectangles auxquels j est topologiquement lié et que l'on ne peut pas deplacer:
				auto rg2 = span(&topological_edges[start_pos2], end_pos2 - start_pos2) |
					views::filter([&](const TopologicalEdge& e){return e.to < input_rectangles.size();}) |
					views::filter([&](const TopologicalEdge& e){return connected_component[e.to] == cmax;}) |
					views::transform([](const TopologicalEdge& e){return e.to;});

				if (ranges::includes(rg2, rg1)==false)
					continue;

			//ensuite on mappe les liens de i et on regarde si ils figurent bien dans les liens de j
			// en ne gardant que les liens de i {e dont e.j a deja ete mappé}
				auto rg3 = span(&logical_edges[start_pos1], end_pos1 - start_pos1) |
					views::filter([&](const LogicalEdge& e){return mapping[e.to]!=j;}) |
					views::transform([&](const LogicalEdge& e){return TopologicalEdge{.from=mapping[e.from], .to=mapping[e.to], .distance=0};});

				auto rg4 = span(&topological_edges[start_pos2], end_pos2 - start_pos2);
			//rg4 est triée, mais pas rg3;
				auto it = ranges::find_if(rg3, [&](const TopologicalEdge& e){return ranges::count(rg4, e)==0;});
				if (it == ranges::end(rg3))
					continue;

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

	return 0;
}
