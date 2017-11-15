/* index_from.h
*
* Copyright (c) 2005-2017 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _INDEX_FROM_
#define _INDEX_FROM_


#include <vector>


template <typename T, typename U>
inline int index_from(const std::vector<T> & v, const U& value)
{
	for (int i=0; i<v.size(); i++)
	{
		if (v[i] == value)
			return i ;
	}
	return -1 ;
}


template <typename T, typename T2, unsigned N>
int index_from(T (&tab)[N], const T2& value)
{
	for (int i=0; i < N; i++)
	{
		if (tab[i] == value)
			return i ;
	}
	return -1 ;
}


template <typename T, typename Pr>
int index_from_if(const std::vector<T>& tab, const Pr& pr)
{
	for (int i=0; i < tab.size(); i++)
	{
		if (pr(tab[i]))
			return i ;
	}
	return -1 ;
}


/*
Matlab like index() function. Did not find a way to do it with Eigen.
*/
template <typename T, typename Pr>
std::vector<int> index_if(const std::vector<T>& vec, Pr pr)
{
	std::vector<int> result ;
	for (int i=0; i < vec.size(); i++)
	{
		if (pr(vec[i]))
			result.push_back(i) ;
	}
	return result ;
}


#endif
