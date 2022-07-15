//TODO: reserve memory upfront to hold sweep_line data.
//v2.reserve(25);

#include <vector>
#include <span>
#include <ranges>
#include <algorithm>
using namespace std;

/*
           +-------+
           |       |
+------+   |   1   |   +------+
|      |   |       |   |      |
|  0   |   +-------+   |  2   |   +------+
|      |               |      |   |      |
+------+               +------+   |  3   |
+------+   +-------+              |      |
|      |   |       |              +------+
|  4   |   |   5   |
|      |   |       |
+------+   +-------+
*/

enum RectDim{LEFT,RIGHT,TOP,BOTTOM};

struct MyRect{
	int left, right, top, bottom;
	int& operator[](RectDim rectdim){
		switch(rectdim)
		{
		case LEFT:
			return left;
		case RIGHT:
			return right;
		case TOP:
			return top;
		case BOTTOM:
			return bottom;
		}
	}
};

struct MyEdge{int i, j;};

struct TrCandidate{int o, ri, tr;};

int main(){

	vector<MyRect> rects = {
		{0, 100, 50, 150},
		{150, 250, 0, 100},
		{300, 400, 50, 150},
		{450, 550, 100, 200},
		{0, 100, 150, 250},
		{150, 250, 150, 250}
	};
	
	int n = rects.size();
	
	const RectDim minCompactRectDim = LEFT;
	const RectDim maxCompactRectDim = RIGHT;
	
	MyRect frame={
		.left=ranges::min(rects | views::transform(&MyRect::left)),
		.right=ranges::max(rects | views::transform(&MyRect::right)),
		.top=ranges::min(rects | views::transform(&MyRect::top)),
		.bottom=ranges::max(rects | views::transform(&MyRect::bottom))
	};

	vector<MyEdge> edges={{0,1}, {1,2}, {2,3}, {4,5}, {5,3}};

	vector<int> edge_partition={0,1,2,3,3,4,5};

	auto adj_list=[&](int ri)->span<MyEdge>{
		int i=edge_partition[ri], j=edge_partition[ri+1]; 
		return span<MyEdge>(&edges[i], j-i);
	};
	
	vector<TrCandidate> translation_candidates;
	translation_candidates.reserve(256);

	auto rec_query_translation=[&](int o, int ri, auto&& rec_query_translation)->int{
		span<MyEdge> adj = adj_list(ri);
		if (adj.empty())
		{
			int tr = frame[maxCompactRectDim] - rects[ri][maxCompactRectDim];
			translation_candidates.push_back({o, ri, tr});
			return tr;
		}
		int tr = ranges::min(adj | views::transform([&](const MyEdge e){
					return rec_query_translation(o, e.j, rec_query_translation) + rects[e.j][minCompactRectDim]-rects[ri][maxCompactRectDim];
				}
			)
		);
		translation_candidates.push_back({o, ri, tr});
		return tr;
	};
	
	for (int o : views::iota(0,n) | views::filter([&](int i){return rects[i][minCompactRectDim]==frame[minCompactRectDim];}))
	{
		rec_query_translation(o, o, rec_query_translation);
	}

	for (auto& [o, ri, tr] : translation_candidates)
	{
		printf("o=%d ri=%d tr=%d\n", o, ri, tr);
	}
	
	int tr_min = ranges::min( translation_candidates | views::filter([&](const TrCandidate& trc){return trc.o==trc.ri;}) | views::transform(&TrCandidate::tr));
	printf("tr_min=%d\n", tr_min);
	
	vector<int> translations(n,-1);
	
	for (const auto& [o, ri, tr] : translation_candidates | views::filter([&](const TrCandidate& trc){return trc.o==trc.ri;}))
	{
		translations[o] = tr;
	}
	
	for (auto& [o, ri, tr] : translation_candidates)
	{
		tr = tr + min(translations[o], tr_min) - translations[o];
	}
	
	for (auto& [o, ri, tr] : translation_candidates)
	{
		printf("o=%d ri=%d tr=%d\n", o, ri, tr);
	}
	
	for (auto& [o, ri, tr] : translation_candidates | views::filter([&](const TrCandidate& trc){return rects[trc.ri][maxCompactRectDim]==frame[maxCompactRectDim];}))
	{
		tr = 0;
	}
	
	printf("after setting backline to zero:\n");
	for (auto& [o, ri, tr] : translation_candidates)
	{
		printf("o=%d ri=%d tr=%d\n", o, ri, tr);
	}
	
	for (auto& [o, ri, tr] : translation_candidates)
	{
		translations[ri]=tr;
	}
	
	for (int ri=0; ri < n; ri++)
	{
		printf("translations[ri=%d]=%d\n",ri, translations[ri]);
	}
}