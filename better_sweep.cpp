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
}