#ifndef _LATUILE_TEST_JSON_OUTPUT_
#define _LATUILE_TEST_JSON_OUTPUT_


#include <vector>
#include "MyRect.h"


void json_diagdata_output(const std::vector<MyRect> &rectangles,
                        const char* file_name);


void latuile_test_json_output(const std::vector<MyRect> &input_rectangles,
				const std::vector<MyRect> &output_rectangles,
                               // const std::vector<int[2]> &edges,
                                const std::vector<MyRect> &expected_rectangles,
                                const char* test_name,
                                int test_number);


#endif
