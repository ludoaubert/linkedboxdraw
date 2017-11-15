/* Arc.cpp
*
* Copyright (c) 2005-2015 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#include "WidgetContext.h"
#include "MyRect.h"
#include <algorithm>
#include <vector>
#include <iterator>
using namespace std ;

WidgetContext::WidgetContext()
{
}


WidgetContext::WidgetContext(const MyRect &rec):
  type(WidgetType::RECTANGLE),
  r(rec)
{}


vector<WidgetContext> collapse_composite(const vector<WidgetContext>& widgets, const MyPoint& translation)
{
	vector<WidgetContext> result, sub ;

	for (WidgetContext widget : widgets)
	{
		switch (widget.type)
		{
		case COMPOSITE_WIDGET:
			sub = collapse_composite(widget.widgets, translation + MyPoint{widget.r.m_left, widget.r.m_top}) ;
			copy(sub.begin(), sub.end(), back_inserter(result));
			break ;
		case RECTANGLE:
			translate(widget.r, translation) ;
			result.push_back(widget) ;
			break ;
		} 
	}

	return result ;
}
