#include <vector>
#include <array>
#include <string>
#include <deque>
#include <bitset>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <span>
#include <stdio.h>
#include <sys/stat.h>
#include <filesystem>
#include <cstring>
//#include <fmt/ranges.h>
//#include <format>
#include "FunctionTimer.h"
#include "MyRect.h"
using namespace std;
namespace fs = std::filesystem;

//./holes2 | grep -e rectangles -e translations -e selectors | grep -e id=1 -e id=0

#define _TRACE_

#ifdef _TRACE_
#  define D(x) x
#else
#  define D(x)
#endif

//Cf compute_box_rectangles.js
const int RECTANGLE_BOTTOM_CAP=200;


struct LogicalEdge {
	int from;
	int to;

	friend auto operator<=>(const LogicalEdge&, const LogicalEdge&) = default;
};


struct TopologicalEdge {
	int from;
	int to;
	int distance;

	friend auto operator<=>(const TopologicalEdge&, const TopologicalEdge&) = default;
};


struct MyVector
{
	float x=0.0f;
	float y=0.0f;

	operator MyPoint() const {
		return {x, y};
	}

        friend bool operator==(const MyVector&, const MyVector&) = default;
};

inline MyVector operator*(int16_t value, const MyVector& vec)
{
	const auto& [x, y] = vec;
	return {value*x, value*y};
}


struct RectHole {int ri; RectCorner corner; MyVector direction; int value; MyRect rec;};

//	lightweight node

struct RectMap
{
	int i_emplacement_source, i_emplacement_destination;
};

struct DecisionTreeNode
{
	int i;
	int parent_index=-1;
	int depth;
	RectMap recmap;
	int match;
};

struct TranslationRangeItem
{
	int id;
	int ri;
	MyPoint tr;

	friend bool operator==(const TranslationRangeItem&, const TranslationRangeItem&) = default;
};

struct RectangleHoleRangeItem
{
        int id;
        int ri;
        MyRect r;

        friend bool operator==(const RectangleHoleRangeItem&, const RectangleHoleRangeItem&) = default;
};

enum EtatEmplacement
{
	LIBRE,
	OCCUPE
};


struct SweepLineItem
{
	int sweep_value;
	RectDim rectdim;
	int ri;

	auto operator<=>(const SweepLineItem&) const = default;
};


/*
		  |
		  |
		  |
--------------------->tr
		  |
		  |
		  V
		sweep

		  |
		  |
		  |
--------------------->sweep
		  |
		  |
		  V
		 tr
*/

struct CustomLess
{
	inline bool operator()(const SweepLineItem& a, const SweepLineItem& b) const
	{
		if (a.sweep_value != b.sweep_value)
			return a.sweep_value < b.sweep_value;
		if (a.rectdim != b.rectdim)
			return a.rectdim > b.rectdim;   //RIGHT < LEFT and BOTTOM < TOP
		return a.ri < b.ri;
	}
};


struct RectLink
{
	int i;
	int j;
	int min_sweep_value, max_sweep_value=INT16_MAX;
	auto operator<=>(const RectLink&) const = default;
};

/*
links[0] ------>
links[1] <------
*/
struct ActiveLineItem
{
	int i;
	RectLink* links[2]={0,0};
};


struct Score{
	int id, sigma_edge_distance, width, height, total;
};


struct TestInput
{
	int testid;
	vector<MyRect> input_rectangles;
//bi directional edges
	vector<LogicalEdge> logical_edges;
};


const vector<TestInput> test_input={
{
	.testid=0,
	.input_rectangles = {
        	{.m_left=406, .m_right=608, .m_top=20, .m_bottom=164},
        	{.m_left=330, .m_right=552, .m_top=340, .m_bottom=451},
        	{.m_left=463, .m_right=608, .m_top=228, .m_bottom=340},
        	{.m_left=608, .m_right=774, .m_top=20, .m_bottom=212},
        	{.m_left=608, .m_right=774, .m_top=212, .m_bottom=340},
        	{.m_left=760, .m_right=947, .m_top=356, .m_bottom=516},
        	{.m_left=283, .m_right=463, .m_top=164, .m_bottom=324},
        	{.m_left=552, .m_right=760, .m_top=340, .m_bottom=516},
        	{.m_left=345, .m_right=553, .m_top=516, .m_bottom=676},
        	{.m_left=566, .m_right=753, .m_top=516, .m_bottom=660},
        	{.m_left=774, .m_right=947, .m_top=196, .m_bottom=356},
        	{.m_left=753, .m_right=940, .m_top=516, .m_bottom=724},
        	{.m_left=103, .m_right=283, .m_top=163, .m_bottom=291},
        	{.m_left=88, .m_right=283, .m_top=291, .m_bottom=451},
        	{.m_left=130, .m_right=345, .m_top=451, .m_bottom=627}
	},
//bi directional edges
	.logical_edges = {
		{.from=0,.to=3},
		{.from=1,.to=7},
		{.from=2,.to=7},
		{.from=3,.to=0},
		{.from=3,.to=4},
		{.from=4,.to=3},
		{.from=4,.to=7},
		{.from=5,.to=7},
		{.from=6,.to=7},
		{.from=6,.to=12},
		{.from=7,.to=1},
		{.from=7,.to=2},
		{.from=7,.to=4},
		{.from=7,.to=5},
		{.from=7,.to=6},
		{.from=7,.to=8},
		{.from=7,.to=9},
		{.from=7,.to=10},
		{.from=7,.to=11},
		{.from=8,.to=7},
		{.from=9,.to=7},
		{.from=10,.to=7},
		{.from=11,.to=7},
		{.from=12,.to=6},
		{.from=12,.to=13},
		{.from=13,.to=12},
		{.from=13,.to=14},
		{.from=14,.to=13}
	}
},
{
	.testid=1,
	.input_rectangles = {
		{.m_left=0+977-RECT_BORDER, .m_right=119+977+RECT_BORDER, .m_top=0+426-RECT_BORDER, .m_bottom=152+426+RECT_BORDER},//0
		{.m_left=0+977-RECT_BORDER, .m_right=119+977+RECT_BORDER, .m_top=0+250-RECT_BORDER, .m_bottom=136+250+RECT_BORDER},//1
		{.m_left=0+818-RECT_BORDER, .m_right=119+818+RECT_BORDER, .m_top=0+266-RECT_BORDER, .m_bottom=168+266+RECT_BORDER},//2
		{.m_left=0+942-RECT_BORDER, .m_right=154+942+RECT_BORDER, .m_top=0+10-RECT_BORDER, .m_bottom=200+10+RECT_BORDER},//3
		{.m_left=0+176-RECT_BORDER, .m_right=154+176+RECT_BORDER, .m_top=0+74-RECT_BORDER, .m_bottom=200+74+RECT_BORDER},//4
		{.m_left=0+624-RECT_BORDER, .m_right=126+624+RECT_BORDER, .m_top=0+26-RECT_BORDER, .m_bottom=200+26+RECT_BORDER},//6
		{.m_left=0+479-RECT_BORDER, .m_right=154+479+RECT_BORDER, .m_top=0+378-RECT_BORDER, .m_bottom=200+378+RECT_BORDER},//12
		{.m_left=0+388-RECT_BORDER, .m_right=154+388+RECT_BORDER, .m_top=0+10-RECT_BORDER, .m_bottom=88+10+RECT_BORDER},//13
		{.m_left=0+176-RECT_BORDER, .m_right=154+176+RECT_BORDER, .m_top=0+314-RECT_BORDER, .m_bottom=200+314+RECT_BORDER},//14
		{.m_left=0+388-RECT_BORDER, .m_right=196+388+RECT_BORDER, .m_top=0+138-RECT_BORDER, .m_bottom=200+138+RECT_BORDER},//15
		{.m_left=0+10-RECT_BORDER, .m_right=126+10+RECT_BORDER, .m_top=0+266-RECT_BORDER, .m_bottom=120+266+RECT_BORDER},//19
		{.m_left=0+10-RECT_BORDER, .m_right=126+10+RECT_BORDER, .m_top=0+154-RECT_BORDER, .m_bottom=72+154+RECT_BORDER},//20
		{.m_left=0+10-RECT_BORDER, .m_right=126+10+RECT_BORDER, .m_top=0+426-RECT_BORDER, .m_bottom=72+426+RECT_BORDER},//21
		{.m_left=0+673-RECT_BORDER, .m_right=105+673+RECT_BORDER, .m_top=0+266-RECT_BORDER, .m_bottom=200+266+RECT_BORDER},//26
		{.m_left=0+369-RECT_BORDER, .m_right=70+369+RECT_BORDER, .m_top=0+378-RECT_BORDER, .m_bottom=88+378+RECT_BORDER}//30
	},
	.logical_edges = {
		{.from=0, .to=2},
		{.from=1, .to=2},
		{.from=2, .to=0},
		{.from=2, .to=1},
		{.from=2, .to=3},
		{.from=2, .to=9},
		{.from=3, .to=2},
		{.from=4, .to=5},
		{.from=4, .to=6},
		{.from=4, .to=9},
		{.from=4, .to=11},
		{.from=5, .to=4},
		{.from=6, .to=4},
		{.from=6, .to=8},
		{.from=7, .to=9},
		{.from=8, .to=6},
		{.from=8, .to=9},
		{.from=8, .to=12},
		{.from=9, .to=2},
		{.from=9, .to=4},
		{.from=9, .to=7},
		{.from=9, .to=8},
		{.from=9, .to=10},
		{.from=9, .to=13},
		{.from=9, .to=14},
		{.from=10, .to=9},
		{.from=10, .to=11},
		{.from=10, .to=12},
		{.from=11, .to=4},
		{.from=11, .to=10},
		{.from=12, .to=8},
		{.from=12, .to=10},
		{.from=13, .to=9},
		{.from=14, .to=9}
	}
},
{
        .testid=2,
        .input_rectangles = {
		{.m_left=750-RECT_BORDER, .m_right=960+RECT_BORDER, .m_top=282-RECT_BORDER, .m_bottom=434+RECT_BORDER},
		{.m_left=23-RECT_BORDER, .m_right=163+RECT_BORDER, .m_top=314-RECT_BORDER, .m_bottom=434+RECT_BORDER},
		{.m_left=1215-RECT_BORDER, .m_right=1341+RECT_BORDER, .m_top=90-RECT_BORDER, .m_bottom=226+RECT_BORDER},
		{.m_left=1049-RECT_BORDER, .m_right=1175+RECT_BORDER, .m_top=122-RECT_BORDER, .m_bottom=242+RECT_BORDER},
		{.m_left=1166-RECT_BORDER, .m_right=1341+RECT_BORDER, .m_top=298-RECT_BORDER, .m_bottom=386+RECT_BORDER},
		{.m_left=1000-RECT_BORDER, .m_right=1126+RECT_BORDER, .m_top=282-RECT_BORDER, .m_bottom=418+RECT_BORDER},
		{.m_left=203-RECT_BORDER, .m_right=336+RECT_BORDER, .m_top=154-RECT_BORDER, .m_bottom=274+RECT_BORDER},
		{.m_left=708-RECT_BORDER, .m_right=876+RECT_BORDER, .m_top=154-RECT_BORDER, .m_bottom=242+RECT_BORDER},
		{.m_left=370-RECT_BORDER, .m_right=475+RECT_BORDER, .m_top=10-RECT_BORDER, .m_bottom=114+RECT_BORDER},
		{.m_left=204-RECT_BORDER, .m_right=330+RECT_BORDER, .m_top=10-RECT_BORDER, .m_bottom=114+RECT_BORDER},
		{.m_left=10-RECT_BORDER, .m_right=164+RECT_BORDER, .m_top=10-RECT_BORDER, .m_bottom=114+RECT_BORDER},
		{.m_left=85-RECT_BORDER, .m_right=162+RECT_BORDER, .m_top=154-RECT_BORDER, .m_bottom=210+RECT_BORDER},
		{.m_left=376-RECT_BORDER, .m_right=474+RECT_BORDER, .m_top=201-RECT_BORDER, .m_bottom=273+RECT_BORDER},
		{.m_left=708-RECT_BORDER, .m_right=869+RECT_BORDER, .m_top=10-RECT_BORDER, .m_bottom=114+RECT_BORDER},
		{.m_left=514-RECT_BORDER, .m_right=668+RECT_BORDER, .m_top=154-RECT_BORDER, .m_bottom=258+RECT_BORDER},
		{.m_left=203-RECT_BORDER, .m_right=386+RECT_BORDER, .m_top=314-RECT_BORDER, .m_bottom=418+RECT_BORDER},
		{.m_left=514-RECT_BORDER, .m_right=641+RECT_BORDER, .m_top=298-RECT_BORDER, .m_bottom=370+RECT_BORDER}
	},
        .logical_edges = {
		{.from=0, .to=5},
		{.from=0, .to=6},
		{.from=1, .to=6},
		{.from=2, .to=3},
		{.from=3, .to=2},
		{.from=3, .to=4},
		{.from=3, .to=5},
		{.from=4, .to=3},
		{.from=5, .to=0},
		{.from=5, .to=3},
		{.from=5, .to=7},
		{.from=6, .to=0},
		{.from=6, .to=1},
		{.from=6, .to=7},
		{.from=6, .to=8},
		{.from=6, .to=9},
		{.from=6, .to=10},
		{.from=6, .to=11},
		{.from=6, .to=15},
		{.from=7, .to=5},
		{.from=7, .to=6},
		{.from=8, .to=6},
		{.from=9, .to=6},
		{.from=10, .to=6},
		{.from=11, .to=6},
		{.from=12, .to=14},
		{.from=12, .to=15},
		{.from=12, .to=16},
		{.from=13, .to=14},
		{.from=14, .to=12},
		{.from=14, .to=13},
		{.from=14, .to=16},
		{.from=15, .to=6},
		{.from=15, .to=12},
		{.from=16, .to=12},
		{.from=16, .to=14}
	}
},
{
	.testid=3,
        .input_rectangles = {
                {.m_left=10-RECT_BORDER,.m_right=129+RECT_BORDER,.m_top=538-RECT_BORDER,.m_bottom=690+RECT_BORDER},
                {.m_left=10-RECT_BORDER,.m_right=129+RECT_BORDER,.m_top=730-RECT_BORDER,.m_bottom=866+RECT_BORDER},
                {.m_left=169-RECT_BORDER,.m_right=288+RECT_BORDER,.m_top=538-RECT_BORDER,.m_bottom=706+RECT_BORDER},
                {.m_left=328-RECT_BORDER,.m_right=482+RECT_BORDER,.m_top=618-RECT_BORDER,.m_bottom=818+RECT_BORDER},
                {.m_left=564-RECT_BORDER,.m_right=718+RECT_BORDER,.m_top=138-RECT_BORDER,.m_bottom=338+RECT_BORDER},
                {.m_left=564-RECT_BORDER,.m_right=690+RECT_BORDER,.m_top=378-RECT_BORDER,.m_bottom=578+RECT_BORDER},
		{.m_left=370-RECT_BORDER,.m_right=524+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=176-RECT_BORDER,.m_right=330+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=338+RECT_BORDER},
		{.m_left=176-RECT_BORDER,.m_right=330+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=328-RECT_BORDER,.m_right=524+RECT_BORDER,.m_top=378-RECT_BORDER,.m_bottom=578+RECT_BORDER},
		{.m_left=162-RECT_BORDER,.m_right=288+RECT_BORDER,.m_top=378-RECT_BORDER,.m_bottom=498+RECT_BORDER},
		{.m_left=370-RECT_BORDER,.m_right=496+RECT_BORDER,.m_top=266-RECT_BORDER,.m_bottom=338+RECT_BORDER},
		{.m_left=10-RECT_BORDER,.m_right=136+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=322+RECT_BORDER},
		{.m_left=522-RECT_BORDER,.m_right=627+RECT_BORDER,.m_top=618-RECT_BORDER,.m_bottom=818+RECT_BORDER},
		{.m_left=52-RECT_BORDER,.m_right=122+RECT_BORDER,.m_top=376-RECT_BORDER,.m_bottom=464+RECT_BORDER}
	},
	.logical_edges = {
		{.from=0,.to=2},
		{.from=1,.to=2},
		{.from=2,.to=0},
		{.from=2,.to=1},
		{.from=2,.to=3},
		{.from=2,.to=9},
		{.from=3,.to=2},
		{.from=4,.to=5},
		{.from=4,.to=6},
		{.from=4,.to=9},
		{.from=4,.to=11},
		{.from=5,.to=4},
		{.from=6,.to=4},
		{.from=6,.to=8},
		{.from=7,.to=9},
		{.from=8,.to=6},
		{.from=8,.to=9},
		{.from=8,.to=12},
		{.from=9,.to=2},
		{.from=9,.to=4},
		{.from=9,.to=7},
		{.from=9,.to=8},
		{.from=9,.to=10},
		{.from=9,.to=13},
		{.from=9,.to=14},
		{.from=10,.to=9},
		{.from=10,.to=11},
		{.from=10,.to=12},
		{.from=11,.to=4},
		{.from=11,.to=10},
		{.from=12,.to=8},
		{.from=12,.to=10},
		{.from=13,.to=9},
		{.from=14,.to=9}
	}
},
{
	.testid=4,
	.input_rectangles = {
		{.m_left=108-RECT_BORDER,.m_right=234+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=454-RECT_BORDER,.m_right=594+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=66-RECT_BORDER,.m_right=171+RECT_BORDER,.m_top=490-RECT_BORDER,.m_bottom=674+RECT_BORDER},
		{.m_left=274-RECT_BORDER,.m_right=414+RECT_BORDER,.m_top=10-RECT_BORDER,.m_bottom=210+RECT_BORDER},
		{.m_left=211-RECT_BORDER,.m_right=414+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=450+RECT_BORDER},
		{.m_left=648-RECT_BORDER,.m_right=803+RECT_BORDER,.m_top=282-RECT_BORDER,.m_bottom=450+RECT_BORDER},
		{.m_left=211-RECT_BORDER,.m_right=344+RECT_BORDER,.m_top=730-RECT_BORDER,.m_bottom=850+RECT_BORDER},
		{.m_left=211-RECT_BORDER,.m_right=456+RECT_BORDER,.m_top=490-RECT_BORDER,.m_bottom=690+RECT_BORDER},
		{.m_left=31-RECT_BORDER,.m_right=171+RECT_BORDER,.m_top=714-RECT_BORDER,.m_bottom=914+RECT_BORDER},
		{.m_left=454-RECT_BORDER,.m_right=608+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=450+RECT_BORDER},
		{.m_left=634-RECT_BORDER,.m_right=760+RECT_BORDER,.m_top=730-RECT_BORDER,.m_bottom=930+RECT_BORDER},
		{.m_left=746-RECT_BORDER,.m_right=929+RECT_BORDER,.m_top=490-RECT_BORDER,.m_bottom=658+RECT_BORDER},
		{.m_left=10-RECT_BORDER,.m_right=171+RECT_BORDER,.m_top=250-RECT_BORDER,.m_bottom=450+RECT_BORDER},
		{.m_left=496-RECT_BORDER,.m_right=706+RECT_BORDER,.m_top=490-RECT_BORDER,.m_bottom=690+RECT_BORDER},
		{.m_left=454-RECT_BORDER,.m_right=594+RECT_BORDER,.m_top=730-RECT_BORDER,.m_bottom=930+RECT_BORDER}
	},
	.logical_edges = {
		{.from=0,.to=4},
		{.from=1,.to=4},
		{.from=1,.to=9},
		{.from=2,.to=4},
		{.from=2,.to=12},
		{.from=3,.to=4},
		{.from=3,.to=9},
		{.from=4,.to=0},
		{.from=4,.to=1},
		{.from=4,.to=2},
		{.from=4,.to=3},
		{.from=4,.to=7},
		{.from=5,.to=13},
		{.from=6,.to=7},
		{.from=6,.to=8},
		{.from=7,.to=4},
		{.from=7,.to=6},
		{.from=7,.to=13},
		{.from=8,.to=6},
		{.from=9,.to=1},
		{.from=9,.to=3},
		{.from=9,.to=13},
		{.from=10,.to=13},
		{.from=11,.to=13},
		{.from=12,.to=2},
		{.from=13,.to=5},
		{.from=13,.to=7},
		{.from=13,.to=9},
		{.from=13,.to=10},
		{.from=13,.to=11},
		{.from=13,.to=14},
		{.from=14,.to=13}
	}
}};


struct TranslationRangesTestContext
{
	int testid;
	vector<DecisionTreeNode> decision_tree;
	vector<TranslationRangeItem> expected_translation_ranges;
};


const vector<TranslationRangesTestContext> TRTestContexts={
{
	.testid=0,
	.decision_tree={
		{.i=822, .parent_index=-1, .depth=0, .recmap={.i_emplacement_source=0, .i_emplacement_destination=41}, .match=24}
	},
	.expected_translation_ranges={}
},
{
	.testid=1,
	.decision_tree={
		{.i=3492, .parent_index=-1, .depth=0, .recmap={.i_emplacement_source=5, .i_emplacement_destination=33}, .match=24}
	},
	.expected_translation_ranges={}
},
{
	.testid=2,
	.decision_tree={
		{.i=0, .parent_index=-1, .depth=0, .recmap={.i_emplacement_source=0, .i_emplacement_destination=35}, .match=24},
		{.i=1, .parent_index=0, .depth=1, .recmap={.i_emplacement_source=1, .i_emplacement_destination=33}, .match=24},
		{.i=2, .parent_index=1, .depth=2, .recmap={.i_emplacement_source=2, .i_emplacement_destination=1}, .match=24},
		{.i=3, .parent_index=2, .depth=3, .recmap={.i_emplacement_source=6, .i_emplacement_destination=2}, .match=24},
		{.i=4, .parent_index=3, .depth=4, .recmap={.i_emplacement_source=12, .i_emplacement_destination=6}, .match=26},
		{.i=5, .parent_index=4, .depth=5, .recmap={.i_emplacement_source=13, .i_emplacement_destination=0}, .match=24},
		{.i=6, .parent_index=5, .depth=6, .recmap={.i_emplacement_source=14, .i_emplacement_destination=22}, .match=26},
		{.i=7, .parent_index=5, .depth=6, .recmap={.i_emplacement_source=14, .i_emplacement_destination=26}, .match=26},
		{.i=8, .parent_index=5, .depth=6, .recmap={.i_emplacement_source=14, .i_emplacement_destination=27}, .match=26},
		{.i=9, .parent_index=5, .depth=6, .recmap={.i_emplacement_source=14, .i_emplacement_destination=32}, .match=26},
	},
	.expected_translation_ranges={
                {.id=1, .ri=1, .tr={.x=0, .y=111}},
                {.id=1, .ri=8, .tr={.x=0, .y=46}},
                {.id=1, .ri=14, .tr={.x=0, .y=111}},
                {.id=2, .ri=0, .tr={.x=0, .y=-112}},
                {.id=2, .ri=1, .tr={.x=0, .y=-1}},
                {.id=2, .ri=2, .tr={.x=-56, .y=222}},
                {.id=2, .ri=6, .tr={.x=0, .y=-112}},
                {.id=2, .ri=8, .tr={.x=0, .y=46}},
                {.id=2, .ri=14, .tr={.x=0, .y=111}},
                {.id=3, .ri=0, .tr={.x=0, .y=-272}},
                {.id=3, .ri=1, .tr={.x=0, .y=-161}},
                {.id=3, .ri=2, .tr={.x=-56, .y=222}},
                {.id=3, .ri=6, .tr={.x=89, .y=126}},
                {.id=3, .ri=8, .tr={.x=0, .y=46}},
                {.id=3, .ri=14, .tr={.x=0, .y=111}},
                {.id=4, .ri=0, .tr={.x=0, .y=-272}},
                {.id=4, .ri=1, .tr={.x=0, .y=-161}},
                {.id=4, .ri=2, .tr={.x=-56, .y=222}},
                {.id=4, .ri=6, .tr={.x=89, .y=126}},
                {.id=4, .ri=8, .tr={.x=0, .y=46}},
                {.id=4, .ri=12, .tr={.x=89, .y=127}},
                {.id=4, .ri=13, .tr={.x=-180, .y=0}},
                {.id=4, .ri=14, .tr={.x=0, .y=111}},
                {.id=5, .ri=0, .tr={.x=0, .y=-272}},
                {.id=5, .ri=1, .tr={.x=0, .y=-161}},
                {.id=5, .ri=2, .tr={.x=-56, .y=222}},
                {.id=5, .ri=6, .tr={.x=89, .y=126}},
                {.id=5, .ri=8, .tr={.x=0, .y=46}},
                {.id=5, .ri=12, .tr={.x=89, .y=127}},
                {.id=5, .ri=13, .tr={.x=123, .y=-543}},
                {.id=5, .ri=14, .tr={.x=0, .y=111}},
                {.id=6, .ri=0, .tr={.x=0, .y=-272}},
                {.id=6, .ri=1, .tr={.x=0, .y=-161}},
                {.id=6, .ri=2, .tr={.x=-56, .y=222}},
                {.id=6, .ri=6, .tr={.x=89, .y=126}},
                {.id=6, .ri=8, .tr={.x=0, .y=46}},
                {.id=6, .ri=12, .tr={.x=89, .y=127}},
                {.id=6, .ri=13, .tr={.x=123, .y=-543}},
                {.id=6, .ri=14, .tr={.x=-62, .y=-879}},
                {.id=7, .ri=0, .tr={.x=0, .y=-272}},
                {.id=7, .ri=1, .tr={.x=0, .y=-161}},
                {.id=7, .ri=2, .tr={.x=-56, .y=222}},
                {.id=7, .ri=6, .tr={.x=89, .y=126}},
                {.id=7, .ri=8, .tr={.x=0, .y=46}},
                {.id=7, .ri=12, .tr={.x=89, .y=127}},
                {.id=7, .ri=13, .tr={.x=123, .y=-543}},
                {.id=7, .ri=14, .tr={.x=-42, .y=-879}},
                {.id=8, .ri=0, .tr={.x=0, .y=-272}},
                {.id=8, .ri=1, .tr={.x=0, .y=-161}},
                {.id=8, .ri=2, .tr={.x=-56, .y=222}},
                {.id=8, .ri=6, .tr={.x=89, .y=126}},
                {.id=8, .ri=8, .tr={.x=0, .y=46}},
                {.id=8, .ri=12, .tr={.x=89, .y=127}},
                {.id=8, .ri=13, .tr={.x=123, .y=-543}},
                {.id=8, .ri=14, .tr={.x=-42, .y=-879}},
                {.id=9, .ri=0, .tr={.x=0, .y=-272}},
                {.id=9, .ri=1, .tr={.x=0, .y=-161}},
                {.id=9, .ri=2, .tr={.x=-56, .y=222}},
                {.id=9, .ri=6, .tr={.x=89, .y=126}},
                {.id=9, .ri=8, .tr={.x=0, .y=46}},
                {.id=9, .ri=12, .tr={.x=89, .y=127}},
                {.id=9, .ri=13, .tr={.x=123, .y=-543}},
                {.id=9, .ri=14, .tr={.x=182, .y=-879}}
	}
}
};


enum TrimAlgo
{
	SPLIT,
	NOTCH,
	TRIM,
	CORNER
};


const unsigned NR_TRIM_ALGO=4;

const char* TrimAlgoStrings[NR_TRIM_ALGO]={
	"SPLIT",
	"NOTCH",
	"TRIM",
	"CORNER"
};


enum TrimMirrorDirection
{
	HORIZONTAL_MIRROR,
	VERTICAL_MIRROR,
	TILTED_MIRROR
};

enum MirroringState
{
	ACTIVE,
	IDLE
};

struct TrimMirror{
	MirroringState mirroring_state;
	TrimMirrorDirection mirroring_direction;
};

const unsigned NR_TRIM_MIRRORING_OPTIONS = 2*2*2;

const TrimMirror trim_mirrors[NR_TRIM_MIRRORING_OPTIONS][3]={
	{
		{.mirroring_state=IDLE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=VERTICAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=TILTED_MIRROR}
	},
	{
		{.mirroring_state=IDLE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=ACTIVE, .mirroring_direction=VERTICAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=TILTED_MIRROR}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=VERTICAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=TILTED_MIRROR}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=ACTIVE, .mirroring_direction=VERTICAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=TILTED_MIRROR}
	},
	{
		{.mirroring_state=IDLE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=VERTICAL_MIRROR},
		{.mirroring_state=ACTIVE, .mirroring_direction=TILTED_MIRROR}
	},
	{
		{.mirroring_state=IDLE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=ACTIVE, .mirroring_direction=VERTICAL_MIRROR},
		{.mirroring_state=ACTIVE, .mirroring_direction=TILTED_MIRROR}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=VERTICAL_MIRROR},
		{.mirroring_state=ACTIVE, .mirroring_direction=TILTED_MIRROR}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=ACTIVE, .mirroring_direction=VERTICAL_MIRROR},
		{.mirroring_state=ACTIVE, .mirroring_direction=TILTED_MIRROR}
	}
};

const char* TrimMirroringStrings[NR_TRIM_MIRRORING_OPTIONS]={
	"HORIZONTAL_MIRROR:IDLE, VERTICAL_MIRROR:IDLE, TILTED_MIRROR:IDLE",
	"HORIZONTAL_MIRROR:IDLE, VERTICAL_MIRROR:ACTIVE, TILTED_MIRROR:IDLE",
	"HORIZONTAL_MIRROR:ACTIVE, VERTICAL_MIRROR:IDLE, TILTED_MIRROR:IDLE",
	"HORIZONTAL_MIRROR:ACTIVE, VERTICAL_MIRROR:ACTIVE, TILTED_MIRROR:IDLE",
	"HORIZONTAL_MIRROR:IDLE, VERTICAL_MIRROR:IDLE, TILTED_MIRROR:ACTIVE",
	"HORIZONTAL_MIRROR:IDLE, VERTICAL_MIRROR:ACTIVE, TILTED_MIRROR:ACTIVE",
	"HORIZONTAL_MIRROR:ACTIVE, VERTICAL_MIRROR:IDLE, TILTED_MIRROR:ACTIVE",
	"HORIZONTAL_MIRROR:ACTIVE, VERTICAL_MIRROR:ACTIVE, TILTED_MIRROR:ACTIVE"
};


/*      by
      +------+
      |      |
+=====+======+=====+
|     |      |     |
|     |      |     | r
|     |      |     |
+=====+======+=====+
      |      |
      +------+
*/

void split(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (r.m_left < by.m_left && by.m_right < r.m_right && by.m_top < r.m_top && by.m_bottom > r.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=by.m_left, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom}
		};
	}
}

/*
+==================+
|                  |
|     +------+     | r
|     |      |     |
+=====+======+=====+
      |      |
      +------+
         by
*/

void notch(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (r.m_left < by.m_left && by.m_right < r.m_right && by.m_bottom > r.m_bottom && by.m_top < r.m_bottom && by.m_top > r.m_top)
	{
		rects = {
			{.m_left=r.m_left, .m_right=by.m_left, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=by.m_top}
		};
	}
}

/*
      +==================+
      |                  |
      |                  | r
+-----+------------------+----+
|     +==================+    |
|                             |
+-----------------------------+
         by
*/
void trim(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (by.m_left < r.m_left && by.m_right > r.m_right && r.m_top < by.m_top && r.m_bottom > by.m_top && r.m_bottom < by.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=by.m_top}
		};
	}
}

/*
      +==================+
      |                  |
      |                  | r
+-----+-----+            |
|     +=====+============+
|           |
+-----------+
         by
*/
void corner(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (by.m_left < r.m_left && by.m_right > r.m_left && by.m_right < r.m_right &&
		by.m_bottom > r.m_bottom && by.m_top > r.m_top && by.m_top < r.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=by.m_top},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom}
		};
	}
}


void apply_trim_algo(TrimAlgo trim_algo, const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	switch (trim_algo)
	{
	case SPLIT:
		split(r, by, rects);
		break;
	case NOTCH:
		notch(r, by, rects);
		break;
	case TRIM:
		trim(r, by, rects);
		break;
	case CORNER:
		corner(r, by, rects);
		break;
	}
}

void apply_trim_mirror(const TrimMirror& mirror, MyRect& r)
{
	const auto& [mirroring_state, mirroring_direction] = mirror;

	if (mirroring_state == ACTIVE)
	{
		switch(mirroring_direction)
		{
		case VERTICAL_MIRROR:
			r.m_left *= -1;
			r.m_right *= -1;
			swap(r.m_left, r.m_right);
			break;
		case HORIZONTAL_MIRROR:
			r.m_top *= -1;
			r.m_bottom *= -1;
			swap(r.m_top, r.m_bottom);
			break;
		case TILTED_MIRROR:
			swap(r.m_left, r.m_top);
			swap(r.m_right, r.m_bottom);
			break;
		}
	}
}


struct TrimProcessSelector
{
	unsigned trim_algo, mirroring;
};


// TODO: use upcoming C++23 views::cartesian_product()
vector<TrimProcessSelector> cartesian_product_()
{
	vector<TrimProcessSelector> result;

	for (int trim_algo=0; trim_algo < NR_TRIM_ALGO; trim_algo++)
		for (int mirroring=0; mirroring < NR_TRIM_MIRRORING_OPTIONS; mirroring++)
				result.push_back({trim_algo, mirroring});

	return result;
}

const vector<TrimProcessSelector> trim_process_selectors = cartesian_product_();


vector<MyRect> trimmed(MyRect r, MyRect by)
{
	vector<MyRect> rects;

	for (const auto [trim_algo, mirroring] : trim_process_selectors)
	{
		const TrimMirror (&trim_mirror_process)[3] = trim_mirrors[mirroring];

		auto rg = views::counted(trim_mirror_process,3);

		ranges::for_each(rg, [&](const TrimMirror mirror){
			apply_trim_mirror(mirror, r);
			apply_trim_mirror(mirror, by);
			}
		);

		apply_trim_algo((TrimAlgo)trim_algo, r, by, rects);

		ranges::for_each(rg | views::reverse, [&](const TrimMirror mirror){
			apply_trim_mirror(mirror, r);
			apply_trim_mirror(mirror, by);
			ranges::for_each(rects, [&](MyRect& rec){apply_trim_mirror(mirror,rec);});
			}
		);

		if (!rects.empty())
		{
			D(printf("trim_algo=%u, mirroring=%u\n", trim_algo, mirroring));
			D(printf("TrimAlgoString = %s\n", TrimAlgoStrings[trim_algo]));
			D(printf("TrimMirroringString = %s\n", TrimMirroringStrings[mirroring]));
			auto rg = rects | views::transform([](const MyRect& r)->string{
								char buffer[80];
								sprintf(buffer,"{.m_left=%d, .m_right=%d, .top=%d, .m_bottom=%d}\n",r.m_left,r.m_right,r.m_top,r.m_bottom);
								return buffer;})
					| views::join;
			for (char c : rg)
				D(printf("%c", c));

			return rects;
		}
	}
	return {};
}

/*
TODO: use C++23 deducing this
*/
MyRect trimmed(const MyRect& r, const vector<MyRect> rectangles)
{
	struct RectNode{int id; MyRect r; int parent_id;};
	vector<RectNode> rect_tree;

	auto rec_trim=[&](int parent_id, int i, auto&& rec_trim)->void
	{
		const vector<MyRect> rects = trimmed(rect_tree[parent_id].r, rectangles[i]);
		for (const MyRect& rec : rects)
		{
			int size = rect_tree.size();
			rect_tree.push_back({.id=size, .r=rec, .parent_id=parent_id});
			if (i+1 < rectangles.size())
				rec_trim(size, i+1, rec_trim);
		}

		if (rects.empty())
		{
                        if (i+1 < rectangles.size())
                                rec_trim(parent_id, i+1, rec_trim);
		}
	};

	rect_tree = {{.id=0, .r=r, .parent_id=-1}};

	int parent_id=0;
	int i=0;
	rec_trim(parent_id, i, rec_trim);

/*
TODO : use views::set_difference
auto v1 = std::vector<int> {3,4,5,6,7}; // sort!
auto v2 = std::vector<int> {4,5}; // sort!
ranges::sort(v1); // sort!
ranges::sort(v2); // sort!
auto rng = ranges::views::set_difference(v1,v2); // [3,6,7]
*/

	D(printf("building index:\n"));
	vector<RectNode> index = rect_tree;
	ranges::sort(index, {}, &RectNode::parent_id);
	vector<int> leaves;
	ranges::set_difference(rect_tree | views::transform(&RectNode::id),
				index | views::transform(&RectNode::parent_id),
				std::back_inserter(leaves));

	D(printf("leaves: "));
	for (int id : leaves)
		D(printf("%d, ", id));
	D(printf("\n"));

	return ranges::max(leaves | views::transform([&](int id){return rect_tree[id].r;}),
			{},
			[](const MyRect& r){return width(r)*height(r);}
		);
}


struct RectTrimTestContext{
	int testid;
	MyRect r;
	vector<MyRect> input_rectangles;
	MyRect expected;
};


const vector<RectTrimTestContext> rect_trim_test_contexts={
/*
             80  120   180  220         300
              | | |     |    |           |
               100
-80			+----+
			| 1  |
-100		+=======+====+===========+
		|       |    |           |
-120		|       +----+           | r
-180	      +-+-+                      |
-200	    0 |	+=|======================+
-220	      +---+
*/
	{
		.testid=0,
		.r={.m_left=100, .m_right=300, .m_top=100, .m_bottom=200},
		.input_rectangles={
			{.m_left=80, .m_right=120, .m_top=180, .m_bottom=220},//0
			{.m_left=180, .m_right=220, .m_top=80, .m_bottom=120} //1
		},
		.expected={.m_left=120, .m_right=300, .m_top=120, .m_bottom=200}
	},
/*
           50  100 150               350 400 450
            |   |   |                 |   |   |

-50	    +-------+                 +-------+
	    |   2   |                 |   3   |
-100	    |	+===+=================+===+   |
	    |	|   |                 |   |   |
-150	    +---+---+                 +---+---+
		|           r             |
-200	    +---+---+                 +---+---+
	    |	|   |                 |   |   |
-250	    |	+===+=================+===+   |
            |   0   |                 |   1   |
-300        +-------+                 +-------+
*/
	{
		.testid=1,
		.r={.m_left=100, .m_right=400, .m_top=100, .m_bottom=250},
                .input_rectangles={
                        {.m_left=50, .m_right=150, .m_top=200, .m_bottom=300},//0
                        {.m_left=350, .m_right=450, .m_top=200, .m_bottom=300},//1
                        {.m_left=50, .m_right=150, .m_top=50, .m_bottom=150},//2
                        {.m_left=350, .m_right=450, .m_top=50, .m_bottom=150} //3
                },
                .expected={.m_left=150, .m_right=350, .m_top=100, .m_bottom=250}
	},
/*
               100                    350 400 450
		|                      |   |   |

-100		+==========================+
		|                          |
-150		|                      +---+---+
		|             r        |   | 0 |
-200		|                      +---+---+
		|                          |
-250		+==========================+
*/
        {
                .testid=2,
                .r={.m_left=100, .m_right=400, .m_top=100, .m_bottom=250},
                .input_rectangles={
                        {.m_left=350, .m_right=450, .m_top=150, .m_bottom=200}//0
                },
                .expected={.m_left=100, .m_right=350, .m_top=100, .m_bottom=250}
        },
/*
           50  100                        400 450
            |   |                          |   |
                              r
-100            +==========================+
                |                          |
-150        +---+--------------------------+---+
            |   |             0            |   |
-200        +---+------------------------- +---+
                |                          |
		|			   |
		|			   |
-300            +==========================+
*/
        {
                .testid=3,
                .r={.m_left=100, .m_right=400, .m_top=100, .m_bottom=300},
                .input_rectangles={
                        {.m_left=50, .m_right=450, .m_top=150, .m_bottom=200}//0
                },
                .expected={.m_left=100, .m_right=400, .m_top=200, .m_bottom=300}
        },
/*
           50  100 150                    400 450
            |   |   |                      |   |

-50         +-------+
            |       |
-100        |   +===+======================+
            |   |   |                      |
-150        |   |   |                      |
            |   |   |         r            |
-200        |   |   |                      |
            |   |   |                      |
-250        |   +===+======================+
            |   0   |
-300        +-------+
*/
        {
                .testid=4,
                .r={.m_left=100, .m_right=400, .m_top=100, .m_bottom=250},
                .input_rectangles={
                        {.m_left=50, .m_right=150, .m_top=50, .m_bottom=300}//0
                },
                .expected={.m_left=150, .m_right=400, .m_top=100, .m_bottom=250}
        }
};


void test_rect_trim()
{
	for (const auto [testid, r, input_rectangles, expected] : rect_trim_test_contexts)
	{
		MyRect result = trimmed(r, input_rectangles);
		D(printf("result={.m_left=%d, .m_right=%d, .m_top=%d, .m_bottom=%d}\n", result.m_left, result.m_right, result.m_top, result.m_bottom));
		bool bOK = result == expected;
		printf("rect trim testid=%d : %s\n", testid, bOK ? "OK" : "KO");
	}
}


vector<RectLink> sweep(Direction update_direction, const vector<MyRect>& rectangles)
{
	FunctionTimer ft("sweep");

	const int N=30;
	int n = rectangles.size();

	const MyPoint translation2[2]={{.x=1, .y=0}, {.x=0, .y=1}};

	SweepLineItem sweep_line_buffer[2*N];
	span sweep_line(sweep_line_buffer, 2*n);

	ActiveLineItem active_line[N];
	RectLink rect_links_buffer[256];

//use the sweep_line that is not impacted by selected translation
	Direction sweep_direction = Direction(1-update_direction);

	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
	auto [minSweepRectDim, maxSweepRectDim] = rectDimRanges[sweep_direction];
{
        FunctionTimer ft("cft_fill_sweepline");
		//sweep_line.reserve(2*n);

	for (int ri=0; ri < n; ri++)
	{
		sweep_line_buffer[2*ri]={.sweep_value=rectangles[ri][minSweepRectDim], .rectdim=minSweepRectDim, .ri=ri};
		sweep_line_buffer[2*ri+1]={.sweep_value=rectangles[ri][maxSweepRectDim], .rectdim=maxSweepRectDim, .ri=ri};
	}
}

	const MyPoint& translation = translation2[update_direction] ;
{
        FunctionTimer ft("cft_sort_sweepline");
	ranges::sort(sweep_line, CustomLess());
}
	int active_line_size=0;
	int rect_links_size=0;

	auto cmp=[&](int i, int j){
		return rectangles[i][minCompactRectDim]<rectangles[j][minCompactRectDim];
	};

	auto erase=[&](SweepLineItem& sweep_line_item){

		auto& [sweep_value, rectdim, i] = sweep_line_item;

		ActiveLineItem& lower = *ranges::lower_bound(active_line,active_line+active_line_size, i, cmp, &ActiveLineItem::i);
		D(printf("lower = %d\n", lower.i));
		int pos = distance(active_line, &lower);
		D(printf("pos = %d\n", pos));

		for (RectLink* rl : active_line[pos].links)
		{
			if (rl != 0)
				rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
		}

		for (int ii=pos; ii<active_line_size; ii++)
			swap(active_line[ii], active_line[ii+1]);
		active_line_size -= 1;

		if (pos > 0 && pos < active_line_size)
		{
			rect_links_buffer[rect_links_size++] = {
				.i=active_line[pos-1].i,
				.j=active_line[pos].i,
				.min_sweep_value=sweep_value
			};

			if (RectLink *rl=active_line[pos-1].links[1]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			if (RectLink *rl=active_line[pos].links[0]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			active_line[pos-1].links[1] = active_line[pos].links[0] = & rect_links_buffer[rect_links_size - 1];
		}
	};

	auto insert=[&](SweepLineItem& sweep_line_item){

		auto& [sweep_value, rectdim, i] = sweep_line_item;

		ActiveLineItem& upper = *ranges::upper_bound(active_line,active_line+active_line_size, i, cmp, &ActiveLineItem::i);
		D(printf("upper = %d\n", upper.i));
		int pos = distance(active_line, &upper);
		D(printf("pos = %d\n", pos));

		for (int ii=active_line_size-1; ii>=pos; ii--)
			swap(active_line[ii],active_line[ii+1]);
		active_line_size += 1;
		active_line[pos].i=i;

		if (pos > 0)
		{
			rect_links_buffer[rect_links_size++] = {
				.i=active_line[pos-1].i,
				.j=active_line[pos].i,
				.min_sweep_value=sweep_value
			};

			if (RectLink *rl=active_line[pos].links[0]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			if (RectLink *rl=active_line[pos-1].links[1]; rl!=0)
				rl->max_sweep_value = min(sweep_value,rl->max_sweep_value);
			active_line[pos].links[0] = active_line[pos-1].links[1] = & rect_links_buffer[rect_links_size - 1];
		}
		if (pos+1 < active_line_size)
		{
			rect_links_buffer[rect_links_size++] = {
				.i=active_line[pos].i,
				.j=active_line[pos+1].i,
				.min_sweep_value=sweep_value
			};

			if (RectLink *rl=active_line[pos].links[1]; rl!=0)
				rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
			if (RectLink *rl=active_line[pos+1].links[0]; rl!=0)
				rl->max_sweep_value = min(sweep_value, rl->max_sweep_value);
			active_line[pos].links[1] = active_line[pos+1].links[0] = &rect_links_buffer[rect_links_size - 1];
		}
	};

	auto print_active_line=[&](){
		char buffer[5000];
		int pos=0;
		pos += sprintf(buffer + pos, ".active_line={");
		for (auto& [i, links] : span(active_line, active_line_size))
		{
			pos += sprintf(buffer + pos, " %d,", i);
		}
		pos += sprintf(buffer + --pos, "}\n");
		buffer[pos]=0;
		printf("%s", buffer);
	};

{
        FunctionTimer ft("cft_sweep");
	for (SweepLineItem& item : sweep_line)
	{
		const auto& [sweep_value, rectdim, ri] = item;
		switch(rectdim)
		{
		case LEFT:
		case TOP:
			D(printf("sweep reaching %d %s\n", ri, RectDimString[rectdim]));
			D(printf("before insert\n"));
//			print_active_line();
			insert(item);
			D(printf("after insert\n"));
			D(print_active_line());
			break;
		case RIGHT:
		case BOTTOM:
			D(printf("sweep leaving %d %s\n", ri, RectDimString[rectdim]));
			D(printf("before erase\n"));
//			print_active_line();
			erase(item);
			D(printf("after erase\n"));
			D(print_active_line());
			break;
		}
	}

	for (const auto [sweep_value, rectdim, ri] : sweep_line)
	{
		D(printf("{.sweep_value=%d, .rectdim=%s, .ri=%d},\n", sweep_value, RectDimString[rectdim], ri));
	}
}

{
        FunctionTimer ft("cft_rectlinks_sort");
	sort(rect_links_buffer, rect_links_buffer + rect_links_size);
}

        auto rg = views::counted(rect_links_buffer, rect_links_size) |
                views::filter([](const RectLink& rl){return rl.min_sweep_value != rl.max_sweep_value;});
#ifdef _TRACE_
	D(printf("rect_links:\n"));
	for (const auto& [i, j, min_sweep_value, max_sweep_value] : rg)
	{
		D(printf("{.i=%d, .j=%d, .%s=%d, .%s=%d},\n", i, j,
			RectDimString[minSweepRectDim], min_sweep_value, RectDimString[maxSweepRectDim], max_sweep_value));
	}
#endif

	// return rg | views::to<vector>; TODO C++23

	return vector(ranges::begin(rg), ranges::end(rg));
}

struct HoleOrigin
{
	RectCorner corner;
	MyVector dir;
        friend bool operator==(const HoleOrigin&, const HoleOrigin&) = default;
};


const HoleOrigin hole_origins[12]={
	{.corner=TOP_LEFT, .dir={.x=-1, .y=-1}},
	{.corner=TOP_LEFT, .dir={.x=+1, .y=-1}},
	{.corner=TOP_LEFT, .dir={.x=-1, .y=+1}},
	{.corner=BOTTOM_LEFT, .dir={.x=-1, .y=+1}},
	{.corner=BOTTOM_LEFT, .dir={.x=+1, .y=+1}},
	{.corner=BOTTOM_LEFT, .dir={.x=-1, .y=-1}},
	{.corner=TOP_RIGHT, .dir={.x=+1, .y=+1}},
	{.corner=TOP_RIGHT, .dir={.x=+1, .y=-1}},
	{.corner=TOP_RIGHT, .dir={.x=-1, .y=-1}},
	{.corner=BOTTOM_RIGHT, .dir={.x=-1, .y=+1}},
	{.corner=BOTTOM_RIGHT, .dir={.x=+1, .y=+1}},
	{.corner=BOTTOM_RIGHT, .dir={.x=+1, .y=-1}}
};

const HoleOrigin matched_origins[8][2]={
	{{.corner=TOP_LEFT, .dir={.x=+1, .y=-1}}, {.corner=TOP_RIGHT, .dir={.x=-1, .y=-1}}},
	{{.corner=TOP_RIGHT, .dir={.x=+1, .y=+1}}, {.corner=BOTTOM_RIGHT, .dir={.x=+1, .y=-1}}},
	{{.corner=TOP_LEFT, .dir={.x=-1, .y=+1}}, {.corner=BOTTOM_LEFT, .dir={.x=-1, .y=-1}}},
	{{.corner=BOTTOM_LEFT, .dir={.x=+1, .y=+1}}, {.corner=BOTTOM_RIGHT, .dir={.x=-1, .y=+1}}}
};

struct HoleMatch{
	int i, j;
        friend auto operator<=>(const HoleMatch&, const HoleMatch&) = default;
};


vector<MyRect> compute_holes(const vector<MyRect>& input_rectangles)
{
	const MyRect frame={
		.m_left=ranges::min(input_rectangles | views::transform(&MyRect::m_left)),
		.m_right=ranges::max(input_rectangles | views::transform(&MyRect::m_right)),
		.m_top=ranges::min(input_rectangles | views::transform(&MyRect::m_top)),
		.m_bottom=ranges::max(input_rectangles | views::transform(&MyRect::m_bottom))
	};

	const MyRect borders[4] = {
		{.m_left=frame.m_left-10, .m_right=frame.m_left, .m_top=frame.m_top, .m_bottom=frame.m_bottom},
		{.m_left=frame.m_right, .m_right=frame.m_right+10, .m_top=frame.m_top, .m_bottom=frame.m_bottom},
		{.m_left=frame.m_left, .m_right=frame.m_right, .m_top=frame.m_top-10, .m_bottom=frame.m_top},
		{.m_left=frame.m_left, .m_right=frame.m_right, .m_top=frame.m_bottom, .m_bottom=frame.m_bottom+10}
	};

	auto next=[&](const vector<MyRect>& in_holes){

		vector<MyRect> rectangles;
		ranges::copy(input_rectangles, back_inserter(rectangles));
		ranges::copy(in_holes, back_inserter(rectangles));
		ranges::copy(borders, back_inserter(rectangles));

		struct SweepContext{Direction update_direction, sweep_direction;};
		const SweepContext ctx2[2]={
			{.update_direction=EAST_WEST, .sweep_direction=NORTH_SOUTH},
			{.update_direction=NORTH_SOUTH, .sweep_direction=EAST_WEST}
		};
		vector<MyRect> holes;
		int n2[2];
		for (const auto [update_direction, sweep_direction] : ctx2)
		{
			vector<RectLink> rect_links = sweep(update_direction, rectangles);
			auto rg = rect_links |
				views::transform([&](const RectLink& lnk)->MyRect{
					const auto [i, j, min_sweep_value, max_sweep_value] = lnk;
					const MyRect &ri=input_rectangles[i], &rj=rectangles[j];
					switch(update_direction)
					{
					case EAST_WEST:
						return {.m_left=ri.m_right, .m_right=rj.m_left, .m_top=min_sweep_value, .m_bottom=max_sweep_value};
					case NORTH_SOUTH:
						return {.m_left=min_sweep_value, .m_right=max_sweep_value, .m_top=ri.m_bottom, .m_bottom=rj.m_top};
					}
				}) | views::filter([](const MyRect& r){
					return r.m_left != r.m_right && r.m_top != r.m_bottom;
				}) | views::filter([](const MyRect& r){
					return 5*min<int>(width(r), height(r)) >= RECTANGLE_BOTTOM_CAP;
				});

			ranges::copy(rg, back_inserter(holes));
			n2[sweep_direction] = holes.size();
		}

		auto [m, n] = ranges::minmax(n2);

		vector<HoleMatch> intersections;

		for (int i=0; i < m; i++)
		{
			for (int j=m; j < n; j++)
			{
				if (intersect_strict(holes[i], holes[j]))
				{
					intersections.push_back({i,j});
					intersections.push_back({j,i});
				}
			}
		}

		ranges::sort(intersections);

		auto dim_spread = [](const MyRect& r)->float{
			const float dim[2] = {width(r), height(r)};
			auto [min,Max] = ranges::minmax(dim);
			return (Max - min) / (Max + min);
		};

		auto next=[&](const vector<int>& suppressed)->int{
                	auto rg = intersections | views::filter([&](const HoleMatch& match){
                       		auto [i,j]=match;
                        	return suppressed[i]==0 && suppressed[j]==0;
                	});

                	if (ranges::empty(rg))
                        	return -1;
                	int i = ranges::max(rg | views::transform([](const HoleMatch& match){auto [i,j]=match; return array<int,2>{i,j};}) |
                        	                views::join, {}, [&](int i){
                                	auto rng = ranges::equal_range(intersections, i, {}, &HoleMatch::i) |
                                                views::transform(&HoleMatch::j) |
                                                views::filter([&](int j){return suppressed[j]==0;}) |
                                                views::transform([&](int j){return dim_spread(holes[j]);}) ;
                        		return dim_spread(holes[i]) - ranges::max(rng);
                        	});
			return i;
		};

		vector<int> suppressed(holes.size(), 0);

		vector<vector<int> > vv(30);
		vv[0] = suppressed;
		partial_sum(vv.begin(), vv.end(), vv.begin(),
			[&](const vector<int>& prev, const vector<int>&)->vector<int>{
				if (prev.empty()) return {};
				int i = next(prev);
				if (i==-1)return {};
				vector<int> next = prev;
				next[i]=1;
				return next;}
			);

		suppressed = *(ranges::find(vv, vector<int>())-1);

		auto rg = views::iota(0,n) | views::filter([&](int i){return suppressed[i]==0;})
				| views::transform([&](int i){return holes[i];});
		return vector<MyRect>(ranges::begin(rg), ranges::end(rg));
	};

	vector<vector<MyRect> > vv(2);
	partial_sum(vv.begin(), vv.end(), vv.begin(),
		[&](const vector<MyRect>& prev, const vector<MyRect>&){
			return next(prev);
		});
	auto rg = vv | views::join;
	return vector<MyRect>(ranges::begin(rg), ranges::end(rg));
}


vector<RectHole> compute_holes_(const vector<MyRect>& input_rectangles)
{
	MyRect frame={
		.m_left=ranges::min(input_rectangles | views::transform(&MyRect::m_left)),
		.m_right=ranges::max(input_rectangles | views::transform(&MyRect::m_right)),
		.m_top=ranges::min(input_rectangles | views::transform(&MyRect::m_top)),
		.m_bottom=ranges::max(input_rectangles | views::transform(&MyRect::m_bottom))
	};

	int n = input_rectangles.size();

	vector<RectHole> holes;

//TODO: use C++23 views::cartesian_product()

	for (int i=0; i<n; i++)
	{
		const MyRect& r = input_rectangles[i];

		for (const auto [corner, dir] : hole_origins)
		{
			const MyPoint pt = r[corner] ;

			int intervalle[2]={2, INT16_MAX};
			auto& [m, M] = intervalle;
			while (M > 1+m)
			{
				int value = M==INT16_MAX ? 2*m : (m+M)/2 ;
				MyRect rec = rect(pt, pt + value*dir);
				auto rg = input_rectangles | views::filter([&](const MyRect& r){return intersect_strict(rec,r) /*|| is_inside(r, rec)*/;});
				(rg.empty() && is_inside(rec,frame) ? m : M) = value;
				D(printf("{.i=%d, .RectCorner=%s, .direction={.x=%.0f, .y=%.0f} [.m=%d .M=%d]}\n", i, RectCornerString[corner], dir.x, dir.y, m, M));
			}

			if (m > 2)
			{
 				MyRect rec = rect(pt, pt + m*dir);
				holes.push_back({.ri=i, .corner=corner, .direction=dir, .value=m, .rec=rec});
			}
		}
	}

        D(printf("original holes.size()=%zu.\n", holes.size()));

	vector<RectHole> dd;
        for (const auto& [ri, corner, direction, value, rec] : holes)
        {
                if (5*value >= RECTANGLE_BOTTOM_CAP)
                {
                        dd.push_back({ri, corner, direction, value, rec});
                }
        }
	holes = dd;

        D(printf("holes.size() after removing small ones=%zu.\n", holes.size()));

	assert(ranges::all_of(matched_origins, [](const HoleOrigin (&ho2)[2]){return ho2[0].corner < ho2[1].corner;}));

//TODO use C++23 cartesian_product()

        struct Match{int hi; int hj;};

        vector<Match> cp;
        for (int hi=0; hi < holes.size(); hi++)
                for (int hj=0; hj < holes.size(); hj++)
                        cp.push_back({hi, hj});

        auto rn0 = cp | views::filter([&](const Match& m) {
                                const auto [hi, hj] = m;
				return holes[hi].ri == holes[hj].ri && holes[hi].corner < holes[hj].corner;
			}) |
			views::filter([&](const Match& m){
                                const auto [hi, hj] = m;
                                const HoleOrigin ho2[2] = {
					{.corner=holes[hi].corner, .dir=holes[hi].direction},
					{.corner=holes[hj].corner, .dir=holes[hj].direction}
				};
                                return ranges::any_of(matched_origins,[&](const HoleOrigin (&mo2)[2]){return ranges::equal(ho2, mo2);});
                        }) |
                        views::filter([&](const Match& m){
                                const auto [hi, hj] = m;
                                MyRect ri = holes[hi].rec, rj = holes[hj].rec;
                                MyRect h = enveloppe(ri, rj);
                                return (ri[EAST_WEST]==rj[EAST_WEST] || ri[NORTH_SOUTH]==rj[NORTH_SOUTH]) &&
                                        ranges::none_of(input_rectangles, [&](const MyRect& r){return intersect_strict(r,h);});
                        }) |
			views::transform([&](const Match& m){
                                const auto [hi, hj] = m;
                                MyRect ri = holes[hi].rec, rj = holes[hj].rec;
                                MyRect h = enveloppe(ri, rj);
                                RectHole rh = holes[hi];
                                rh.rec = h;
                                return rh;
			});

        auto rg = holes |
                views::filter([&](const RectHole& rhi){
                        return ranges::none_of(rn0, [&](const RectHole& rhj){return is_inside(rhi.rec, expanded_by(rhj.rec, +1));});
                }) |
		views::filter([&](const RectHole& rhi){
			return ranges::none_of(holes, [&](const RectHole& rhj){return is_inside(rhi.rec, expanded_by(rhj.rec, +1));});
		});


	ranges::sort(holes, {}, &RectHole::rec);
	vector<RectHole> holes_dedup;
	ranges::unique_copy(holes, back_inserter(holes_dedup), {}, &RectHole::rec);

	holes = holes_dedup;

	int nh = holes.size();
	D(printf("holes.size()=%d.\n", nh));

  for (int i=0; i<1; i++)
  {
//TODO: use C++23 views::to<vector>()
	auto rg = views::iota(0,nh) |
		views::filter([&](int hi){
			auto rng = views::iota(0,nh) | views::filter([&](int hj){return hj != hi;});
			return ranges::none_of(rng, [&](int hj){return is_inside(holes[hi].rec, expanded_by(holes[hj].rec, +1));});
		}) |
		views::transform([&](int hi){return holes[hi];});
	holes = vector<RectHole>(ranges::begin(rg), ranges::end(rg));
	nh = holes.size();
        D(printf("holes.size()=%d after removing inside holes.\n", nh));

//TODO use C++23 cartesian_product()

	vector<Match> cp;
	for (int hi=0; hi < nh; hi++)
		for (int hj=hi+1; hj < nh; hj++)
			cp.push_back({hi, hj});

	auto rng = cp | views::filter([&](const Match& m){
				const auto [hi, hj] = m;
				MyRect ri = holes[hi].rec, rj = holes[hj].rec;
				MyRect h = enveloppe(ri, rj);
				return (ri[EAST_WEST]==rj[EAST_WEST] || ri[NORTH_SOUTH]==rj[NORTH_SOUTH]) &&
					ranges::none_of(input_rectangles, [&](const MyRect& r){return intersect_strict(r,h);});
		});

	auto rg3 = rng | views::transform([](const Match& m)->array<int,2>{return {m.hi, m.hj};}) | views::join ;

	auto rngf = rng |
                views::filter([&](const Match& m){
                                const auto [hi, hj] = m;
                                const HoleOrigin ho2[2] = {
                                        {.corner=holes[hi].corner, .dir=holes[hi].direction},
                                        {.corner=holes[hj].corner, .dir=holes[hj].direction}
                                };
                                return (ranges::count(rg3, hi)==1 && ranges::count(rg3, hj)==1) ||
					(holes[hi].ri == holes[hj].ri &&
                                        ranges::any_of(matched_origins, [&](const HoleOrigin (&mo2)[2]){return ranges::equal(ho2, mo2);})
					);
				});

	D(printf(".rngf={\n"));
	for (const auto [hi, hj] : rngf)
		D(printf("{.hi=%d, .hj=%d}\n", hi, hj));
	D(printf("}\n"));

	auto rng1 = rngf |
		views::transform([](const Match& m)->array<int,2>{return {m.hi, m.hj};}) |
		views::join ;

	D(printf(".rng1={"));
	for (int hi : rng1)
		D(printf("%d,", hi));
        D(printf("}\n"));

	auto rng2 = views::iota(0,nh) |
		views::filter([&](int hi){return ranges::count(rng1,hi)!=1;}) |
		views::transform([&](int hi){return holes[hi];});

	auto rng3 = rngf |
		views::transform([&](const Match& m){
				const auto [hi, hj] = m;
				MyRect h = enveloppe(holes[hi].rec, holes[hj].rec);
				RectHole rh = holes[hi];
				rh.rec = h;
				return rh;});

//TODO: use C++23 views::concat() and views::to<vector>
	vector<RectHole> tmp;
	ranges::copy(rng2, back_inserter(tmp));
	ranges::copy(rng3, back_inserter(tmp));
	holes = tmp;

        D(printf("holes.size()=%zu after merging holes.\n", holes.size()));
  }
	return holes;
};


vector<float> compute_page_rank(const int n,
				const vector<LogicalEdge>& logical_edges,
				const vector<TopologicalEdge>& topological_edges)
{
	const int nr_rec = 40;
	vector<vector<float> > m(40, vector<float>(n, 1.0f / n));

	D(printf("\n"));
	for (float& value : m[0])
		D(printf("%.2f\t", value));
	auto topological_edges_ = topological_edges | views::transform([](const TopologicalEdge& e){return LogicalEdge{e.from, e.to};});
	vector<LogicalEdge> inter;
	ranges::set_intersection(logical_edges, topological_edges_, back_inserter(inter));

	const float d = 0.85f;

	auto next=[&](const vector<float>& pr)->vector<float>
	{
		auto rg = views::iota(0,n) |
			views::transform([&](int i)->float{
				auto rg1 = ranges::equal_range(inter, i, {}, &LogicalEdge::from) |
					views::transform([&](const LogicalEdge& e)->float{
						auto rg = ranges::equal_range(logical_edges, e.to, {}, &LogicalEdge::from);
						return pr[e.to] / ranges::size(rg) ;
					});
				return (1.0f - d) + d * accumulate(ranges::begin(rg1), ranges::end(rg1), 0.0f);
			});
		vector<float> result(ranges::begin(rg), ranges::end(rg));
		D(printf("\n"));
		for (float& value : result)
			D(printf("%.2f\t", value));
		return result;
	};

	partial_sum(m.begin(), m.end(), m.begin(),
		[&](const vector<float>& prev, const vector<float>&){
					return next(prev);}
				);
        D(printf("\n"));
	for (int i : views::iota(0,n))
		D(printf("%d .00\t", i));
	return m[nr_rec - 1];
}


vector<int> compute_connected_components(const vector<MyRect>& input_rectangles,
					const vector<LogicalEdge>& logical_edges)
{
	assert(ranges::is_sorted(logical_edges));
	int n = input_rectangles.size();
	vector<int> connected_component(n, -1);

	auto filter=[&](const LogicalEdge& e){
        	int dist = rect_distance(input_rectangles[e.from], input_rectangles[e.to]);
        	return dist <= 20;
	};

	auto rec_compute_connected_components = [&](int i, int c, auto&& rec_compute_connected_components)->void{
		connected_component[i] = c;

		span adj_list = ranges::equal_range(logical_edges, i, {}, &LogicalEdge::from);

		for (const LogicalEdge& e : adj_list)
		{
			if (connected_component[e.to] == -1 && filter(e))
				rec_compute_connected_components(e.to, c, rec_compute_connected_components);
		}
	};

	int c=0;
	while (true)
	{
		auto it=ranges::find(connected_component, -1);
		if (it == end(connected_component))
			break;
		int i = ranges::distance(begin(connected_component), it);
		rec_compute_connected_components(i, c++, rec_compute_connected_components);
	}
	return connected_component;
}


void spread(Direction update_direction, const vector<RectLink>& rect_links, vector<MyRect>& rectangles)
{
//TODO: use chunk_by C++23
	const int N=30;
	int n = rectangles.size();

        MyPoint translations[N];

        ranges::fill(translations, MyPoint{0,0});

	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}

	auto rec_push_hole=[&](int ri, int tr, auto&& rec_push_hole)->void{

		D(printf("entering rec_push_hole(ri=%d ,tr=%d)\n", ri, tr));

		span adj_list = ranges::equal_range(rect_links, ri, {}, &RectLink::i);

		for (const RectLink& rl : adj_list)
		{
			int tr2= rectangles[ri][maxCompactRectDim] - rectangles[rl.j][minCompactRectDim];

			if (tr2 < 0)
				tr2 = 0;

			rec_push_hole(rl.j, tr+tr2, rec_push_hole);
		}
		int16_t &tri = translations[ri][update_direction];
		tri = max<int16_t>(tri, tr) ;
	};

	auto push_hole=[&](){

		vector<RectLink> index = rect_links;
		ranges::sort(index, {}, &RectLink::j);
		vector<int> root_nodes;
		ranges::set_difference(views::iota(0,n),
					index | views::transform(&RectLink::j),
					back_inserter(root_nodes)
					);
		for (int j : root_nodes)
		{
			int tr=0;
			rec_push_hole(j, tr, rec_push_hole);
		}
	};
	push_hole();

	for (const auto& [x, y] : views::counted(translations, n))
	{
		D(printf("{.x=%d, .y=%d},", x, y));
	}
	D(printf("\n"));

	for (int ri=0; ri < n; ri++)
	{
		rectangles[ri] += translations[ri];
	}
}


void compact(Direction update_direction, const vector<RectLink>& rect_links, const vector<LogicalEdge>& logical_edges, vector<MyRect>& rectangles)
{
	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}

	const vector<MyRect> rectangles_ = rectangles ;

	int n = rectangles.size();

	vector<vector<TranslationRangeItem> > vv(10);
	int id=0;

	auto next=[&](const vector<TranslationRangeItem>& prev)->vector<TranslationRangeItem>
	{
		vector<MyRect> rectangles = rectangles_;

		id++;

		vector<TranslationRangeItem> ts;

//TODO: views::set_union() and views::gzip_transform() and we wouldn't need to create so many variables.

		ranges::set_union(
			prev,
			views::iota(0,n) | views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i, .tr={.x=0,.y=0}}; }),
			back_inserter(ts),
			{},
			&TranslationRangeItem::ri,
			&TranslationRangeItem::ri);

		auto rects = views::iota(0,n) | views::transform([&](int i){return rectangles_[i]+ts[i].tr;});
		ranges::copy(rects, begin(rectangles));

		bitset<30> partition;

		auto rec_select_partition=[&](int ri, auto&& rec_select_partition)->void{

			auto rg = ranges::equal_range(rect_links, ri, {}, &RectLink::i)
						| views::filter([&](const RectLink& rl){return rectangles[ri][maxCompactRectDim] == rectangles[rl.j][minCompactRectDim];});

			ranges::for_each(rg, [&](const RectLink& rl){
					partition[rl.j]=1;
					D(printf("partition[%d]=1\n", rl.j));
					rec_select_partition(rl.j, rec_select_partition);
			});
		};

		auto rg = rectangles | views::transform([&](const MyRect& r){return r[minCompactRectDim];});
		const int frame_min = ranges::min(rg);
		const int next_min = ranges::min(rg | views::filter([&](int value){return value != frame_min;}));

		D(printf("frame_min=%d\n", frame_min));
		D(printf("next_min=%d\n", next_min));

		auto rng = views::iota(0,n) | views::filter([&](int i){return rectangles[i][minCompactRectDim]==frame_min;});

		for (int ri : rng)
		{
			partition[ri] = 1;
			D(printf("partition[%d] = 1\n", ri));
			rec_select_partition(ri, rec_select_partition);
		}

//TODO : use views::chunk_by() C++23
/*
		auto rng2 = views::iota(0,n) | views::filter([&](int ri){return partition[ri]==0;})
									| views::filter([&](int ri){
											auto rg = ranges::equal_range(logical_edges, ri, {}, &LogicalEdge::from) |
														views::transform([&](const LogicalEdge& e){return partition[e.to];}) ;
											return ranges::count(rg, 0)==0 && ranges::count(rg, 1) > 0; });

		for (int ri : rng2)
			partition[ri]=1;
*/
		auto r = rect_links | views::filter([&](const RectLink& e){return partition[e.i] > partition[e.j];})
				| views::transform([&](const RectLink& e){return rectangles[e.j][minCompactRectDim]-rectangles[e.i][maxCompactRectDim];}) ;

		vector<TranslationRangeItem> translation_ranges;

		if (ranges::empty(r))
			return translation_ranges;

		MyPoint tr={.x=0, .y=0};
		tr[update_direction] = min<int>(ranges::min(r), next_min - frame_min);

		auto rg2 = views::iota(0,n) | views::filter([&](int i){return partition[i]==1;})
					| views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i,.tr=tr};});

		ranges::for_each(rg2, [&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});

		auto rg3 = views::iota(0,n) | views::filter([&](int i){return rectangles[i][minCompactRectDim] != rectangles_[i][minCompactRectDim];})
						| views::transform([&](int i){MyPoint tr;
										tr[update_direction] = rectangles[i][minCompactRectDim] - rectangles_[i][minCompactRectDim];
										return TranslationRangeItem{.id=id, .ri=i, .tr=tr};
									});
		for (const auto [id, ri, tr] : rg3)
		{
			D(printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y));
		}

		ranges::copy(rg3, back_inserter(translation_ranges));
		return translation_ranges;
	};

//10: just had to choose a number. Should not be needed with C++23 partial_fold()
// Cf https://stackoverflow.com/questions/74042325/listing-all-intermediate-recurrence-results

	partial_sum(vv.begin(), vv.end(), vv.begin(),
				[&](const vector<TranslationRangeItem>& prev, const vector<TranslationRangeItem>&){
					return next(prev);}
				);

	auto rg = vv | views::join;

	D(printf("rg = vv | views::join\n"));

	for (const auto [id, ri, tr] : rg)
	{
		D(printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y));
	}

	if (ranges::empty(rg))
		return ;

//TODO: use views::left_fold() when it hopefully becomes available in C++23. It might clarify the design.
// Cf https://stackoverflow.com/questions/74042325/listing-all-intermediate-recurrence-results

//TODO: use C++23 chunk_by()

	const int nb = 1 + ranges::max(rg | views::transform(&TranslationRangeItem::id));

	id = ranges::min( views::iota(0,nb), {}, [&](int id){

			vector<MyRect> rectangles = rectangles_;
			ranges::for_each(ranges::equal_range(rg, id, {}, &TranslationRangeItem::id),
							[&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});

			auto rg1 = logical_edges |
				views::transform([&](const LogicalEdge& le){ return rectangle_distance(rectangles[le.from],rectangles[le.to]);	});

			auto rg2 = ranges::equal_range(rg, id, {}, &TranslationRangeItem::id) |
				views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);});

			auto rg3 = logical_edges |
				views::transform([&](const LogicalEdge& le){	return edge_overlap(rectangles[le.from],rectangles[le.to]);  });

			const int sigma_edge_distance = accumulate(ranges::begin(rg1), ranges::end(rg1),0);
			const int sigma_translation = accumulate(ranges::begin(rg2), ranges::end(rg2),0);
			const auto [width, height] = dimensions(compute_frame(rectangles));
			const int sigma_edge_overlap = accumulate(ranges::begin(rg3), ranges::end(rg3),0);

			D(printf("id = %d\n", id));
			D(printf("sigma_edge_distance = %d\n", sigma_edge_distance));
			D(printf("sigma_translation = %d\n", sigma_translation));
			D(printf("[.width=%d, .height=%d]\n", width, height));
			D(printf("sigma_edge_overlap = %d\n", sigma_edge_overlap));

			int cost = width + height + width*height + sigma_edge_distance + sigma_translation - sigma_edge_overlap;

			D(printf("cost = %d\n", cost));

			return cost;}
                );
	D(printf("id=%d\n", id));

	rectangles = rectangles_;
	ranges::for_each(ranges::equal_range(rg, id, {}, &TranslationRangeItem::id),
			[&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});
}


struct Mirror
{
	MirroringState mirroring_state;
	Direction mirroring_direction;
};

enum Algo
{
	SPREAD,
	COMPACT
};

struct Job
{
	Algo algo;
	Direction update_direction;
};


const unsigned NR_MIRRORING_OPTIONS=4;

const Mirror mirrors[NR_MIRRORING_OPTIONS][2]={
	{
		{.mirroring_state=IDLE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
	},
	{
		{.mirroring_state=IDLE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
	}
};

const char* MirroringStrings[NR_MIRRORING_OPTIONS]={
	"IDLE,IDLE",
	"IDLE,ACTIVE",
	"ACTIVE,IDLE",
	"ACTIVE,ACTIVE"
};

const unsigned NR_JOB_PIPELINES=2;

const Job pipelines[NR_JOB_PIPELINES][1]={
	{
		{.algo=SPREAD, .update_direction=EAST_WEST}
	},
	{
		{.algo=SPREAD, .update_direction=NORTH_SOUTH}
	}
};

const unsigned NR_RECT_CORNERS=4;

const RectDim corners[NR_RECT_CORNERS][2]={
	{LEFT, TOP},
	{LEFT, BOTTOM},
	{RIGHT, TOP},
	{RIGHT, BOTTOM}
};

const char* CornerStrings[NR_RECT_CORNERS]={
	"{LEFT, TOP}",
	"{LEFT, BOTTOM}",
	"{RIGHT, TOP}",
	"{RIGHT, BOTTOM}"
};

//4 mirrors X 4 corners X 2 job pipelines

void apply_mirror(const Mirror& mirror, vector<MyRect>& rectangles)
{
	const auto& [mirroring_state, mirroring_direction] = mirror;

	if (mirroring_state == ACTIVE)
	{
		ranges::for_each(rectangles, [&](MyRect &r){

			switch(mirroring_direction)
			{
			case EAST_WEST:
				r.m_left *= -1;
				r.m_right *= -1;
				swap(r.m_left, r.m_right);
				break;
			case NORTH_SOUTH:
				r.m_top *= -1;
				r.m_bottom *= -1;
				swap(r.m_top, r.m_bottom);
				break;
			}
		});
	}
}


void apply_job(const Job& job, const vector<LogicalEdge>& logical_edges, vector<MyRect>& rectangles)
{
	const auto& [algo, update_direction] = job;

	vector<RectLink> rect_links = sweep(update_direction, rectangles);

	switch (algo)
	{
	case SPREAD:
		spread(update_direction, rect_links, rectangles);
		break;
	case COMPACT:
		compact(update_direction, rect_links, logical_edges, rectangles);
		break;
	}
}


struct ProcessSelector
{
	unsigned pipeline, mirroring, match_corner;
};


// TODO: use upcoming C++23 views::cartesian_product()
vector<ProcessSelector> cartesian_product()
{
	vector<ProcessSelector> result;

	for (int pipeline=0; pipeline < NR_JOB_PIPELINES; pipeline++)
		for (int mirroring=0; mirroring < NR_MIRRORING_OPTIONS; mirroring++)
			for (int match_corner=0; match_corner < NR_RECT_CORNERS; match_corner++)
				result.push_back({pipeline, mirroring, match_corner});

	return result;
}


const vector<ProcessSelector> process_selectors = cartesian_product();


vector<TranslationRangeItem> compute_decision_tree_translations(const vector<DecisionTreeNode>& decision_tree,
								const vector<MyRect>& input_rectangles,
								const vector<LogicalEdge>& logical_edges)
{
	vector<MyRect> input_emplacements;
	vector<MyRect> holes = compute_holes(input_rectangles);
	for (const MyRect &r : input_rectangles)
		input_emplacements.push_back(r);
	for (const MyRect &rec : holes)
		input_emplacements.push_back(rec);

	int m = input_emplacements.size();
	int n = input_rectangles.size();
	vector<ProcessSelector> selectors(decision_tree.size());
	vector<TranslationRangeItem> translation_ranges;
	vector<RectangleHoleRangeItem> rectangle_hole_ranges;
	vector<MyRect> emplacements(m);
	vector<MyRect> rectangles(n);

	auto tf=[&](int id, unsigned pipeline, unsigned mirroring, unsigned match_corner, bool final){

		D(printf("calling tf(id=%d, pipeline=%u, mirroring=%u, match_corner=%u)\n", id, pipeline, mirroring, match_corner));

		auto rg = rectangles | views::transform([](const MyRect& r){return r.i;})
					| views::transform([&](int i)->string{
						char buffer[20];
						if (i < n)
							sprintf(buffer,"%d ", i);
						else
							sprintf(buffer,"h%d ", i-n);
						return buffer;
						}
					 )
					//| views::join_with(" ") ; TODO C++23
					| views::join;

		D(printf("[id=%d pipeline=%u, mirroring=%u, match_corner=%u] rectangles[...r.i]=", id, pipeline, mirroring, match_corner));
		for (char c : rg)
			D(printf("%c", c));
		D(printf("\n"));

		for (MyRect& r : rectangles)
			r = emplacements[r.i];

		const auto& [i_emplacement_source, i_emplacement_destination] = decision_tree[id].recmap;
		const auto [RectDimX, RectDimY] = corners[match_corner];
		MyRect &r1 = rectangles[i_emplacement_source];
		MyRect r2 = i_emplacement_destination < n ? input_rectangles[i_emplacement_destination] : emplacements[i_emplacement_destination];
		if (i_emplacement_destination < n)
		{
			r2 = trimmed(r2, rectangles);
			if (final)
			{
				rectangle_hole_ranges.push_back({.id=id, .ri=i_emplacement_destination, .r=r2});
			}
		}
		r1 += MyPoint{.x=r2[RectDimX] - r1[RectDimX], .y=r2[RectDimY] - r1[RectDimY]};

		for (const Mirror& mirror : mirrors[mirroring])
			apply_mirror(mirror, rectangles);
		for (const auto& [algo, update_direction] : pipelines[pipeline])
		{
			vector<RectLink> rect_links = sweep(update_direction, rectangles);
			assert(algo == SPREAD);
			spread(update_direction, rect_links, rectangles);
		}
		for (const Mirror& mirror : mirrors[mirroring])
			apply_mirror(mirror, rectangles);

		auto rg2 = rectangles | views::transform([&](const MyRect& r)->TranslationRangeItem{
                                        	const MyRect &ir = emplacements[r.i];
                                        	return {.id=id, .ri=r.i, .tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top}};})
				| views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};})
				| views::transform([&](const TranslationRangeItem& item)->string{
					const auto [id,i,tr]=item;
					char buffer[50];
					if (i < n)
						sprintf(buffer, "{id=%d, ri=%d, tr={x=%d, y=%d}} ", id, i, tr.x, tr.y);
					else
						sprintf(buffer, "{id=%d, ri=h%d, tr={x=%d, y=%d}} ", id, i-n, tr.x, tr.y);
					return buffer;})
				| views::join;

                D(printf("[id=%d pipeline=%u, mirroring=%u, match_corner=%u] tf output translations=", id, pipeline, mirroring, match_corner));
                for (char c : rg2)
                        D(printf("%c", c));
                D(printf("\n"));
	};

	for (int id=0; id < decision_tree.size(); id++)
	{

//TODO: use C++23 Deducing this.
		auto rec_tf=[&](int pid, auto&& rec_tf)->void{

			if (int parent_index = decision_tree[pid].parent_index; parent_index != -1)
			{
				rec_tf(parent_index, rec_tf);
			}

			const auto [pipeline, mirroring, match_corner] = selectors[pid];

			D(printf("[pipeline=%u, mirroring=%u, match_corner=%u] = selectors[id=%d]\n", pipeline, mirroring, match_corner, pid));

//gather the rectangles of interest
			rectangles.resize(n);
			memcpy(&rectangles[0], &emplacements[0], sizeof(MyRect)*n);
			for (int i=id; i != pid; i=decision_tree[i].parent_index)
			{
				const auto& [i_emplacement_source, i_emplacement_destination] = decision_tree[i].recmap;
				if (i_emplacement_destination >= n)
					rectangles.push_back( emplacements[i_emplacement_destination] );
			}

			const bool final=false;
			tf(pid, pipeline, mirroring, match_corner, final);

			for (const MyRect& r : rectangles)
				emplacements[r.i] = r;
		};

		memcpy(&emplacements[0], &input_emplacements[0], sizeof(MyRect)*m);
		for (int i=0; i<m; i++)
			emplacements[i].i = i;

		rectangles.resize(n);
		memcpy(&rectangles[0], &emplacements[0], sizeof(MyRect)*n);

		if (int parent_index = decision_tree[id].parent_index; parent_index != -1)
		{
			rec_tf(parent_index, rec_tf);
		}

/*
// TODO: use upcoming C++23 views::cartesian_product()
	auto rg = views::cartesian_product( views::iota(0, NR_JOB_PIPELINES),
										views::iota(0, NR_MIRRORING_OPTIONS),
										views::iota(0, NR_RECT_CORNERS));
	const auto& [pipeline, mirroring, match_corner] = ranges::min(rg, {}, [&](const auto [pipeline, mirroring, match_corner]{...

*/

		rectangles.resize(n);

		const auto [pipeline, mirroring, match_corner] = ranges::min(process_selectors, {}, [&](const ProcessSelector& ps){
			D(printf("pipeline=%u\n", ps.pipeline));
			D(printf("MirroringStrings[mirroring]=%s\n", MirroringStrings[ps.mirroring]));
			D(printf("CornerStrings[match_corner]=%s\n", CornerStrings[ps.match_corner]));

			const bool final=false;
			tf(id, ps.pipeline, ps.mirroring, ps.match_corner, final);

			auto rg1 = logical_edges |
				views::transform([&](const auto& le){ return rectangle_distance(rectangles[le.from],rectangles[le.to]);	});

			auto rg2 = views::iota(0,n) |
				views::transform([&](int i)->TranslationRangeItem{
					const MyRect &ir = input_rectangles[i], &r = rectangles[i];
					MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
					return {id, i, tr};}) |
				views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
				views::filter([&](const TranslationRangeItem& item){return item.ri != decision_tree[id].recmap.i_emplacement_source;}) |
				views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);});

			const int sigma_edge_distance = accumulate(ranges::begin(rg1), ranges::end(rg1),0);
			const int sigma_translation = accumulate(ranges::begin(rg2), ranges::end(rg2),0);
			const auto [width, height] = dimensions(compute_frame(rectangles));

			D(printf("sigma_edge_distance = %d\n", sigma_edge_distance));
			D(printf("sigma_translation = %d\n", sigma_translation));
			D(printf("[.width=%d, .height=%d]\n", width, height));

			int cost = width + height + sigma_edge_distance + sigma_translation ;

			D(printf("cost=%d\n", cost));
			return cost;
		});

		selectors[id] = {pipeline, mirroring, match_corner};

		D(printf("selectors[id=%d] = {pipeline=%u, mirroring=%u, match_corner=%u}\n", id, pipeline, mirroring, match_corner));

		D(printf("MirroringStrings[mirroring]=%s\n", MirroringStrings[mirroring]));
		D(printf("CornerStrings[match_corner]=%s\n", CornerStrings[match_corner]));

		const bool final=true;
		tf(id, pipeline, mirroring, match_corner, final);

		for (const MyRect& r : rectangles)
			emplacements[r.i] = r;

		auto rg = views::iota(0,n) |
					views::transform([&](int i)->TranslationRangeItem{
                                        const MyRect &ir = input_emplacements[i], &r = emplacements[i];
                                        MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
                                        return {id, i, tr};}) |
					views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};});

		for (TranslationRangeItem item : rg)
		{
			translation_ranges.push_back(item);
		}
	}

{
	FILE *f=fopen("translation_ranges.json", "w");
	fprintf(f, "[\n");
	for (int i=0; i < translation_ranges.size(); i++)
	{
		const auto [id, ri, tr] = translation_ranges[i];
		fprintf(f, "{\"id\":%d, \"ri\":%d, \"x\":%d, \"y\":%d}%s\n", id, ri, tr.x, tr.y,
			i+1 == translation_ranges.size() ? "": ",");
	}
	fprintf(f, "]\n");
	fclose(f);
}

{
	FILE* f=fopen("rectangle_hole_ranges.json", "w");
	const int size = rectangle_hole_ranges.size();
	fprintf(f, "[\n");
	for (int i=0; i < size; i++)
	{
		const auto [id, ri, r] = rectangle_hole_ranges[i];
		fprintf(f, "\t{\"id\":%d, \"ri\":%d, \"r\":{\"m_left\":%d, \"m_right\":%d, \"m_top\":%d, \"m_bottom\":%d}}%s\n",
			id, ri, r.m_left, r.m_right, r.m_top, r.m_bottom, i+1<n ? "," : "");
	}
	fprintf(f, "]\n");
	fclose(f);
}

	return translation_ranges;
}

struct JobMirror
{
	Job job;
	Mirror mirror;
};

const int NR_JOB_PIPELINES2=2;

const JobMirror pipelines2[NR_JOB_PIPELINES2][4]={
	{
		{
			.job={.algo=COMPACT, .update_direction=EAST_WEST},
			.mirror={.mirroring_state=IDLE, .mirroring_direction=EAST_WEST}
		},
                {
                        .job={.algo=COMPACT, .update_direction=EAST_WEST},
                        .mirror={.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST}
                },
		{
			.job={.algo=COMPACT, .update_direction=NORTH_SOUTH},
			.mirror={.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
		},
                {
                        .job={.algo=COMPACT, .update_direction=NORTH_SOUTH},
                        .mirror={.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
                }
	},
	{
                {
                        .job={.algo=COMPACT, .update_direction=NORTH_SOUTH},
                        .mirror={.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
                },
                {
                        .job={.algo=COMPACT, .update_direction=NORTH_SOUTH},
                        .mirror={.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
                },
                {
                        .job={.algo=COMPACT, .update_direction=EAST_WEST},
                        .mirror={.mirroring_state=IDLE, .mirroring_direction=EAST_WEST}
                },
                {
                        .job={.algo=COMPACT, .update_direction=EAST_WEST},
                        .mirror={.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST}
                }
	}
};


//./holes2 --dt --skip | awk '/begin cmpt_tr2 id=4 /,/end cmpt_tr2 id=4 /'
//TODO: use views::chunk_by() C++23

vector<TranslationRangeItem> compute_decision_tree_translations2(const vector<DecisionTreeNode>& decision_tree,
								const vector<TranslationRangeItem>& translation_ranges,
								const vector<MyRect>& input_rectangles,
								const vector<LogicalEdge>& logical_edges)
{
	int n = input_rectangles.size();

	vector<MyRect> rectangles(n), rectangles2(n);

	vector<TranslationRangeItem> translation_ranges2;

	auto tf=[&](unsigned pipeline){

		D(printf("calling tf(pipeline=%u)\n", pipeline));

		ranges::copy(rectangles, begin(rectangles2));

		for (const auto& [job, mirror] : pipelines2[pipeline])
		{
			apply_mirror(mirror, rectangles2);
			const auto& [algo, update_direction] = job;
			vector<RectLink> rect_links = sweep(update_direction, rectangles2);
			assert(algo == COMPACT);
			compact(update_direction, rect_links, logical_edges, rectangles2);
			apply_mirror(mirror, rectangles2);
		}
	};


	for (int id=0; id < decision_tree.size(); id++)
	{
		D(printf("begin cmpt_tr2 id=%d \n", id));
		ranges::copy(input_rectangles, begin(rectangles));

		vector<TranslationRangeItem> ts;

		ranges::set_union(
			ranges::equal_range(translation_ranges, id, {}, &TranslationRangeItem::id),
			views::iota(0,n) | views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i, .tr={.x=0,.y=0}}; }),
			back_inserter(ts),
			{},
			&TranslationRangeItem::ri,
			&TranslationRangeItem::ri);

		auto rg = views::iota(0,n) | views::transform([&](int i){return input_rectangles[i]+ts[i].tr;});
		ranges::copy(rg, begin(rectangles));

		const auto pipeline = ranges::min(views::iota(0,NR_JOB_PIPELINES2), {}, [&](int pipeline){
			D(printf("pipeline=%u\n", pipeline));

			tf(pipeline);

			auto rg1 = logical_edges |
				views::transform([&](const auto& le){ return rectangle_distance(rectangles2[le.from],rectangles2[le.to]);	});

			auto rg2 = views::iota(0,n) |
				views::transform([&](int i)->TranslationRangeItem{
					const MyRect &ir = rectangles[i], &r = rectangles2[i];
					MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
					return {id, i, tr};}) |
				views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
				views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);});

			const int sigma_edge_distance = accumulate(ranges::begin(rg1), ranges::end(rg1),0);
			const int sigma_translation = accumulate(ranges::begin(rg2), ranges::end(rg2),0);
			const auto [width, height] = dimensions(compute_frame(rectangles));

			D(printf("sigma_edge_distance = %d\n", sigma_edge_distance));
			D(printf("sigma_translation = %d\n", sigma_translation));
			D(printf("[.width=%d, .height=%d]\n", width, height));

			int cost = width + height + sigma_edge_distance + sigma_translation ;

			D(printf("cost=%d\n", cost));
			return cost;
		});

		D(printf("selection[id=%d] = {pipeline=%u}\n", id, pipeline));

		tf(pipeline);

		auto rng = views::iota(0,n) |
			views::transform([&](int i)->TranslationRangeItem{
				const MyRect &ir = rectangles[i], &r = rectangles2[i];
				MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
				return {id, i, tr};}) |
			views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};});

		for (TranslationRangeItem item : rng)
		{
			translation_ranges2.push_back(item);
		}

                D(printf("end cmpt_tr2 id=%d \n", id));
	}

{
	FILE *f=fopen("translation_ranges2.json", "w");
	fprintf(f, "[\n");
	for (int i=0; i < translation_ranges2.size(); i++)
	{
		const auto [id, ri, tr] = translation_ranges2[i];
		fprintf(f, "{\"id\":%d, \"ri\":%d, \"x\":%d, \"y\":%d}%s\n", id, ri, tr.x, tr.y,
			i+1 == translation_ranges2.size() ? "": ",");
	}
	fprintf(f, "]\n");
	fclose(f);
}
	return translation_ranges2;
}


struct TranslationItem {
	int i, x, y;
	friend bool operator==(const TranslationItem&, const TranslationItem&) = default;
};

struct TestContext {
	int testid;
	vector<MyRect> input_rectangles;
	vector<Job> pipeline;
	vector<TranslationItem> expected_translations;
};

const vector<TestContext> test_contexts={
/*
       +-------+
       |       |
+------+   1   +------+
|      |       |      |
|  0   +---+---+  2   +------+
|      |rh |   |      |      |
+------+---+---+------+  3   |
|      |       |      |      |
|  4   |   5   |      +------+
|      |       |
+------+-------+
3 => rh
*/
	{
		.testid=0,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
			{.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
			{.m_left=300-200, .m_right=400-200, .m_top=100, .m_bottom=200},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=150, .m_bottom=250}
		},
		.pipeline = {
			{.algo=SPREAD,.update_direction=EAST_WEST}
		},
		.expected_translations={
			{.i=5, .x=100, .y=0}
		}
	},
/*
       +-------+
       |       |
+------+   1   +------+
|      |       |      |
|  0   +---+---+  2   +---+
|      |       |      | 3 |
+------+   rh  +------+---+
|      |       |
|  4   +-------+
|      |       |
+------+   5   |
       |       |
       +-------+
3 => rh
*/
	{
		.testid=1,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
			{.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
			{.m_left=300-200, .m_right=350-200, .m_top=100, .m_bottom=150},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=200, .m_bottom=300}
		},
                .pipeline = {
                        {.algo=COMPACT,.update_direction=NORTH_SOUTH}
                },
		.expected_translations={
			{.i=1, .x=0, .y=50},
			{.i=3, .x=0, .y=50}
		}
	},
/*
       +-------+
       |       |
+------+   1   |
|      |       |
|  0   +---+---+------+---+
|      |       |      | 3 |
+------+   rh  |  2   +---+
|      |       |      |
|  4   +-------+------+
|      |       |
+------+   5   |
       |       |
       +-------+
3 => rh
*/
	{
		.testid=2,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
			{.m_left=200, .m_right=300, .m_top=100, .m_bottom=200},
			{.m_left=300-200, .m_right=350-200, .m_top=100, .m_bottom=150},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=200, .m_bottom=300}
		},
                .pipeline = {
                        {.algo=COMPACT,.update_direction=EAST_WEST}
                },
		.expected_translations={
			{.i=0, .x=50, .y=0},
			{.i=1, .x=50, .y=0},
			{.i=3, .x=50, .y=0},
			{.i=4, .x=50, .y=0},
			{.i=5, .x=50, .y=0}
		}
	},
/*
       +-------+
       |       |
+------+   1   +------+
|      |       |      |
|  0   +-------+  2   +------+
|      |       |      |      |
+------+-------+---+--+  3   |
|      |       |rh |  |      |
|  4   |   5   +---+  +------+
|      |       |
+------+-------+
3 => rh
*/
	{
		.testid=3,
		.input_rectangles = {
			{.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
			{.m_left=100, .m_right=200, .m_top=0, .m_bottom=100},
			{.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
			{.m_left=300-100, .m_right=400-100, .m_top=100+50, .m_bottom=200+50},
			{.m_left=0, .m_right=100, .m_top=150, .m_bottom=250},
			{.m_left=100, .m_right=200, .m_top=150, .m_bottom=250}
		},
                .pipeline = {
                        {.algo=COMPACT,.update_direction=NORTH_SOUTH}
                },
		.expected_translations={
			{.i=1, .x=0, .y=50}
		}
	},

/*
       +---+
       |rh |
+------+---+   +------+
|      |       |      |
|  0   +       +  1   +---------+
|      |       |      |         |
+------+       +------+  2      |
                      |         |
                      +---------+
2 => rh
*/
        {
                .testid=4,
                .input_rectangles = {
                        {.m_left=0, .m_right=100, .m_top=50, .m_bottom=150},
                        {.m_left=200, .m_right=300, .m_top=50, .m_bottom=150},
                        {.m_left=300-200, .m_right=450-200, .m_top=100-100, .m_bottom=200-100}
                },
                .pipeline = {
                        {.algo=SPREAD,.update_direction=EAST_WEST}
                },
                .expected_translations={
			{.i=1, .x=50, .y=0}
                }
        },

/*
+------+       +------+------+
|      |       |      |      |
|      |       |      |  2   |
|      |       |      |      |
|      |       |      +------+
|      +---+   |      |
|      |rh |   |      |
|  0   +---+   |  1   |
|      |       |      |
|      |       |      |
|      |       |      |
|      |       |      |
+------+       +------+
2 => rh
*/
        {
                .testid=5,
                .input_rectangles = {
                        {.m_left=0, .m_right=100, .m_top=0, .m_bottom=700},
                        {.m_left=300, .m_right=400, .m_top=0, .m_bottom=700},
                        {.m_left=400-300, .m_right=450-300, .m_top=0+250, .m_bottom=100+250}
                },
                .pipeline = {
                        {.algo=COMPACT,.update_direction=EAST_WEST}
                },
                .expected_translations={
			{.i=0, .x=150, .y=0},
			{.i=2, .x=150, .y=0}
                }
        },
	{
		.testid=6,
		.input_rectangles = {
                        {.m_left=328, .m_right=530, .m_top=10, .m_bottom=154},
			{.m_left=252, .m_right=474, .m_top=490, .m_bottom=601},
			{.m_left=385, .m_right=530, .m_top=218, .m_bottom=330},
			{.m_left=530, .m_right=696, .m_top=10, .m_bottom=202},
                        {.m_left=530, .m_right=696, .m_top=202, .m_bottom=330},
			{.m_left=682, .m_right=869, .m_top=346, .m_bottom=506},
			{.m_left=267, .m_right=447, .m_top=601, .m_bottom=761},
                        {.m_left=474, .m_right=682, .m_top=330, .m_bottom=506},
                        {.m_left=266, .m_right=474, .m_top=330, .m_bottom=490},
                        {.m_left=488, .m_right=675, .m_top=506, .m_bottom=650},
                        {.m_left=744, .m_right=917, .m_top=186, .m_bottom=346},
                        {.m_left=675, .m_right=862, .m_top=506, .m_bottom=714},
                        {.m_left=25, .m_right=205, .m_top=153, .m_bottom=281},
                        {.m_left=10, .m_right=205, .m_top=281, .m_bottom=441},
                        {.m_left=37, .m_right=252, .m_top=441, .m_bottom=617}
		},
		.pipeline = {
                        {.algo=COMPACT,.update_direction=EAST_WEST}
                },
                .expected_translations={
                }
	}
};


void test_fit()
{
	for (const auto& [testid, input_rectangles, pipeline, expected_translations] : test_contexts)
	{
		vector<MyRect> rectangles = input_rectangles;
		int dm1 = dim_max(compute_frame(input_rectangles));
		for (const auto& [algo, update_direction] : pipeline)
		{
			assert(algo == SPREAD);
			vector<RectLink> rect_links = sweep(update_direction, rectangles);
			spread(update_direction, rect_links, rectangles);
		}

		int n=rectangles.size();
		auto rg = views::iota(0, n) |
			views::filter([&](int i){return input_rectangles[i] != rectangles[i];}) |
			views::transform([&](int i)->TranslationItem{
				const MyRect &r1 = input_rectangles[i], &r2 = rectangles[i];
				return {.i=i,.x=r2.m_left - r1.m_left,.y=r2.m_top - r1.m_top};
			});

		int dm2 = dim_max(compute_frame(rectangles));
		bool bOK = ranges::equal(rg, expected_translations);
		printf("fit_to_hole testid=%d : %s\n", testid, bOK ? "OK" : "KO");
		printf("dim_max(frame) : %d => %d\n", dm1, dm2);
//		(bOK ? nbOK : nbKO)++;
	}
}


void test_translations()
{
	const auto& [testid, input_rectangles, logical_edges] = test_input[0];

	for (const auto [testid, decision_tree, expected_translation_ranges] : TRTestContexts)
	{
		vector<TranslationRangeItem> translation_ranges = compute_decision_tree_translations(decision_tree,
                        	                                        			input_rectangles,
												logical_edges);
		bool bOK = translation_ranges == expected_translation_ranges;
		printf("translation ranges testid=%d : %s\n", testid, bOK ? "OK" : "KO");
		if (bOK == false)
		{
        		auto rg = translation_ranges
				| views::transform([](const TranslationRangeItem& item)->string{
					const auto [id, ri, tr] = item;
					char buffer[200];
					sprintf(buffer, "\t\t{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y);
					return buffer;})
				| views::join;
			for (char const c : rg)
				D(printf("%c", c));
			D(printf("\n"));
		}
	}
};


enum RectStability {STABLE, UNSTABLE};

const vector<vector<RectStability> > Strategies={
        {STABLE,STABLE,STABLE,UNSTABLE,UNSTABLE,UNSTABLE,UNSTABLE},
        {STABLE,STABLE,UNSTABLE,UNSTABLE,UNSTABLE,UNSTABLE},
        {STABLE,UNSTABLE,UNSTABLE,UNSTABLE,UNSTABLE},
        {UNSTABLE,UNSTABLE,UNSTABLE,UNSTABLE}
};

vector<DecisionTreeNode> compute_decision_tree(const vector<MyRect>& input_rectangles,
						const vector<LogicalEdge>& logical_edges,
						vector<MyRect>& emplacements)
{
	const int n = input_rectangles.size();

{
/* TODO
	auto jv = input_rectangles |
		views::transform([&](const MyRect& r)->string{
			char buffer[200];
			sprintf(buffer, "\t{\"m_left\":%d, \"m_right\":%d, \"m_top\":%d, \"m_bottom\":%d},\n",r.m_left, r.m_right, r.m_top, r.m_bottom);
			return buffer;}) |
		views ::join;
*/
	char buffer[10*1000];
	int pos=0;
	pos += sprintf(buffer + pos,"{\n\"input_rectangles\":[");
	for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : input_rectangles)
        {
                pos += sprintf(buffer + pos, "\n\t{\"m_left\":%d, \"m_right\":%d, \"m_top\":%d, \"m_bottom\":%d},", m_left, m_right, m_top, m_bottom);
        }
	pos += sprintf(buffer + --pos,"\n],\n\"logical_edges\":[");
	for (const auto& [from, to] : logical_edges)
	{
		pos += sprintf(buffer + pos, "\n\t{\"from\":%d, \"to\":%d},", from, to);
	}
	pos += sprintf(buffer + --pos,"\n],\n\"topological_edges\":[");

        for (int ri=0; ri < n; ri++)
        {
                for (int rj : views::iota(0, n) | views::filter([&](int rj){return ri != rj && edge_overlap(input_rectangles[ri], input_rectangles[rj]);}))
                {
                        pos += sprintf(buffer + pos, "\n\t{\"from\":%d, \"to\":%d},", ri, rj);
                }
	}
	pos += sprintf(buffer + --pos,"\n]\n}");

        FILE *f=fopen("logical_graph.json", "w");
	fprintf(f, "%s", buffer);
	fclose(f);
}

	for (const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] : input_rectangles)
	{
		D(printf("{.m_left=%d, .m_right=%d, .m_top=%d, .m_bottom=%d}\n", m_left, m_right, m_top, m_bottom));
	}
	vector<MyRect> holes = compute_holes(input_rectangles);

{
	char buffer[100*1000];
	int pos=0;
        pos += sprintf(buffer + pos,"{\n\"holes\":[");
	for (const MyRect& r : holes)
	{
		pos += sprintf(buffer+pos, "\n\t{\"m_left\":%d,\"m_right\":%d,\"m_top\":%d,\"m_bottom\":%d},",
			r.m_left, r.m_right, r.m_top, r.m_bottom);
	}
	pos += sprintf(buffer + --pos,"\n],\n\"topological_contact\":[");

	for (int hi=0; hi < holes.size(); hi++)
	{
		MyRect& rec = holes[hi];
		for (int rj : views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rec, input_rectangles[rj]);}))
		{
			pos += sprintf(buffer+pos, "\n\t{\"hi\":%d, \"rj\":%d},", hi, rj);
		}
	}
	pos += sprintf(buffer + --pos, "\n]\n}");

	FILE *f=fopen("holes.json", "w");
	fprintf(f, "%s", buffer);
	fclose(f);
}

//La liste des rectangles et des trous devient une liste d'emplacements, et un graphe topologique.

	for (const MyRect &r : input_rectangles)
		emplacements.push_back(r);
	for (const MyRect &r : holes)
		emplacements.push_back(r);

	const int m = emplacements.size();

//TODO: use C++23 views::cartesian_product()

	vector<TopologicalEdge> topological_edges;
	for (int i=0; i < emplacements.size(); i++)
	{
		for (int j=0; j < emplacements.size(); j++)
		{
			int dist = rect_distance(emplacements[i], emplacements[j]);
			if (dist < 20)
				topological_edges.push_back({.from=i, .to=j, .distance=dist});
		}
	}

	ranges::sort(topological_edges);

	vector<int> connected_component = compute_connected_components(input_rectangles, logical_edges);

//	fmt::print("connected_component: {}\n", connected_component);

	D(printf("connected_component: "));
	for (int c : connected_component)
		D(printf("%d, ", c));
        D(printf("\n"));
	for (int i=0; i < n; i++)
		D(printf("connected_component[%d] = %d\n", i, connected_component[i]));
	D(printf("\n"));

	D(printf("page rank:\n"));
	vector<float> page_rank = compute_page_rank(n, logical_edges, topological_edges);
	D(printf("\n"));

	int nb = ranges::max(connected_component);
	vector<int> cc_size(nb, 0);
	for (int c : connected_component)
		cc_size[c]++;
	auto it = ranges::max_element(cc_size);
	int cmax = ranges::distance(begin(cc_size), it);
// si connected_component[i]==cmax alors i ne doit pas etre deplacé.

	vector<int> recmap(input_rectangles.size());
	for (int i=0; i < recmap.size(); i++)
		recmap[i] = i;

	vector<DecisionTreeNode> decision_tree;

	const unsigned BITSET_MAX_SIZE=1024;

	int strategy=0;

//TODO: use C++23 deducing this.

	auto build_decision_tree = [&](int parent_index, const bitset<BITSET_MAX_SIZE>& etat_emplacement, auto&& build_decision_tree)->void{

		auto mapping=[&](int i){
			assert(i < n);
			for (int pos=parent_index; pos != -1; pos = decision_tree[pos].parent_index)
			{
				const auto& [i_emplacement_source, i_emplacement_destination]=decision_tree[pos].recmap;
				if (i_emplacement_source == i)
					return i_emplacement_destination;
			}
			return i;
		};

 		int depth = 0;
		for (int pos=parent_index; pos != -1; pos = decision_tree[pos].parent_index)
			depth++;

//TODO: use C++23 cartesian_product() to generate (i,j) and views::filter() to filter out i==j, ...upfront
		for (int i=0; i < input_rectangles.size(); i++)
		{
			if (depth >= Strategies[strategy].size())
				continue;

			D(printf("i=%d\n", i));

			if (connected_component[i] == cmax && Strategies[strategy][depth] == STABLE)
			{
				D(printf("connected_component[%d] == cmax && Strategies[strategy=%d][depth=%d] == STABLE <= 2\n", i, strategy, depth));
			}
			else if (connected_component[i] != cmax && Strategies[strategy][depth] == UNSTABLE)
			{
				D(printf("connected_component[%d] != cmax && Strategies[strategy=%d][depth=%d] == UNSTABLE\n", i, strategy, depth));
			}
			else
			{
				D(printf("connected_component[%d] == %d. strategy=%d. depth=%d. skipping %d\n", i, connected_component[i], strategy, depth, i));
				continue;
			}

			if (mapping(i) != i)
			{
				D(printf("on ne mappe pas 2 fois un meme emplacement.\n"));
				D(printf("mapping[%d] != %d\n", i, i));
				continue;
			}

			span le_adj_list = ranges::equal_range(logical_edges, i, {}, &LogicalEdge::from);

		//si tous les liens de i {e sont des liens dont e.j n'a pas ete mappé et e.j que l'on peut deplacer}, alors i n'est pas
		// stable
			if (ranges::all_of(le_adj_list,
					[&](const LogicalEdge& e){return mapping(e.to)==e.to && connected_component[e.to] != cmax;}
					)
			)
			{
				D(printf("tous les liens de %d {e sont des liens dont e.j n'a pas ete mappé et e.j que l'on peut deplacer}\n", i));
				D(printf("%d is not stable\n", i));
				continue;
			}

			for (int j=0; j < emplacements.size(); j++)
			{
				D(printf("i=%d j=%d h=%d\n", i, j, j-n));

				if (j == i)
					continue;

				if (etat_emplacement[j] == OCCUPE)
				{
					D(printf("etat_emplacement[%d] == OCCUPE\n", j));
					continue;
				}

			//on regarde si emplacements[j] intersecte un emplacement deja occupé
				if (ranges::any_of(views::iota(input_rectangles.size()) |
									views::take(emplacements.size() - input_rectangles.size()) |
									views::filter([&](int i){return i!=j;}) |
									views::filter([&](int i){return etat_emplacement[i]==OCCUPE;}),
									[&](int i){return intersect_strict(emplacements[i], emplacements[j]);}
									)
					)
				{
					D(printf("emplacements[%d] intersecte un emplacement deja occupé\n", j));
					continue;
				}

// l'emplacement j est-il topologiquement lié aux rectangles auxquels i est logiquement lié et que l'on ne peut pas deplacer ?
			//les rectangles auxquels i est logiquement lié et que l'on ne peut pas déplacer:
				auto rg1 = le_adj_list |
// si connected_component[i]==cmax alors i ne doit pas etre deplacé.
					views::filter([&](const LogicalEdge& e){return connected_component[e.to] == cmax;}) |
					views::transform([](const LogicalEdge& e){return e.to;});

				D(printf("rg1={"));
				for (int i : rg1)
					D(printf(" %d,", i));
				D(printf("}\n"));


				span te_adj_list = ranges::equal_range(topological_edges, j, {}, &TopologicalEdge::from);

			//les rectangles auxquels j est topologiquement lié:
				auto rg2 = te_adj_list |
					views::filter([&](const TopologicalEdge& e){return e.to < input_rectangles.size();}) |
					views::filter([&](const TopologicalEdge& e){return mapping(e.to)==e.to /*connected_component[e.to] == cmax*/;}) |
					views::transform([](const TopologicalEdge& e){return e.to;});

                                D(printf("rg2={"));
                                for (int i : rg2)
                                        D(printf(" %d,", i));
                                D(printf("}\n"));

				assert(ranges::is_sorted(rg1));
				assert(ranges::is_sorted(rg2));

				if (ranges::includes(rg2, rg1)==false)
				{
					D(printf("l'emplacement %d(h=%d) est-il topologiquement lié aux rectangles auxquels %d est logiquement lié et que l'on ne peut pas deplacer ?\n", j, j-n, i));
					D(printf("ranges::includes(rg2, rg1)==false\n"));
					continue;
				}

			//ensuite on mappe les liens de i et on regarde si ils figurent bien dans les liens de j
			// en ne gardant que les liens de i {e dont e.j a deja ete mappé ou e.j que l'on ne peut deplacer}

				auto rg3 = le_adj_list |
					views::filter([&](const LogicalEdge& e){return (e.to==i ? j : mapping(e.to)) != e.to || connected_component[e.to] == cmax;}) |
					views::transform([&](const LogicalEdge& e){
						return TopologicalEdge{
							.from = e.from==i ? j : mapping(e.from),
							.to = e.to==i ? j : mapping(e.to),
							.distance=0};
						}
					);

				assert(ranges::is_sorted(te_adj_list));
			//rg4 est triée, mais pas rg3 because mapping shuffles the ordering
				auto it = ranges::find_if(rg3, [&](const TopologicalEdge& e){auto rg=ranges::equal_range(te_adj_list, e); return ranges::empty(rg);});
				if (it != ranges::end(rg3))
				{
					const TopologicalEdge& e = *it;
					D(printf("ensuite on mappe les liens de %d et on regarde si ils figurent bien dans les liens de %d\n", i, j));
					D(printf("it != ranges::end(rg3)\n"));
					D(printf("mapped TopologicalEdge={.from=%d, .to=%d} ne figure pas parmi les liens topologiques de %d\n", e.from, e.to, j));
					continue;
				}

				decision_tree.push_back({
					.parent_index=parent_index,
					.depth=depth,
					.recmap={
						.i_emplacement_source=i,
						.i_emplacement_destination=j
					}
				});

//1:OCCUPE, 0:LIBRE
				auto bitset_swap=[&](int i, int j)->bitset<BITSET_MAX_SIZE>{
					bitset<BITSET_MAX_SIZE> etat_emplacement_ =  etat_emplacement;
					D(printf("bitset_swap(etat_emplacement[%d], etat_emplacement[%d])\n", i, j));
					int bi = (int)etat_emplacement_[i], bj = (int)etat_emplacement_[j];
					swap(bi, bj);
					etat_emplacement_[i]=bi;
					etat_emplacement_[j]=bj;
					D(printf("etat_emplacement_[%d]=%d\n", i, bi));
					D(printf("etat_emplacement_[%d]=%d\n", j, bj));
					return etat_emplacement_;
				};

				build_decision_tree(decision_tree.size()-1, bitset_swap(i, j), build_decision_tree);
			}
		}
	};

//1:OCCUPE, 0:LIBRE
	const bitset<BITSET_MAX_SIZE> etat_emplacement(string(m-n,'0')+string(n,'1'));
	for (strategy=0; strategy < Strategies.size(); strategy++)
		build_decision_tree(-1, etat_emplacement, build_decision_tree);

	D(printf("decision_tree.size()=%ld\n", decision_tree.size()));

	for (int i=0; i < decision_tree.size(); i++)
	{
		deque<RectMap> chemin;

		for (int pos=i; pos != -1; pos = decision_tree[pos].parent_index)
		{
			chemin.push_front( decision_tree[pos].recmap );
		}
		vector<int> mapping(n);
		for (int ii=0; ii<input_rectangles.size(); ii++)
			mapping[ii]=ii;
		for (const auto& [i_emplacement_source, i_emplacement_destination] : chemin)
		{
			mapping[i_emplacement_source] = i_emplacement_destination;
		}

		auto rg = logical_edges |
			views::transform([&](const LogicalEdge& e)->TopologicalEdge{return {
				.from=mapping[e.from],
				.to=mapping[e.to],
				.distance=0
			};});
		vector<TopologicalEdge> v(logical_edges.size()), inter;
		ranges::copy(rg, &v[0]);
		ranges::sort(v);
		ranges::set_intersection(v, topological_edges, back_inserter(inter));
                decision_tree[i].match = inter.size();
		D(printf("i=%d inter.size()=%ld\n", i, inter.size()));
	}

{
        FILE *f=fopen("decision_tree.json", "w");
        fprintf(f, "[\n");
        for (int i=0; i < decision_tree.size(); i++)
        {
                const auto& [_, parent_index, depth, recmap, match] = decision_tree[i];
                const auto& [i_emplacement_source, i_emplacement_destination] = recmap;
                fprintf(f, "{\"i\":%d, \"parent_index\":%d, \"depth\":%d, \"i_emplacement_source\":%d, \"i_emplacement_destination\":%d, \"match\":%d}%s\n",
                        i, parent_index, depth, i_emplacement_source, i_emplacement_destination, match,
                        i+1 == decision_tree.size() ? "": ",");
        }
        fprintf(f, "]\n");
        fclose(f);
}
	return decision_tree;
}

//TODO: use C++23 views::chunk_by() and  views::zip_transform(), views::to<vector>().
vector<Score> compute_scores(const vector<DecisionTreeNode>& decision_tree,
			const vector<TranslationRangeItem>& translation_ranges,
			const vector<MyRect>& input_rectangles,
			const vector<LogicalEdge>& logical_edges)
{
	int n = input_rectangles.size();
	int m = decision_tree.size();

	auto rg = views::iota(0, m) |
		views::transform([&](int id)->Score{

			vector<TranslationRangeItem> ts;

			ranges::set_union(
				ranges::equal_range(translation_ranges, id, {}, &TranslationRangeItem::id),
				views::iota(0,n) | views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i, .tr={.x=0,.y=0}}; }),
				back_inserter(ts),
				{},
				&TranslationRangeItem::ri,
				&TranslationRangeItem::ri);

			auto rg = views::iota(0,n) | views::transform([&](int i){return input_rectangles[i]+ts[i].tr;});
			vector<MyRect> rectangles(ranges::begin(rg), ranges::end(rg));

			auto rg1 = logical_edges | views::transform([&](const auto& le){
				return rectangle_distance(rectangles[le.from],rectangles[le.to]); });
			const int sigma_edge_distance = accumulate(ranges::begin(rg1), ranges::end(rg1),0);
			const auto [width, height] = dimensions(compute_frame(rectangles));

			return {
				.id=id,
				.sigma_edge_distance=sigma_edge_distance,
				.width=width,
				.height=height,
				.total= width + height + sigma_edge_distance
			};
		});

//TODO: use C++23 views::join_with(",") and avoid allocation of 'vector<DiagramScore> scores'

	vector<Score> scores(ranges::begin(rg), ranges::end(rg));
{
	FILE *f=fopen("scores.json", "w");
	fprintf(f, "[\n");
	for (int i=0; i < scores.size(); i++)
	{
		const auto [id, sigma_edge_distance, width, height, total] = scores[i];
		fprintf(f, "{\"id\":%d, \"sigma_edge_distance\":%d, \"width\":%d, \"height\":%d, \"total\":%d}%s\n",
			id, sigma_edge_distance, width, height, total,
			i+1 == m ? "": ",");
	}
	fprintf(f, "]\n");
	fclose(f);
}

	return scores;
}


// ./holes2 --dt --skip | awk '/begin cmpt_tr2 id=4 /,/end cmpt_tr2 id=4 /'
// ./holes2 --dt | awk '/begin testid=1 /,/end testid=1 /' | awk '/begin compute_decision_tree/,/end compute_decision_tree/' > holes2.log

int main(int argc, char* argv[])
{
for (const auto& [testid, input_rectangles, logical_edges] : test_input)
{
	D(printf("begin testid=%d \n", testid));
	char file_name[50];

	if (argc==2 && strcmp(argv[1], "--dt")==0)
	{
		vector<MyRect> emplacements;

		D(printf("begin compute_decision_tree()\n"));
		vector<DecisionTreeNode> decision_tree = compute_decision_tree(input_rectangles, logical_edges, emplacements);
                D(printf("end compute_decision_tree()\n"));

		sprintf(file_name, "logical_graph%d.json", testid);
		fs::copy("logical_graph.json", file_name, fs::copy_options::update_existing);
		sprintf(file_name, "holes%d.json", testid);
		fs::copy("holes.json", file_name, fs::copy_options::update_existing);
		sprintf(file_name, "decision_tree%d.json", testid);
		fs::copy("decision_tree.json", file_name, fs::copy_options::update_existing);

		sprintf(file_name, "decision_tree%d.dat", testid);
		if(FILE* f = fopen(file_name, "wb")) {
			fwrite(&decision_tree[0], sizeof decision_tree[0], decision_tree.size(), f);
			fclose(f);
		}

		vector<TranslationRangeItem> translation_ranges = compute_decision_tree_translations(decision_tree, input_rectangles, logical_edges);

                sprintf(file_name, "translation_ranges_%d.json", testid);
		fs::copy("translation_ranges.json", file_name, fs::copy_options::update_existing);
                sprintf(file_name, "rectangle_hole_ranges%d.json", testid);
		fs::copy("rectangle_hole_ranges.json", file_name, fs::copy_options::update_existing);

		sprintf(file_name, "translation_ranges%d.dat", testid);
		if(FILE* f = fopen(file_name, "wb")) {
			fwrite(&translation_ranges[0], sizeof translation_ranges[0], translation_ranges.size(), f);
			fclose(f);
		}

		vector<TranslationRangeItem> translation_ranges2 = compute_decision_tree_translations2(decision_tree, translation_ranges, input_rectangles, logical_edges);

                sprintf(file_name, "translation_ranges2_%d.json", testid);
                fs::copy("translation_ranges2.json", file_name, fs::copy_options::update_existing);

		vector<Score> scores = compute_scores(decision_tree, translation_ranges, input_rectangles, logical_edges);

                sprintf(file_name, "scores%d.json", testid);
		fs::copy("scores.json", file_name, fs::copy_options::update_existing);
	}

	if (argc==3 && strcmp(argv[1], "--dt")==0 && strcmp(argv[2], "--skip")==0)
        {
                vector<DecisionTreeNode> decision_tree ;

                sprintf(file_name, "decision_tree%d.dat", testid);
                if(FILE* f = fopen(file_name, "rb")) {
    			struct stat stat_buf;
			int rc = stat(file_name, &stat_buf);
			int n = stat_buf.st_size / sizeof(DecisionTreeNode);
			decision_tree.resize(n);
			size_t ret_code = fread(&decision_tree[0], sizeof (DecisionTreeNode), n, f);
                        fclose(f);
                }

                vector<TranslationRangeItem> translation_ranges ;

                sprintf(file_name, "translation_ranges%d.dat", testid);
                if(FILE* f = fopen(file_name, "rb")) {
                        struct stat stat_buf;
                        int rc = stat(file_name, &stat_buf);
                        int n = stat_buf.st_size / sizeof(TranslationRangeItem);
                        translation_ranges.resize(n);
                        size_t ret_code = fread(&translation_ranges[0], sizeof (TranslationRangeItem), n, f);
                        fclose(f);
                }

                vector<TranslationRangeItem> translation_ranges2 = compute_decision_tree_translations2(decision_tree, translation_ranges, input_rectangles, logical_edges);

                vector<Score> scores = compute_scores(decision_tree, translation_ranges, input_rectangles, logical_edges);
        }
        D(printf("end testid=%d \n", testid));
}

	if (argc == 1)
	{
		test_rect_trim();

		test_fit();

		test_translations();
	}

	return 0;
}
