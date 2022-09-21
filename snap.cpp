

/*
void dim_swap(MyRect& r)
{
	swap(r.m_left, r.m_top);
	swap(r.m_right, r.m_bottom);
}
*/

enum TrimAlgo
{
	SPLIT,
	CHIP,
	TRIM,
	CORNER
};

enum TrimMirrorDirection
{
	HORIZONTAL_MIRROR,
	VERTICAL_MIRROR,
	TILTED_MIRROR
};

const TrimMirrorDirection trim_mirror_directions[3] = {HORIZONTAL_MIRROR,VERTICAL_MIRROR,TILTED_MIRROR} ;

const MirroringState TrimMirrorSates[2] = {ACTIVE, IDLE};

const Mirror mirrors[NR_MIRRORING_OPTIONS][3]={
	{
		{.mirroring_state=IDLE, .mirroring_direction=HORIZONTAL_MIRROR},
		{.mirroring_state=IDLE, .mirroring_direction=VERTICAL_MIRROR}
	},
	{
		{.mirroring_state=IDLE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=IDLE, .mirroring_direction=NORTH_SOUTH}
	},
	{
		{.mirroring_state=ACTIVE, .mirroring_direction=EAST_WEST},
		{.mirroring_state=ACTIVE, .mirroring_direction=NORTH_SOUTH}
	}
};

const char* MirroringStrings[NR_MIRRORING_OPTIONS]={
	"IDLE,IDLE",
	"IDLE,ACTIVE",
	"ACTIVE,IDLE",
	"ACTIVE,ACTIVE"
};

const int NR_TRIM_CONFIG=4;
const int NR_MIRRORING=3;

struct TrimSelector
{
	int trim_algo;
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


void apply_trim(TrimAlgo trim_algo, const MyRect& r, const MyRect& by, vector<MyRect>& rects)
{
	switch (trim_algo)
	{
	case SPLIT:
		split(r, by, rects);
		break;
	case CHIP:
		chip(r, by, rects);
		break;
	case TRIM:
		trim(r, by, rects);
		break;
	case CORNER:
		corner(r, by, rects);
		break;
	}
}