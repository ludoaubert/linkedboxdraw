/* permutation.cpp
*
* Copyright (c) 2005-2015 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#include "permutation.h"
#include <vector>
#include <algorithm>
using namespace std ;


vector<int> compute_reverse_permutation(const vector<int>& permutation)
{
	int n = permutation.size() ;
	vector<int> result(n) ;
	for (int i=0; i < n; i++)
	{
		int pi = permutation[i] ;
		result[pi] = i ;
	}
	return result ;
}
