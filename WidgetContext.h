/* WidgetContext.h
*
* Copyright (c) 2005-2017 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#ifndef _WIDGET_CONTEXT_
#define _WIDGET_CONTEXT_


#include "MyRect.h"
#include <vector>


enum WidgetType
{
	RECTANGLE,
	COMPOSITE_WIDGET	//Gang of four Composite Design Pattern
} ;


struct WidgetContext
{
	WidgetContext() ;
	WidgetContext(const MyRect &rec) ;	//provide automatic conversion from vector<MyRect> to vector<WidgetContext>  

	WidgetType type ;

//nested widgets
	std::vector<WidgetContext> widgets ;
	MyRect r ;

	operator MyRect() const {return r ;}	//provide automatic conversion from vector<WidgetContext> to vector<MyRect> 
} ;


std::vector<WidgetContext> collapse_composite(const std::vector<WidgetContext>& widgets, const MyPoint& translation={0,0}) ;


template <typename Fn>
void walk_composite(std::vector<WidgetContext>& widgets, Fn f)
{
	for (WidgetContext &widget : widgets)
	{
		switch (widget.type)
		{
		case COMPOSITE_WIDGET:
			walk_composite(widget.widgets, f) ;
		case RECTANGLE:
			f(widget) ;
		} 
	}
}


#endif
