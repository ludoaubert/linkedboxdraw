#ifndef _STAIR_STEPS_
#define _STAIR_STEPS_


#include "MPD_Arc.h"
#include "MyRect.h"
#include <vector>
#include <string>


const int max_nb_boxes_per_diagram=20;

struct Context
{
        std::vector<MyRect> rectangles ;
        std::vector<std::vector<MPD_Arc> > adjacency_list ;
        MyRect frame ;
        std::string title ;
} ;

void write_json(const std::vector<MyRect>& rectangles, const std::vector<Context>& contexts, char (&buffer)[100000]);

void test_stair_steps(int rect_border) ;
void test_stair_steps_layout() ;
void test_stair_steps_layout_from_111_boxes() ;
void test_expand_rectangles() ;

void select_neighbours(const std::vector<MPD_Arc>& edges, std::vector<bool>& filter) ;

void compute_contexts(std::vector<MyRect> &rectangles,
                      const std::vector<std::vector<MPD_Arc> > &adjacency_list,
                      int max_nb_boxes_per_diagram,
                      std::vector<Context> &contexts);

#endif
