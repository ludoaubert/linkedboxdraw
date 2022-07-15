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
                                  |      |
								  +------+
*/

enum RectDim{LEFT,RIGHT,TOP,BOTTOM};

struct MyRect{int left, right, top, bottom;};

struct MyEdge{int i, j;};

int main(){

	vector<MyRect> rects = {
		{0, 100, 50, 150},
		{150, 250, 0, 100},
		{300, 400, 50, 150},
		{450, 550, 100, 200}
	};
	
	MyRect frame={
		.left=ranges::min(rects | views::transform(&MyRect::left)),
		.right=ranges::max(rects | views::transform(&MyRect::right)),
		.top=ranges::min(rects | views::transform(&MyRect::top)),
		.bottom=ranges::max(rects | views::transform(&MyRect::bottom))
	};

	vector<MyEdge> edges={{0,1}, {1,2}, {2,3}};

	vector<int> edge_partition={0,1,2,3,3};

	auto adj_list=[&](int ri)->span<MyEdge>{
		int i=edge_partition[ri], j=edge_partition[ri+1]; 
		return span<MyEdge>(&edges[i], j-i);
	};

	auto rec_query_translation=[&](int ri, auto&& rec_query_translation)->int{
		span<MyEdge> adj = adj_list(ri);
		if (adj.empty())
		{
			int tr = frame.right - rects[ri].right;
			printf("tr[%d] = %d\n", ri, tr);
			return tr;
		}
		int tr = ranges::min(adj | views::transform([&](const MyEdge e){
					return rec_query_translation(e.j, rec_query_translation) + rects[e.j].left-rects[ri].right;
				}
			)
		);
		printf("tr[%d] = %d\n", ri, tr);
		return tr;
	};
	
	int tr = rec_query_translation(0, rec_query_translation);
	printf("tr = %d\n", tr);
}