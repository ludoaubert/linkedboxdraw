/* binpack.cpp
*
* Copyright (c) 2005-2015 Ludovic Aubert. ALL RIGHTS RESERVED.
* ludo.aubert@gmail.com
* This file should not be transmitted nor published.
*
*/
#include "binpack.h"
#include "MyRect.h"
#include "index_from.h"
#include "FunctionTimer.h"
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <stdio.h>
#include <cstdint>
#include "latuile_test_json_output.h"
using namespace std ;


struct BPNode
{
	BPNode(int w=0,int h=0,int x=0,int y=0): used(false)
	{
		assert (w >= 0) ;
		assert (h >= 0) ;
		this->w = w ;
		this->h = h ;
		this->x = x ;
		this->y = y ;
		suppress_id[0] = suppress_id[1] = -1 ;
	}
    int w,h,x,y ;
	bool used ;

	int id ;
	int suppress_id[2] ;
} ;


MyRect rect(const BPBlock& b)
{
	return {b.x, b.x + b.w, b.y, b.y + b.h} ;
}


MyRect rect(const BPNode& n)
{
	return {n.x, n.x+n.w, n.y, n.y+n.h} ;
}

BPNode node(const MyRect& r)
{
	return BPNode(width(r), height(r), r.m_left, r.m_top) ;
}


/*si aucun node ne peut contenir le block*/
void split_and_fit(BPBlock& b, vector<BPNode>& nodes)
{
	for (int i=0; i < nodes.size(); i++)
	{
		BPNode &ni = nodes[i] ;
		if (ni.used)
			continue ;
		BPBlock bb = b ;
//move bb to the top left corner of ni
		bb.x = ni.x ;
		bb.y = ni.y ;

		stack<MyRect> sub_rects ;
		sub_rects.push(rect(bb)) ;
//update a copy of the nodes. It will be committed back if successful.
		vector<BPNode> mynodes = nodes ;

		for (int j=0; !sub_rects.empty() && j < mynodes.size(); j++)
		{
			BPNode &nj = mynodes[j] ;
			if (nj.used)
				continue ;
			MyRect r = sub_rects.top() ;

			if (!intersect_strict(r, rect(nj)))
				continue ;

//there is an intersection

			sub_rects.pop() ;

			nj.used = true ;
			for (BPNode& n : mynodes)
			{
				if (index_from(nj.suppress_id, n.id) != -1)
					n.used = true ;
			}

			vector<MyRect> sub[2] ;
			symmetric_diff(r, rect(nj), sub) ;
//rectangle areas that are covered by r but not nj. These rectangles will have to be matched inside nodes later.
			for (MyRect &rec : sub[0])
				sub_rects.push(rec) ;
//rectangle areas that are covered by nj but not r. They correspond to new available areas 
			for (MyRect &rec : sub[1])
			{
				BPNode n = node(rec) ;
				n.id = mynodes.size() ;
				mynodes.push_back(n) ;
			}
			j=-1 ;
		}
		if (sub_rects.empty())
		{
//the block was successfully mapped onto nodes. commit the updated node array
			b = bb ;
			b.placed = true ;
			nodes = mynodes ;
			return ;
		}
	}
}

/*
2 possibilities for splitting the node:

# split1()
+----------+--------------------+
|          |                     |
|  block   |    node_right       |
|          |                     |
+----------+--------------------+
|                                |
|       node_down                |
+-------------------------------+

or
# split2()
+----------+--------------------+
|          |                     |
|  block   |                     |
|          |                     |
+----------+    node_right       |
|          |                     |
|node_down |                     |
+----------+--------------------+
*/


void split1(BPNode& node, const BPBlock& block, BPNode& node_down, BPNode& node_right)
{
	node_down = BPNode(node.w, node.h - block.h, node.x, node.y + block.h) ;
	node_right = BPNode(node.w - block.w, block.h, node.x + block.w, node.y) ;
}

void split2(BPNode& node, const BPBlock& block, BPNode& node_down, BPNode& node_right)
{
	node_down = BPNode(block.w, node.h - block.h, block.x, node.y + block.h) ;
	node_right = BPNode(node.w - block.w, node.h, node.x + block.w, node.y) ;
}


void binpack(vector<BPBlock>& blocks, int &w, int &h)
{
#ifdef _DEBUG
	FILE *f = fopen("bp.txt", "w") ;
	for (BPBlock& b : blocks)
		fprintf(f, "{%d,%d},\n", b.w, b.h) ;
	fclose(f) ;
#endif

	BPNode root_node(w, h) ;
	root_node.id = 0 ;
	vector<BPNode> nodes ;
	nodes.push_back(root_node) ;

	ranges::sort(blocks, ranges::greater(), [](BPBlock& b){return b.h*b.w; }) ;

	for (BPBlock& block : blocks)
	{
		ranges::sort(nodes, {}, [](BPNode& n){return n.w*n.h ;}) ;

		int n = nodes.size() ;

		for (int i=0; i < nodes.size(); i++)
		{
			if (!nodes[i].used && nodes[i].w >= block.w && nodes[i].h >= block.h)
			{
				block.placed = true ;
				block.x = nodes[i].x ;
				block.y = nodes[i].y ;

				nodes[i].used = true ;
				for (BPNode& n : nodes)
				{
					if (n.id == nodes[i].suppress_id[0] || n.id == nodes[i].suppress_id[1])
						n.used = true ;
				}

				BPNode node_down, node_right ;
				split1(nodes[i], block, node_down, node_right) ;

				node_down.suppress_id[0] = node_right.suppress_id[0] = n+2 ;
				node_down.suppress_id[1] = node_right.suppress_id[1] = n+3 ;

				assert (node_down.w >= 0 && node_down.h >= 0) ;
				node_down.id = n ;
				nodes.push_back(node_down) ;
				assert (node_right.w >= 0 && node_right.h >= 0) ;
				node_right.id = n+1 ;
				nodes.push_back(node_right) ;

				split2(nodes[i], block, node_down, node_right) ;

				node_down.suppress_id[0] = node_right.suppress_id[0] = n ;
				node_down.suppress_id[1] = node_right.suppress_id[1] = n+1 ;

				assert (node_down.w >= 0 && node_down.h >= 0) ;
				node_down.id = n+2 ;
				nodes.push_back(node_down) ;
				assert (node_right.w >= 0 && node_right.h >= 0) ;
				node_right.id = n+3 ;
				nodes.push_back(node_right) ;

				break ;
			}
		}

/*
2X2 possibilities for expanding root_node:
(case 1)
+----------------------------+---------+
|                             |         |
|                             | block   |
|        root_node            +---------+
|                             |         |
|                             |node_down|
+----------------------------+---------+

(case 3)
+----------------------------+---------+
|                             |         |
|                             |         |
|        root_node            | block   |
|                             |         |
|                             |         |
+----------------------------+         |
|         node_down           |         |
+----------------------------+---------+

or
(case 2)
+----------------------------+
|                             |
|                             |
|        root_node            |
|                             |
|                             |
+---------+------------------+
|         |                   |
| block   |   node_down       |
+---------+------------------+

(case 4)
+----------------------------+----------+
|                             |          |
|                             |          |
|        root_node            |node_right|
|                             |          |
|                             |          |
+----------------------------+----------+
|                                        |
|            block                       |
+---------------------------------------+
*/

		if (!block.placed)
		{
//before reserving a bigger frame, try to map the block onto several nodes. 
			split_and_fit(block, nodes) ;
		}

		if (!block.placed)
		{
			if (root_node.h + block.h > root_node.w + block.w)
			{
				if (root_node.h > block.h)
				{
//case 1:
					BPNode node_down(block.w, root_node.h - block.h, root_node.w, block.h) ;
					node_down.id = n ;
					nodes.push_back(node_down) ;
					block.x = root_node.w ;
					block.y = 0 ;
					block.placed = true ;
					root_node.w += block.w ;
				}
				else
				{
//case 3:
					BPNode node_down(root_node.w, block.h - root_node.h, 0, root_node.h) ;
					node_down.id = n ;
					nodes.push_back(node_down) ;
					block.x = root_node.w ;
					block.y = 0 ;
					block.placed = true ;
					root_node.w += block.w ;
					root_node.h = block.h ;
				}
			}
			else
			{
				if (root_node.w > block.w)
				{
//case 2:
					BPNode node_down(root_node.w - block.w, block.h, block.w, root_node.h) ;
					node_down.id = n ;
					nodes.push_back(node_down) ;
					block.x = 0 ;
					block.y = root_node.h ;
					block.placed = true ;
					root_node.h += block.h ;
				}
				else
				{
//case 4:
					BPNode node_right(block.w - root_node.w, root_node.h, root_node.w, 0) ;
					node_right.id = n ;
					nodes.push_back(node_right) ;
					block.x = 0 ;
					block.y = root_node.h ;
					block.placed = true ;
					root_node.w += node_right.w ;
					root_node.h += block.h ;
				}
			}
		}
	}

	w = root_node.w ;
	h = root_node.h ;
}


void test_binpack()
{
        TestFunctionTimer ft("test_binpack");

        struct TestContext { int testid; vector<MyRect> input_rectangles; int w,h; vector<MyRect> expected_rectangles;};

        const vector<TestContext> test_contexts = {
        	{	/*testid*/ 1,
			/*rectangles*/ {
				{0,90,0,230},
				{0,115,0,166},
				{0,186,0,129},
				{0,60,0,23},
				{0,280,0,15}
			},
                	/*w*/ 346,
                	/*h*/ 568,
                	/*expected_rectangles*/ {
				{0,90,129,359},
				{0,115,359,525},
				{0,186,0,129},
				{90,150,129,152},
				{0,280,525,540}
                	}
		},

		{	/*testid*/ 2,
			/*rectangles*/ {
				{0,181,0,56},
				{0,101,0,56},
				{0,234,0,120},
				{0,234,0,88},
				{0,128,0,56},
				{0,30,0,56},
				{0,20,0,50}
			},
              		/*w*/ 346,
                	/*h*/ 568,
                	/*expected_rectangles*/ {
				{0,181,208,264},
				{0,101,320,376},
				{0,234,0,120},
				{0,234,120,208},
				{0,128,264,320},
				{128,158,264,320},
				{158,178,264,314}
                	}
		},

		{	/*testid*/ 3,
			/*rectangles*/ {
				{0,147,0,104},
				{0,173,0,248},
				{0,162,0,152},
				{0,126,0,88},

				{0,114,0,56},
				{0,131,0,56},
				{0,116,0,88},
				{0,96,0,40},

				{0,66,0,56},
				{0,109,0,104},
				{0,104,0,72},
				{0,100,0,72},

				{0,147,0,72},
				{0,113,0,72},
				{0,162,0,40},
				{0,116,0,56}
			},
                	/*w*/ 0,
                	/*h*/ 0,
                	/*expected_rectangles*/ {
				{0,147,248,352},
				{0,173,0,248},
				{173,335,0,152},
				{173,299,152,240},
				{260,374,368,424},
				{260,391,312,368},
				{335,451,0,88},
				{162,258,424,464},
				{356,422,216,272},
				{147,256,248,352},
				{335,439,88,160},
				{256,356,240,312},
				{0,147,352,424},
				{147,260,352,424},
				{0,162,424,464},
				{299,415,160,216}
                	}
		}
	};

	for (auto [testid, input_rectangles, w,h, expected_rectangles] : test_contexts)
	{
		vector<MyRect> rectangles = input_rectangles;

		binpack(rectangles, w,h) ;

		vector<Edge> edges;

                latuile_test_json_output(input_rectangles,
					rectangles,
                                        edges,
                                        expected_rectangles,
                                        "binpack",
                                        testid);

		bool bOK = rectangles == expected_rectangles;
                printf("%s\n", bOK ? "OK" : "KO");
		(bOK ? nbOK : nbKO)++;
	}
}



void test_split_and_fit()
{
        TestFunctionTimer ft("test_split_and_fit");

	BPBlock block(3,2) ;

	vector<MyRect> rectangles = {
		{2,2,0,0},
		{2,2,2,0}
	} ;
	vector<BPNode> nodes ;
	for (MyRect &r : rectangles)
		nodes.push_back(BPNode(r.m_left, r.m_right, r.m_top, r.m_bottom)) ;
	split_and_fit(block, nodes) ;
}


void binpack(vector<MyRect>& rectangles, int& w, int& h)
{
	vector<BPBlock> blocks ;
	for (int i=0; i < rectangles.size(); i++)
	{
		MyRect& r = rectangles[i] ;
		blocks.push_back(BPBlock(width(r), height(r))) ;
		blocks[i].i = i ;
	}
	binpack(blocks, w, h) ;
	for (BPBlock &b : blocks)
	{
		MyRect &r = rectangles[b.i] ;
		int i = r.i ;						// information must be preserved (position into table name array)
		int no_sequence = r.no_sequence ;	// information must be preserved (used for html5 import of metadata.html by connected_rectangles.html)
		r = rect(b) ;
		r.i = i ;
		r.no_sequence = no_sequence ;
	}

	for (int iter=0; iter<2; iter++)
	{
		gravity(rectangles) ;
		for (MyRect &r : rectangles)
			rect_swap_dimensions(r) ;
		gravity(rectangles) ;
		for (MyRect &r : rectangles)
			rect_swap_dimensions(r) ;
	}

	collapse(rectangles) ;
	for (MyRect &r : rectangles)
		rect_swap_dimensions(r) ;
	collapse(rectangles) ;
	for (MyRect &r : rectangles)
		rect_swap_dimensions(r) ;
}


void collapse(vector<MyRect> &rectangles)
{
//ensuring max_element will return a valid result
	if (rectangles.empty())
		return ;

	vector<MyRect*> rects ;
	for (MyRect &r : rectangles)
		rects.push_back(&r) ;

	FILE *f = fopen("collapse.txt", "w") ;

	while (true)
	{
		MyRect * r1 = * ranges::max_element(rects, {}, [](MyRect* r){return r->m_right;}) ;

		vector<MyRect> neighboors ;

		for (MyRect *r2: rects)
		{
			if (r1==r2)
				continue ;

	/*         +-----+      +-----+
			   | r1  |      |  r1 |
		  +----+------------------+----+
		  | r1 |                  | r1  |
		  +----|        r2        |-----+
		  +----|                  |-----+
		  | r1 |                  | r1  |
		  +----+------------------+-----+
			   | r1 |        | r1 |
			   +----+        +----+
	*/

			int tab[8][2]={
				{r2->m_left-r1->m_left, r2->m_top-r1->m_bottom},
				{r2->m_left-r1->m_right, r2->m_top-r1->m_top},
				{r2->m_left-r1->m_right, r2->m_bottom-r1->m_bottom},
				{r2->m_left-r1->m_left, r2->m_bottom-r1->m_top},
				{r2->m_right-r1->m_right, r2->m_bottom-r1->m_top},
				{r2->m_right-r1->m_left, r2->m_bottom-r1->m_bottom},
				{r2->m_right-r1->m_left, r2->m_top-r1->m_top},
				{r2->m_right-r1->m_right, r2->m_top-r1->m_bottom}
			} ;

			for (int (&vec)[2] : tab)
			{
				MyRect r = *r1 ;
				MyPoint translation ;
				translation.x = vec[0] ;
				translation.y = vec[1] ;
	//TODO: C++11 initializer list => 'translate(r, vec)'
				translate(r, translation) ;
				neighboors.push_back(r) ;
			}
		}

//select neighboors that do not intersect or are inside rectangles
		vector<MyRect> selected_neighboors ;
		for (MyRect &rec1 : neighboors)
		{
			bool select = true ;
			for (MyRect& rec2 : rectangles)
			{
				if (intersect_strict(rec1, rec2) || is_inside(rec1, rec2))
					select = false ;
			}
			if (select)
			{
				selected_neighboors.push_back(rec1) ;
			}
		}
		neighboors.clear() ;

//expand selected neighboors: they are our computed available rectangles
		for (MyRect& rec1 : selected_neighboors)
		{
			for (Direction direction : directions)
			{
				Direction other_direction = transpose(direction) ;

				int16_t range_begin = -INT16_MAX ;
				int16_t range_end = INT16_MAX ;

				for (MyRect &rec2 : rectangles)
				{
					if (!range_intersect_strict(min(rec1, other_direction), max(rec1, other_direction), min(rec2, other_direction), max(rec2, other_direction)))
						continue ;

					if (max(rec1, direction) <= min(rec2, direction))
						range_end = min(range_end, min(rec2, direction)) ;

					if (min(rec1, direction) >= max(rec2, direction))
						range_begin = max(range_begin, max(rec2, direction)) ;
				}
/*
values of -INT_MAX or INT_MAX might be applied, which is used as a marker.
if for example rec1.m_right = INT_MAX => rec1 is on the right edge of the frame.
*/
				min(rec1, direction) = range_begin ;
				max(rec1, direction) = range_end ;
			}
		}

//remove duplicates.
		{
			ranges::sort(selected_neighboors) ;
			auto r = ranges::unique(selected_neighboors) ;
			selected_neighboors.resize(r.size()) ;
		}

		vector<MyRect> holes, right_edge ;

		for (MyRect &rec : selected_neighboors)
		{
			if (rec.m_bottom == INT16_MAX)
				;
			else if (rec.m_left == -INT16_MAX)
				;
			else if (rec.m_top == -INT16_MAX)
				;
			else if (rec.m_right == INT16_MAX)
				right_edge.push_back(rec) ;
			else
				holes.push_back(rec) ;
		}

		fprintf(f, "holes:\n") ;
		for (MyRect &h : holes)
			fprintf(f, "(%d,%d,%d,%d) widht=%d height=%d\n", h.m_left, h.m_right, h.m_top, h.m_bottom, width(h), height(h)) ;

		MyRect *selected_hole=0, *selected_horizontal_strip=0, *selection=0 ;
		ranges::sort(holes, ranges::greater(), [](MyRect& r){return width(r)*height(r);}) ;
		for (MyRect &hole : holes)
		{
			if (width(hole)>=width(*r1) && height(hole)>=width(*r1))
			{
				selected_hole = &hole ;
				break ;
			}
		}
		ranges::sort(right_edge, {}, &MyRect::m_left) ;
		for (MyRect &strip : right_edge)
		{
			if (height(strip) >= height(*r1) && strip.m_left < r1->m_left)
			{
				selected_horizontal_strip = &strip ;
				break ;
			}
		}

//take the strip if it is available. Not sure if the strategy is good
		selection = selected_horizontal_strip ? selected_horizontal_strip : selected_hole ;

		if (selection==0)
//could not find an improvement for r1. break the improvement search loop
			break ;

		MyPoint translation ;
		translation.x = selection->m_left - r1->m_left ;
		translation.y = selection->m_top - r1->m_top ;
		fprintf(f, 
			"(%d,%d,%d,%d) width=%d height=%d translated from (%d,%d)\n", 
			r1->m_left, r1->m_right, r1->m_top, r1->m_bottom, 
			width(*r1), height(*r1), 
			translation.x, translation.y) ;
		translate(*r1, translation) ;
	}

	fclose(f) ;
}

void gravity(vector<MyRect> &rectangles)
{
	vector<MyRect*> rects ;
	for (MyRect &r : rectangles)
		rects.push_back(&r) ;

	ranges::sort(rects, {}, [](MyRect* r){return r->m_left;}) ;

	for (MyRect *r1 : rects)
	{
		int16_t right_max = -INT16_MAX ;
		for (MyRect *r2 : rects)
		{
			if (r1 == r2)
				continue ;
			if (range_intersect_strict(r1->m_top, r1->m_bottom, r2->m_top, r2->m_bottom) && r2->m_right <= r1->m_left)
			{
				right_max = max(right_max, r2->m_right) ;
			}
		}

		if (right_max != -INT16_MAX)
		{
			MyPoint translation ;
			translation.x = right_max - r1->m_left ;
			translation.y = 0 ;
			translate(*r1, translation) ;
		}
	}
}
