


void dim_swap(MyRect& r)
{
	swap(r.m_left, r.m_top);
	swap(r.m_right, r.m_bottom);
}

enum RectTrim
{
	SPLIT,
	CHIP,
	TRIM,
	CORNER
};

const int NR_TRIM_CONFIG=4;
const int NR_MIRRORING=3;

struct TrimSelector
{
	int rect_trim;
	int mirroring;
};

const vector<TrimSelector> trim_cartesian_product =...


/*      by
      +------+
      |      |
+=====+======+=====+
|     |      |     |
|     |      |     | r
|     |      |     |
+=====+======+=====+
      |      |
	  +------+ 
*/

void split(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (r.m_left < by.m_left && by.m_right < r.m_right && by.m_top < r.m_top && by.m_bottom > r.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=by.m_left, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom}
		};
	}
}

/*
+==================+
|                  |
|     +------+     | r
|     |      |     |
+=====+======+=====+
      |      |
	  +------+
         by	  
*/

void chip(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (r.m_left < by.m_left && by.m_right < r.m_right && by.m_bottom > r.m_bottom && by.m_top < r.m_bottom && by.m_top > r.m_top)
	{
		rects = {
			{.m_left=r.m_left, .m_right=by.m_left, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom},
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=by.m_top}
		};
	}
}

/*
      +==================+
      |                  |
      |                  | r
+-----+------------------+----+
|     +==================+    |
|                             |
+-----------------------------+
         by	  
*/
void trim(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (by.m_left < r.m_left && by.m_right > r.m_right && r.m_top < by.m_top && r.m_bottom > by.m_top && r.m_bottom < by.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, m_bottom=by.m_top}
		}
	}
}

/*
      +==================+
      |                  |
      |                  | r
+-----+-----+            |
|     +=====+============+
|           |
+-----------+
         by	  
*/
void corner(const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	if (by.m_left < r.m_left && by.m_right > r.m_left && by.m_right < r.m_right &&
		by.m_bottom > r.m_bottom && by.m_top > r.m_top && by.m_top < r.m_bottom)
	{
		rects = {
			{.m_left=r.m_left, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=by.m_top},
			{.m_left=by.m_right, .m_right=r.m_right, .m_top=r.m_top, .m_bottom=r.m_bottom}
		};
	}
}