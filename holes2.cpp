#include <vector>
#include <string>
#include <algorithm>
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

struct DecisionTreeNode
{
	int parent_index=-1;
	int depth;
	RectHole rh;
//KPIs:
	MyPoint dim;
	float rect_distances;
	float potential;
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

vector<LogicalEdge> edges = {
	{.from=0,.to=3},
	{.from=1,.to=7},
	{.from=2,.to=7},
	{.from=3,.to=4},
	{.from=4,.to=7},
	{.from=5,.to=7},
	{.from=6,.to=7},
	{.from=7,.to=11},
	{.from=8,.to=7},
	{.from=9,.to=7},
	{.from=10,.to=7},
	{.from=12,.to=6},
	{.from=13,.to=12},
	{.from=13,.to=14}
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


string print_html(const vector<MyRect>& input_rectangles, const vector<RectHole>& holes)
{
	int n = input_rectangles.size();
	char buffer[10*1000];
	int pos=0;

	pos+= sprintf(buffer+pos, "<html>\n<body>\n");
	pos+= sprintf(buffer+pos, "<svg width=\"%d\" height=\"%d\">\n", width(frame)+100, height(frame));
	for (const MyRect& r : input_rectangles)
	{
		pos += sprintf(buffer+pos, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:blue;stroke:pink;stroke-width:5;opacity:0.5\" />\n",r.m_left, r.m_top, width(r), height(r));
		pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"red\">r-%d</text>\n", r.m_left, r.m_top, r.i);

		int dy = 0;
//TODO: C++23 introduces views::set_union range adapter. No longer need for vector<int> contacts.
		vector<int> contacts;
		ranges::set_union(
			edges | views::filter([&](const Edge& e){return e.from==r.i;}) | views::transform(&Edge::to),
			edges | views::filter([&](const Edge& e){return e.to==r.i;}) | views::transform(&Edge::from),
			std::back_inserter(contacts)
		);
		for (int ri : contacts)
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"white\">r-%d</text>\n", r.m_left + 8, r.m_top + dy, ri);
		}

		dy = 0;
		for (int ri : views::iota(0, n) | views::filter([&](int rj){return r.i != rj && edge_overlap(r, /*input_*/rectangles[rj]);}))
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", r.m_left + 30, r.m_top + dy, ri);
		}
	}

	for (int hi=0; hi < holes.size() && hi < 15; hi++)
	{
			const auto& [ri, rj, RectCorner, direction, value, rec] = holes[hi];

			fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:red;stroke:green;stroke-width:5;opacity:0.5\" />\n",
							rec.m_left, rec.m_top, width(rec), height(rec));
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">hole-%d</text>\n", rec.m_left, rec.m_top, hi);

			int dy = 0;
			for (int rj : views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rec, input_rectangles[rj]);}))
			{
							dy += 14;
							fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", rec.m_left + 8, rec.m_top + dy, rj);
			}
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">ri=%d</text>\n", rec.m_left + 30, rec.m_top + 1*14, ri);
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">rj=%d</text>\n", rec.m_left + 30, rec.m_top + 2*14, rj);
	}

	pos += sprintf(buffer+pos, "</svg>\n</html>");
	buffer[pos]=0;
	return buffer;
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
	
	vector<TopologicalEdge> topological_graph;
	for (int i=0; i < emplacements.size(); i++)
	{
		for (int j=0; j < emplacements.size(); j++)
		{
			int dist = rect_distance(emplacements[i], emplacements[j]);
			if (dist < 20)
				topological_graph.push_back({.from=i, .to=j, .distance=dist});
		}
	}
	
	ranges::sort(topological_graph);
	
	vector<int> edge_partition(emplacements.size()+1);
	edge_partition[0]=0;
	for (int pos=0, ii=0; ii<n; ii++)
	{
		int &start_pos = edge_partition[ii];
		int &end_pos = edge_partition[ii+1];
		end_pos = start_pos;
		for ( ; pos < topological_graph.size() && topological_graph[pos].i==ii; pos++)
		{
			end_pos = max(end_pos, pos+1);
		}
	}

	printf("edge_partition: ");
	for (int pos : edge_partition)
		printf("%d,", pos);
	printf("\n");

	return 0;
}