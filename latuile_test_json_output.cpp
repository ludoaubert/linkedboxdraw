#include <vector>
#include <algorithm>
#include <ranges>
#include "MyRect.h"
#include "latuile_test_json_output.h"
using namespace std;


void json_context_output(const vector<MyRect> &rectangles,
			const char* file_name)
{
        char buffer[10 * 1024];
        int pos = 0;

        MyRect frame = compute_frame(rectangles);

        pos += sprintf(buffer + pos, R"(
{"contexts":[{
"frame":{"left":%hu,"right":%hu,"top":%hu,"bottom":%hu},
"translatedBoxes":[
)", frame.m_left, frame.m_right, frame.m_top, frame.m_bottom);

        int i=0;
        for (const auto& [left, right, top, bottom, j, jj, selected] : rectangles)
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
        fprintf(f, "%s", buffer);
        fclose(f);
}

void json_diagdata_output(const vector<MyRect> &rectangles,
			const std::vector<Edge> &edges,
                        const char* file_name)
{
	char buffer[100 * 1024];
	int pos = 0;

	int testid=0;

	pos += sprintf(buffer + pos, "{\"documentTitle\":\"reg-test-%d\",\n\"boxes\":[\n", testid);
	for (int i=0; i < rectangles.size(); i++)
	{
		pos += sprintf(buffer + pos, "\t{\"title\":\"rec-%d\",\n",i);
		pos += sprintf(buffer + pos, "\t\"id\":%d,\n",i);
		pos += sprintf(buffer + pos, "\t\"fields\":[\n");
		auto r = edges | ranges::views::filter( [=](const Edge& e){return e.from==i || e.to==i;}) |
			ranges::views::transform( [=](const Edge& e){return e.from==i ? e.to : e.from;});
		for (int j : r)
			pos += sprintf(buffer + pos, "\t\t{\"name\":\"rec-%d\",\"isPrimaryKey\":false,\"isForeignKey\":false},\n", j);
        	if (buffer[pos-2]==',')
 		{
 			buffer[pos-2]='\n';
 			pos--;
 		}
		pos += sprintf(buffer + pos, "\t]},\n");
	}

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

	for (auto [from, to] : edges)
	{
		pos += sprintf(buffer + pos, "\t{\"from\":%d,\"fromField\":-1,\"fromCardinality\":\"undefined\",\"to\":%d,\"toField\":-1,\"toCardinality\":\"undefined\"},\n",from, to);

	}

	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}

	pos += sprintf(buffer + pos, R"(],
"fieldColors":[],
"rectangles":[
)");
	for (const auto& [left, right, top, bottom, i, ii, selected] : rectangles)
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
        fprintf(f, "%s", buffer);
        fclose(f);
}


void latuile_test_json_output(const vector<MyRect> &input_rectangles,
				const vector<MyRect> &output_rectangles,
				const vector<Edge> &edges,
				const vector<MyRect> &expected_rectangles,
				const char* test_name,
				int test_number)
{
	char file_name[100];
        sprintf(file_name, "test-latuile-%s-%d-diagdata.json", test_name, test_number);
	json_diagdata_output(input_rectangles, edges, file_name);
        sprintf(file_name, "test-latuile-%s-%d-input-contexts.json", test_name, test_number);
	json_context_output(input_rectangles, file_name);
	sprintf(file_name, "test-latuile-%s-%d-expected-contexts.json", test_name, test_number);
	json_context_output(expected_rectangles, file_name);
        sprintf(file_name, "test-latuile-%s-%d-output-contexts.json", test_name, test_number);
        json_context_output(output_rectangles, file_name);
}
