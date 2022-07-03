#include <vector>
#include <algorithm>
#include <ranges>
#include <stdio.h>
#include <assert.h>
#include "MyRect.h"
#include "compact_frame.h"
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


vector<MyRect> operator+(const vector<MyRect> m1, const vector<MyRect>& m2)
{
	assert(m1.size() == m2.size());
	int n = m1.size();
	vector<MyRect> m(n);
	for (int i=0; i < n; i++)
	{
		m[i] = m1[i] + m2[i];
	}
	return m;
}


struct RectMat
{
	RectMat(vector<MyRect>& m):_m(m){}

	RectMat& operator+=(const vector<MyRect>& m)
	{
		assert(_m.size() == m.size());

		int n = _m.size();

		for (int i=0; i < n; i++)
			_m[i] += m[i];

		return *this;
	}

	vector<MyRect>& _m;
};


struct RectHole {int ri; int rj; RectCorner corner; MyVector direction; int value; MyRect rec;};


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
			{.m_left=118-RECT_BORDER+FRAME_BORDER, .m_right=118+155+RECT_BORDER+FRAME_BORDER, .m_top=281-RECT_BORDER+FRAME_BORDER, .m_bottom=281+120+RECT_BORDER+FRAME_BORDER, .i=13},//52
			{.m_left=160-RECT_BORDER+FRAME_BORDER, .m_right=160+175+RECT_BORDER+FRAME_BORDER, .m_top=441-RECT_BORDER+FRAME_BORDER, .m_bottom=441+136+RECT_BORDER+FRAME_BORDER, .i=14}//53
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

		const MyRect frame = compute_frame(input_rectangles);

		auto compute_holes = [&](int ri)->vector<RectHole>{

			const MyRect shape = input_rectangles[ri];
			auto [width_, height_] = dimensions(shape);

			const float k = 1.0f * height_ / width_;

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
							printf("[%d %d]\n", m, M);
						}
						holes.push_back({ri, ir.i, rectCorner, dir, m, rect(pt, pt + m*dir)});
					}
				}
			}
			return holes;
		};

		vector<RectHole> holes = compute_holes(2);

		int m = holes.size();
		ranges::sort(holes, std::ranges::greater{}, [](const RectHole& h){return width(h.rec);});
		for (int i=0; i < m; i++)
			holes[i].rec.i = i;

		int n = input_rectangles.size();

		FILE *f=fopen("holes.html", "w");
		fprintf(f, "<html>\n<body>\n");
		fprintf(f, "<svg width=\"%d\" height=\"%d\">\n", width(frame), height(frame));
		for (const MyRect& r : input_rectangles)
		{
			fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:blue;stroke:pink;stroke-width:5;opacity:0.5\" />\n",
				r.m_left, r.m_top, width(r), height(r));
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"red\">r-%d</text>\n", r.m_left, r.m_top, r.i);

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
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"white\">r-%d</text>\n", r.m_left + 8, r.m_top + dy, ri);
			}

			dy = 0;
			for (int ri : views::iota(0, n) | views::filter([&](int rj){return r.i != rj && edge_overlap(r, input_rectangles[rj]);}))
			{
				dy += 14;
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", r.m_left + 30, r.m_top + dy, ri);
			}
		}
		for (const RectHole& h : holes | views::take(18))
		{
			const auto& [ri, rj, RectCorner, direction, value, rec] = h;
			fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:red;stroke:green;stroke-width:5;opacity:0.5\" />\n",
				rec.m_left, rec.m_top, width(rec), height(rec));
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">hole-%d</text>\n", rec.m_left, rec.m_top, rec.i);

			int dy = 0;
			for (int rj : views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rec, input_rectangles[rj]);}))
			{
				dy += 14;
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", rec.m_left + 8, rec.m_top + dy, rj);
			}
		}
		fprintf(f, "</svg>\n</html>");
		fclose(f);

	//r2 => h17
		enum TransformationType {STRETCH_WIDTH, STRETCH_HEIGHT};
		struct ST { MyRect initial_tf, tf; };
		const ST Transformations[2][2]={
			{
				{.initial_tf = {.m_left=-1, .m_right=0, .m_top=0, .m_bottom=0}, .tf = {.m_left=-1, .m_right=-1, .m_top=0, .m_bottom=0}},
				{.initial_tf = {.m_left=0, .m_right=+1, .m_top=0, .m_bottom=0}, .tf = {.m_left=+1, .m_right=+1, .m_top=0, .m_bottom=0}},
			},
			{
				{.initial_tf = {.m_left=0, .m_right=0, .m_top=-1, .m_bottom=0}, .tf = {.m_left=0, .m_right=0, .m_top=-1, .m_bottom=-1}},
				{.initial_tf = {.m_left=0, .m_right=0, .m_top=0, .m_bottom=+1}, .tf = {.m_left=0, .m_right=0, .m_top=+1, .m_bottom=+1}},
			}
		};

		vector<int> stress_line[2];
		compute_stress_line(input_rectangles, stress_line);

		vector<RectHole> kept_holes;

		Direction direction = width(frame) > height(frame) ? EAST_WEST : NORTH_SOUTH;
		for (int ri : stress_line[direction])
		{
			vector<RectHole> holes = compute_holes(ri);
			auto rg = holes | views::filter([&](const RectHole& rh){
                               const auto& [ri, rj, corner, direction, value, rec] = rh;

                               vector<int> logical_contacts;
                               ranges::set_union(
                                               edges | views::filter([&](const Edge& e){return e.from==ri;}) | views::transform(&Edge::to),
                                                edges | views::filter([&](const Edge& e){return e.to==ri;}) | views::transform(&Edge::from),
                                                std::back_inserter(logical_contacts)
                                                 );

                                auto geometric_contacts = views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rec, input_rectangles[rj]);});
                                return ranges::includes(geometric_contacts, logical_contacts);
                        });
                	ranges::copy(rg, back_inserter(kept_holes));
		}
/*
		auto rg = stress_line[direction] | views::transform([&](int ri)->vector<RectHole>{return compute_holes(ri);})
					| views::join
					| views::filter([&](const RectHole& rh){
                               const auto& [ri, corner, direction, value, rec] = rh;

		               vector<int> logical_contacts;
        	               ranges::set_union(
                                               edges | views::filter([&](const Edge& e){return e.from==ri;}) | views::transform(&Edge::to),
                      	                        edges | views::filter([&](const Edge& e){return e.to==ri;}) | views::transform(&Edge::from),
                               	                std::back_inserter(logical_contacts)
                                       	         );

				auto geometric_contacts = views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rec, input_rectangles[rj]);});
				return ranges::includes(geometric_contacts, logical_contacts);
			});
		ranges::copy(rg, back_inserter(kept_holes));
*/
		printf("kept_holes.size()=%ld\n", kept_holes.size());
		for (auto [ri, rj, corner, direction, value, rec] : kept_holes)
		{
			printf("ri=%d width(ri)=%d rj=%d corner=%d dir={.x=%.2f, .y=%.2f} value=%d\n", ri, width(input_rectangles[ri]), rj, corner, direction.x, direction.y, value);
		}
		printf("hard coded i_select=2\n");
		int i_select=2;
		MyRect r2 = input_rectangles[i_select];
		vector<MyRect> rectangles = input_rectangles;
		const auto [ri, rj, rectCorner, dir, value, hrec] = holes[17];
		printf("holes[17]=\n");
                printf("ri=%d width(ri)=%d rj=%d corner=%d dir={.x=%.2f, .y=%.2f} value=%d\n", ri, width(input_rectangles[ri]), rj, rectCorner, dir.x, dir.y, value);
		MyRect& r = rectangles[i_select];
		r = hrec;
		r.i = i_select;

		vector<MyRect> accumulated_transformation(n);
		const MyRect zero;

		int n1 = width(r2) - width(hrec);
		int n2 = height(r2) - height(hrec);
		for (TransformationType transformationType : views::iota(0,n1+n2) | views::transform([&](int i){return i < n1 ? STRETCH_WIDTH : STRETCH_HEIGHT;}))
		{

			auto ff=[&](const ST& st)->vector<MyRect> {

				const auto& [initial_tf, tf] = st;

				vector<MyRect> transformation(n);

				transformation[i_select] = initial_tf;

				for (bool stop=false; stop==false; )
				{
					stop=true;
					for (int i : views::iota(0,n) | views::filter([&](int i){return transformation[i]==zero;}))
					{
						for (int j : views::iota(0,n) | views::filter([&](int i){return transformation[i]!=zero;}))
						{
							if (intersect_strict(rectangles[i] + accumulated_transformation[i] + transformation[i],
										rectangles[j] + accumulated_transformation[j] + transformation[j]))
							{
								transformation[i] = tf;
								stop=false;
							}
						}
					}
				}
				return transformation;
			};

			vector<MyRect> transformation = ranges::min(Transformations[transformationType] | views::transform(ff), {},
									[&](const vector<MyRect>& tf){
													const auto [width_, height_] = dimensions(compute_frame(rectangles + tf));
													int nb = n - ranges::count(tf, zero);
													return make_tuple(width_, height_, nb);
													 }
								);
			RectMat(accumulated_transformation) += transformation;
		}

		RectMat(rectangles) += accumulated_transformation;

		MyRect frame_ = compute_frame(rectangles);

		f=fopen("rects.html", "w");
		fprintf(f, "<html>\n<body>\n");
		fprintf(f, "<svg width=\"%d\" height=\"%d\">\n", width(frame_), height(frame_));
		for (const MyRect& r : rectangles)
		{
			fprintf(f, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" style=\"fill:blue;stroke:pink;stroke-width:5;opacity:0.5\" />\n",
				r.m_left, r.m_top, width(r), height(r));
			fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"red\">r-%d</text>\n", r.m_left, r.m_top, r.i);

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
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"white\">r-%d</text>\n", r.m_left + 8, r.m_top + dy, ri);
			}

			dy = 0;
			for (int ri : views::iota(0, n) | views::filter([&](int rj){return r.i != rj && edge_overlap(r, rectangles[rj]);}))
			{
				dy += 14;
				fprintf(f, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", r.m_left + 30, r.m_top + dy, ri);
			}
		}
		fprintf(f, "</svg>\n</html>");
		fclose(f);

	}
}
