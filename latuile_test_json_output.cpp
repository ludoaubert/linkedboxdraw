#include <vector>
#include <algorithm>
#include "MyRect.h"
using namespace std;

void json_context_output(const vector<MyRect> &rectangles,
			const char* file_name)
{
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
"links":[]
}
]}
)");

        FILE *f = fopen(file_name, "w");
        fwrite(f, buffer);
        fclose(f);
}

void json_diagdata_output(const vector<MyRect> &rectangles,
                        const char* file_name)
{
	char buffer[10 * 1024];
	int pos = 0;

	int testid=0;

	pos += sprintf(buffer + pos, "{\"documentTitle\":\"reg-test-%d\",\n\"boxes\":[\n", testid);
	for (int i=0; i < rectangles.size(); i++)
		pos += sprintf(buffer + pos, "\t{\"title\":\"rec-%d\", \"fields\":[]},\n", i);

	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}

	pos += sprintf(buffer + pos, R"(],
"values":[],
"boxComments":[],
"fieldComments":[],
"links":[
)");
/*
	for (const auto& [from, to] : links)
	{
		pos += sprintf(buffer + pos, "{\"from\":%d,\"fromField\":-1,\"fromCardinality\":\"undefined\",\"to\":%d,\"toField\":-1,\"toCardinality\":\"undefined\"},\n", from, to);
	}
*/
	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}

	pos += sprintf(buffer + pos, R"(],
"fieldColors":[],
"rectangles":[
)");
	for (const auto& [left, right, top, bottom] : rectangles)
	{
		pos += sprintf(buffer + pos, "\t{\"left\":%hu,\"right\":%hu,\"top\":%hu,\"bottom\":%hu},\n", 0, right - left, 0, bottom - top);
	}

	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}

	pos += sprintf(buffer + pos, "]}\n");

	FILE *f = fopen(file_name, "w");
        fwrite(f, buffer);
        fclose(f);
}

template <unsigned N>
void latuile_test_json_output(const vector<MyRect> &input_rectangles,
				int (&edges)[N][2],
				const vector<MyRect> &expected_rectangles,
				const char* test_name,
				int test_number)
{
	char file_name[100];
        sprintf(file_name, "test-latuile-%s-%d-diagdata.json", test_name, test_number);
	json_diagdata_output(input_rectangles, file_name);
        sprintf(file_name, "test-latuile-%s-%d-input-contexts.json", test_name, test_number);
	json_context_output(input_rectangles, file_name);
	sprintf(file_name, "test-latuile-%s-%d-expected-contexts.json", test_name, test_number);
	json_context_output(expected_rectangles, file_name);
}
