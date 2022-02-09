/* MPD_Arc.cpp
*
* Copyright (c) 2005-2015 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#include "MPD_Arc.h"
#include <string.h>
#include <vector>
using namespace std ;


vector<MPD_Arc> list_edges_(const vector<vector<MPD_Arc> >& graph)
{
	vector<MPD_Arc> edges ;
	for (const vector<MPD_Arc>& adj : graph){
		for (const MPD_Arc& arc : adj){
			edges.push_back(arc) ;
		}
	}
	return edges ;
}


vector<const MPD_Arc*> list_edges(const vector<vector<MPD_Arc> >& graph)
{
	vector<const MPD_Arc*> edges ;
	for (const vector<MPD_Arc>& adj : graph){
		for (const MPD_Arc& arc : adj){
			edges.push_back(&arc) ;
		}
	}
	return edges ;
}

vector<MPD_Arc*> list_edges(vector<vector<MPD_Arc> >& graph)
{
	vector<MPD_Arc*> edges ;
	for (vector<MPD_Arc>& adj : graph){
		for (MPD_Arc& arc : adj){
			edges.push_back(&arc) ;
		}
	}
	return edges ;
}



