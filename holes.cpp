#include <vector>
#include <algorithm>
#include <tuple>
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
			{.m_left=78-RECT_BORDER+FRAME_BORDER, .m_right=78+155+RECT_BORDER+FRAME_BORDER, .m_top=281-RECT_BORDER+FRAME_BORDER, .m_bottom=281+120+RECT_BORDER+FRAME_BORDER, .i=13},//52
			{.m_left=120-RECT_BORDER+FRAME_BORDER, .m_right=120+175+RECT_BORDER+FRAME_BORDER, .m_top=441-RECT_BORDER+FRAME_BORDER, .m_bottom=441+136+RECT_BORDER+FRAME_BORDER, .i=14}//53
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
							//printf("[%d %d]\n", m, M);
						}
						if (m > 2 && 3*m >= width(input_rectangles[ri]))
						{
							MyRect rec = rect(pt, pt + m*dir);
							holes.push_back({ri, ir.i, rectCorner, dir, m, rec});
						}
					}
				}
			}
  //                      printf("holes.size()=%ld\n", holes.size());

                        ranges::sort(holes, {}, &RectHole::rec);
                        vector<RectHole> holes_dedup;
                        ranges::unique_copy(holes, back_inserter(holes_dedup), {}, &RectHole::rec);

//                        printf("holes_dedup.size()=%ld\n", holes_dedup.size());
			return holes_dedup;
		};

		vector<int> stress_line[2];
		compute_stress_line(input_rectangles, stress_line);


		auto compute_all_holes=[&](const vector<MyRect>& input_rectangles)->vector<RectHole>
		{
			vector<RectHole> holes;

			int n = input_rectangles.size();

			auto rgh = views::iota(0,n) | views::transform(compute_holes);
/*						| views::join;
*/
//TODO: use views::join when g++-12 become available
			for (int ri : views::iota(0,n))
			{
				ranges::copy(compute_holes(ri), back_inserter(holes));
			}

/*
			for (auto [ri, rj, corner, direction, value, rec, distance] : holes | views::take(15))
			{
				printf("ri=%d width(ri)=%d rj=%d corner=%s dir={.x=%.2f, .y=%.2f} value=%d distance:%.2f => %.2f\n",
					ri, width(input_rectangles[ri]), rj, RectCornerString[corner], direction.x, direction.y, value, distance[0], distance[1]);
			}
*/
			return holes;
		};

		vector<RectHole> holes = compute_all_holes(input_rectangles);


		auto compute_transformation = [&](const vector<MyRect>& input_rectangles, const RectHole& rh)->vector<MyRect>{

			enum TransformationType {STRETCH_WIDTH, SQUEEZE_WIDTH, STRETCH_HEIGHT, SQUEEZE_HEIGHT};
			struct ST { MyRect initial_tf, tf; };
			const ST Transformations[4][2]={
				{
					{.initial_tf = {.m_left=-1, .m_right=0, .m_top=0, .m_bottom=0}, .tf = {.m_left=-1, .m_right=-1, .m_top=0, .m_bottom=0}},
					{.initial_tf = {.m_left=0, .m_right=+1, .m_top=0, .m_bottom=0}, .tf = {.m_left=+1, .m_right=+1, .m_top=0, .m_bottom=0}},
				},
                                {
                                        {.initial_tf = {.m_left=+1, .m_right=0, .m_top=0, .m_bottom=0}, .tf = {.m_left=+1, .m_right=+1, .m_top=0, .m_bottom=0}},
                                        {.initial_tf = {.m_left=0, .m_right=-1, .m_top=0, .m_bottom=0}, .tf = {.m_left=-1, .m_right=-1, .m_top=0, .m_bottom=0}},
                                },
				{
					{.initial_tf = {.m_left=0, .m_right=0, .m_top=-1, .m_bottom=0}, .tf = {.m_left=0, .m_right=0, .m_top=-1, .m_bottom=-1}},
					{.initial_tf = {.m_left=0, .m_right=0, .m_top=0, .m_bottom=+1}, .tf = {.m_left=0, .m_right=0, .m_top=+1, .m_bottom=+1}},
				},
                                {
                                        {.initial_tf = {.m_left=0, .m_right=0, .m_top=+1, .m_bottom=0}, .tf = {.m_left=0, .m_right=0, .m_top=+1, .m_bottom=+1}},
                                        {.initial_tf = {.m_left=0, .m_right=0, .m_top=0, .m_bottom=-1}, .tf = {.m_left=0, .m_right=0, .m_top=-1, .m_bottom=-1}},
                                }
			};

			const auto& [ri, rj, rectCorner, dir, value, hrec] = rh;

	        //printf("ri=%d width(ri)=%d rj=%d corner=%s dir={.x=%.2f, .y=%.2f} value=%d\n", ri, width(input_rectangles[ri]), rj, RectCornerString[rectCorner], dir.x, dir.y, value);

			int n=input_rectangles.size();
			vector<MyRect> accumulated_transformation(n);
			const MyRect dr = hrec - input_rectangles[ri];
			accumulated_transformation[ri] = dr;
			const MyRect zero;

                        auto ff=[&](const ST& st)->vector<MyRect> {

                                const auto& [initial_tf, tf] = st;

                                vector<MyRect> transformation(n);

                                transformation[ri] = initial_tf;

                                for (bool stop=false; stop==false; )
                           	{
                                        stop=true;
                                        for (int i : views::iota(0,n) | views::filter([&](int i){return transformation[i]==zero;}))
                                 	{
                                                for (int j : views::iota(0,n) | views::filter([&](int j){return i!=j && transformation[j]!=zero;}))
                                    		{
                                                	if (intersect_strict(input_rectangles[i] + accumulated_transformation[i] + transformation[i],
                                                                             input_rectangles[j] + accumulated_transformation[j] + transformation[j]))
                                                        {
                                                                transformation[i] = tf;
								stop=false;
							}
						}
					}
				}
				return transformation;
			};

			const auto [n1, n2] = dimensions(-dr);

			auto tt = [&](int i)->TransformationType{
                        	if (i < abs(n1))
                                	return n1<0 ? SQUEEZE_WIDTH : STRETCH_WIDTH;
                                else
                                       	return n2<0 ? SQUEEZE_HEIGHT : STRETCH_HEIGHT;
			};

			for (TransformationType transformationType : views::iota(0, abs(n1)+abs(n2)) | views::transform(tt))
			{
				vector<MyRect> transformation = ranges::min(Transformations[transformationType] | views::transform(ff), {},
										[&](const vector<MyRect>& tf){
														const auto [width_, height_] = dimensions(compute_frame(input_rectangles + tf));
														int nb = n - ranges::count(tf, zero);
														return make_tuple(width_, height_, nb);
														 }
									);
				RectMat(accumulated_transformation) += transformation;
			}
			return accumulated_transformation;
		};

		vector<DecisionTreeNode> decision_tree;

		auto build_decision_tree = [&](int parent_index, const vector<MyRect>& input_rectangles, auto&& build_decision_tree)->void{

			vector<RectHole> holes = compute_all_holes(input_rectangles);

			vector<int> v;
			for (int ri=parent_index; ri!=-1; ri=decision_tree[ri].parent_index)
			{
				v.push_back(ri);
			}
			ranges::reverse(v);
			int depth = v.size();

			if (depth >= 5)
				return;

			auto rgh = holes | views::filter([&](const RectHole& rh){return ranges::binary_search(v,rh.ri)==false;});
			vector<RectHole> holes_;
			ranges::copy(rgh, back_inserter(holes_));

                        auto compute_ranking=[](int n, auto&& proj)->vector<int>{
                                vector<int> indices(n), ranking(n);
                                for (int ii=0; ii<n; ii++)
                                        indices[ii]=ii;
                                ranges::sort(indices, {}, proj);
                                for (int rk=0; rk<n; rk++)
                                {
                                        int ii = indices[rk];
                                        ranking[ii]=rk;
                                }
                                return ranking;
                        };

			auto edge_distance_gain=[&](int ii)->float{
				const auto& [ri, rj, rectCorner, dir, value, hrec] = holes_[ii];
				float gain = 0;
				for (const auto [i, j] : edges)
				{
					int k = i==ri ? j : i;
					gain -= rect_distance(input_rectangles[ri], input_rectangles[k]);
					gain += rect_distance(hrec, input_rectangles[k]);
				}
				return gain;
			};

			auto hole_potential=[&](int ii)->float{
                                const auto& [ri, rj, rectCorner, dir, value, hrec] = holes_[ii];
                                float potential=0;
                                for (const auto [i, j] : edges)
                                {
                                        if (i != ri && j != ri)
                                        {
                                                const MyRect &r_h=input_rectangles[ri], &r_i=input_rectangles[i],&r_j=input_rectangles[j];
                                                float d = rect_distance(r_i, r_j);
                                                float d_ = rect_distance(r_i, r_h) + rect_distance(r_j, r_h);
                                                if (d_ < d)
                                                        potential += d - d_;
                                        }
                                }
				return potential;
			};

                        vector<int> ranking0 = compute_ranking(holes_.size(), edge_distance_gain);
			vector<int> ranking00 = compute_ranking(holes_.size(), hole_potential);
			vector<int> ranking01 = compute_ranking(holes_.size(), [&](int ii){return ranking0[ii]+ranking00[ii];});

			vector<RectHole> keeper_holes;

			int nn = holes_.size();

                        ranges::copy(views::iota(0,nn) | views::filter([&](int ii){return ranking01[ii] < 15;})
                                                        | views::transform([&](int ii){return holes_[ii];}),
                                        back_inserter(keeper_holes));

			auto rgr = keeper_holes | views::transform([&](const RectHole& rh)->vector<MyRect>{
				vector<MyRect> rectangles = input_rectangles + compute_transformation(input_rectangles, rh);
				vector<MyRect> tf = compute_compact_frame_transform(rectangles);
				RectMat(rectangles) += tf;
				return rectangles;
			});

			vector<vector<MyRect> > node_rectangles;
			ranges::copy(rgr, back_inserter(node_rectangles));

			nn = keeper_holes.size();
			auto rg = views::iota(0, nn) | views::transform([&](int ii)->DecisionTreeNode{

				const vector<MyRect>& rectangles = node_rectangles[ii];
				const RectHole& rh = keeper_holes[ii];
				const auto& [ri, rj, rectCorner, dir, value, hrec] = rh;

				MyRect frame = compute_frame(rectangles);
				MyPoint dim = dimensions(frame);
				float rect_distances=0;
				for (const auto [i, j] : edges)
				{
					rect_distances += rect_distance(rectangles[i], rectangles[j]);
				}

				float potential=0;
				for (const auto [i, j] : edges)
				{
					if (i != ri && j != ri)
					{
						const MyRect &r_h=input_rectangles[ri], &r_i=input_rectangles[i],&r_j=input_rectangles[j];
						float d = rect_distance(r_i, r_j);
						float d_ = rect_distance(r_i, r_h) + rect_distance(r_j, r_h);
						if (d_ < d)
							potential += d - d_;
					}
				}

				return {parent_index, depth, rh, dim, rect_distances, potential};
			});

			vector<DecisionTreeNode> nodes;
			ranges::copy(rg, back_inserter(nodes));

                        vector<int> ranking1 = compute_ranking(nodes.size(), [&](int ii){auto [w, h] = nodes[ii].dim; return max(w,h);});
			vector<int> ranking2 = compute_ranking(nodes.size(), [&](int ii){return nodes[ii].rect_distances;});
			vector<int> ranking3 = compute_ranking(nodes.size(), [&](int ii){return nodes[ii].potential;});
			for (int& rk : ranking3)
				rk = nn - rk;
			vector<int> ranking = compute_ranking(nodes.size(), [&](int ii){return ranking1[ii]+ranking2[ii]+ranking3[ii];});

			size_t index = decision_tree.size();
			ranges::copy(views::iota(0,nn) | views::filter([&](int ii){return ranking[ii] < 7;})
							| views::transform([&](int ii){return nodes[ii];}),
					back_inserter(decision_tree));
			printf("decision_tree.size()=%ld\n", decision_tree.size());
			for (int ii : views::iota(0,nn) | views::filter([&](int ii){return ranking[ii] < 7;}))
			{
				build_decision_tree(index, node_rectangles[ii], build_decision_tree);
				if (decision_tree.size() % 100==0)
					printf("decision_tree.size()=%ld\n", decision_tree.size());
			}
		};

		printf("calling build_decision_tree()\n");
                build_decision_tree(-1, input_rectangles, build_decision_tree);

		vector<MyRect> rectangles = input_rectangles + compute_transformation(input_rectangles, holes[5]);
		MyRect frame_ = compute_frame(rectangles);

	//TODO: compute rankings and select best node.
		int n = input_rectangles.size();

		FILE *f=fopen("holes.html", "w");

		fprintf(f, "<html>\n<body>\n");
		fprintf(f, "<svg width=\"%d\" height=\"%d\">\n", width(frame)+100, height(frame));
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
		fprintf(f, "</svg>\n</html>");
		fclose(f);

		f=fopen("rects.html", "w");
		fprintf(f, "<html>\n<body>\n");
		fprintf(f, "<svg width=\"%d\" height=\"%d\">\n", width(frame_)+100, height(frame_));
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
