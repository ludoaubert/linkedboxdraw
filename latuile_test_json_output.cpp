#include <vector>
#include <algorithm>
#include "MyRect.h"
using namespace std;


template <unsigned N>
void latuile_test_json_output(const vector<MyRect> &input_rectangles,
				int (&edges)[N][2],
				const vector<MyRect> &expected_rectangles,
				const char* test_name,
				int test_number)
{
	char file_name[100];
        sprintf(file_name, "test-latuile-%s-%d-diagdata.json", test_name, test_number);

        sprintf(file_name, "test-latuile-%s-%d-input-contexts.json", test_name, test_number);
	sprintf(file_name, "test-latuile-%s-%d-expected-contexts.json", test_name, test_number);


	char buffer[10 * 1024];
	int pos = 0;

	MyRect frame = compute_frame(input_rectangles);
	expand_by(frame, FRAME_BORDER);

	pos += sprintf(buffer + pos, R"(
{"contexts":[{
"frame":{"left":%hu,"right":%hu,"top":%hu,"bottom":%hu},
"translatedBoxes":[
)", frame.left, frame.right, frame.top, frame.bottom);

	int i=0;
	for (const auto& [left, right, top, bottom] : input_rectangles)
	{
		pos += sprintf(buffer + pos, "{\"id\":%d,\"translation\":{\"x\":%hu,\"y\":%hu}},\n", i++, left, top);
	}

	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}

	pos += sprintf(buffer + pos, R"(],
"links":%s
}
]}
)", pjson.c_str());

	return buffer;
}
