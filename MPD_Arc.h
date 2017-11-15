/* MPD_Arc.h
*
* Copyright (c) 2005-2017 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _MPD_ARC_
#define _MPD_ARC_


#include <vector>


struct MPD_Arc
{
	int _i ;
	int _j ;
} ;

bool operator==(const MPD_Arc& a, const MPD_Arc& b) ;

std::vector<const MPD_Arc*> list_edges(const std::vector<std::vector<MPD_Arc> >& graph) ;
std::vector<MPD_Arc*> list_edges(std::vector<std::vector<MPD_Arc> >& graph) ;
std::vector<MPD_Arc> list_edges_(const std::vector<std::vector<MPD_Arc> >& graph) ;


#endif
