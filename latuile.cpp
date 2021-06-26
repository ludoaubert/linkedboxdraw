//
// latuile.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2016-2017 Ludovic Aubert - All Rights reserved
//
//

#include <string>
#include <vector>
#include <regex>
#include <unordered_map>
#include <assert.h>
#include "index_from.h"
#include "MyRect.h"
#include "MPD_Arc.h"
#include "permutation.h"
#include "stair_steps.h"
#include "compact_rectangles.h"
#include "fit_together.h"
#include "optimize_rectangle_positions.h"
#include "swap_rectangles.h"
#include "binpack.h"
#include "FunctionTimer.h"
#include <chrono>
#include <cmath>
#include <numeric>
using namespace std ;
using namespace std::chrono;


void test()
{
    TestFunctionTimer ft("test_latuile");

    struct Rect {int left,right,top,bottom;};
    struct Edge {int from,to;};
    struct TestInput{vector<Rect> rectangles; vector<Edge> edges;};
    struct TestOutputContext{string title; Rect frame; vector<TranslatedBox> translatedBoxes;};
    struct TestContext{TestInput input; vector<TestOutputContext> output;};


/*
                         +------+
            +------+     |      |
            |      |     |      |
            |  2   |     |  6   |
            |      |     |      |    +-----------------+
            |      |     |      |    |                 |    +------+
            +------+     +------+    |                 |    |      |
                                     |       14        |    |  12  |
  +-----------------------------+    |                 |    +------+
  |                             |    +-----------------+
  |             5               |                           +-------------------+
  |                             |    +---------------+      |                   |
  +-----------------------------+    |       0       |      |         1         |
                                     |               |      |                   |
      +-----------+   +---------+    +---------------+      +-------------------+
      |           |   |         |
      |    17     |   |    7    |    +------------+
      |           |   |         |    |            |
      +-----------+   +---------+    |            |
                                     |            |    +------------------+
        +-----+   +-------------+    |     3      |    |                  |
        |     |   |             |    |            |    |        4         |
        |  18 |   |      16     |    |            |    |                  |
        |     |   |             |    +------------+    +------------------+
        +-----+   +-------------+
                                     +------------------------+   +------------+
                    +-----------+    |                        |   |            |
                    |           |    |                        |   |            |
  +------------+    |           |    |           19           |   |     11     |
  |            |    |    10     |    |                        |   |            |
  |     15     |    |           |    |                        |   |            |
  +------------+    |           |    |                        |   |            |
                    +-----------+    +------------------------+   +------------+
  +--------------+
  |              |  +-------------------------+                   +-------+
  |       9      |  |                         |                   |       |
  |              |  |           8             |                   |   13  |
  |              |  |                         |                   |       |
  +--------------+  +-------------------------+                   +-------+
*/


    TestContext context = {
        {//input
            {//rectangles
 /*0*/{/*left*/0,/*right*/141,/*top*/0,/*bottom*/40 },
 /*1*/{/*left*/0,/*right*/162,/*top*/0,/*bottom*/56 },
 /*2*/{/*left*/0,/*right*/64,/*top*/0,/*bottom*/104 },
 /*3*/{/*left*/0,/*right*/120,/*top*/0,/*bottom*/120 },
 /*4*/{/*left*/0,/*right*/141,/*top*/0,/*bottom*/56 },
 /*5*/{/*left*/0,/*right*/267,/*top*/0,/*bottom*/72 },
 /*6*/{/*left*/0,/*right*/71,/*top*/0,/*bottom*/136 },
 /*7*/{/*left*/0,/*right*/78,/*top*/0,/*bottom*/72 },
 /*8*/{/*left*/0,/*right*/211,/*top*/0,/*bottom*/72 },
 /*9*/{/*left*/0,/*right*/133,/*top*/0,/*bottom*/88 },
 /*10*/{/*left*/0,/*right*/106,/*top*/0,/*bottom*/104 },
 /*11*/{/*left*/0,/*right*/105,/*top*/0,/*bottom*/120 },
 /*12*/{/*left*/0,/*right*/57,/*top*/0,/*bottom*/56 },
 /*13*/{/*left*/0,/*right*/63,/*top*/0,/*bottom*/72 },
 /*14*/{/*left*/0,/*right*/154,/*top*/0,/*bottom*/88 },
 /*15*/{/*left*/0,/*right*/98,/*top*/0,/*bottom*/56 },
 /*16*/{/*left*/0,/*right*/112,/*top*/0,/*bottom*/56 },
 /*17*/{/*left*/0,/*right*/112,/*top*/0,/*bottom*/72 },
 /*18*/{/*left*/0,/*right*/50,/*top*/0,/*bottom*/56 },
 /*19*/{/*left*/0,/*right*/196,/*top*/0,/*bottom*/120 }
    	    },//rectangles
            {//edges
 {/*source*/1, /*target*/14 },
 {/*source*/2, /*target*/14 },
 {/*source*/2, /*target*/5 },
 {/*source*/3, /*target*/4 },
 {/*source*/5, /*target*/17 },
 {/*source*/6, /*target*/14 },
 {/*source*/6, /*target*/5 },
 {/*source*/7, /*target*/0 },
 {/*source*/7, /*target*/16 },
 {/*source*/8, /*target*/9 },
 {/*source*/9, /*target*/15 },
 {/*source*/10, /*target*/18 },
 {/*source*/10, /*target*/16 },
 {/*source*/10, /*target*/9 },
 {/*source*/11, /*target*/13 },
 {/*source*/12, /*target*/14 },
 {/*source*/14, /*target*/7 },
 {/*source*/16, /*target*/3 },
 {/*source*/17, /*target*/7 },
 {/*source*/19, /*target*/10 },
 {/*source*/19, /*target*/7 },
 {/*source*/19, /*target*/8 },
 {/*source*/19, /*target*/11 }
   	     }//edges
	},//input
	{//output
            {//context 0
	        "",//title
	        {0, 707, 0, 744},//frame
	        {//translatedBoxes
		    {0, {329,250}},
		    {1, {523,235}},
		    {2, {114,42}},
		    {3, {329,330}},
		    {4, {489,394}},
		    {5, {22,186}},
		    {6, {218,10}},
		    {7, {211,298}},
		    {8, {183,650}},
		    {9, {10,634}},
		    {10, {183,506}},
		    {11, {565,490}},
		    {12, {523,139}},
		    {13, {564,650}},
		    {14, {329,122}},
		    {15, {10,538}},
		    {16, {177,410}},
		    {17, {59,298}},
		    {18, {87,410}},
		    {19, {329,490}}
	        }//translatedBoxes
            }//context 0
	}//output
    };

    vector<MyRect> rectangles;
    int i=0;
    bool selected=true;
    for (Rect& rec : context.input.rectangles)
    {
	MyRect rect = {rec.left, rec.right, rec.top, rec.bottom, i, i, selected};
	i++;
	rectangles.push_back(rect);
    }
    int n = rectangles.size() ;
    vector<vector<MPD_Arc> > adjacency_list(n) ;

    for (Edge& e : context.input.edges)
    {
        MPD_Arc edge;
	edge._i = e.from;
	edge._j = e.to;
        assert(edge._i < n);
        assert(edge._j < n);
//si on ne met rien, compute_adjacency_list_() ne va pas voir ces arcs
        adjacency_list[edge._i].push_back(edge) ;
    }
    int no_sequence_from_center = -1;
    vector<Context> contexts ;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    compute_contexts(rectangles, adjacency_list, max_nb_boxes_per_diagram, no_sequence_from_center,contexts) ;
//on ne conserve que les rectangles
    for (int i=0; i < contexts.size(); i++)
    {
        Context &ctx = contexts[i];
        sort(begin(ctx.rectangles), end(ctx.rectangles), [](MyRect& r1, MyRect& r2){return r1.no_sequence < r2.no_sequence;});
        MyRect &frame = ctx.frame;
        printf("frame={%d, %d, %d, %d}\n", frame.m_left, frame.m_right, frame.m_top, frame.m_bottom);
        for (MyRect& rec : ctx.rectangles)
        {
            printf("i=%d, no_sequence=%d, x=%d, y=%d\n", rec.i, rec.no_sequence, rec.m_left, rec.m_top);
        }
        vector<TranslatedBox> translatedBoxes;
        for (MyRect& rec : ctx.rectangles)
        {
            translatedBoxes.push_back({rec.no_sequence, {rec.m_left, rec.m_top}});
        }
        printf("%s\n", context.output[i].translatedBoxes == translatedBoxes ? "OK" : "KO");
    }
}

void parse_command(const char* rectdim,
					const char* slinks,
					vector<MyRect> &rectangles,
					vector<MPD_Arc> &edges)
{
	int pos;

	pos = 0;
    MyRect r{0,0,0,0};
	int nn;
	while (sscanf(rectdim + pos, "%3hx%3hx%n", &r.m_right, &r.m_bottom, &nn) == 2)
	{
	//use variable 'MyRect.no_sequence' to keep the original position of the box.
	//keep in mind that variable 'MyRect.i' is used internally by some algorithms and cannot be used for that purpose.
		r.no_sequence = r.i = rectangles.size();
		rectangles.push_back(r);
		pos += nn;
	}

    int n = rectangles.size();

	pos = 0;
	MPD_Arc edge;
	while (sscanf(slinks + pos, "%3x%3x%n", &edge._i, &edge._j, &nn) == 2)
	{
        assert(edge._i < n);
        assert(edge._j < n);
		edges.push_back(edge);
		pos += nn;
	}
}

int main(int argc, char* argv[])
{
    log_file=fopen("perf.log","w");

	vector<MyRect> rectangles;
	vector<MPD_Arc> edges;

	if (argc == 1)
	{
		test();
        test_optimize_rectangle_positions();
		test_compact_rectangles();
		test_fit_together();
		test_swap_rectangles();
		test_binpack();
		test_split_and_fit();
		test_stair_steps_layout_from_111_boxes();
		test_stair_steps(RECT_BORDER);
		test_stair_steps_layout();
	}
	else if (argc == 5)
	{
		const regex hexa("^[0-9a-z]+$");

		unordered_map<string, const char*> args={
			 {"--rectdim", 0},
			 {"--links", 0}
		};        

		for (int i = 1; i + 1 < argc; i += 2)
		{
			if (args.count(argv[i]))
				 args[ argv[i] ] = argv[i + 1];
		}

		bool check = true;
		check &= strlen(args["--rectdim"]) % 6 == 0;
		check &= regex_match(args["--rectdim"], hexa);
		check &= strlen(args["--links"]) % 6 == 0;
		check &= regex_match(args["--links"], hexa);

		if (!check)
			return false;
	
		parse_command(args["--rectdim"], args["--links"], rectangles, edges);

		vector<vector<MPD_Arc> > adjacency_list(rectangles.size());
		for (MPD_Arc &edge : edges)
			adjacency_list[edge._i].push_back(edge);
		int no_sequence_from_center = -1;
        vector<Context> contexts ;
        compute_contexts(rectangles, adjacency_list, max_nb_boxes_per_diagram, no_sequence_from_center,contexts) ;
//on ne conserve que les rectangles
        for (Context &ctx : contexts)
        {
            sort(begin(ctx.rectangles), end(ctx.rectangles), [](MyRect& r1, MyRect& r2){return r1.no_sequence < r2.no_sequence;});
        }
		char res[100000];
        write_json(contexts, res);
		printf("%s", res);
	}

	return 0;
}

//interface for emscripten wasm
extern "C" {
const char* latuile(const char *rectdim, const char *slinks)
{
	vector<MyRect> rectangles;
	vector<MPD_Arc> edges;
	
	parse_command(rectdim, slinks, rectangles, edges);

	vector<vector<MPD_Arc> > adjacency_list(rectangles.size());
	for (MPD_Arc &edge : edges)
		adjacency_list[edge._i].push_back(edge);
	int no_sequence_from_center = -1;
	vector<Context> contexts ;
	compute_contexts(rectangles, adjacency_list, max_nb_boxes_per_diagram, no_sequence_from_center,contexts) ;
//on ne conserve que les rectangles
	for (Context &ctx : contexts)
	{
		sort(begin(ctx.rectangles), end(ctx.rectangles), [](MyRect& r1, MyRect& r2){return r1.no_sequence < r2.no_sequence;});
	}
	char res[100000];
	write_json(contexts, res);
	return res;
}	
}