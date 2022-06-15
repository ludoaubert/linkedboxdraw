#ifndef _LATUILE_TEST_JSON_OUTPUT_
#define _LATUILE_TEST_JSON_OUTPUT_


#include <vector>
#include "MyRect.h"

extern int nbOK;
extern int nbKO;

struct Edge
{
	int from, to;
};


void json_diagdata_output(int n,
			const std::vector<Edge> &edges,
                        const char* file_name,
			int testid);


void latuile_test_json_output(const std::vector<MyRect> &input_rectangles,
				const std::vector<MyRect> &output_rectangles,
                               	const std::vector<Edge> &edges,
                                const std::vector<MyRect> &expected_rectangles,
                                const char* test_name,
                                int testid);


#endif
