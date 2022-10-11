#include <vector>
#include <algorithm>
#include <ranges>
#include <bitset>
#include <stdio.h>
#include "MyRect.h"
using namespace std;


struct RectLink{
	int i, j, TOP, BOTTOM;
};


struct TranslationRangeItem
{
	int id;
	int ri;
	MyPoint tr;

	friend bool operator==(const TranslationRangeItem&, const TranslationRangeItem&) = default;
};

const vector<MyRect> input_rectangles = {
	{m_left: 328, m_right: 530, m_top: 10, m_bottom: 154},
	{m_left: 252, m_right: 474, m_top: 490, m_bottom: 601},
	{m_left: 385, m_right: 530, m_top: 218, m_bottom: 330},
	{m_left: 530, m_right: 696, m_top: 10, m_bottom: 202},
	{m_left: 530, m_right: 696, m_top: 202, m_bottom: 330},
	{m_left: 682, m_right: 869, m_top: 346, m_bottom: 506},
	{m_left: 267, m_right: 447, m_top: 601, m_bottom: 761},
	{m_left: 474, m_right: 682, m_top: 330, m_bottom: 506},
	{m_left: 266, m_right: 474, m_top: 330, m_bottom: 490},
	{m_left: 488, m_right: 675, m_top: 506, m_bottom: 650},
	{m_left: 744, m_right: 917, m_top: 186, m_bottom: 346},
	{m_left: 675, m_right: 862, m_top: 506, m_bottom: 714},
	{m_left: 25, m_right: 205, m_top: 153, m_bottom: 281},
	{m_left: 10, m_right: 205, m_top: 281, m_bottom: 441},
	{m_left: 37, m_right: 252, m_top: 441, m_bottom: 617}
} ;

const vector<RectLink> rect_links={
	{.i=0, .j=3, .TOP=10, .BOTTOM=154},
	{.i=1, .j=7, .TOP=490, .BOTTOM=506},
	{.i=1, .j=9, .TOP=506, .BOTTOM=601},
	{.i=2, .j=4, .TOP=218, .BOTTOM=330},
	{.i=3, .j=10, .TOP=186, .BOTTOM=202},
	{.i=4, .j=10, .TOP=202, .BOTTOM=330},
	{.i=6, .j=9, .TOP=601, .BOTTOM=650},
	{.i=6, .j=11, .TOP=650, .BOTTOM=714},
	{.i=7, .j=5, .TOP=346, .BOTTOM=506},
	{.i=7, .j=10, .TOP=330, .BOTTOM=346},
	{.i=8, .j=7, .TOP=330, .BOTTOM=490},
	{.i=9, .j=11, .TOP=506, .BOTTOM=650},
	{.i=12, .j=0, .TOP=153, .BOTTOM=154},
	{.i=12, .j=2, .TOP=218, .BOTTOM=281},
	{.i=12, .j=3, .TOP=154, .BOTTOM=202},
	{.i=12, .j=4, .TOP=202, .BOTTOM=218},
	{.i=13, .j=2, .TOP=281, .BOTTOM=330},
	{.i=13, .j=8, .TOP=330, .BOTTOM=441},
	{.i=14, .j=1, .TOP=490, .BOTTOM=601},
	{.i=14, .j=6, .TOP=601, .BOTTOM=617},
	{.i=14, .j=8, .TOP=441, .BOTTOM=490}
};

void compact(Direction update_direction, const vector<RectLink>& rect_links, vector<MyRect>& rectangles)
{
	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
	
	vector<TranslationRangeItem> translation_ranges;

	const int n = rectangles.size();

	for (int id=0; ; id++)
	{
		printf("id=%d\n", id);
		
		bitset<30> partition;
		
		auto rec_select_partition=[&](int ri, auto&& rec_select_partition){
	
			auto rg = ranges::equal_range(rect_links, ri, {}, &RectLink::i) 
						| views::filter([&](const RectLink& rl){return rectangles[ri][maxCompactRectDim] == rectangles[rl.j][minCompactRectDim];})
			ranges::for_each(rg, [&](const RectLink& rl){
					partition[rl.j]=1;
					printf("partition[%d]=1\n", rl.j);
					rec_select_partition(rl.j, rec_select_partition);
			});

			}
		};
		
		auto rg = rectangles | views::transform([&](const MyRect& r){return r[minCompactRectDim];});
		const int frame_min = min(rg);
		const int next_min = min(rg | views::filter([&](int value){return value != frame_min;}));
		
		printf("frame_min=%d\n", frame_min);
		printf("next_min=%d\n", next_min);
		
		auto rg = views::iota(0,n) | views::filter([&](int i){return rectangles[i][minCompactRectDim]==frame_min;});
		
		for (int ri : rg)
		{
			partition[ri] = 1;
			printf("partition[%d] = 1\n", ri);
			rec_select_partition(ri, rec_select_partition);
		}

		auto rg = rect_links | views::filter([&](const RectLink& e){return partition[e.i] != partition[e.j];})
				| views::transform([&](const RectLink& e){return rectangles[e.j][minCompactRectDim]-rectangles[e.i][maxCompactRectDim];}) ;

		MyPoint tr={.x=0, .y=0};
		tr[update_direction] = min<int>(min(rg), next_min - frame_min);

		if (tr[update_direction] <= 0)
			break;

		auto rg2 = views::iota(0,n) | views::filter([&](int i){return partition[i]==1;})
					| views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i,.tr=tr};});
		
		ranges::for_each(rg2, [&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});
		
		auto rg3 = views::iota(0,n) | views::select([&](int i){return rectangles[i][minCompactRectDim] != input_rectangles[i][minCompactRectDim];})
								| views::transform([&](int i){MyPoint tr;
										tr[update_direction] = rectangles[i][minCompactRectDim] - input_rectangles[i][minCompactRectDim];
										return TranslationRangeItem{.id=id, .ri=i, .tr=tr};
									});
		for (const auto [id, ri, tr] : rg3)
		{
			printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y);
		}

		ranges::copy(rg3, back_inserter(translation_ranges));
	}

	return translation_ranges;
}


int main(int argc, char* argv[])
{
	vector<MyRect> rectangles = input_rectangles;
	vector<TranslationRangeItem> translation_ranges = compact(EAST_WEST, rect_links, rectangles);
	
	for (const auto [id, ri, tr] : translation_ranges)
	{
		printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y);
	}
	
	return 0;
}

//g++-12 -fmodules-ts -std=c++2b snap.cpp MyRect.cpp