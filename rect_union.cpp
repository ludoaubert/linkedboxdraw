#include <algorithm>
#include <ranges>
#include <cmath>
#include <array>
using namespace std;


int main(int argc, char* argv[])
{
	int n=18;	//18 rectangles
//generate all subsets containing p=3 rectangles
	int p=3;
	auto rg = views::iota(0, pow(n,p)) | views::transform([&](int number){
		array<int, 3> a;
		for (int i=0; i<3; i++)
			a[i] = (number / (int)pow(n,i)) % n;
		return a;
	}) | views::filter([](const array<int, 3>& a){
		for (int i=0; i+1 < a.size(); i++)
		{
			if (a[i] >= a[i+1])
				return false;
		}
		return true;
	});

	for (const array<int, 3>& a : rg)
	{
		printf("[%d, %d, %d]\n", a[0], a[1], a[2]);
	}

	return 0;
}
