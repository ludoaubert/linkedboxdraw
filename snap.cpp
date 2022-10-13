#include <vector>
#include <algorithm>
#include <numeric>
#include <ranges>
#include <bitset>
#include <stdio.h>
#include "MyRect.h"
using namespace std;

struct LogicalEdge {
	int from;
	int to;

	friend auto operator<=>(const LogicalEdge&, const LogicalEdge&) = default;
};

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

//bi directional edges
const vector<LogicalEdge> logical_edges = {
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
};

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


void compact(Direction update_direction, const vector<RectLink>& rect_links, const vector<LogicalEdge>& logical_edges, vector<MyRect>& rectangles)
{
	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
	
	const vector<MyRect> rectangles_ = rectangles ;
	
	int n = rectangles.size();
	
	auto next=[&](const vector<TranslationRangeItem>& prev)->vector<TranslationRangeItem>
	{
		vector<MyRect> rectangles = rectangles_;
		
		const int id = prev.empty() ? 1 : prev[0].id+1;

		vector<TranslationRangeItem> ts;
		
//TODO: views::set_union() and views::gzip_transform() and we wouldn't need to create so many variables.

		ranges::set_union(
			prev,
			views::iota(0,n) | views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i, .tr={.x=0,.y=0}}; }),
			back_inserter(ts),
			{},
			&TranslationRangeItem::ri,
			&TranslationRangeItem::ri);

		auto rg = views::iota(0,n) | views::transform([&](int i){return rectangles_[i]+ts[i].tr;});
		ranges::copy(rg, begin(rectangles));
			
		bitset<30> partition;

		auto rec_select_partition=[&](int ri, auto&& rec_select_partition)->void{

			auto rg = ranges::equal_range(rect_links, ri, {}, &RectLink::i)
						| views::filter([&](const RectLink& rl){return rectangles[ri][maxCompactRectDim] == rectangles[rl.j][minCompactRectDim];});

			ranges::for_each(rg, [&](const RectLink& rl){
					partition[rl.j]=1;
					printf("partition[%d]=1\n", rl.j);
					rec_select_partition(rl.j, rec_select_partition);
			});
		};

		auto rg = rectangles | views::transform([&](const MyRect& r){return r[minCompactRectDim];});
		const int frame_min = ranges::min(rg);
		const int next_min = ranges::min(rg | views::filter([&](int value){return value != frame_min;}));

		printf("frame_min=%d\n", frame_min);
		printf("next_min=%d\n", next_min);

		auto rng = views::iota(0,n) | views::filter([&](int i){return rectangles[i][minCompactRectDim]==frame_min;});

		for (int ri : rng)
		{
			partition[ri] = 1;
			printf("partition[%d] = 1\n", ri);
			rec_select_partition(ri, rec_select_partition);
		}

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
			printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y);
		}

		ranges::copy(rg3, back_inserter(translation_ranges));
		return translation_ranges;
	};
	
//10: just had to choose a number. Should not be needed with C++23 partial_fold()
// Cf https://stackoverflow.com/questions/74042325/listing-all-intermediate-recurrence-results
	vector<vector<TranslationRangeItem> > vv(10);
	partial_sum(vv.begin(), vv.end(), vv.begin(), 
				[&](const vector<TranslationRangeItem>& prev, const vector<TranslationRangeItem>&){
					return next(prev);}
				);
					
	auto rg = vv | views::join;
					
	

	vector<TranslationRangeItem> translation_ranges;

	const int n = rectangles.size();
	
//TODO: use views::left_fold() when it hopefully becomes available in C++23. It might clarify the design.
// Cf https://stackoverflow.com/questions/74042325/listing-all-intermediate-recurrence-results

	for (int id=0; ; id++)
	{
		printf("id=%d\n", id);

		bitset<30> partition;

		auto rec_select_partition=[&](int ri, auto&& rec_select_partition)->void{

			auto rg = ranges::equal_range(rect_links, ri, {}, &RectLink::i)
						| views::filter([&](const RectLink& rl){return rectangles[ri][maxCompactRectDim] == rectangles[rl.j][minCompactRectDim];});

			ranges::for_each(rg, [&](const RectLink& rl){
					partition[rl.j]=1;
					printf("partition[%d]=1\n", rl.j);
					rec_select_partition(rl.j, rec_select_partition);
			});
		};

		auto rg = rectangles | views::transform([&](const MyRect& r){return r[minCompactRectDim];});
		const int frame_min = ranges::min(rg);
		const int next_min = ranges::min(rg | views::filter([&](int value){return value != frame_min;}));

		printf("frame_min=%d\n", frame_min);
		printf("next_min=%d\n", next_min);

		auto rng = views::iota(0,n) | views::filter([&](int i){return rectangles[i][minCompactRectDim]==frame_min;});

		for (int ri : rng)
		{
			partition[ri] = 1;
			printf("partition[%d] = 1\n", ri);
			rec_select_partition(ri, rec_select_partition);
		}

		auto r = rect_links | views::filter([&](const RectLink& e){return partition[e.i] > partition[e.j];})
				| views::transform([&](const RectLink& e){return rectangles[e.j][minCompactRectDim]-rectangles[e.i][maxCompactRectDim];}) ;

		if (ranges::empty(r))
			break;

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
			printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y);
		}

		ranges::copy(rg3, back_inserter(translation_ranges));
	}

	for (const auto [id, ri, tr] : translation_ranges)
	{
		printf("{.id=%d, .ri=%d, .tr={.x=%d, .y=%d}},\n", id, ri, tr.x, tr.y);
	}

//TODO: use C++23 chunk_by()

	const int nb = 1 + ranges::max(translation_ranges | views::transform(&TranslationRangeItem::id));

	int id = ranges::min( views::iota(0,nb), {}, [&](int id){

			vector<MyRect> rectangles = rectangles_;
			ranges::for_each(ranges::equal_range(translation_ranges, id, {}, &TranslationRangeItem::id),
							[&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});

			auto rg1 = logical_edges |
				views::transform([&](const auto& le){ return rectangle_distance(rectangles[le.from],rectangles[le.to]);	});

			auto rg2 = ranges::equal_range(translation_ranges, id, {}, &TranslationRangeItem::id) |
				views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);});

			const int sigma_edge_distance = accumulate(ranges::begin(rg1), ranges::end(rg1),0);
			const int sigma_translation = accumulate(ranges::begin(rg2), ranges::end(rg2),0);
			const auto [width, height] = dimensions(compute_frame(rectangles));

			printf("sigma_edge_distance = %d\n", sigma_edge_distance);
			printf("sigma_translation = %d\n", sigma_translation);
			printf("[.width=%d, .height=%d]\n", width, height);

			int cost = width + height + sigma_edge_distance + sigma_translation ;
                        return cost;}
                );
	printf("id=%d\n", id);

	rectangles = rectangles_;
	ranges::for_each(ranges::equal_range(translation_ranges, id, {}, &TranslationRangeItem::id),
			[&](const TranslationRangeItem& item){const auto [id, ri, tr]=item; rectangles[ri]+=tr;});
}


int main(int argc, char* argv[])
{
	vector<MyRect> rectangles = input_rectangles;
	compact(EAST_WEST, rect_links, logical_edges, rectangles);

	return 0;
}
