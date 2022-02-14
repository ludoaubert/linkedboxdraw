#include "fit_together.h"
#include "MyRect.h"
#include "WidgetContext.h"
#include "FunctionTimer.h"
#include <vector>
#include <unordered_set>
#include <stdio.h>
#include "latuile_test_json_output.h"
using namespace std ;

/* in this example: +-----> direction = EAST_WEST
                    |
                    |
                    V
                  normal_direction = NORTH_SOUTH

<-----------------A---------------->        <------------B----------->

+------------+
|            |
|            |
|            |                                      +-----------------+
|            |                                      |                 |
|            |                                      |                 |
+------------+                                      |                 |
      |         +---------+                         |                 |
+---------+     |         |                         +-----------------+
|         |     |         |                   +----------+       |
|         |     |         |                   |          |   +--------+
|         |-----|         |                   |          |   |        |
|         |     |         |                   |    r2    |---|        |
|         |     |         |                   |          |   |        |
|         |     |         |  +--------+       |          |   +--------+
+---------+     |         |  |   r1   |       +----------+       |
                |         |--|        |                      +--------+
		+---------+  +--------+                      |        |
	                                                     |        |
							     |        |
							     +--------+
*/


void test_fit_together()
{
        TestFunctionTimer ft("test_fit_together");

	vector<MyRect> A = {
		{84,237,80,224},
		{237,439,0,272},
		{237,418,272,560},
		{56,237,224,512},
		{237,467,560,656},
		{0,237,512,656},
		{467,690,224,656},
		{690,836,528,656}
	};

	vector<MyRect> B = {
		{1212,1449,272,496},
		{1038,1212,336,496},
		{975,1212,96,336},
		{836,1038,336,592},
		{1038,1205,496,656}
	};

        const MyPoint expected_translation = {-146,-64};

	MyPoint translation ;
	fit_together(A, B, EAST_WEST, translation) ;

	bool bOK = translation == expected_translation;
        printf("%s\n", bOK ? "OK" : "KO");
	(bOK ? nbOK : nbKO)++;
}


void fit_together(vector<WidgetContext*> (&composite_partition)[2], 
				  Direction direction,
				  vector<MyRect> &B,
				  MyPoint& translation_B,
				  int &diameter)
{
        FunctionTimer ft("fit_together");        

	vector<MyRect> A2[2] ;
	
	for (int LEG=0; LEG < 2; LEG++)
	{
		for (WidgetContext* widget_ctx : composite_partition[LEG])
		{
			for (WidgetContext &widg : widget_ctx->widgets)
			{
				if (widg.type == WidgetType::RECTANGLE)
				{
					A2[LEG].push_back(widg.r) ;
				}
			}
		}
	}

	fit_together(A2[0], A2[1], direction, translation_B) ;
	B = A2[1] ;
	for (MyRect& r : A2[1])
		translate(r, translation_B) ;
	diameter = rectangle_diameter(enveloppe(compute_frame(A2[0]), compute_frame(A2[1]))) ;
}



// Works on expanded rectangles.

void fit_together(vector<MyRect> A, 
		  vector<MyRect> B, 
		  Direction direction, 
		  MyPoint& translation_B)
{
	Direction normal_direction = transpose(direction) ;
	
	vector<MyPoint> candidate_translations ;
	
	for (const MyRect& r1 : A)
	{
		for (const MyRect& r2 : B)
		{
			MyPoint target, source ;

	// compute the translation to bring r2 above (resp. before) if direction==EAST_WEST (resp. NORTH_SOUTH) r1:
			value(target, normal_direction) = min(r1, normal_direction) ;
			value(target, direction) = min(r1, direction) ;
			value(source, direction) = min(r2, direction) ;
			value(source, normal_direction) = max(r2, normal_direction) ;
			candidate_translations.push_back(target - source) ;
			
	// compute the translation to bring r2 below (resp. after) if direction==EAST_WEST (resp. NORTH_SOUTH) r1:
			value(target, normal_direction) = max(r1, normal_direction) ;
			value(target, direction) = min(r1, direction) ;
			value(source, direction) = min(r2, direction) ;
			value(source, normal_direction) = min(r2, normal_direction) ;
			candidate_translations.push_back(target - source) ;
		}
	}
	
	MyPoint best_translation = {0,0} ;
	MyPoint best_dimension = dimensions(enveloppe(compute_frame(A), compute_frame(B))) ;
			
	for (MyPoint& translation : candidate_translations)
	{
// apply the translation to all elements of B
		vector<MyRect> Bt = B ;
		for (MyRect& r2 : Bt)
			translate(r2, translation) ;
			
		bool no_overlap = true ;
		for (const MyRect& r1 : A)
		{
			for (const MyRect& r2 : Bt)
			{
				if (intersect_strict(r1, r2))
					no_overlap = false ;
			}
		}

		MyRect env = enveloppe(compute_frame(A), compute_frame(Bt)) ;
		MyPoint dim = dimensions(env) ;
		
			
		if (no_overlap && dim.x <= best_dimension.x && dim.y <= best_dimension.y)
		{
			best_dimension = dim ;
			best_translation = translation ; 
		}
	}
	
	translation_B = best_translation ;
}
