//TODO: reserve memory upfront to hold sweep_line data.
//v2.reserve(25);

#include <vector>
#include <span>
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


struct MyRect{int left,right,top,bottom;};

struct MyEdge{
	int i, j;
	friend auto operator<=>(const MyEdge&, const MyEdge&) = default;
};


int main(){

	vector<MyRect> rects = {
		{0, 100, 50, 150},
		{150, 250, 0, 100},
		{300, 400, 50, 150},
		{450, 550, 100, 200},
		{0, 100, 150, 250},
		{150, 250, 150, 250}
	};
	
	int a[40]={4,5,3};
    int n=3;

    auto erase=[&](int i){
	   int& lower = *ranges::lower_bound(span(a,n), i, [&](int i, int j){return rects[i].left<rects[j].left;});
	   printf("lower = %d\n", lower);
        int pos = distance(a, &lower);
        printf("pos = %d\n", pos);
        for (int ii=pos; ii<n; ii++)
            swap(a[ii],a[ii+1]);
        n -= 1;
    };
	
	auto insert=[&](int i){
		int& upper = *ranges::upper_bound(span(a,n), i, [&](int i, int j){return rects[i].left<rects[j].left;});
		printf("upper = %d\n", upper);
        int pos = distance(a, &upper);
        printf("pos = %d\n", pos);
        for (int ii=n-1; ii>=pos; ii--)
            swap(a[ii],a[ii+1]);
        n += 1;
		a[pos]=i;
	};
	
	printf("initial status line:\n");
    for (int i : span(a,n))
    {
        printf("%d,", i);        
    }
    printf("\n");
	
	printf("erasing 5\n");

    erase(5);
	
	printf("erased 5\n");

    for (int i : span(a,n))
    {
        printf("%d,", i);        
    }
    printf("\n");
	
	printf("inserting 5\n");
	
	insert(5);
	
	printf("inserted 5\n");
	
    for (int i : span(a,n))
    {
        printf("%d,", i);        
    }
    printf("\n");	
	
	vector<MyEdge> edges, forbidden_edges, allowed_edges;
	edges.reserve(256);
	forbidden_edges.reserve(256);
	allowed_edges.reserve(256);
	
	for (int i=0; i+1 < n; i++)
	{
		edges.push_back({a[i], a[i+1]});
	}
	
	for (int i=0; i+2 < n; i++)
	{
		forbidden_edges.push_back({a[i], a[i+2]});
	}
	
	ranges::sort(edges);
	auto ret1 = ranges::unique(edges);
	edges.erase(ret1.begin(), ret1.end());

	ranges::sort(forbidden_edges);
	auto ret2 = ranges::unique(forbidden_edges);
	forbidden_edges.erase(ret2.begin(), ret2.end());

	ranges::set_difference(edges, forbidden_edges, back_inserter(allowed_edges));
	
	printf("edges:\n");
	for (auto [i, j] : edges)
	{
		printf("%d => %d\n", i, j);
	}
	printf("forbidden_edges:\n");
	for (auto [i, j] : forbidden_edges)
	{
		printf("%d => %d\n", i, j);
	}
	printf("allowed_edges:\n");
	for (auto [i, j] : allowed_edges)
	{
		printf("%d => %d\n", i, j);
	}
}