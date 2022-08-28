#include <algorithm>
#include <ranges>
#include <cmath>
#include <array>
using namespace std;


int main(int argc, char* argv[])
{
	int n=18;	//18 rectangles
//generate all subsets containing p=3 rectangles
	const int p=3;
	auto rg = views::iota(0, pow(n,p)) | views::transform([&](int number){
		array<int, p> a;
		for (int i=0; i<p; i++)
			a[i] = (number / (int)pow(n,i)) % n;
		return a;
	}) | views::filter([](const array<int, p>& a){
		for (int i=0; i+1 < p; i++)
		{
			if (a[i] >= a[i+1])
				return false;
		}
		return true;
	});

	size_t size=0;

	for (const array<int, p>& a : rg)
	{
		size++;
		printf("[%d, %d, %d]\n", a[0], a[1], a[2]);
	}

	printf("size:%ld\n", size);

// size = 816
// n! / p! / (n-p)! = 18!/3!/15! = 18*17*16/6 = 3*17*16 = 816
	return 0;
}
