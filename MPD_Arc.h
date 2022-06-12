/* MPD_Arc.h
*
* Copyright (c) 2005-2017 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _MPD_ARC_
#define _MPD_ARC_


struct MPD_Arc
{
	int _i ;
	int _j ;

	bool operator==(const MPD_Arc&) const = default;
} ;


#endif
