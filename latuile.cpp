//
// latuile.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2016-2022 Ludovic Aubert - All Rights reserved
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
#include "compact_frame.h"
#include "fit_together.h"
#include "optimize_rectangle_positions.h"
#include "swap_rectangles.h"
#include "binpack.h"
#include "FunctionTimer.h"
#include "latuile_test_json_output.h"
#include <chrono>
#include <cmath>
#include <numeric>
#include <cstring>
using namespace std ;
using namespace std::chrono;


void test()
{
    TestFunctionTimer ft("test_latuile");

    struct TestOutputContext{MyRect frame; vector<TranslatedBox> translatedBoxes;};
    struct TestContext{int testid; string title; vector<MyRect> input_rectangles; vector<Edge> edges; vector<TestOutputContext> expected_contexts;};


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


    const vector<TestContext> test_contexts = {
{
	.testid=1,
	.title="Verlan",
	.input_rectangles={
		{.m_left=0, .m_right=141, .m_top=0, .m_bottom=40, .no_sequence=0},
		{.m_left=0, .m_right=162, .m_top=0, .m_bottom=56, .no_sequence=1 },
		{.m_left=0, .m_right=64, .m_top=0, .m_bottom=104, .no_sequence=2 },
		{.m_left=0, .m_right=120, .m_top=0, .m_bottom=120, .no_sequence=3 },
		{.m_left=0, .m_right=141, .m_top=0, .m_bottom=56, .no_sequence=4 },
		{.m_left=0, .m_right=267, .m_top=0, .m_bottom=72, .no_sequence=5 },
		{.m_left=0, .m_right=71, .m_top=0, .m_bottom=136, .no_sequence=6 },
		{.m_left=0, .m_right=78, .m_top=0, .m_bottom=72, .no_sequence=7 },
		{.m_left=0, .m_right=211, .m_top=0, .m_bottom=72, .no_sequence=8 },
		{.m_left=0, .m_right=133, .m_top=0, .m_bottom=88, .no_sequence=9 },
		{.m_left=0, .m_right=106, .m_top=0, .m_bottom=104, .no_sequence=10 },
		{.m_left=0, .m_right=105, .m_top=0, .m_bottom=120, .no_sequence=11 },
		{.m_left=0, .m_right=57, .m_top=0, .m_bottom=56, .no_sequence=12 },
		{.m_left=0, .m_right=63, .m_top=0, .m_bottom=72, .no_sequence=13 },
		{.m_left=0, .m_right=154, .m_top=0, .m_bottom=88, .no_sequence=14 },
		{.m_left=0, .m_right=98, .m_top=0, .m_bottom=56, .no_sequence=15 },
		{.m_left=0, .m_right=112, .m_top=0, .m_bottom=56, .no_sequence=16 },
		{.m_left=0, .m_right=112, .m_top=0, .m_bottom=72, .no_sequence=17 },
		{.m_left=0, .m_right=50, .m_top=0, .m_bottom=56, .no_sequence=18 },
		{.m_left=0, .m_right=196, .m_top=0, .m_bottom=120, .no_sequence=19 }
 	},
	.edges={
 		{.from=1,  .to=14 },
 		{.from=2,  .to=14 },
 		{.from=2,  .to=5 },
 		{.from=3,  .to=4 },
 		{.from=5,  .to=17 },
 		{.from=6,  .to=14 },
 		{.from=6,  .to=5 },
 		{.from=7,  .to=0 },
 		{.from=7,  .to=16 },
 		{.from=8,  .to=9 },
 		{.from=9,  .to=15 },
 		{.from=10,  .to=18 },
 		{.from=10,  .to=16 },
 		{.from=10,  .to=9 },
 		{.from=11,  .to=13 },
 		{.from=12,  .to=14 },
 		{.from=14,  .to=7 },
 		{.from=16,  .to=3 },
 		{.from=17,  .to=7 },
 		{.from=19,  .to=10 },
 		{.from=19,  .to=7 },
 		{.from=19,  .to=8 },
 		{.from=19,  .to=11 }
   	},

	.expected_contexts={
            {
	        .frame={.m_left=0, .m_right=715, .m_top=0, .m_bottom=752},
	        .translatedBoxes={
		    {.id=0, .translation={.x=329, .y=250}},
		    {.id=1, .translation={.x=523, .y=234}},
		    {.id=2, .translation={.x=44, .y=42}},
		    {.id=3, .translation={.x=329, .y=330}},
		    {.id=4, .translation={.x=489, .y=394}},
		    {.id=5, .translation={.x=22, .y=186}},
		    {.id=6, .translation={.x=218, .y=10}},
		    {.id=7, .translation={.x=211, .y=298}},
		    {.id=8, .translation={.x=183, .y=650}},
		    {.id=9, .translation={.x=10, .y=634}},
		    {.id=10, .translation={.x=183, .y=506}},
		    {.id=11, .translation={.x=565, .y=490}},
		    {.id=12, .translation={.x=523, .y=138}},
		    {.id=13, .translation={.x=565, .y=650}},
		    {.id=14, .translation={.x=329, .y=122}},
		    {.id=15, .translation={.x=10, .y=538}},
		    {.id=16, .translation={.x=177, .y=410}},
		    {.id=17, .translation={.x=59, .y=298}},
		    {.id=18, .translation={.x=87, .y=410}},
		    {.id=19, .translation={.x=329, .y=490}}
	        }
            }
	}
},
{
	.testid=2,
	.title="LOLA",
	.input_rectangles={
		{.m_left=0,.m_right=162,.m_top=0,.m_bottom=104, .no_sequence=0},//8-0
		{.m_left=0,.m_right=182,.m_top=0,.m_bottom=72, .no_sequence=1},//9-1
		{.m_left=0,.m_right=105,.m_top=0,.m_bottom=72, .no_sequence=2},//10-2
		{.m_left=0,.m_right=126,.m_top=0,.m_bottom=152, .no_sequence=3},//21-3
		{.m_left=0,.m_right=126,.m_top=0,.m_bottom=88, .no_sequence=4},//24-4
		{.m_left=0,.m_right=147,.m_top=0,.m_bottom=120, .no_sequence=5},//25-5
		{.m_left=0,.m_right=140,.m_top=0,.m_bottom=120, .no_sequence=6},//26-6
		{.m_left=0,.m_right=168,.m_top=0,.m_bottom=136, .no_sequence=7},//27-7
		{.m_left=0,.m_right=168,.m_top=0,.m_bottom=120, .no_sequence=8},//28-8
		{.m_left=0,.m_right=147,.m_top=0,.m_bottom=104, .no_sequence=9},//30-9
		{.m_left=0,.m_right=133,.m_top=0,.m_bottom=120, .no_sequence=10},//32-10
		{.m_left=0,.m_right=147,.m_top=0,.m_bottom=168, .no_sequence=11},//44-11
		{.m_left=0,.m_right=140,.m_top=0,.m_bottom=88, .no_sequence=12},//48-12
		{.m_left=0,.m_right=155,.m_top=0,.m_bottom=120, .no_sequence=13},//52-13
		{.m_left=0,.m_right=175,.m_top=0,.m_bottom=136, .no_sequence=14}//53-14
	},
        .edges={
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
        .expected_contexts={
	    {
		.frame={.m_left=0,.m_right=884,.m_top=0,.m_bottom=552},
		.translatedBoxes={
			{.id=0,.translation={.x=396,.y=10}},
			{.id=1,.translation={.x=320,.y=330}},
			{.id=2,.translation={.x=453,.y=218}},
			{.id=3,.translation={.x=598,.y=10}},
			{.id=4,.translation={.x=598,.y=202}},
			{.id=5,.translation={.x=750,.y=346}},
			{.id=6,.translation={.x=273,.y=154}},
			{.id=7,.translation={.x=542,.y=330}},
			{.id=8,.translation={.x=335,.y=506}},
			{.id=9,.translation={.x=556,.y=506}},
			{.id=10,.translation={.x=764,.y=186}},
			{.id=11,.translation={.x=743,.y=506}},
			{.id=12,.translation={.x=93,.y=153}},
			{.id=13,.translation={.x=10,.y=281}},
			{.id=14,.translation={.x=11,.y=441}}
                },
            }
	}
}
};

	for (const auto& [testid, title, input_rectangles, edges, expected_contexts] : test_contexts)
	{
		vector<MyRect> rectangles = input_rectangles;
		int n = rectangles.size() ;
 		vector<vector<MPD_Arc> > adjacency_list(n) ;

    		for (const Edge& e : edges)
    		{
        		adjacency_list[e.from].push_back({e.from, e.to}) ;
    		}

    		vector<Context> contexts ;
    		high_resolution_clock::time_point t1 = high_resolution_clock::now();
    		compute_contexts(rectangles, adjacency_list, max_nb_boxes_per_diagram, contexts) ;

    		for (int i=0; i < contexts.size(); i++)
    		{
        		Context &ctx = contexts[i];
        		ranges::sort(ctx.rectangles, {}, &MyRect::no_sequence);
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
			bool bOK = expected_contexts[i].translatedBoxes == translatedBoxes;
        		printf("test latuile %s %s\n", title.c_str(), bOK ? "OK" : "KO");
			(bOK ? nbOK : nbKO)++;

        		vector<MyRect> expected_rectangles = input_rectangles;

        		for (int j=0; j<n; j++)
        		{
				expected_rectangles[j] = translate(input_rectangles[j], expected_contexts[i].translatedBoxes[j].translation);
        		}

                        printf("dim_max(compute_frame(expected_rectangles)) : %d\n", dim_max(compute_frame(expected_rectangles)));
			printf("dim_max(compute_frame(output_rectangles)) : %d\n", dim_max(compute_frame(ctx.rectangles)));

			latuile_test_json_output(input_rectangles,
                        		        ctx.rectangles,
                                		edges,
                                		expected_rectangles,
                                		"test_latuile",
                                		testid);
    		}
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
		nbOK = nbKO = 0;
		test();
        	test_optimize_rectangle_positions();
		test_compact_rectangles();
		test_compact_frame();
		test_fit_together();
		test_swap_rectangles();
		test_binpack();
		test_split_and_fit();
		test_stair_steps_layout_from_111_boxes();
		test_stair_steps(RECT_BORDER);
		test_stair_steps_layout();
		printf("latuile: %d/%d tests successful.\n", nbOK, nbOK + nbKO);
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

        vector<Context> contexts ;
        compute_contexts(rectangles, adjacency_list, max_nb_boxes_per_diagram, contexts) ;
//on ne conserve que les rectangles
        for (Context &ctx : contexts)
        {
            ranges::sort(ctx.rectangles, {}, &MyRect::no_sequence);
        }
		char res[100000];
        write_json(rectangles, contexts, res);
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

	vector<Context> contexts ;
	compute_contexts(rectangles, adjacency_list, max_nb_boxes_per_diagram, contexts) ;
//on ne conserve que les rectangles
	for (Context &ctx : contexts)
	{
		ranges::sort(ctx.rectangles, {}, &MyRect::no_sequence);
	}
	static char res[100000];
	write_json(rectangles, contexts, res);
	return res;
}
}

/*
Linux command to install eigen3 directory:
 sudo apt-get install libeigen3-dev
Linux command to lookup eigen3 directory:
 sudo find / -type d -name "eigen3"

To generate latuile.wasm and latuile.js:
emcc bombix.cpp latuile.cpp binpack.cpp compact_frame.cpp compact_rectangles.cpp fit_together.cpp KMeansRexCore.cpp MyRect.cpp optimize_rectangle_positions.cpp permutation.cpp stair_steps.cpp swap_rectangles.cpp WidgetContext.cpp FunctionTimer.cpp -o latuile.js -I/usr/include/eigen3 -Wno-c++11-narrowing -s EXPORTED_FUNCTIONS='["_latuile","_bombix"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -s ALLOW_MEMORY_GROWTH=1  -s EXPORT_ES6=1 -sMODULARIZE -s EXPORT_NAME="createMyModule"

in bombix.cpp replace main by main69 to avoid duplicate symbols braking the compilation process.
using cmake seemed more complicated.
*/
