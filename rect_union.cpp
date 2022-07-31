#include <algorithm>
#include <ranges>
#include <cmath>
using namespace std;


int main(int argc, char* argv[])
{
	int n=18;	//18 rectangles
//generate all subsets containing p=3 rectangles
	int p=3;
	views::iota(0, pow(n,p));

	return 0;
}
