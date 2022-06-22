#include <vector>
#include <algorithm>
#include <ranges>
#include <stdio.h>
#include <assert.h>
#include "MyRect.h"
using namespace std;

struct Edge {
	int from;
	int to;

	friend auto operator<=>(const Edge&, const Edge&) = default;
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



int main()
{

	struct TestContext {int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<MyRect> expected_rectangles; };

	const TestContext test_contexts[1]={
	{
		.testid=3,
		.input_rectangles = {
			{.m_left=396-RECT_BORDER+FRAME_BORDER, .m_right=396+162+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+104+RECT_BORDER+FRAME_BORDER, .i=0},//8
			{.m_left=320-RECT_BORDER+FRAME_BORDER, .m_right=320+182+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+72+RECT_BORDER+FRAME_BORDER, .i=1},//9
			{.m_left=453-RECT_BORDER+FRAME_BORDER, .m_right=453+105+RECT_BORDER+FRAME_BORDER, .m_top=218-RECT_BORDER+FRAME_BORDER, .m_bottom=218+72+RECT_BORDER+FRAME_BORDER, .i=2},//10
			{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+152+RECT_BORDER+FRAME_BORDER, .i=3},//21
			{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=202-RECT_BORDER+FRAME_BORDER, .m_bottom=202+88+RECT_BORDER+FRAME_BORDER, .i=4},//24
			{.m_left=750-RECT_BORDER+FRAME_BORDER, .m_right=750+147+RECT_BORDER+FRAME_BORDER, .m_top=346-RECT_BORDER+FRAME_BORDER, .m_bottom=346+120+RECT_BORDER+FRAME_BORDER, .i=5},//25
			{.m_left=273-RECT_BORDER+FRAME_BORDER, .m_right=273+140+RECT_BORDER+FRAME_BORDER, .m_top=154-RECT_BORDER+FRAME_BORDER, .m_bottom=154+120+RECT_BORDER+FRAME_BORDER, .i=6},//26
			{.m_left=542-RECT_BORDER+FRAME_BORDER, .m_right=542+168+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+136+RECT_BORDER+FRAME_BORDER, .i=7},//27
			{.m_left=335-RECT_BORDER+FRAME_BORDER, .m_right=335+168+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+120+RECT_BORDER+FRAME_BORDER, .i=8},//28
			{.m_left=556-RECT_BORDER+FRAME_BORDER, .m_right=556+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+104+RECT_BORDER+FRAME_BORDER, .i=9},//30
			{.m_left=764-RECT_BORDER+FRAME_BORDER, .m_right=764+133+RECT_BORDER+FRAME_BORDER, .m_top=186-RECT_BORDER+FRAME_BORDER, .m_bottom=186+120+RECT_BORDER+FRAME_BORDER, .i=10},//32
			{.m_left=743-RECT_BORDER+FRAME_BORDER, .m_right=743+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+168+RECT_BORDER+FRAME_BORDER, .i=11},//44
			{.m_left=93-RECT_BORDER+FRAME_BORDER, .m_right=93+140+RECT_BORDER+FRAME_BORDER, .m_top=153-RECT_BORDER+FRAME_BORDER, .m_bottom=153+88+RECT_BORDER+FRAME_BORDER, .i=12},//48
			{.m_left=20-RECT_BORDER+FRAME_BORDER, .m_right=10+155+RECT_BORDER+FRAME_BORDER, .m_top=281-RECT_BORDER+FRAME_BORDER, .m_bottom=281+120+RECT_BORDER+FRAME_BORDER, .i=13},//52
			{.m_left=21-RECT_BORDER+FRAME_BORDER, .m_right=21+175+RECT_BORDER+FRAME_BORDER, .m_top=441-RECT_BORDER+FRAME_BORDER, .m_bottom=441+136+RECT_BORDER+FRAME_BORDER, .i=14}//53
		},

		.edges = {
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
		},

		.expected_rectangles = {
			{.m_left=396,.m_right=396+162,.m_top=10,.m_bottom=10+104},//8
			{.m_left=320,.m_right=320+182,.m_top=330,.m_bottom=330+72},//9
			{.m_left=453,.m_right=453+105,.m_top=218,.m_bottom=218+72},//10
			{.m_left=598,.m_right=598+126,.m_top=10,.m_bottom=10+152},//21
			{.m_left=598,.m_right=598+126,.m_top=202,.m_bottom=202+88},//24
			{.m_left=750,.m_right=750+147,.m_top=346,.m_bottom=346+120},//25
			{.m_left=273,.m_right=273+140,.m_top=154,.m_bottom=154+120},//26
			{.m_left=542,.m_right=542+168,.m_top=330,.m_bottom=330+136},//27
			{.m_left=335,.m_right=335+168,.m_top=506,.m_bottom=506+120},//28
			{.m_left=556,.m_right=556+147,.m_top=506,.m_bottom=506+104},//30
			{.m_left=764,.m_right=764+133,.m_top=186,.m_bottom=186+120},//32
			{.m_left=743,.m_right=743+147,.m_top=506,.m_bottom=506+168},//44
			{.m_left=93,.m_right=93+140,.m_top=153,.m_bottom=153+88},//48
			{.m_left=10,.m_right=10+155,.m_top=281,.m_bottom=281+120},//52
			{.m_left=11,.m_right=11+175,.m_top=441,.m_bottom=441+136}//53
		}
	}
	};

	for (const auto& [testid, input_rectangles, edges, expected_rectangles] : test_contexts)
	{
		assert( ranges::is_sorted(edges) );

		auto contacts = [&](int i) -> vector<int> {
			vector<int> v;
			ranges::set_union(edges | views::filter([&](const Edge& e){return e.from==i;}) | views::transform(&Edge::to),
							edges | views::filter([&](const Edge& e){return e.to==i;}) | views::transform(&Edge::from),
							std::back_inserter(v));
			return v;
		};

		const MyRect frame = compute_frame(input_rectangles);

		const MyRect shape = input_rectangles[2];
		auto [width_, height_] = dimensions(shape);

		const float k = 1.0f * height_ / width_;

		vector<MyRect> holes;


		for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : input_rectangles)
		{
			const MyPoint pt4[4]={
					{.x=m_left, .y=m_top},
					{.x=m_left, .y=m_bottom},
					{.x=m_right, .y=m_top},
					{.x=m_right, .y=m_bottom}
			};

			const MyVector directions[4][3]={
					{{.x=-1, .y=-k},{.x=+1, .y=-k},{.x=-1, .y=+k}},
					{{.x=-1, .y=+k},{.x=+1, .y=+k},{.x=-1, .y=-k}},
					{{.x=+1, .y=+k},{.x=+1, .y=-k},{.x=-1, .y=-k}},
					{{.x=-1, .y=+k},{.x=+1, .y=+k},{.x=+1, .y=-k}}
			};

			for (int corner=0; corner<4; corner++)
			{
				const MyPoint& pt = pt4[corner];

				for (const MyVector& dir : directions[corner])
				{
					auto rect = [&](int value){
						const auto [x1, y1] = pt;
                                                const auto [x2, y2] = pt + value*dir ;
                                                return MyRect{.m_left=min(x1,x2), .m_right=max(x1,x2), .m_top=min(y1,y2), .m_bottom = max(y1, y2)};
					};

					int intervalle[2]={2, INT16_MAX};
					auto& [m, M] = intervalle;
					while (M > 1+m)
					{
						int value = M==INT16_MAX ? 2*m : (m+M)/2 ;
						MyRect rec = rect(value);
						auto rg = input_rectangles | views::filter([&](const MyRect& r){return intersect_strict(rec,r) || is_inside(r, rec);});
						(rg.empty() && is_inside(rec,frame) ? m : M) = value;
						printf("[%d %d]\n", m, M);
					}
					holes.push_back(rect(m));
				}
			}
		}

		int m = holes.size();
		ranges::sort(holes, std::ranges::greater{}, [](const MyRect& r){return width(r);});
		for (int i=0; i < m; i++)
			holes[i].i = i;
		printf("top 12 largest holes:\n");
		for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : holes | views::take(12))
		{
			printf("[.m_left=%d, .m_right=%d, .m_top=%d, .m_bottom=%d, .i=%d]\n", m_left, m_right, m_top, m_bottom, i);
		}

		vector<Edge> hole_topology;
		for (const MyRect& h : holes)
		{
			for (const MyRect& r : input_rectangles)
			{
				if (edge_overlap(h, r))
				{
					hole_topology.push_back({h.i, r.i});
				}
			}
		}
		ranges::sort(hole_topology);

		auto hole_contacts =  [&](int hi) -> vector<int> {
			vector<int> v;
			ranges::copy(hole_topology | views::filter([&](const Edge& e){return e.from==hi;}) | views::transform(&Edge::to),
							std::back_inserter(v));
			return v;
		};

		printf("hole topology:\n");
		for (const auto [hi, ri] : hole_topology)
		{
			printf("{.h.i=%d, .r.i=%d}\n", hi, ri);
		}

		FILE *f=fopen("holes.html", "w");
		fprintf(f, "<html>\n<body>\n");
		fprintf(f, "<svg width=\"%d\" height=\"%d\">\n", width(frame), height(frame));
		for (const MyRect& r : input_rectangles)
		{
			fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:blue;stroke:pink;stroke-width:5;opacity:0.5\" />\n",
				r.m_left, r.m_top, width(r), height(r));
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"red\">rec-%d</text>\n", r.m_left, r.m_top, r.i);

			int dy = 0;
			for (int ri : contacts(r.i))
			{
				dy += 14;
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">rec-%d</text>\n", r.m_left + 8, r.m_top + dy, ri);
			}
		}
		for (const MyRect& h : holes | views::take(12))
		{
			fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:red;stroke:green;stroke-width:5;opacity:0.5\" />\n",
				h.m_left, h.m_top, width(h), height(h));
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">hole-%d</text>\n", h.m_left, h.m_top, h.i);

			int dy = 0;
			for (int ri : hole_contacts(h.i))
			{
				dy += 14;
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">rec-%d</text>\n", h.m_left + 8, h.m_top + dy, ri);
			}
		}
		fprintf(f, "</svg>\n</html>");
		fclose(f);

		for (auto [from, to] : edges)
		{
			const MyRect &r1 = input_rectangles[from], &r2 = input_rectangles[to];
			int eo = edge_overlap(r1, r2);
			if (eo == 0)
				printf("no edge overlap between %d and %d\n", from, to);
		}

	}
}
