/**
* \copyright Copyright (c) 2015-2021 Ludovic Aubert
*            All Rights Reserved
*            DO NOT ALTER OR REMOVE THIS COPYRIGHT NOTICE
*
* \file bombix.cpp
*
* - 11/23/2016 by Ludovic Aubert : creation
*/
#include <cstdint>
#include <vector>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <assert.h>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <chrono>
#include <regex>
//#include <omp.h>
using namespace std;
using namespace std::chrono;

/*
Convention for geometry:
  +------> x
  |
  |
  |
  V
  y
  
Convention for matrix:
  +------> j
  |
  |
  |
  V
  i
  
i (resp. j) corresponds to y (resp. x), which might seem surprising.
*/


enum Direction : uint16_t
{
	HORIZONTAL,
	VERTICAL
};

const char* dir[2] = { "HORIZONTAL", "VERTICAL"};
const char* way_string[3] = {"DECREASE",0,"INCREASE"};

Direction other(Direction direction)
{
	switch (direction)
	{
	case HORIZONTAL:
		return VERTICAL;
	case VERTICAL:
		return HORIZONTAL;
	}
}

enum Way : int16_t
{
	DECREASE=-1,
	INCREASE=+1
};

void reverse(Way &way)
{
	switch (way)
	{
	case INCREASE:
		way = DECREASE;
		break;
	case DECREASE:
		way = INCREASE;
		break;
	}
}

struct Coord
{
	int16_t i,j;
	int16_t& operator[](Direction direction)
	{
		switch (direction)
		{
		case HORIZONTAL:
			return j;
		case VERTICAL:
			return i;
		}
	}

	int16_t operator[](Direction direction) const
	{
		switch (direction)
		{
		case HORIZONTAL:
			return j;
		case VERTICAL:
			return i;
		}
	}
};

struct Span
{
	int min, max;
};


struct Range;

struct RangeProjection
{
	RangeProjection& operator=(const Span& s);
	operator Span() const;
	Range *r;
	Direction direction;
};

struct RangeExtremity
{
	RangeExtremity& operator=(const Coord& c);
	RangeExtremity& operator+=(Way way);
	operator Coord() const;
	Range *r;
	Way way;
};

struct Range
{
	Direction direction;
	Way way;
	int16_t value, min, max;
	
	RangeExtremity operator[](Way way)
	{
		return RangeExtremity{this, way};
	}

	RangeProjection operator[](Direction dir)
	{
		assert(direction == other(dir));
		return RangeProjection{this, dir};
	}
};

RangeExtremity& RangeExtremity::operator+=(Way way)
{
	switch (way)
	{
	case INCREASE:
		r->max += way;
		break;
	case DECREASE:
		r->min += way;
		break;
	}
	return *this;
}

RangeExtremity& RangeExtremity::operator=(const Coord& c)
{
	r->value = c[r->direction];
	switch (way)
	{
	case INCREASE:
		r->max = c[other(r->direction)];
		break;
	case DECREASE:
		r->min = c[other(r->direction)];
		break;
	}
	return *this;
}

RangeExtremity::operator Coord() const
{
	Coord c;
	c[r->direction] = r->value;
	switch (way)
	{
	case INCREASE:
		c[other(r->direction)] = r->max;
		break;
	case DECREASE:
		c[other(r->direction)] = r->min;
		break;
	}
	return c;
}

RangeProjection::operator Span() const 
{
	assert(direction==other(r->direction));
	 return Span{r->min, r->max};
}

RangeProjection& RangeProjection::operator=(const Span& s)
{
	r->min = s.min;
	r->max = s.max;
	return *this;
}


//TODO: impl par default

bool operator==(const Range& r1, const Range& r2)
{
	return memcmp(&r1, &r2, sizeof(Range)) == 0;
}

bool operator!=(const Range& r1, const Range& r2)
{
	return memcmp(&r1, &r2, sizeof(Range)) != 0;
}

struct Maille
{
	Direction direction;
	Way way;
	int16_t i, j;
	
	int16_t& operator[](Direction input_direction)
	{
		switch (input_direction)
		{
		case HORIZONTAL:
			return j;
		case VERTICAL:
			return i;
		}
	}
	int16_t operator[](Direction input_direction) const
	{
		switch (input_direction)
		{
		case HORIZONTAL:
			return j;
		case VERTICAL:
			return i;
		}
	}

};

//TODO: impl par default

bool operator==(const Maille& m1, const Maille& m2)
{
	return memcmp(&m1, &m2, sizeof(Maille)) == 0;
}

bool operator!=(const Maille& m1, const Maille& m2)
{
	return memcmp(&m1, &m2, sizeof(Maille)) != 0;
}

static_assert(sizeof(Maille)==sizeof(uint64_t),"");

uint64_t serialize(const Maille& m)
{
	assert(m.i <= UINT8_MAX);
	assert(m.j <= UINT8_MAX);
	uint64_t u = 1 + (m.i << 1) + (m.j << (8 + 1)) + (m.direction << (8 + 8 + 1)) + ((m.way + 1) << (8 + 8 + 1 + 1));
	assert(u < 1000 * 1000);
	return u;
}

Maille parse(uint64_t u)
{
	u -= 1;
	u >>= 1;
	Maille m;
	m.i = u & 0xFF;
	assert(m.i <= UINT8_MAX);
	u >>= 8;
	m.j = u & 0xFF;
	assert(m.j <= UINT8_MAX);
	u >>= 8;
	m.direction = (Direction)(u & 0x01);
	u >>= 1;
	m.way = (Way)((u & 0x03)-1);
	return m;
}

struct Edge
{
	uint64_t u,v;
	int weight;
};

struct Point
{
	int& operator[](Direction direction)
	{
		switch (direction)
		{
		case HORIZONTAL:
			return x;
		case VERTICAL:
			return y;
		}
	}
	int x, y;
};

//TODO: impl par default

bool operator==(const Point& p1, const Point& p2)
{
	return memcmp(&p1, &p2, sizeof(Point))==0;
}

bool operator!=(const Point& p1, const Point& p2)
{
	return memcmp(&p1, &p2, sizeof(Point)) != 0;
}


struct Rect;

struct RectangleProjection
{
	operator Span() const;
	RectangleProjection& operator=(const Span& s);
	Direction direction;
	Rect* r=0;
};


struct Rect
{
	RectangleProjection operator[](Direction direction)
	{
		return RectangleProjection{direction, this};
	}
	const RectangleProjection operator[](Direction direction) const
	{
		return (const RectangleProjection){direction, const_cast<Rect*>(this)};
	}
	int left, right, top, bottom;
};

RectangleProjection::operator Span() const
{
	switch (direction)
	{
	case HORIZONTAL:
		return Span{r->left, r->right};
	case VERTICAL:
		return Span{r->top, r->bottom};
	}
}


RectangleProjection& RectangleProjection::operator=(const Span& s)
{
	switch (direction)
	{
	case HORIZONTAL:
		r->left = s.min;
		r->right = s.max;
		break;
	case VERTICAL:
		r->top = s.min;
		r->bottom = s.max;
		break;
	}
	return *this;
}


Span intersection(const Span& r1, const Span& r2)
{
	Span r = r1;
	r.min = max<int>(r.min, r2.min);
	r.max = min<int>(r.max, r2.max);
	
#if 0
	FILE* f = fopen("/tmp/intersection_io_log.txt","a");	
	fprintf(f, "r1:{min=%d, max=%d}, r2:{min=%d, max=%d}, r:{min:%d, max:%d}\n", r1.min, r1.max, r2.min, r2.max, r.min, r.max);
	fclose(f);
#endif	
	
	return r;
}

template <typename T>
struct Matrix
{
	Matrix() {}
	Matrix(int n, int m) : _n(n), _m(m)
	{
		_data = new T[n*m];
	}
	Matrix(int n, int m, T value) : _n(n), _m(m)
	{
		_data = new T[n*m];
		fill(_data, _data + n*m, value);
	}
	
	Matrix(const Matrix& m) : Matrix(m._n, m._m, T())
	{
		memcpy(_data, m._data, sizeof(T)*_n*_m);
	}
	
	Matrix& operator=(const Matrix& m)
	{
		_n = m._n;
		_m = m._m;
		_data = new T[_n*_m];
		memcpy(_data, m._data, _n*_m*sizeof(T));
		return *this;
	}
	
	~Matrix()
	{
		delete [] _data;
	}
	
	T& operator()(int i, int j)
	{
		assert(0 <= i);
		assert(i < _n);
		assert(0 <= j);
		assert(j < _m);
		return _data[i*_m + j];
	}
	

	T operator()(int i, int j) const
	{
		assert(0 <= i);
		assert(i < _n);
		assert(0 <= j);
		assert(j < _m);
		return _data[i*_m + j];
	}
	
	int dim(Direction direction) const
	{
		switch (direction)
		{
		case HORIZONTAL:
			return _m;
		case VERTICAL:
			return _n;
		}
	}
	
	bool isdefined(int i, int j) const
	{
		if (i < 0)
			return false;
		if (i >= _n)
			return false;
		if (j < 0)
			return false;
		if (j >= _m)
			return false;
		return true;		
	}
	
	int _n=0, _m=0;
	T *_data = nullptr;
};


struct Graph
{
	const Matrix<bool> &definition_matrix;
	const Matrix<Span> (&range_matrix)[2];	
	const vector<int> (&coords)[2];
};

enum InputOutputSwitch
{
	INPUT,
	OUTPUT
};

struct Target
{
	int from;
	int to;
	vector<Maille> expected_path;
};

//TODO: implementation par default

bool operator==(const Target& t1, const Target& t2)
{
	return t1.from == t2.from && t1.to == t2.to && t1.expected_path == t2.expected_path;
}

struct Polyline
{
	int from;
	int to;
	vector<Point> data;
};

//TODO: impl par default

bool operator==(const Polyline& p1, const Polyline& p2)
{
	return p1.from == p2.from && p1.to == p2.to && p1.data == p2.data;
}

bool operator!=(const Polyline& p1, const Polyline& p2)
{
	return p1.from != p2.from || p1.to != p2.to || p1.data != p2.data;
}

struct Link
{
	int from, to;
};

//TODO: impl par default

bool operator==(const Link& lk1, const Link& lk2)
{
	return memcmp(&lk1, &lk2, sizeof(Link)) == 0;
}

//TODO: could use a default generated impl ? C++23 ?
namespace std {

	template <>
	struct hash<Maille> {
		size_t operator()(const Maille &m) const
		{
			uint64_t u = serialize(m);
			return hash<uint64_t>()(u);
		}
	};
	
	template <>
	struct hash<Link> {
		size_t operator()(const Link &lk) const
		{
			const int k = 31;
			return lk.from + k * lk.to;
		}
	};
	
	template <>
	struct hash<Range> {
		size_t operator()(const Range &r) const
		{
			const int k = 31;
		//TODO: use destructuring
			return r.direction + k * r.way + k ^ 2 * r.value + k ^ 3 * r.min + k ^ 4 * r.max;
		}
	};
	
	template <>
	struct hash<Point> {
		size_t operator()(const Point &p) const
		{
			const int k = 31;
			return p.x + k * p.y;
		}
	};
}



struct FaiceauOutput
{
	vector<Target> targets;
	unordered_map<Maille, Range> enlarged;
};

//TODO: generer une impl par default

bool operator==(const FaiceauOutput& f1, const FaiceauOutput& f2)
{
	return f1.targets == f2.targets && f1.enlarged == f2.enlarged;
}


struct FaiceauPath
{
	unordered_map<Maille, Range> enlarged;
	vector<Maille> path;
};

struct TestContext
{
	int testid;
	vector<Rect> rects;
	Rect frame;
	vector<Link> links;
	
	vector<FaiceauOutput> faisceau_output;
	vector<Polyline> polylines;
};

struct InnerRange
{
	int16_t min, max;
	int32_t range_index;
};

static_assert(sizeof(InnerRange) == sizeof(uint64_t), "");

struct InnerRangeGraph
{
	const vector<Range> &path;
	const Matrix<bool> &definition_matrix;
	const vector<int> (&coords)[2];
};

struct OuterRangeGraph
{
	const vector<Range> &path;
	const Matrix<bool> &definition_matrix;
	const vector<int> (&coords)[2];
};

uint64_t serialize(const InnerRange& ir)
{
	uint64_t u = *(uint64_t*)&ir;
	u += 1;
	return u;
}

InnerRange parse_ir(uint64_t u)
{
	u -= 1;
	return *(InnerRange*)&u;
}


void print(const vector<Polyline>& polylines, string& serialized)
{
	char buffer[100 * 1024];
	int pos = 0;
	
	pos += sprintf(buffer + pos, "\t\t/*polylines*/ {\n");
	for (const auto& [from, to, data] : polylines)
	{
		pos += sprintf(buffer + pos, "\t\t\t{\n");
		pos += sprintf(buffer + pos, "\t\t\t\t/*from*/%d,\n", from);
		pos += sprintf(buffer + pos, "\t\t\t\t/*to*/%d,\n", to);
		pos += sprintf(buffer + pos, "\t\t\t\t/*data*/{");
		
		for (const auto& [x,y] : data)
		{
			pos += sprintf(buffer + pos, "{%d, %d},", x, y);
		}
		pos--;
		pos += sprintf(buffer + pos, "}\n");
		pos += sprintf(buffer + pos, "\t\t\t},\n");
	}
	pos += sprintf(buffer + pos, "\t\t}\n");
	serialized = buffer;
}

void print(const vector<FaiceauOutput>& faiceau_output, string& serialized)
{
	char buffer[100 * 1024];
	int pos = 0;
	
	pos += sprintf(buffer + pos, "\t\t/*faiceau output*/{\n");
	
	for (const /*FaiceauOutput*/auto& [targets, enlarged] : faiceau_output)
	{
		pos += sprintf(buffer + pos, "\t\t\t{\n");
		pos += sprintf(buffer + pos, "\t\t\t\t/*targets*/{\n");
		
		for (const /*Target*/ auto& [from, to, expected_path] : targets)
		{
			pos += sprintf(buffer + pos, "\t\t\t\t\t{\n");
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t/*from*/%d,\n", from);
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t/*to*/%d,\n", to);
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t/*expected path*/{\n");
			
			for (const Maille& m : expected_path)
			{
				pos += sprintf(buffer + pos, "\t\t\t\t\t\t\t{%s, %s, %hu, %hu},\n", dir[m.direction], way_string[1+m.way], m.i, m.j);
			}
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t}\n");
			pos += sprintf(buffer + pos, "\t\t\t\t\t}\n");
		}
		
		pos += sprintf(buffer + pos, "\t\t\t\t},\n");
		
		pos += sprintf(buffer + pos, "\t\t\t\t/*enlarged*/{\n");

		for (const /*pair<Maille, Range>*/ auto& [m, r] : enlarged)
		{
			pos += sprintf(buffer + pos, "\t\t\t\t\t{{%s,%s,%hu,%hu},{%s,%s,%hu,%hu,%hu}},\n", dir[m.direction], way_string[1+m.way], m.i, m.j, dir[r.direction], way_string[1+r.way], r.value, r.min, r.max);
		}
		pos += sprintf(buffer + pos, "\t\t\t\t},\n");
		
		pos += sprintf(buffer + pos, "\t\t\t},\n");
	}
	
	pos += sprintf(buffer + pos, "\t\t}\n");
	
	serialized = buffer;
}


vector<Edge> adj_list(const Graph& graph, const vector<Edge>& predecessor, uint64_t u)
{
	vector<Edge> adj;

	const int TURN_PENALTY = 1;
	const int MIN_CORRIDOR_WIDTH = 5;
	const int NARROW_CORRIDOR_PENALTY = 1000;
	const int WITHIN_RECTANGLE_PENALTY = 1000;
	
	const auto& [definition_matrix, range_matrix, coords] = graph;
	
	Maille r = parse(u);
	
	Maille next = r;
	next[next.direction] += next.way;
		
	if (definition_matrix.isdefined(next.i, next.j))
	{
		int distance = 0;
		
		uint64_t v = serialize(next);
		
		for (Maille* m : {&r, &next})
		{
			if (definition_matrix(m->i, m->j) == false)
				distance += WITHIN_RECTANGLE_PENALTY;		
			
			auto& tab = coords[m->direction];
			int16_t value = (*m)[m->direction];
			distance += tab[value+1] - tab[value];
		}

		const Matrix<Span> &rm = range_matrix[next.direction];
		Span span = rm(next.i, next.j);
		uint64_t w = u;
		while (w)
		{
			Maille m = parse(w);
			if (m.direction != next.direction)
				break;
			Span sp = rm(m.i, m.j);
			span = intersection(span, sp);
			const Edge& edge = predecessor.at(w);
			assert(edge.v == w);
			w = edge.u;
		}
		
		const vector<int>& c = coords[other(next.direction)];
		int range_width = c[span.max+1] - c[span.min];
		if (range_width < MIN_CORRIDOR_WIDTH)
		{
			printf("detected narrow corridor range_witdh=%d at location (i=%hu, j=%hu, direction=%s, way=%s).\n", 
					range_width, next.i, next.j, dir[next.direction], way_string[next.way+1]);
			distance += NARROW_CORRIDOR_PENALTY;
		}
		
		adj.push_back({u, v, distance});
	}
	
	for (Way way : { DECREASE, INCREASE})
	{
		Maille next = r;
		next.direction = other(r.direction);
		next.way = way;
		
		int distance = TURN_PENALTY;
		
		if (definition_matrix(r.i, r.j) == false)
			distance += 2 * WITHIN_RECTANGLE_PENALTY;		
		
		uint64_t v = serialize(next);
		adj.push_back({ u, v, distance });
	}
	
	return adj;
}


int compute_distance(const InnerRange &next, const vector<Range>& path, const vector<int> (&coords)[2])
{
	const int FACTOR = 1000 * 100;
	const Range& next_r = path[next.range_index];
	auto& tab1 = coords[next_r.direction];
	float num = tab1[next_r.value + 1] - tab1[next_r.value];
	auto& tab2 = coords[1 - next_r.direction];
	float denom = 1 + tab2[next.max+1] - tab2[next.min];
	int distance = num * FACTOR / denom;
	return distance;
}

template <typename Graph, typename PredecessorMap>
vector<Edge> adj_list(const Graph& graph, const PredecessorMap &predecesor, uint64_t u)
{
	static_assert(is_same<Graph, InnerRangeGraph>::value || is_same<Graph, OuterRangeGraph>::value, "");
	
	const auto& [path, definition_matrix, coords] = graph;
	
	InnerRange ir = parse_ir(u);
	
	vector<Edge> adj;
	
	if (ir.range_index + 1 == path.size())
		return adj;

	const Range &r = path[ir.range_index], &next_r = path[ir.range_index + 1];
	
	if (next_r.direction == r.direction)
	{
		InnerRange next = ir;
		next.range_index++;
		uint64_t v = serialize(next);
		adj.push_back({ u, v, compute_distance(next, path, coords)});
	}
	else
	{
		Rect rec = {0,0,0,0};
		rec[other(r.direction)] = Span{ ir.min, ir.max };
		
		struct Bound
		{
			int16_t min, max;
		};
		
		vector<Bound> bounds;

		if constexpr (is_same<Graph, InnerRangeGraph>::value)
		{
			for (int16_t min = next_r.min; min <= next_r.max; min++)
			{
				for (int16_t max = min; max <= next_r.max; max++)
				{
					bounds.push_back({ min, max});
 				}
			}
		}
		if constexpr (is_same<Graph, OuterRangeGraph>::value)
		{
			bounds.push_back({ next_r.min, next_r.max });
		}
	
		for (const auto& [min, max] : bounds)
		{	
			rec[other(next_r.direction)] = Span{ min, max};
				
			bool detect = false;
			for (int j = rec.left; j <= rec.right; j++)
			{
				for (int i = rec.top; i <= rec.bottom; i++)
				{
				//detect si n'est pas definie
					detect |= !definition_matrix(i, j);
				}
			}
			if (!detect)
			{
				InnerRange next = { min, max, ir.range_index + 1 };
				uint64_t v = serialize(next);
				adj.push_back({ u, v, compute_distance(next, path, coords) });
			}
		}
	}
	
	return adj;
}


int binary_search(const vector<int>& v, int val)
{
	return distance(begin(v), lower_bound(begin(v), end(v), val));
}


Rect index(const vector<int>(&coords)[2], const Rect& r)
{
	uint16_t left = binary_search(coords[HORIZONTAL], r.left);
	uint16_t right = binary_search(coords[HORIZONTAL], r.right);
	uint16_t top = binary_search(coords[VERTICAL], r.top);
	uint16_t bottom = binary_search(coords[VERTICAL], r.bottom);
	
	return { left, right - 1, top, bottom-1 };
}


Matrix<bool> compute_definition_matrix(const vector<Rect>& rects, const vector<int> (&coords)[2])
{
	Matrix<bool> m(coords[1].size() - 1, coords[0].size() - 1, true);
	for (const Rect& r : rects)
	{
		auto [left, right, top, bottom] = index(coords, r);
		
		for (int j = left; j <= right; j++)
		{
			for (int i = top; i <= bottom; i++)
			{
				m(i,j) = false;
			}
		}
	}
	
	return m;
}

Way input_output(InputOutputSwitch input_output_switch, Way normale)
{
	switch (input_output_switch)
	{
	case INPUT:
		return Way(-normale);
	case OUTPUT:
		return Way(normale);
	}
}


unordered_set<uint64_t> compute_nodes(const vector<int> (&coords)[2], const Matrix<bool>& definition_matrix, const Rect& r, InputOutputSwitch ioswitch)
{
	unordered_set<Maille> result;
	
	auto [left, right, top, bottom] = index(coords, r);
	
	for (int16_t j = left; j <= right; j++)
	{
		result.insert({VERTICAL, input_output(ioswitch, DECREASE), (int16_t)top, j});
		result.insert({VERTICAL, input_output(ioswitch, INCREASE), (int16_t)bottom, j});
	}
	
	for (int16_t i = top; i <= bottom; i++)
	{
		result.insert({HORIZONTAL, input_output(ioswitch, DECREASE), i, (int16_t)left});
		result.insert({HORIZONTAL, input_output(ioswitch, INCREASE), i, (int16_t)right});
	}
	
	unordered_set<uint64_t> defined;
	
	for (const Maille& m : result)
	{
		defined.insert(serialize(m));
	}
	return defined;
}

void add_rect(vector<int>(&coords)[2], const Rect& r, int nblink=1)
{
	if (nblink == 0)
		nblink = 1;
	for (int i=0; i <= nblink; i++)
	{
		coords[HORIZONTAL].push_back(r.left + i * (r.right - r.left) / nblink);
		coords[VERTICAL].push_back(r.top + i * (r.bottom - r.top) / nblink);
	}
}

struct QueuedEdge
{
	uint64_t u,v;
	int weight;
	int distance_v;
};

/*
It is important to have a strong ordering here. (!(A < B) and !(B < A)) => A=B.
Before Mai 18th 2018, order used to be e1.distance_v > e2.distance_v, and so it was not deterministic
in case e1 != e2 having e1.distance_v = e2.distance_v. It led to some tricking testing issues. The tests were
all OK on 32 bit platforms but some were KO on 64 bit platforms.
*/

//TODO: generer une impl par default

bool operator<(const QueuedEdge& e1, const QueuedEdge& e2)
{		
	if (e1.distance_v != e2.distance_v)
		return e1.distance_v > e2.distance_v;
	else if (e1.u != e2.u)
		return e1.u > e2.u;
	else if (e1.v != e2.v)
		return e1.v > e2.v;
	else
		return e1.weight > e2.weight;
};


struct Distance
{
	operator int64_t&()
	{
		return i;
	}
	Distance& operator=(int64_t ii){
		i=ii;
		return *this;
	}

	int64_t i= INT32_MAX;
};


template <typename GraphStruct, typename DistanceMap, typename PredecessorMap>
void dijkstra(const GraphStruct& graph, const unordered_map<uint64_t, int> &source_node_distance, 
			DistanceMap &distance, PredecessorMap & predecessor)
{	
	static_assert(is_same<DistanceMap, vector<int> >::value || is_same<DistanceMap, unordered_map<uint64_t, Distance> >::value, "");
	static_assert(is_same<PredecessorMap, vector<Edge> >::value || is_same<PredecessorMap, unordered_map<uint64_t, Edge> >::value, "");

	distance[0] = 0;
	
	priority_queue<QueuedEdge> Q;
	
	for (const auto& [u, distance_v] : source_node_distance)
	{
		int weight = distance_v;
		Q.push({ 0, u, weight, distance_v});
	}
	
	while (!Q.empty())
	{
		QueuedEdge queued_edge = Q.top();
		Q.pop();
		
		if (queued_edge.distance_v < distance[queued_edge.v])
		{
			predecessor[queued_edge.v] = { queued_edge.u, queued_edge.v, queued_edge.weight};
			distance[queued_edge.v] = queued_edge.distance_v;
			
			for (const Edge& adj_edge : adj_list(graph, predecessor, queued_edge.v))
			{
				int distance_v = distance[adj_edge.u] + adj_edge.weight;
				if (distance_v < distance[adj_edge.v])
				{
					Q.push({adj_edge.u, adj_edge.v, adj_edge.weight, distance_v});
				}
			}
		}
	}
}


void compute_target_candidates(const unordered_set<uint64_t> &source_nodes,
								const unordered_set<uint64_t> &target_nodes,
								const vector<int> &distance,
								const vector<Edge> &predecessor,
								vector<uint64_t> &target_candidates)
{	
	uint64_t u = *min_element(begin(target_nodes), end(target_nodes), [&](uint64_t u, uint64_t v){return distance.at(u) < distance.at(v);});

	copy_if(begin(target_nodes),
		end(target_nodes),
		back_inserter(target_candidates),
		[&](uint64_t v){
			return distance.at(v) == distance.at(u);
		}
	);

	sort(begin(target_candidates), end(target_candidates));
}

vector<Maille> parse_optimal_path(const vector<Edge>& optimal_path)
{
	vector<Maille> result;
	for (const Edge& edge : optimal_path)
		result.push_back(parse(edge.v));
	return result;
}

vector<Range> enlarge(const vector<Range>& path, const Matrix<bool>& m, const Rect& rfrom, const Rect& rto)
{
	vector<Range> result;
	
	for (int i=0; i < path.size();)
	{
		vector<int> index_range;
		
		int j = i;
		while (j < path.size() && path[i].direction == path[j].direction)
		{
			index_range.push_back(j);
			j++;
		}
		
	//TODO: use destructuring
	
		vector<Range> ranges;
		for (int k = i; k < j; k++)
		{
			const Range &r = path[k];
			ranges.push_back(r);
		}
		
		for (Way way : {DECREASE, INCREASE})
		{
			if (all_of(begin(index_range), end(index_range), [&](int k){
				
				Range r = path[k];
				r[way] += way;
				Coord c = r[way];

				return 0 <= r.min && r.max < m.dim(other(r.direction)) && m(c.i, c.j);
			}))
			{
				for (Range &r : ranges)
				{
					r[way] += way;
				}
			}
		}
		
		if (i == 0)
		{
			for (Range &r : ranges)
			{
				Direction direction = other( path[i].direction );
				r[direction] = intersection(r[direction], rfrom[direction]);
			}
		}
		
		if (j == path.size())
		{
			for (Range &r : ranges)
			{
				Direction direction = other(path[i].direction) ;
				r[direction] = intersection(r[direction], rto[direction]);
			}
		}
		
		for (Range &r : ranges)
			result.push_back(r);
		
		i = j;
	}
	
	assert(path.size() == result.size());
	return result;
}


vector<Range> compute_inner_ranges(const InnerRangeGraph &graph)
{
	const auto& [ranges/*path*/, definition_matrix, coords] = graph;
	
	unordered_map<uint64_t, int> source_node_distance;
	unordered_map<uint64_t, Distance> distance;
	unordered_map<uint64_t, Edge> predecessor;
	const Range &r = ranges.front();
	for (int16_t min = r.min; min <= r.max; min++)
	{
		for (int16_t max = min; max <= r.max; max++)
		{
			InnerRange ir = {min, max, 0};
			int64_t u = serialize(ir);
			source_node_distance[u] = compute_distance(ir, ranges, coords);
		}
	}

	dijkstra(graph, source_node_distance, distance, predecessor);
	vector<uint64_t> target_nodes;
//TODO: use destructuring
	for (auto& p : distance)
	{
		int64_t u = p.first;
		InnerRange ir = parse_ir(u);
		if (ir.range_index + 1 == ranges.size())
			target_nodes.push_back(u);
	}
	int64_t u = *min_element(begin(target_nodes), end(target_nodes), [&](int64_t u, int64_t v){return distance.at(u) < distance.at(v); });
	vector<Range> inner_ranges;
	while (u)
	{
		InnerRange ir = parse_ir(u);
		const Range& r = ranges[ir.range_index];
		inner_ranges.push_back({r.direction, r.way, r.value, ir.min, ir.max});
		Edge& edge = predecessor.at(u);
		assert(edge.v == u);
		u = edge.u;
	}
	reverse(begin(inner_ranges), end(inner_ranges));
	return inner_ranges;
}

bool connect_outer_ranges(const OuterRangeGraph& graph)
{
	const vector<Range> &path = graph.path;
	const Range &r = path.front();
	InnerRange ir = {r.min, r.max, 0};
	int64_t u = serialize(ir);

	unordered_map<uint64_t, int> source_node_distance = {{u,0}};
	unordered_map<uint64_t, Distance> distance;
	unordered_map<uint64_t, Edge> predecessor;
	
	dijkstra(graph, source_node_distance, distance, predecessor);
	vector<uint64_t> target_nodes;

	for (auto& p : distance)
	{
		int64_t u = p.first;
		InnerRange ir = parse_ir(u);
		if (ir.range_index + 1 == path.size())
			target_nodes.push_back(u);
	}
	return !target_nodes.empty();
}

vector<Point> compute_polyline(const vector<int>(&coords)[2], const vector<Range>& path)
{
	vector<Range> compact_path;
	
	for (int i=0; i < path.size(); i++)
	{
		if (i==0 || i + 1 == path.size() || path[i].direction != path[i-1].direction)
			compact_path.push_back(path[i]);
	}
	
	vector<Point> polyline;
	Point p;
	
	Range r = compact_path.front();
	
	p[r.direction] = coords[r.direction][r.way == DECREASE ? r.value + 1 : r.value];
	
	for (Range& r : compact_path)
	{
		Direction direction = other(r.direction);
		auto& tab = coords[direction];
		p[direction] = (tab[r.min] + tab[r.max + 1]) / 2;
		polyline.push_back(p);
	}
	
	r = compact_path.back();
	
	p[r.direction] = coords[r.direction][r.way == DECREASE ? r.value : r.value + 1];
	
	polyline.push_back(p);
	
	auto it = unique(begin(polyline), end(polyline));
	int n = distance(begin(polyline), it);
	polyline.resize(n);
	
	return polyline;
}

int overlap(const vector<Link> &adj_links, const unordered_map<int, vector<uint64_t> >& target_candidates, const vector<Edge>& predecessor)
{
	unordered_map<uint64_t, int> hit_count;
	for (const auto [from, to] : adj_links)
	{
		for (uint64_t u : target_candidates.at(to))
		{
			while (u != 0)
			{
				hit_count[u]++;
				u = predecessor[u].u;
			}
		}
	}
	
	int n=0;

	for (auto [u, c] : hit_count)
	{
		if (c >= 2)
			n += c;
	}
	
	return n;
}


string polyline2json(const vector<Polyline>& polylines)
{
	char buffer[10 * 1024];
	int pos = 0;
	
	pos += sprintf(buffer + pos, "[\n");
	
	for (const auto& [from, to, data] : polylines)
	{
		pos += sprintf(buffer + pos, "{\"polyline\":[");

		for (const auto& [x, y] : data)
		{
			pos += sprintf(buffer + pos, "{\"x\":%d,\"y\":%d},", x, y);
		}
		if (buffer[pos-1]==',')
			pos--;
		pos += sprintf(buffer + pos, "],\"from\":%d,\"to\":%d},\n", from, to);
	}
	
	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}
	pos += sprintf(buffer + pos, "]");
	
	return buffer;
}

string diagdata(const TestContext& ctx)
{
	const auto& [testid, rects, frame, links, faisceau_output, polylines] = ctx;
	
	char buffer[10 * 1024];
	int pos = 0;
	
	pos += sprintf(buffer + pos, "{\"documentTitle\":\"\",\n\"boxes\":[\n");
	for (int i=0; i < rects.size(); i++)
		pos += sprintf(buffer + pos, "\t{\"title\":\"\", \"fields\":[]},\n");
	
	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}	
	
	pos += sprintf(buffer + pos, R"(],
"values":[],
"boxComments":[],
"fieldComments":[],
"links":[],
"fieldColors":[],
"rectangles":[
)");
	for (const auto& [left, right, top, bottom] : rects)
	{
		pos += sprintf(buffer + pos, "\t{\"left\":%hu,\"right\":%hu,\"top\":%hu,\"bottom\":%hu},\n", 0, right - left, 0, bottom - top);
	}
	
	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}
	
	pos += sprintf(buffer + pos, "]}\n");
	
	return buffer;
}

string contexts_(const TestContext& ctx, const vector<Polyline>& polylines)
{
	const auto& [testid, rects, frame, links, faisceau_output, polylines_] = ctx;
	
	string pjson = polyline2json(polylines);
	
	char buffer[10 * 1024];
	int pos = 0;
	
	pos += sprintf(buffer + pos, R"(
{"contexts":[{
"frame":{"left":%hu,"right":%hu,"top":%hu,"bottom":%hu},
"translatedBoxes":[
)", frame.left, frame.right, frame.top, frame.bottom);
	
	int i=0;
	for (const auto& [left, right, top, bottom] : rects)
	{
		pos += sprintf(buffer + pos, "{\"id\":%d,\"translation\":{\"x\":%hu,\"y\":%hu}},\n", i, left, top);
	}
	
	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}
	
	pos += sprintf(buffer + pos, R"(],
"reduced_edges":[],
"links":%s
}
]}
)", pjson.c_str());

	return buffer;
}


const TestContext contexts[] = {
#if 0
{
	/*testid*/0,
	/*rectangles*/{
/*0:*/ {/*name: 'DOS_ETAPE',*/ /*left*/ 622, /*right*/ 790, /*top*/ 507, /*bottom*/ 739},
/*1:*/ {/*name: 'DOS_DATE',*/ /*left*/ 434, /*right*/ 602, /*top*/ 523, /*bottom*/ 739},
/*2:*/ {/*name: 'DOS_TIERS',*/ /*left*/ 796, /*right*/ 1006, /*top*/ 187, /*bottom*/ 339},
/*3:*/ {/*name: 'DOS_CARACTERISTIQUE',*/ /*left*/ 30, /*right*/ 212, /*top*/ 377, /*bottom*/ 449},
/*4:*/ {/*name: 'DOS_NOTE',*/ /*left*/ 611, /*right*/ 716, /*top*/ 321, /*bottom*/ 393},
/*5:*/ {/*name: 'DOS_RESPONSABLE',*/ /*left*/ 78, /*right*/ 253, /*top*/ 566, /*bottom*/ 702},
/*6:*/ {/*name: 'DOS_ETAPE_DELAI_HISTORIQUE',*/ /*left*/ 811, /*right*/ 1001, /*top*/ 589, /*bottom*/ 741},
/*7:*/ {/*name: 'DOS_ANNUITE',*/ /*left*/ 610, /*right*/ 736, /*top*/ 398, /*bottom*/ 486},
/*8:*/ {/*name: 'DOS_CRITERE',*/ /*left*/ 457, /*right*/ 604, /*top*/ 161, /*bottom*/ 281},
/*9:*/ {/*name: 'DOS_DOSSIER',*/ /*left*/ 358, /*right*/ 526, /*top*/ 310, /*bottom*/ 446},
/*10:*/ {/*name: 'DOS_TITRE',*/ /*left*/ 618, /*right*/ 786, /*top*/ 153, /*bottom*/ 273},
/*11:*/ {/*name: 'DOS_LIEN',*/ /*left*/ 51, /*right*/ 198, /*top*/ 455, /*bottom*/ 559},
/*12:*/ {/*name: 'DOS_ETAPE_DELAI',*/ /*left*/ 771, /*right*/ 897, /*top*/ 350, /*bottom*/ 470},
/*13:*/ {/*name: 'DOS_INTERVENANT',*/ /*left*/ 49, /*right*/ 182, /*top*/ 249, /*bottom*/ 369},
/*14:*/ {/*name: 'DOS_ETAPE_DETAIL',*/ /*left*/ 824, /*right*/ 999, /*top*/ 486, /*bottom*/ 574},
/*15:*/ {/*name: 'DOS_GENERALITE',*/ /*left*/ 414, /*right*/ 554, /*top*/ 30, /*bottom*/ 150},
/*16:*/ {/*name: 'LEG_VTQP',*/ /*left*/ 279, /*right*/ 426, /*top*/ 591, /*bottom*/ 759},
/*17:*/ {/*name: 'COM_PARAMETRAGE_FICHE',*/ /*left*/ 32, /*right*/ 187, /*top*/ 123, /*bottom*/ 243},
/*18:*/ {/*name: 'PRE_PRESTATION',*/ /*left*/ 225, /*right*/ 400, /*top*/ 31, /*bottom*/ 167},
/*19:*/ {/*name: 'PAR_GENERALITE',*/ /*left*/ 244, /*right*/ 384, /*top*/ 201, /*bottom*/ 289}	
	},
	/*frame*/{/*left*/ 0, /*right*/ 1016, /*top*/ 0, /*bottom*/ 769},
	/*links*/{
/*0*/ {/*source*/ 17, /*target*/ 19},
/*1*/ {/*source*/ 17, /*target*/ 18},
/*2*/ {/*source*/ 7, /*target*/ 9},
/*3*/ {/*source*/ 7, /*target*/ 0},
/*4*/ {/*source*/ 3, /*target*/ 9},
/*5*/ {/*source*/ 8, /*target*/ 9},
/*6*/ {/*source*/ 1, /*target*/ 9},
/*7*/ {/*source*/ 1, /*target*/ 0},
/*8*/ {/*source*/ 9, /*target*/ 16},
/*9*/ {/*source*/ 0, /*target*/ 9},
/*10*/ {/*source*/ 0, /*target*/ 0},
/*11*/ {/*source*/ 12, /*target*/ 0},
/*12*/ {/*source*/ 6, /*target*/ 0},
/*13*/ {/*source*/ 14, /*target*/ 0},
/*14*/ {/*source*/ 15, /*target*/ 9},
/*15*/ {/*source*/ 13, /*target*/ 9},
/*16*/ {/*source*/ 11, /*target*/ 9},
/*17*/ {/*source*/ 4, /*target*/ 9},
/*18*/ {/*source*/ 5, /*target*/ 9},
/*19*/ {/*source*/ 2, /*target*/ 9},
/*20*/ {/*source*/ 10, /*target*/ 9},
/*21*/ {/*source*/ 19, /*target*/ 15}
	},
	/*faiceau output*/{},
	/*polylines*/{}
},
#endif

/*
				+-----+
				|  1  |
				+-----+

		   +-----+
		   |  0  |
		   +-----+
*/
{
    /*testid*/0,
    /*rectangles*/{
        {/*left*/10,/*right*/30,/*top*/40,/*bottom*/60 },
        {/*left*/28,/*right*/50,/*top*/10,/*bottom*/30 }
    },
    /*frame*/{ 0, 60, 0, 70 },
    /*links*/{{/*source*/0,/*target*/1}},
	
	/*faiceau output*/{
			{
				/*targets*/{
					{
						/*from*/0,
						/*to*/1,
						/*expected path*/{
							{VERTICAL, DECREASE, 2, 1},
							{VERTICAL, DECREASE, 1, 1},
							{HORIZONTAL, INCREASE, 1, 1}
						}
					}
				},
				/*enlarged*/{
				}
			}
		},
		/*polylines*/{
          	{
				/*from*/0,
				/*to*/1,
				/*data*/{ { 19, 40 }, {19, 20}, {28, 20} }
			}
		}
},

/*
                        +-----+
                        |  1  |
                        +-----+

 +-----+                +-----+
 |  0  |                |  2  |
 +-----+                +-----+

                        +-----+
                        |  3  |
                        +-----+
*/
{
    /*testid*/0,
    /*rectangles*/{
        {/*left*/10,/*right*/30,/*top*/40,/*bottom*/60 },
        {/*left*/80,/*right*/100,/*top*/20,/*bottom*/30 },
        {/*left*/80,/*right*/100,/*top*/40,/*bottom*/60 },
        {/*left*/80,/*right*/100,/*top*/70,/*bottom*/80 }
    },
    /*frame*/{ 0, 120, 0, 100 },
    /*links*/{{/*source*/0,/*target*/1},{/*source*/0,/*target*/2},{/*source*/0,/*target*/3}},
    /*faiceau output*/{
			{
				/*target*/{
					{
						/*from*/0,
						/*to*/1,
						/*expected path*/{
							{HORIZONTAL, INCREASE, 3, 4},
							{VERTICAL, DECREASE, 3, 4},
							{VERTICAL, DECREASE, 2, 4},
							{VERTICAL, DECREASE, 1, 4},
							{HORIZONTAL, INCREASE, 1, 4}
						}
					},
					{
						/*from*/0,
						/*to*/2,
						/*expected path*/{
							{HORIZONTAL, INCREASE, 4, 4}
						}
					},
					{
						/*from*/0,
						/*to*/3,
						/*expected path*/{
							{HORIZONTAL, INCREASE, 5, 4},
							{VERTICAL, INCREASE, 5, 4},
							{VERTICAL, INCREASE, 6, 4},
							{VERTICAL, INCREASE, 7, 4},
							{HORIZONTAL, INCREASE, 7, 4}
						}
					}
				},
				/*enlarged*/{
				}
			}
		},
		/*polylines*/{
          	{
				/*from*/0,
				/*to*/1,
				/*data*/{ { 30, 43 }, {55, 43}, {55, 25}, {80, 25} }
			},
			{
				/*from*/0,
				/*to*/2,
				/*data*/{{30, 49},{80,49 } }
			},
			{
				/*from*/0,
				/*to*/3,
				/*data*/{{30,56},{55,56},{55,75},{80,75}}
			}
		}
},

/*
              +-------------+
   +----+     |             |
   | 2  |     |      0      |
   +----+     |             |
              +-------------+



              +-------------+
              |             |
              |      1      |
              |             |
              +-------------+

  +----------------+
  |                |
  |       3        |
  |                |
  +----------------+
*/

{
	/*testid*/1,
	/*rectangles*/{
		{/*left*/25,/*right*/45,/*top*/15,/*bottom*/35 },
		{/*left*/25,/*right*/45,/*top*/45,/*bottom*/65 },
		{/*left*/10,/*right*/15,/*top*/25,/*bottom*/28 },
		{/*left*/4,/*right*/30,/*top*/70,/*bottom*/90 }
	},
	/*frame*/{ 0, 55, 0, 100 },
	/*links*/{{/*source*/0,/*target*/1},{/*source*/0,/*target*/3}},
	/*faiceau output*/{
			{
				/*targets*/{
					{
						/*from*/0,
						/*to*/1,
						/*expected path*/{
							{ VERTICAL, INCREASE, 4, 5 }
						}
					},
					{
						/*from*/0,
						/*to*/3,
						/*expected path*/{
							{ VERTICAL, INCREASE, 4, 4 },
							{ HORIZONTAL, DECREASE, 4, 4 },
							{ HORIZONTAL, DECREASE, 4, 3},
							{ VERTICAL, INCREASE, 4, 3 },
							{ VERTICAL, INCREASE, 5, 3 },
							{ VERTICAL, INCREASE, 6, 3 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,INCREASE,4,5 },{ VERTICAL,INCREASE,4,5,6 } },
					{ { VERTICAL,INCREASE,4,3 },{ VERTICAL,INCREASE,4,1,3 } },
					{ { VERTICAL,INCREASE,5,3 },{ VERTICAL,INCREASE,5,1,3 } },
					{ { VERTICAL,INCREASE,6,3 },{ VERTICAL,INCREASE,6,1,3 } }
				}
			}
		},
       /*polylines*/{
			{
				/*from*/0,
				/*to*/1,
				/*data*/{ { 37, 35 },{ 37, 45} }
			},
			{
				/*from*/0,
				/*to*/3,
				/*data*/{ { 27, 35 },{ 27, 40 },{ 14, 40 }, { 14, 70} }
			}
		}
},
/*
                +-------------+
    +----+      |             |
    | 2  |      |     0       |
    +----+      |             |
                +-------------+



                +-------------+
                |             |
                |     1       |
                |             |
                +-------------+

 +----------------+
 |                |
 |      3         |
 |                |
 +----------------+
*/
{
	/*testid*/2,
	/*rectangles*/{
		{/*left*/25,/*right*/45,/*top*/15,/*bottom*/35 },
		{/*left*/25,/*right*/45,/*top*/45,/*bottom*/65 },
		{/*left*/10,/*right*/15,/*top*/25,/*bottom*/28 },
		{/*left*/4,/*right*/30,/*top*/70,/*bottom*/90 }
	},
	/*frame*/{ 0, 55, 0, 100 },
	/*links*/{{/*source*/3,/*target*/0}},
	/*faiceau output*/{
			{
				/*targets*/{
					{
						/*from*/0,
						/*to*/3,
						/*expected path*/{
							{ VERTICAL, INCREASE, 4, 4 },
							{ HORIZONTAL, DECREASE, 4, 4 },
							{ HORIZONTAL, DECREASE, 4, 3 },
							{ VERTICAL, INCREASE, 4, 3 },
							{ VERTICAL, INCREASE, 5, 3 },
							{ VERTICAL, INCREASE, 6, 3 }
						}
					}	
				},
				/*enlarged*/{
					{ { VERTICAL, INCREASE, 4, 4 },{ VERTICAL, INCREASE,4,4,5 } },
					{ { VERTICAL, INCREASE, 4, 3 },{ VERTICAL, INCREASE,4,1,3 } },
					{ { VERTICAL, INCREASE, 5, 3 },{ VERTICAL, INCREASE,5,1,3 } },
					{ { VERTICAL, INCREASE, 6, 3 },{ VERTICAL, INCREASE,6,1,3 } }
				}
			}
		},
		/*polylines*/{
			{
				/*from*/3,
				/*to*/0,
				/*data*/{ { 14, 70 },{ 14, 40 }, { 35, 40 },{ 35, 35} }
			}
		}
},
/*
                         +------+
            +------+     |      |
            |      |     |      |
            |  2   |     |  6   |
            |      |     |      |    +-----------------+
            |      |     |      |    |                 |    +------+
            +------+     +------+    |                 |    |      |
                                     |       14        |    |  12  |
  +-----------------------------+    |                 |    +------+
  |                             |    +-----------------+
  |             5               |                           +-------------------+
  |                             |    +---------------+      |                   |
  +-----------------------------+    |       0       |      |         1         |
                                     |               |      |                   |
      +-----------+   +---------+    +---------------+      +-------------------+
      |           |   |         |
      |    17     |   |    7    |    +------------+
      |           |   |         |    |            |
      +-----------+   +---------+    |            |
                                     |            |    +------------------+
        +-----+   +-------------+    |     3      |    |                  |
        |     |   |             |    |            |    |        4         |
        |  18 |   |      16     |    |            |    |                  |
        |     |   |             |    +------------+    +------------------+
        +-----+   +-------------+
                                     +------------------------+   +------------+
                    +-----------+    |                        |   |            |
                    |           |    |                        |   |            |
  +------------+    |           |    |           19           |   |     11     |
  |            |    |    10     |    |                        |   |            |
  |     15     |    |           |    |                        |   |            |
  +------------+    |           |    |                        |   |            |
                    +-----------+    +------------------------+   +------------+
  +--------------+
  |              |  +-------------------------+                   +-------+
  |       9      |  |                         |                   |       |
  |              |  |           8             |                   |   13  |
  |              |  |                         |                   |       |
  +--------------+  +-------------------------+                   +-------+
*/

{
 /*testid*/3,
 /*rectangles*/{
 /*0*/{/*left*/329,/*right*/141 + 329,/*top*/250,/*bottom*/40 + 250 },
 /*1*/{/*left*/523,/*right*/162 + 523,/*top*/235,/*bottom*/56 + 235 },
 /*2*/{/*left*/114,/*right*/64 + 114,/*top*/42,/*bottom*/104 + 42 },
 /*3*/{/*left*/329,/*right*/120 + 329,/*top*/330,/*bottom*/120 + 330 },
 /*4*/{/*left*/489,/*right*/141 + 489,/*top*/394,/*bottom*/56 + 394 },
 /*5*/{/*left*/22,/*right*/267 + 22,/*top*/186,/*bottom*/72 + 186 },
 /*6*/{/*left*/218,/*right*/71 + 218,/*top*/10,/*bottom*/136 + 10 },
 /*7*/{/*left*/211,/*right*/78 + 211,/*top*/298,/*bottom*/72 + 298 },
 /*8*/{/*left*/183,/*right*/211 + 183,/*top*/650,/*bottom*/72 + 650 },
 /*9*/{/*left*/10,/*right*/133 + 10,/*top*/634,/*bottom*/88 + 634 },
 /*10*/{/*left*/183,/*right*/106 + 183,/*top*/506,/*bottom*/104 + 506 },
 /*11*/{/*left*/565,/*right*/105 + 565,/*top*/490,/*bottom*/120 + 490 },
 /*12*/{/*left*/523,/*right*/57 + 523,/*top*/139,/*bottom*/56 + 139 },
 /*13*/{/*left*/564,/*right*/63 + 564,/*top*/650,/*bottom*/72 + 650 },
 /*14*/{/*left*/329,/*right*/154 + 329,/*top*/122,/*bottom*/88 + 122 },
 /*15*/{/*left*/10,/*right*/98 + 10,/*top*/538,/*bottom*/56 + 538 },
 /*16*/{/*left*/177,/*right*/112 + 177,/*top*/410,/*bottom*/56 + 410 },
 /*17*/{/*left*/59,/*right*/112 + 59,/*top*/298,/*bottom*/72 + 298 },
 /*18*/{/*left*/87,/*right*/50 + 87,/*top*/410,/*bottom*/56 + 410 },
 /*19*/{/*left*/329,/*right*/196 + 329,/*top*/490,/*bottom*/120 + 490 }
 },
 /*frame*/{ 0, 707, 0, 744 },
 /*links*/{
 {/*source*/1, /*target*/14 },
 {/*source*/2, /*target*/14 },
 {/*source*/2, /*target*/5 },
 {/*source*/3, /*target*/4 },
 {/*source*/5, /*target*/17 },
 {/*source*/6, /*target*/14 },
 {/*source*/6, /*target*/5 },
 {/*source*/7, /*target*/0 },
 {/*source*/7, /*target*/16 },
 {/*source*/8, /*target*/9 },
 {/*source*/9, /*target*/15 },
 {/*source*/10, /*target*/18 },
 {/*source*/10, /*target*/16 },
 {/*source*/10, /*target*/9 },
 {/*source*/11, /*target*/13 },
 {/*source*/12, /*target*/14 },
 {/*source*/14, /*target*/7 },
 {/*source*/16, /*target*/3 },
 {/*source*/17, /*target*/7 },
 {/*source*/19, /*target*/10 },
 {/*source*/19, /*target*/7 },
 {/*source*/19, /*target*/8 },
 {/*source*/19, /*target*/11 }
 },
/*faiceau output*/{
			{/*[0]*/
				/*targets*/{
					{
						/*from*/7,
						/*to*/0,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 20, 32 },
							{ VERTICAL, DECREASE, 20, 32 },
							{ VERTICAL, DECREASE, 19, 32 },
							{ VERTICAL, DECREASE, 18, 32 },
							{ VERTICAL, DECREASE, 17, 32 },
							{ HORIZONTAL, INCREASE, 17, 32 }
						}
					},
					{
						/*from*/7,
						/*to*/16,
						/*expected path*/{
							{ VERTICAL, INCREASE, 27, 20 },
							{ VERTICAL, INCREASE, 28, 20 },
							{ VERTICAL, INCREASE, 29, 20 }
						}
					},
					{
						/*from*/7,
						/*to*/14,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 20, 32 },
							{ VERTICAL, DECREASE, 20, 32 },
							{ VERTICAL, DECREASE, 19, 32 },
							{ VERTICAL, DECREASE, 18, 32 },
							{ VERTICAL, DECREASE, 17, 32 },
							{ VERTICAL, DECREASE, 16, 32 },
							{ VERTICAL, DECREASE, 15, 32 },
							{ VERTICAL, DECREASE, 14, 32 },
							{ VERTICAL, DECREASE, 13, 32 },
							{ VERTICAL, DECREASE, 12, 32 },
							{ HORIZONTAL, INCREASE, 12, 32 }
						}
					},
					{
						/*from*/7,
						/*to*/17,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 20, 19 },
							{ HORIZONTAL, DECREASE, 20, 18 },
							{ HORIZONTAL, DECREASE, 20, 17 },
							{ HORIZONTAL, DECREASE, 20, 16 },
							{ HORIZONTAL, DECREASE, 20, 15 },
							{ HORIZONTAL, DECREASE, 20, 14 }
						}
					},
					{
						/*from*/7,
						/*to*/19,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 26, 32 },
							{ VERTICAL, INCREASE, 26, 32 },
							{ VERTICAL, INCREASE, 27, 32 },
							{ VERTICAL, INCREASE, 28, 32 },
							{ VERTICAL, INCREASE, 29, 32 },
							{ VERTICAL, INCREASE, 30, 32 },
							{ VERTICAL, INCREASE, 31, 32 },
							{ VERTICAL, INCREASE, 32, 32 },
							{ VERTICAL, INCREASE, 33, 32 },
							{ VERTICAL, INCREASE, 34, 32 },
							{ VERTICAL, INCREASE, 35, 32 },
							{ HORIZONTAL, INCREASE, 35, 32 }
						}
					}
				},
				/*enlarged*/{
					{ { HORIZONTAL,DECREASE,20,16 },{ HORIZONTAL,DECREASE,16,20,26 } },
					{ { HORIZONTAL,INCREASE,12,32 },{ HORIZONTAL,INCREASE,32,5,12 } },
					{ { VERTICAL,INCREASE,29,20 },{ VERTICAL,INCREASE,29,20,31 } },
					{ { HORIZONTAL,INCREASE,20,32 },{ HORIZONTAL,INCREASE,32,20,24 } },
					{ { VERTICAL,INCREASE,27,20 },{ VERTICAL,INCREASE,27,20,31 } },
					{ { HORIZONTAL,DECREASE,20,18 },{ HORIZONTAL,DECREASE,18,20,26 } },
					{ { HORIZONTAL,DECREASE,20,17 },{ HORIZONTAL,DECREASE,17,20,26 } },
					{ { VERTICAL,INCREASE,28,20 },{ VERTICAL,INCREASE,28,20,31 } },
					{ { HORIZONTAL,DECREASE,20,19 },{ HORIZONTAL,DECREASE,19,20,26 } },
					{ { HORIZONTAL,DECREASE,20,15 },{ HORIZONTAL,DECREASE,15,20,26 } },
					{ { HORIZONTAL,DECREASE,20,14 },{ HORIZONTAL,DECREASE,14,20,26 } },
					{ { HORIZONTAL,INCREASE,26,32 },{ HORIZONTAL,INCREASE,32,25,26 } },
					{ { HORIZONTAL,INCREASE,35,32 },{ HORIZONTAL,INCREASE,32,35,44 } }
				}
			},
			{/*[1]*/
				/*targets*/{
					{
						/*from*/14,
						/*to*/1,
						/*expected path*/{
							{ VERTICAL, INCREASE, 13, 44 },
							{ VERTICAL, INCREASE, 14, 44 },
							{ VERTICAL, INCREASE, 15, 44 },
							{ HORIZONTAL, INCREASE, 15, 44 },
							{ HORIZONTAL, INCREASE, 15, 45 },
							{ HORIZONTAL, INCREASE, 15, 46 }
						}
					},
					{
						/*from*/14,
						/*to*/2,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 7, 32 },
							{ HORIZONTAL, DECREASE, 7, 31 },
							{ HORIZONTAL, DECREASE, 7, 30 },
							{ HORIZONTAL, DECREASE, 7, 29 },
							{ HORIZONTAL, DECREASE, 7, 28 },
							{ HORIZONTAL, DECREASE, 7, 27 },
							{ HORIZONTAL, DECREASE, 7, 26 },
							{ HORIZONTAL, DECREASE, 7, 25 },
							{ HORIZONTAL, DECREASE, 7, 24 },
							{ HORIZONTAL, DECREASE, 7, 23 },
							{ HORIZONTAL, DECREASE, 7, 22 },
							{ HORIZONTAL, DECREASE, 7, 21 },
							{ HORIZONTAL, DECREASE, 7, 20 },
							{ HORIZONTAL, DECREASE, 7, 19 },
							{ HORIZONTAL, DECREASE, 7, 18 },
							{ HORIZONTAL, DECREASE, 7, 17 },
							{ HORIZONTAL, DECREASE, 7, 16 },
							{ HORIZONTAL, DECREASE, 7, 15 },
							{ VERTICAL, DECREASE, 7, 15 }
						}
					},
					{
						/*from*/14,
						/*to*/6,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 5, 32 }
						}
					},
					{
						/*from*/14,
						/*to*/12,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 6, 45 },
							{ HORIZONTAL, INCREASE, 6, 46 }
						}
					},
					{
						/*from*/14,
						/*to*/7,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 12, 32 },
							{ VERTICAL, INCREASE, 12, 32 },
							{ VERTICAL, INCREASE, 13, 32 },
							{ VERTICAL, INCREASE, 14, 32 },
							{ VERTICAL, INCREASE, 15, 32 },
							{ VERTICAL, INCREASE, 16, 32 },
							{ VERTICAL, INCREASE, 17, 32 },
							{ VERTICAL, INCREASE, 18, 32 },
							{ VERTICAL, INCREASE, 19, 32 },
							{ VERTICAL, INCREASE, 20, 32 },
							{ HORIZONTAL, DECREASE, 20, 32 }
						}
					}
				},
				/*enlarged*/{
					{ { HORIZONTAL,DECREASE,7,25 },{ HORIZONTAL,DECREASE,25,7,9 } },
					{ { HORIZONTAL,INCREASE,15,45 },{ HORIZONTAL,INCREASE,45,15,18 } },
					{ { VERTICAL,INCREASE,13,44 },{ VERTICAL,INCREASE,13,43,44 } },
					{ { HORIZONTAL,DECREASE,7,30 },{ HORIZONTAL,DECREASE,30,7,9 } },
					{ { HORIZONTAL,DECREASE,7,18 },{ HORIZONTAL,DECREASE,18,7,9 } },
					{ { HORIZONTAL,INCREASE,15,46 },{ HORIZONTAL,INCREASE,46,15,18 } },
					{ { HORIZONTAL,DECREASE,7,26 },{ HORIZONTAL,DECREASE,26,7,9 } },
					{ { VERTICAL,INCREASE,14,44 },{ VERTICAL,INCREASE,14,43,44 } },
					{ { HORIZONTAL,DECREASE,7,31 },{ HORIZONTAL,DECREASE,31,7,9 } },
					{ { VERTICAL,INCREASE,15,44 },{ VERTICAL,INCREASE,15,43,44 } },
					{ { HORIZONTAL,DECREASE,7,32 },{ HORIZONTAL,DECREASE,32,7,9 } },
					{ { HORIZONTAL,INCREASE,15,44 },{ HORIZONTAL,INCREASE,44,15,18 } },
					{ { HORIZONTAL,DECREASE,12,32 },{ HORIZONTAL,DECREASE,32,10,12 } },
					{ { HORIZONTAL,DECREASE,7,29 },{ HORIZONTAL,DECREASE,29,7,9 } },
					{ { HORIZONTAL,DECREASE,7,28 },{ HORIZONTAL,DECREASE,28,7,9 } },
					{ { HORIZONTAL,DECREASE,7,27 },{ HORIZONTAL,DECREASE,27,7,9 } },
					{ { HORIZONTAL,DECREASE,7,24 },{ HORIZONTAL,DECREASE,24,7,9 } },
					{ { HORIZONTAL,DECREASE,7,23 },{ HORIZONTAL,DECREASE,23,7,9 } },
					{ { HORIZONTAL,DECREASE,7,22 },{ HORIZONTAL,DECREASE,22,7,9 } },
					{ { HORIZONTAL,DECREASE,20,32 },{ HORIZONTAL,DECREASE,32,20,26 } },
					{ { HORIZONTAL,DECREASE,7,21 },{ HORIZONTAL,DECREASE,21,7,9 } },
					{ { HORIZONTAL,DECREASE,7,20 },{ HORIZONTAL,DECREASE,20,7,9 } },
					{ { HORIZONTAL,DECREASE,7,19 },{ HORIZONTAL,DECREASE,19,7,9 } },
					{ { HORIZONTAL,DECREASE,7,17 },{ HORIZONTAL,DECREASE,17,7,9 } },
					{ { HORIZONTAL,DECREASE,7,16 },{ HORIZONTAL,DECREASE,16,7,9 } },
					{ { HORIZONTAL,DECREASE,7,15 },{ HORIZONTAL,DECREASE,15,7,9 } },
					{ { VERTICAL,DECREASE,7,15 },{ VERTICAL,DECREASE,7,9,15 } },
					{ { HORIZONTAL,DECREASE,5,32 },{ HORIZONTAL,DECREASE,32,5,6 } },
					{ { HORIZONTAL,INCREASE,6,45 },{ HORIZONTAL,INCREASE,45,6,11 } },
					{ { HORIZONTAL,INCREASE,6,46 },{ HORIZONTAL,INCREASE,46,6,11 } }
				}
			},
			{/*[2]*/
				/*targets*/{
					{
						/*from*/10,
						/*to*/18,
						/*expected path*/{
							{ VERTICAL, DECREASE, 35, 17 },
							{ VERTICAL, DECREASE, 34, 17 },
							{ HORIZONTAL, DECREASE, 34, 17 },
							{ HORIZONTAL, DECREASE, 34, 16 },
							{ HORIZONTAL, DECREASE, 34, 15 },
							{ HORIZONTAL, DECREASE, 34, 14 },
							{ HORIZONTAL, DECREASE, 34, 13 },
							{ HORIZONTAL, DECREASE, 34, 12 },
							{ HORIZONTAL, DECREASE, 34, 11 },
							{ HORIZONTAL, DECREASE, 34, 10 },
							{ VERTICAL, DECREASE, 34, 10 }
						}
					},
					{
						/*from*/10,
						/*to*/16,
						/*expected path*/{
							{ VERTICAL, DECREASE, 35, 18 },
							{ VERTICAL, DECREASE, 34, 18 }
						}
					},
					{
						/*from*/10,
						/*to*/9,
						/*expected path*/{
							{ VERTICAL, INCREASE, 45, 17 },
							{ HORIZONTAL, DECREASE, 45, 17 },
							{ HORIZONTAL, DECREASE, 45, 16 },
							{ HORIZONTAL, DECREASE, 45, 15 },
							{ HORIZONTAL, DECREASE, 45, 14 },
							{ HORIZONTAL, DECREASE, 45, 13 },
							{ HORIZONTAL, DECREASE, 45, 12 },
							{ HORIZONTAL, DECREASE, 45, 11 },
							{ VERTICAL, INCREASE, 45, 11 }
						}
					},
					{
						/*from*/10,
						/*to*/19,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 36, 32 }
						}
					}
				},
				/*enlarged*/{
					{ { HORIZONTAL,DECREASE,34,11 },{ HORIZONTAL,DECREASE,11,34,35 } },
					{ { HORIZONTAL,DECREASE,34,15 },{ HORIZONTAL,DECREASE,15,34,35 } },
					{ { VERTICAL,INCREASE,45,11 },{ VERTICAL,INCREASE,45,1,11 } },
					{ { HORIZONTAL,DECREASE,34,14 },{ HORIZONTAL,DECREASE,14,34,35 } },
					{ { HORIZONTAL,DECREASE,34,13 },{ HORIZONTAL,DECREASE,13,34,35 } },
					{ { HORIZONTAL,DECREASE,34,17 },{ HORIZONTAL,DECREASE,17,34,35 } },
					{ { HORIZONTAL,DECREASE,34,12 },{ HORIZONTAL,DECREASE,12,34,35 } },
					{ { HORIZONTAL,DECREASE,34,16 },{ HORIZONTAL,DECREASE,16,34,35 } },
					{ { HORIZONTAL,DECREASE,34,10 },{ HORIZONTAL,DECREASE,10,34,35 } },
					{ { VERTICAL,DECREASE,34,10 },{ VERTICAL,DECREASE,34,5,10 } },
					{ { VERTICAL,DECREASE,35,18 },{ VERTICAL,DECREASE,35,18,31 } },
					{ { VERTICAL,DECREASE,34,18 },{ VERTICAL,DECREASE,34,18,31 } },
					{ { VERTICAL,INCREASE,45,17 },{ VERTICAL,INCREASE,45,17,31 } },
					{ { HORIZONTAL,INCREASE,36,32 },{ HORIZONTAL,INCREASE,32,36,44 } }
				}
			},
			{/*[3]*/
				/*targets*/{
					{
						/*from*/19,
						/*to*/10,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 36, 32 }
						}
					},
					{
						/*from*/19,
						/*to*/7,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 35, 32 },
							{ VERTICAL, DECREASE, 35, 32 },
							{ VERTICAL, DECREASE, 34, 32 },
							{ VERTICAL, DECREASE, 33, 32 },
							{ VERTICAL, DECREASE, 32, 32 },
							{ VERTICAL, DECREASE, 31, 32 },
							{ VERTICAL, DECREASE, 30, 32 },
							{ VERTICAL, DECREASE, 29, 32 },
							{ VERTICAL, DECREASE, 28, 32 },
							{ VERTICAL, DECREASE, 27, 32 },
							{ VERTICAL, DECREASE, 26, 32 },
							{ HORIZONTAL, DECREASE, 26, 32 }
						}
					},
					{
						/*from*/19,
						/*to*/8,
						/*expected path*/{
							{ VERTICAL, INCREASE, 45, 33 },
							{ VERTICAL, INCREASE, 46, 33 }
						}
					},
					{
						/*from*/19,
						/*to*/11,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 35, 48 },
							{ HORIZONTAL, INCREASE, 35, 49 }
						}
					}
				},
				/*enlarged*/{
					{ { HORIZONTAL,INCREASE,35,49 },{ HORIZONTAL,INCREASE,49,35,44 } },
					{ { VERTICAL,INCREASE,46,33 },{ VERTICAL,INCREASE,46,33,37 } },
					{ { VERTICAL,INCREASE,45,33 },{ VERTICAL,INCREASE,45,33,37 } },
					{ { HORIZONTAL,DECREASE,26,32 },{ HORIZONTAL,DECREASE,32,20,26 } },
					{ { HORIZONTAL,INCREASE,35,48 },{ HORIZONTAL,INCREASE,48,35,44 } },
					{ { HORIZONTAL, DECREASE, 36, 32 },{ HORIZONTAL, DECREASE, 32, 36, 44 } }
				}
			},
			{/*[4]*/
				/*targets*/{
					{
						/*from*/5,
						/*to*/2,
						/*expected path*/{
							{ VERTICAL, DECREASE, 9, 9 },
							{ VERTICAL, DECREASE, 8, 9 },
							{ VERTICAL, DECREASE, 7, 9 }
						}
					},
					{
						/*from*/5,
						/*to*/17,
						/*expected path*/{
							{ VERTICAL, INCREASE, 17, 4 },
							{ VERTICAL, INCREASE, 18, 4 },
							{ VERTICAL, INCREASE, 19, 4 }
						}
					},
					{
						/*from*/5,
						/*to*/6,
						/*expected path*/{
							{ VERTICAL, DECREASE, 9, 22 },
							{ VERTICAL, DECREASE, 8, 22 },
							{ VERTICAL, DECREASE, 7, 22 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,DECREASE,9,9 },{ VERTICAL,DECREASE,9,9,15 } },
					{ { VERTICAL,DECREASE,7,22 },{ VERTICAL,DECREASE,7,22,31 } },
					{ { VERTICAL,INCREASE,17,4 },{ VERTICAL,INCREASE,17,4,13 } },
					{ { VERTICAL,DECREASE,8,9 },{ VERTICAL,DECREASE,8,9,15 } },
					{ { VERTICAL,INCREASE,18,4 },{ VERTICAL,INCREASE,18,4,13 } },
					{ { VERTICAL,DECREASE,8,22 },{ VERTICAL,DECREASE,8,22,31 } },
					{ { VERTICAL,DECREASE,7,9 },{ VERTICAL,DECREASE,7,9,15 } },
					{ { VERTICAL,DECREASE,9,22 },{ VERTICAL,DECREASE,9,22,31 } },
					{ { VERTICAL,INCREASE,19,4 },{ VERTICAL,INCREASE,19,4,13 } }
				}
			},
			{/*[5]*/
				/*targets*/{
					{
						/*from*/9,
						/*to*/8,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 47, 12 },
							{ HORIZONTAL, INCREASE, 47, 13 },
							{ HORIZONTAL, INCREASE, 47, 14 },
							{ HORIZONTAL, INCREASE, 47, 15 },
							{ HORIZONTAL, INCREASE, 47, 16 }
						}
					},
					{
						/*from*/9,
						/*to*/15,
						/*expected path*/{
							{ VERTICAL, DECREASE, 45, 1 },
							{ VERTICAL, DECREASE, 44, 1 },
						}
					},
					{
						/*from*/9,
						/*to*/10,
						/*expected path*/{
							{ VERTICAL, DECREASE, 45, 11 },
							{ HORIZONTAL, INCREASE, 45, 11 },
							{ HORIZONTAL, INCREASE, 45, 12 },
							{ HORIZONTAL, INCREASE, 45, 13 },
							{ HORIZONTAL, INCREASE, 45, 14 },
							{ HORIZONTAL, INCREASE, 45, 15 },
							{ HORIZONTAL, INCREASE, 45, 16 },
							{ HORIZONTAL, INCREASE, 45, 17 },
							{ VERTICAL, DECREASE, 45, 17 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,DECREASE,45,11 },{ VERTICAL,DECREASE,45,7,11 } },
					{ { HORIZONTAL,INCREASE,47,12 },{ HORIZONTAL,INCREASE,12,47,50 } },
					{ { VERTICAL,DECREASE,45,1 },{ VERTICAL,DECREASE,45,1,6 } },
					{ { HORIZONTAL,INCREASE,47,16 },{ HORIZONTAL,INCREASE,16,47,50 } },
					{ { VERTICAL,DECREASE,44,1 },{ VERTICAL,DECREASE,44,1,6 } },
					{ { HORIZONTAL,INCREASE,47,13 },{ HORIZONTAL,INCREASE,13,47,50 } },
					{ { VERTICAL,DECREASE,45,17 },{ VERTICAL,DECREASE,45,17,31 } },
					{ { HORIZONTAL,INCREASE,47,14 },{ HORIZONTAL,INCREASE,14,47,50 } },
					{ { HORIZONTAL,INCREASE,47,15 },{ HORIZONTAL,INCREASE,15,47,50 } }
				}
			},
			{/*[6]*/
				/*targets*/{
					{
						/*from*/16,
						/*to*/7,
						/*expected path*/{
							{ VERTICAL, DECREASE, 29, 20 },
							{ VERTICAL, DECREASE, 28, 20 },
							{ VERTICAL, DECREASE, 27, 20 }
						}
					},
					{
						/*from*/16,
						/*to*/10,
						/*expected path*/{
							{ VERTICAL, INCREASE, 34, 17 },
							{ VERTICAL, INCREASE, 35, 17 }
						}
					},
					{
						/*from*/16,
						/*to*/3,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 30, 32 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,INCREASE,34,17 },{ VERTICAL,INCREASE,34,17,31 } },
					{ { VERTICAL,DECREASE,29,20 },{ VERTICAL,DECREASE,29,20,31 } },
					{ { HORIZONTAL,INCREASE,30,32 },{ HORIZONTAL,INCREASE,32,30,32 } },
					{ { VERTICAL,INCREASE,35,17 },{ VERTICAL,INCREASE,35,17,31 } },
					{ { VERTICAL,DECREASE,28,20 },{ VERTICAL,DECREASE,28,20,31 } },
					{ { VERTICAL,DECREASE,27,20 },{ VERTICAL,DECREASE,27,20,31 } }
				}
			},
			{/*[7]*/
				/*targets*/{
					{
						/*from*/2,
						/*to*/14,
						/*expected path*/{
							{ VERTICAL, INCREASE, 7, 15 },
							{ HORIZONTAL, INCREASE, 7, 15 },
							{ HORIZONTAL, INCREASE, 7, 16 },
							{ HORIZONTAL, INCREASE, 7, 17 },
							{ HORIZONTAL, INCREASE, 7, 18 },
							{ HORIZONTAL, INCREASE, 7, 19 },
							{ HORIZONTAL, INCREASE, 7, 20 },
							{ HORIZONTAL, INCREASE, 7, 21 },
							{ HORIZONTAL, INCREASE, 7, 22 },
							{ HORIZONTAL, INCREASE, 7, 23 },
							{ HORIZONTAL, INCREASE, 7, 24 },
							{ HORIZONTAL, INCREASE, 7, 25 },
							{ HORIZONTAL, INCREASE, 7, 26 },
							{ HORIZONTAL, INCREASE, 7, 27 },
							{ HORIZONTAL, INCREASE, 7, 28 },
							{ HORIZONTAL, INCREASE, 7, 29 },
							{ HORIZONTAL, INCREASE, 7, 30 },
							{ HORIZONTAL, INCREASE, 7, 31 },
							{ HORIZONTAL, INCREASE, 7, 32 }
						}
					},
					{
						/*from*/2,
						/*to*/5,
						/*expected path*/{
							{ VERTICAL, INCREASE, 7, 9 },
							{ VERTICAL, INCREASE, 8, 9 },
							{ VERTICAL, INCREASE, 9, 9 }
						}
					}
				},
				/*enlarged*/{
					{ { HORIZONTAL,INCREASE,7,22 },{ HORIZONTAL,INCREASE,22,7,9 } },
					{ { HORIZONTAL,INCREASE,7,18 },{ HORIZONTAL,INCREASE,18,7,9 } },
					{ { VERTICAL,INCREASE,7,15 },{ VERTICAL,INCREASE,7,12,15 } },
					{ { HORIZONTAL,INCREASE,7,19 },{ HORIZONTAL,INCREASE,19,7,9 } },
					{ { HORIZONTAL,INCREASE,7,15 },{ HORIZONTAL,INCREASE,15,7,9 } },
					{ { HORIZONTAL,INCREASE,7,20 },{ HORIZONTAL,INCREASE,20,7,9 } },
					{ { HORIZONTAL,INCREASE,7,16 },{ HORIZONTAL,INCREASE,16,7,9 } },
					{ { HORIZONTAL,INCREASE,7,21 },{ HORIZONTAL,INCREASE,21,7,9 } },
					{ { HORIZONTAL,INCREASE,7,17 },{ HORIZONTAL,INCREASE,17,7,9 } },
					{ { HORIZONTAL,INCREASE,7,23 },{ HORIZONTAL,INCREASE,23,7,9 } },
					{ { HORIZONTAL,INCREASE,7,24 },{ HORIZONTAL,INCREASE,24,7,9 } },
					{ { HORIZONTAL,INCREASE,7,25 },{ HORIZONTAL,INCREASE,25,7,9 } },
					{ { VERTICAL,INCREASE,7,9 },{ VERTICAL,INCREASE,7,9,11 } },
					{ { HORIZONTAL,INCREASE,7,26 },{ HORIZONTAL,INCREASE,26,7,9 } },
					{ { HORIZONTAL,INCREASE,7,27 },{ HORIZONTAL,INCREASE,27,7,9 } },
					{ { HORIZONTAL,INCREASE,7,28 },{ HORIZONTAL,INCREASE,28,7,9 } },
					{ { HORIZONTAL,INCREASE,7,29 },{ HORIZONTAL,INCREASE,29,7,9 } },
					{ { HORIZONTAL,INCREASE,7,30 },{ HORIZONTAL,INCREASE,30,7,9 } },
					{ { HORIZONTAL,INCREASE,7,31 },{ HORIZONTAL,INCREASE,31,7,9 } },
					{ { HORIZONTAL,INCREASE,7,32 },{ HORIZONTAL,INCREASE,32,7,9 } },
					{ { VERTICAL,INCREASE,8,9 },{ VERTICAL,INCREASE,8,9,11 } },
					{ { VERTICAL,INCREASE,9,9 },{ VERTICAL,INCREASE,9,9,11 } }
				}
			},
			{/*[8]*/
				/*targets*/{
					{
						/*from*/3,
						/*to*/4,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 29, 41 },
							{ HORIZONTAL, INCREASE, 29, 42 },
							{ HORIZONTAL, INCREASE, 29, 43 },
							{ HORIZONTAL, INCREASE, 29, 44 },
							{ HORIZONTAL, INCREASE, 29, 45 }
						}
					},
					{
						/*from*/3,
						/*to*/16,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 30, 32 }
						}
					}
				},
				/*enlarged*/{
					{ { HORIZONTAL,INCREASE,29,45 },{ HORIZONTAL,INCREASE,45,29,32 } },
					{ { HORIZONTAL,INCREASE,29,41 },{ HORIZONTAL,INCREASE,41,29,32 } },
					{ { HORIZONTAL,INCREASE,29,42 },{ HORIZONTAL,INCREASE,42,29,32 } },
					{ { HORIZONTAL,INCREASE,29,43 },{ HORIZONTAL,INCREASE,43,29,32 } },
					{ { HORIZONTAL,DECREASE,30,32 },{ HORIZONTAL,DECREASE,32,30,32 } },
					{ { HORIZONTAL,INCREASE,29,44 },{ HORIZONTAL,INCREASE,44,29,32 } }
				}
			},
			{/*[9]*/
				/*targets*/{
					{
						/*from*/6,
						/*to*/14,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 5, 32 }
						}
					},
					{
						/*from*/6,
						/*to*/5,
						/*expected path*/{
							{ VERTICAL, INCREASE, 7, 22 },
							{ VERTICAL, INCREASE, 8, 22 },
							{ VERTICAL, INCREASE, 9, 22 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,INCREASE,7,22 },{ VERTICAL,INCREASE,7,22,31 } },
					{ { HORIZONTAL,INCREASE,5,32 },{ HORIZONTAL,INCREASE,32,5,6 } },
					{ { VERTICAL,INCREASE,8,22 },{ VERTICAL,INCREASE,8,22,31 } },
					{ { VERTICAL,INCREASE,9,22 },{ VERTICAL,INCREASE,9,22,31 } }
				}
			},
			{/*[10]*/
				/*targets*/{
					{
						/*from*/8,
						/*to*/9,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 47, 16 },
							{ HORIZONTAL, DECREASE, 47, 15 },
							{ HORIZONTAL, DECREASE, 47, 14 },
							{ HORIZONTAL, DECREASE, 47, 13 },
							{ HORIZONTAL, DECREASE, 47, 12 }
						}
					},
					{
						/*from*/8,
						/*to*/19,
						/*expected path*/{
							{ VERTICAL, DECREASE, 46, 33 },
							{ VERTICAL, DECREASE, 45, 33 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,DECREASE,46,33 },{ VERTICAL,DECREASE,46,33,37 } },
					{ { HORIZONTAL,DECREASE,47,12 },{ HORIZONTAL,DECREASE,12,47,50 } },
					{ { HORIZONTAL,DECREASE,47,16 },{ HORIZONTAL,DECREASE,16,47,50 } },
					{ { VERTICAL,DECREASE,45,33 },{ VERTICAL,DECREASE,45,33,37 } },
					{ { HORIZONTAL,DECREASE,47,15 },{ HORIZONTAL,DECREASE,15,47,50 } },
					{ { HORIZONTAL,DECREASE,47,14 },{ HORIZONTAL,DECREASE,14,47,50 } },
					{ { HORIZONTAL,DECREASE,47,13 },{ HORIZONTAL,DECREASE,13,47,50 } }
				}
			},
			{/*[11]*/
				/*targets*/{
					{
						/*from*/11,
						/*to*/13,
						/*expected path*/{
							{ VERTICAL, INCREASE, 45, 50 },
							{ VERTICAL, INCREASE, 46, 50 }
						}
					},
					{
						/*from*/11,
						/*to*/19,
						/*expected path*/{
							{ HORIZONTAL, DECREASE, 35, 49 },
							{ HORIZONTAL, DECREASE, 35, 48 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,INCREASE,45,50 },{ VERTICAL,INCREASE,45,50,52 } },
					{ { HORIZONTAL,DECREASE,35,49 },{ HORIZONTAL,DECREASE,49,35,44 } },
					{ { VERTICAL,INCREASE,46,50 },{ VERTICAL,INCREASE,46,50,52 } },
					{ { HORIZONTAL,DECREASE,35,48 },{ HORIZONTAL,DECREASE,48,35,44 } }
				}
			},
			{/*[12]*/
				/*targets*/{
					{
						/*from*/17,
						/*to*/5,
						/*expected path*/{
							{ VERTICAL, DECREASE, 19, 4 },
							{ VERTICAL, DECREASE, 18, 4 },
							{ VERTICAL, DECREASE, 17, 4 }
						}
					},
					{
						/*from*/17,
						/*to*/7,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 20, 14 },
							{ HORIZONTAL, INCREASE, 20, 15 },
							{ HORIZONTAL, INCREASE, 20, 16 },
							{ HORIZONTAL, INCREASE, 20, 17 },
							{ HORIZONTAL, INCREASE, 20, 18 },
							{ HORIZONTAL, INCREASE, 20, 19 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,DECREASE,19,4 },{ VERTICAL,DECREASE,19,4,13 } },
					{ { HORIZONTAL,INCREASE,20,16 },{ HORIZONTAL,INCREASE,16,20,26 } },
					{ { HORIZONTAL,INCREASE,20,17 },{ HORIZONTAL,INCREASE,17,20,26 } },
					{ { VERTICAL,DECREASE,18,4 },{ VERTICAL,DECREASE,18,4,13 } },
					{ { VERTICAL,DECREASE,17,4 },{ VERTICAL,DECREASE,17,4,13 } },
					{ { HORIZONTAL,INCREASE,20,18 },{ HORIZONTAL,INCREASE,18,20,26 } },
					{ { HORIZONTAL,INCREASE,20,14 },{ HORIZONTAL,INCREASE,14,20,26 } },
					{ { HORIZONTAL,INCREASE,20,19 },{ HORIZONTAL,INCREASE,19,20,26 } },
					{ { HORIZONTAL,INCREASE,20,15 },{ HORIZONTAL,INCREASE,15,20,26 } }
				}/*enlarged*/
			}
		},
		/*polylines*/ {
			{/*[0]*/
				/*from*/1,
				/*to*/14,
				/*data*/{{523, 263},{476, 263},{476, 210}}
			},
			{/*[1]*/
				/*from*/2,
				/*to*/14,
				/*data*/{{160, 146},{160, 166},{329, 166}}
			},
			{/*[2]*/
				/*from*/2,
				/*to*/5,
				/*data*/{{128, 146},{128, 186}}
			},
			{/*[3]*/
				/*from*/3,
				/*to*/4,
				/*data*/{{449, 422},{489, 422}}
			},
			{/*[4]*/
				/*from*/5,
				/*to*/17,
				/*data*/{{115, 258},{115, 298}}
			},
			{/*[5]*/
				/*from*/6,
				/*to*/14,
				/*data*/{{289, 134},{329, 134}}
			},
			{/*[6]*/
				/*from*/6,
				/*to*/5,
				/*data*/{{253, 146},{253, 186}}
			},
			{/*[7]*/
				/*from*/7,
				/*to*/0,
				/*data*/{{289, 319},{309, 319},{309, 274},{329, 274}}
			},
			{/*[8]*/
				/*from*/7,
				/*to*/16,
				/*data*/{{250, 370},{250, 410}}
			},
			{/*[9]*/
				/*from*/8,
				/*to*/9,
				/*data*/{{183, 686},{143, 686}}
			},
			{/*[10]*/
				/*from*/9,
				/*to*/15,
				/*data*/{{59, 634},{59, 594}}
			},
			{/*[11]*/
				/*from*/10,
				/*to*/18,
				/*data*/{{191, 506},{191, 486},{112, 486},{112, 466}}
			},
			{/*[12]*/
				/*from*/10,
				/*to*/16,
				/*data*/{{244, 506},{244, 466}}
			},
			{/*[13]*/
				/*from*/10,
				/*to*/9,
				/*data*/{{236, 610},{236, 622},{125, 622},{125, 634}}
			},
			{/*[14]*/
				/*from*/11,
				/*to*/13,
				/*data*/{{596, 610},{596, 650}}
			},
			{/*[15]*/
				/*from*/12,
				/*to*/14,
				/*data*/{{523, 167},{483, 167}}
			},
			{/*[16]*/
				/*from*/14,
				/*to*/7,
				/*data*/{{329, 198},{309, 198},{309, 319},{289, 319}}
			},
			{/*[17]*/
				/*from*/16,
				/*to*/3,
				/*data*/{{289, 430},{329, 430}}
			},
			{/*[18]*/
				/*from*/17,
				/*to*/7,
				/*data*/{{171, 334},{211, 334}}
			},
			{/*[19]*/
				/*from*/19,
				/*to*/10,
				/*data*/{{329, 558},{289, 558}}
			},
			{/*[20]*/
				/*from*/19,
				/*to*/7,
				/*data*/{{329, 498},{309, 498},{309, 355},{289, 355}}
			},
			{/*[21]*/
				/*from*/19,
				/*to*/8,
				/*data*/{{361, 610},{361, 650}}
			},
			{/*[22]*/
				/*from*/19,
				/*to*/11,
				/*data*/{{525, 550},{565, 550}}
			}
		}/*polylines*/
},


/*
      +--------+ 
      |        |
      |   0    |     +---------+
      |        |     |         |
      +--------+     |    1    |    +--------+
                     |         |    |        |
                     +---------+    |   2    |
                                    |        |
                                    +--------+
*/

{
        /*testid*/4,
        /*rectangles*/{
            { 2/*left*/,4/*right*/,2/*top*/,4/*bottom*/ },
            { 6/*left*/,8/*right*/,3/*top*/,5/*bottom*/ },
            { 10/*left*/,12/*right*/,4/*top*/,6/*bottom*/ }
        },
        /*frame*/{ 0/*left*/,14/*right*/,0/*top*/,10/*bottom*/ },
        /*links*/{{/*source*/0,/*target*/2}},
        /*faiceau output*/{
                       {
				/*targets*/{
					{
						/*from*/0,
						/*to*/2,
						/*expected path*/{
							{ HORIZONTAL, INCREASE, 1, 2 },
							{ HORIZONTAL, INCREASE, 1, 3 },
							{ HORIZONTAL, INCREASE, 1, 4 },
							{ VERTICAL, INCREASE, 1, 4 },
							{ VERTICAL, INCREASE, 2, 4 },
							{ VERTICAL, INCREASE, 3, 4 },
							{ HORIZONTAL, INCREASE, 3, 4 }
						}
					}
				},
				/*enlarged*/{
						{ {HORIZONTAL, INCREASE, 3, 4},{ HORIZONTAL, INCREASE, 4, 3, 4 }}
				}
			}
		},
		/*polylines*/{
			{
				/*from*/0,
				/*to*/2,
				/*data*/{{4,2},{9,2},{9,5},{10,5}}
			}
		}
},
/*
                     +-------+
                     |       |
                     |       |
                     |   0   |
                     |       |
                     |       |
                     +-------+


       +-------------------------------------+
       |                                     |
       |                                     |
       |                1                    |
       |                                     |
       |                                     |
       +-------------------------------------+


               +----------------------+
               |                      |
               |                      |
               |          2           |
               |                      |
               |                      |
               +----------------------+
*/

{
        /*testid*/5,
        /*rectangles*/{
            { 8/*left*/,10/*right*/,4/*top*/,8/*bottom*/ },
            { 4/*left*/,14/*right*/,12/*top*/,16/*bottom*/ },
            { 6/*left*/,12/*right*/,20/*top*/,24/*bottom*/ }
        },
        /*frame*/{ 0/*left*/,20/*right*/,0/*top*/,30/*bottom*/ },
        /*links*/{{/*source*/2,/*target*/1},{/*source*/2,/*target*/0}},
		/*faiceau output*/{
			{
				/*targets*/{
					{
						/*from*/2,
						/*to*/1,
						/*expected path*/{
							{ VERTICAL, DECREASE, 4, 3 },
						}
					},
					{
						/*from*/2,
						/*to*/0,
						/*expected path*/{
							{ VERTICAL, DECREASE, 4, 2 },
							{ HORIZONTAL, DECREASE, 4, 2 },
							{ HORIZONTAL, DECREASE, 4, 1 },
							{ HORIZONTAL, DECREASE, 4, 0 },
							{ VERTICAL, DECREASE, 4, 0 },
							{ VERTICAL, DECREASE, 3, 0 },
							{ VERTICAL, DECREASE, 2, 0 },
							{ HORIZONTAL, INCREASE, 2, 0 },
							{ HORIZONTAL, INCREASE, 2, 1 },
							{ HORIZONTAL, INCREASE, 2, 2 },
							{ HORIZONTAL, INCREASE, 2, 3 },
							{ VERTICAL, DECREASE, 2, 3 }
						}
					}
				},
				/*enlarged*/{
					{ { VERTICAL,DECREASE,4,3 },{ VERTICAL,DECREASE,4,3,5 } },
					{ { VERTICAL,DECREASE,2,3 },{ VERTICAL,DECREASE,2,3,4 } }
				}
			}
		},
        /*polylines*/{
			{
				/*from*/2,
				/*to*/1,
				/*data*/{{10,20},{10,16}}
			},
			{
				/*from*/2,
				/*to*/0,
				/*data*/{{7,20},{7,18},{2,18},{2,10},{9,10},{9,8}}
			}
		}
},
/*
+----------+
|          |
|    0     |
|          |
+----------+



                      +-----+
                      |  1  |
                      +-----+
*/
{
        /*testid*/6,
        /*rectangles*/{
            { /*left*/0, /*right*/2, /*top*/0, /*bottom*/2 },
            { /*left*/4, /*right*/5, /*top*/4, /*bottom*/5 }
        },
        /*frame*/{0/*left*/, 5/*right*/,0/*top*/,5/*bottom*/},
        /*links*/{{/*source*/0,/*target*/1}},
        /*faiceau output*/{
			{
				/*targets*/{
					{
						/*from*/0,
						/*to*/1,
						/*expected path*/{
							{VERTICAL,INCREASE,1,0},
							{VERTICAL,INCREASE,2,0},
							{HORIZONTAL,INCREASE,2,0},
							{HORIZONTAL,INCREASE,2,1}
						}
					}
				},
				/*enlarged*/{}
			}
		},
		/*polylines*/{
			{
				/*from*/0,
				/*to*/1,
				/*data*/{{1,2},{1,4},{4,4}}
			}
		}
},

/*
    +------------+                                  +------------+
    |            |                                  |            |
    |            |                                  |            |
    |      0     |                                  |      2     |
    |            |                                  |            |
    +------------+                                  +------------+











    +------------+                                  +------------+
    |            |                                  |            |
    |            |                                  |            |
    |      1     |                                  |      3     |
    |            |                                  |            |
    +------------+                                  +------------+
*/
{
        /*testid*/7,
        /*rectangles*/{
            { /*left*/2, /*right*/6, /*top*/2, /*bottom*/6 },
            { /*left*/2, /*right*/6, /*top*/20, /*bottom*/24 },
            { /*left*/36, /*right*/40, /*top*/2, /*bottom*/6 },
            { /*left*/36, /*right*/40, /*top*/20, /*bottom*/24 }
        },
        /*frame*/{0/*left*/,80/*right*/,0/*top*/,60/*bottom*/},
        /*links*/{{/*source*/0,/*target*/3}},
        /*faiceau output*/{
            {
				/*targets*/{
					{
                        /*from*/0,
                        /*to*/3,
						/*expected path*/{
                            {HORIZONTAL,INCREASE,1,2},
                            {VERTICAL,INCREASE,1,2},
                            {VERTICAL,INCREASE,2,2},
                            {VERTICAL,INCREASE,3,2},
                            {HORIZONTAL,INCREASE,3,2}
                        }
					}
				},
                /*enlarged*/{}
            }
        },
        /*polylines*/{
            {
                /*from*/0,
                /*to*/3,
                /*data*/{{6,4},{21,4},{21,22},{36,22}}
            }
        }
},
/*
   +------------+                                    +------------+
   |            |                                    |            |
   |            |                                    |            |
   |      0     |                                    |      2     |
   |            |                                    |            |
   +------------+                                    +------------+











   +------------+                                    +------------+
   |            |                                    |            |
   |            |                                    |            |
   |      1     |                                    |      3     |
   |            |                                    |            |
   +------------+                                    +------------+
*/
{
        /*testid*/8,
        /*rectangles*/{
            { /*left*/20, /*right*/60, /*top*/20, /*bottom*/60 },
            { /*left*/20, /*right*/60, /*top*/200, /*bottom*/240 },
            { /*left*/360, /*right*/400, /*top*/20, /*bottom*/60 },
            { /*left*/360, /*right*/400, /*top*/200, /*bottom*/240 }
        },
        /*frame*/{0/*left*/,800/*right*/,0/*top*/,600/*bottom*/},
        /*links*/{{/*source*/3,/*target*/0},{/*source*/3,/*target*/1}},
		/*faiceau output*/{
			{
				/*targets*/{
					{
						/*from*/3,
						/*to*/0,
						/*expected path*/{
							{HORIZONTAL,DECREASE,3,2},
							{VERTICAL,DECREASE,3,2},
							{VERTICAL,DECREASE,2,2},
							{VERTICAL,DECREASE,1,2},
							{HORIZONTAL,DECREASE,1,2}
						}
					},
					{
						/*from*/3,
						/*to*/1,
						/*expected path*/{
							{HORIZONTAL,DECREASE,4,2}
						}
					}
				},
				/*enlarged*/{}
			}
		},
		/*polylines*/{
			{
				/*from*/3,
				/*to*/0,
				/*data*/{{360,210},{210,210},{210,40},{60,40}}
			},
			{
				/*from*/3,
				/*to*/1,
				/*data*/{{360,230},{60,230}}
			}
		}
}
};

/*
1/ Pour chaque rectangle, creer plusieurs entrees dans coords, suivant le nombre de liens connectes a ce rectangle.
Pour le calcul de coords, il faut passer un tableau nblink (nombre de liens par rectangle)
2/ Garder plusieurs meilleurs chemins, si il y en a plusieurs pour la meme distance.
3/ enlarge() elargit chaque path de 1 unit. Lorsqu'ils y a plusieurs chemins, certains sont elimins. Le meilleur reste.
*/


FaiceauOutput compute_faiceau(const vector<Link>& links, 
				const Matrix<bool>& definition_matrix_, 
				const Matrix<Span>(&range_matrix)[2],
				const vector<int>(&coords)[2], 
				const vector<Rect>& rects,
				int from)
{
	printf("enter compute_faiceau() from=%d \n", from);
	
	vector<Target> targets;

	vector<Link> adj_links;
	for (const Link& link : links)
	{
		if (link.from == from)
			adj_links.push_back({ link.from, link.to });
		else if (link.to == from)
			adj_links.push_back({ link.to, link.from });
	}

	vector<int> distance(1000*1000, INT16_MAX);
	vector<Edge> predecessor(1000*1000);
	unordered_set<uint64_t> source_nodes = compute_nodes(coords, definition_matrix_, rects[from], OUTPUT);
	unordered_map<uint64_t, int> source_node_distance;
	for (uint64_t u : source_nodes)
		source_node_distance[u] = 0;

	dijkstra(Graph{ definition_matrix_, range_matrix, coords }, source_node_distance, distance, predecessor);

	unordered_map<int, vector<uint64_t> > target_candidates_;
	unordered_map<int, uint64_t> best_target_candidate;

	for (const auto& [from, to] : adj_links)
	{
		unordered_set<uint64_t> target_nodes = compute_nodes(coords, definition_matrix_, rects[to], INPUT);
		compute_target_candidates(source_nodes, target_nodes, distance, predecessor, target_candidates_[to]);
	}

	//Selection of the best candidate
	for (const Link& lk : adj_links)
	{
		vector<uint64_t> &candidates = target_candidates_[lk.to];
		uint64_t u = *min_element(begin(candidates), end(candidates), [&](uint64_t u, uint64_t v) {
			unordered_map<int, vector<uint64_t> > target_candidates = target_candidates_;
			target_candidates[lk.to] = { u };
			int n1 = overlap(adj_links, target_candidates, predecessor);
			target_candidates[lk.to] = { v };
			int n2 = overlap(adj_links, target_candidates, predecessor);
			return n1 < n2;
		}
		);
		best_target_candidate[lk.to] = { u };
	}

	//enlarge the faiceau - BEGIN

	unordered_map<Maille, Range> enlarged;

	while (true)
	{
		unordered_map<Maille, Range> enlarged_update = enlarged;

		for (const auto& [from, to] : adj_links)
		{
			vector<Maille> result;
			for (uint64_t u = best_target_candidate[to]; u != 0; u = predecessor[u].u)
			{
				result.push_back(parse(u));
			}
			//remove first (resp. last) node because it is inside the source (resp. target) rectangle.
			result.pop_back();
			reverse(begin(result), end(result));
			result.pop_back();

			Target target = { from, to, result };

			if (find(begin(targets), end(targets), target) == end(targets))
				targets.push_back(target);
			Matrix<bool> definition_matrix = definition_matrix_;
			for (const Link& other_link : adj_links)
			{
				if (other_link.to == to)
					continue;
				for (uint64_t u = best_target_candidate[other_link.to]; u != 0; u = predecessor.at(u).u)
				{
					Maille m = parse(u);
					auto [direction, way, i, j] = m;
					int16_t value = m[m.direction], other_val = m[other(m.direction)];
					Range r = enlarged_update.count(m) ? enlarged_update[m] : Range{ direction, way, value, other_val, other_val };
					for (int16_t other_value = r.min; other_value <= r.max; other_value++)
					{
						m[other(m.direction)] = other_value;
						definition_matrix(m.i, m.j) = false;
					}
				}
			}
			vector<Range> ranges;
			for (Maille& m : result)
			{
				int16_t value = m[m.direction], other_value = m[other(m.direction)];
				ranges.push_back(enlarged_update.count(m) ? enlarged_update[m] : Range{ m.direction, m.way,value,other_value,other_value });
			}
			ranges = enlarge(ranges, definition_matrix, index(coords, rects[from]), index(coords, rects[to]));

			if (!connect_outer_ranges(OuterRangeGraph{ ranges, definition_matrix_, coords }))
				ranges = compute_inner_ranges(InnerRangeGraph{ ranges, definition_matrix_, coords });

			for (Maille& m : result)
			{
				int i = std::distance(&result[0], &m);
				Range& r = ranges[i];
				if (r.min < r.max)
					enlarged_update[m] = r;
			}
		}

		if (enlarged_update == enlarged)
			break;
		enlarged = enlarged_update;
	}

	// enlarge the faiceau - END
	
	printf("exit compute_faiceau() from=%d \n", from);

	return{ targets, enlarged };
}


void compute_range_matrix(const Matrix<bool> &definition_matrix, Matrix<Span> (&range_matrix)[2])
{
	const int n = definition_matrix.dim(VERTICAL);
	const int m = definition_matrix.dim(HORIZONTAL);
	
	for (int i=0; i < n; i++)
	{
		for (int j=0; j < m; j++)
		{
			int imin = i, imax = i, jmin = j, jmax=j;
			
			while (imin > 0 && definition_matrix(imin - 1, j) == definition_matrix(i,j))
				imin--;
			
			while (imax+1 < n && definition_matrix(imax + 1, j) == definition_matrix(i,j))
				imax++;
			
			while (jmin > 0 && definition_matrix(i, jmin - 1) == definition_matrix(i,j))
				jmin--;
			
			while (jmax + 1 < m && definition_matrix(i, jmax + 1) == definition_matrix(i,j))
				jmax++;
			
			range_matrix[HORIZONTAL](i,j) = {imin, imax};
			range_matrix[VERTICAL](i,j) = {jmin, jmax};
		}
	}
}

void compute_polylines(const vector<Rect>& rects,
						const Rect& frame,
						const vector<Link>& links,
						vector<FaiceauOutput>& faiceau_output,
						vector<Polyline>& polylines)
{
	int n = rects.size();
	vector<int> nblinks(n,0 );
	
	for (const auto& [from, to] : links)
	{
		nblinks[from]++;
		nblinks[to]++;
	}
	
	vector<int> coords[2];
	
	for (const Rect& r : rects)
	{
		int i = distance(&rects[0], &r);
		add_rect(coords, r, nblinks[i]);
	}
	
	add_rect(coords, frame);
	
	for (vector<int>& coords_ : coords)
	{
		sort(begin(coords_), end(coords_));
		auto it = unique(begin(coords_), end(coords_));
		coords_.resize(distance(begin(coords_), it));
	}
	
	Matrix<bool> definition_matrix_ = compute_definition_matrix(rects, coords);
	
	const int n1=definition_matrix_.dim(VERTICAL);
	const int n2=definition_matrix_.dim(HORIZONTAL);
	
	Matrix<Span> range_matrix[2] = {Matrix<Span>(n1,n2), Matrix<Span>(n1,n2)};
	compute_range_matrix(definition_matrix_, range_matrix);

	vector<const Link*> link_pointers;
	std::transform(begin(links),
					end(links),
					std::back_inserter(link_pointers),
					[](const Link& lk){return &lk;}
					);

	vector<int> origins;
	while (int &nn = *max_element(begin(nblinks), end(nblinks)))
	{
		int from = distance(&nblinks[0], &nn);
		origins.push_back(from);
			
		for (const Link*& lkp : link_pointers)
		{
			if (lkp == 0)
				continue;
			if (lkp->from == from || lkp->to == from)
			{
				nblinks[lkp->from]--;
				nblinks[lkp->to]--;
				lkp=0;
			}
		}
	}
	
	assert(nblinks == vector<int>(n,0));

	faiceau_output.resize(origins.size());

	#pragma omp parallel for
	for (int i = 0; i < origins.size(); i++)
	{
		faiceau_output[i] = compute_faiceau(links, definition_matrix_, range_matrix, coords, rects, origins[i]);
	}
	
	unordered_map<Link, FaiceauPath> faiceau_paths;
	for (/*FaiceauOutput*/auto& [targets, enlarged] : faiceau_output)
	{
		for (auto& [from, to, expected_path] : targets)
		{
			faiceau_paths[{from, to}] = {enlarged, expected_path};
		}
	}
	
	for (const auto& [from, to] : links)
	{
		vector<Range> mypath;
		
		if (!faiceau_paths.count({from,to}))
		{
			auto& [enlarged, path] = faiceau_paths[{to,from}];
			for (Maille &maille : path)
			{
				auto [direction, way, i, j] = maille;
				int16_t value = maille[maille.direction], other_value = maille[other(maille.direction)];
				mypath.push_back({enlarged.count(maille) ? enlarged[maille] : Range{direction, way, value, other_value, other_value}});
			}
			reverse(begin(mypath),end(mypath));
			for (Range &r : mypath)
				reverse(r.way);
		}
		else if(!faiceau_paths.count({to,from}))
		{
			auto& [enlarged, path] = faiceau_paths[{from,to}];
			for (Maille &maille : path)
			{
				auto [direction, way, i, j] = maille;
				int16_t value = maille[maille.direction], other_value = maille[other(maille.direction)];
				mypath.push_back(enlarged.count(maille) ? enlarged[maille] : Range{direction, way, value, other_value, other_value});
			}
		}
		else
		{
			unordered_map<Range, int> entgegen_ranges;
			
			auto& [r_enlarged, r_path] = faiceau_paths[{to,from}];
			for (Maille& maille : r_path)
			{
				auto [direction, way, i, j] = maille;
				int16_t value = maille[maille.direction], other_value = maille[other(maille.direction)];				
				Range range = r_enlarged.count(maille) ? r_enlarged[maille] : Range{direction, way, value, other_value, other_value};
				reverse(range.way);
				entgegen_ranges[range] = distance(&r_path[0], &maille);
			}
			
			int i = -1;
			
			auto& [enlarged, path] = faiceau_paths[{from,to}];
			for (const Maille& maille : path)
			{
				auto [direction, way, ii, j] = maille;
				int16_t value = maille[maille.direction], other_value = maille[other(maille.direction)];
				Range range = enlarged.count(maille) ? enlarged[maille] : Range{direction, way, value, other_value, other_value};
				mypath.push_back(range);
				if (entgegen_ranges.count(range))
				{
					i = entgegen_ranges[range] - 1;
					break;
				}
			}
			
			for (; i >= 0; i--)
			{
				Maille &maille = r_path[i];
				auto [direction, way, ii, j] = maille;
				int16_t value = maille[maille.direction], other_value = maille[other(maille.direction)];
				Range range = r_enlarged.count(maille) ? r_enlarged[maille] : Range{direction, way, value, other_value, other_value};
				reverse(range.way);
				mypath.push_back(range);
			}
		}
		
		polylines.push_back({from, to, compute_polyline(coords, mypath)});
	}
}

struct PolylinePoint
{
	Polyline* polyline;
	Point* p;
	int next;
};

struct PointCollision
{
	PolylinePoint p1, p2;
};

const int TRANSLATION_ON_COLLISION = 4;

int query_distance_to_rectangle_vertices(const Rect& rec, const Point& p)
{
	const auto& [left, right, top, bottom] = rec ;
	const auto& [x, y] = p;

	assert(x == left || x == right || y == top || y == bottom);
	
	if (x == left || x == right)
		return std::min(bottom-y, y-top);
	
	if (y == top || y == bottom)
		return std::min(x-left, right-x);
}

vector<PointCollision> intersection_of_polyline_extremities(const vector<Rect>& rects, vector<Polyline> &polylines)
{
	vector<PolylinePoint> points;
	for (Polyline& polyline : polylines)
	{
		auto& [from, to, data] = polyline;
		if (data.size() <= 2)
			continue;
		if (int distance = query_distance_to_rectangle_vertices(rects[from], data[0]); distance > TRANSLATION_ON_COLLISION)
			points.push_back({&polyline, &data[0], 1});
		if (int distance = query_distance_to_rectangle_vertices(rects[to], data.back()); distance > TRANSLATION_ON_COLLISION)
			points.push_back({&polyline, &data.back(), -1});
	}
	
	vector<PointCollision> collisions;
	
	unordered_set<Point> locations;
	
	for (int i=0; i < points.size(); i++)
	{
		PolylinePoint& pi = points[i];
		for (int j=i+2; j < points.size() && collisions.size()<2; j++)
		{
			PolylinePoint& pj = points[j];
			if (*pi.p == *pj.p && locations.count(*pi.p)==0)
			{
				collisions.push_back({pi,pj});
				locations.insert(*pi.p);
			}
		}
	}
	
	return collisions;
}

/*
direction = HORIZONTAL:
xmin=min
xmax=max
x=value

direction = VERTICAL:
ymin=min
ymax=max
y=value
*/

struct PolylineSegment
{
	Polyline* polyline;
	vector<Point>* data;
	Point* p1;
	Point* p2;
	int min, max, value;
	Direction direction;
};

struct SegmentIntersection
{
	PolylineSegment vertical_polyline_segment, horizontal_polyline_segment;
};


vector<SegmentIntersection> intersection_of_polylines(vector<Polyline> &polylines)
{
	vector<PolylineSegment> horizontal_polyline_segments, vertical_polyline_segments;
	
	for (Polyline& polyline : polylines)
	{
		auto& [from, to, data] = polyline;
		for (int i=0; i+1 < data.size(); i++)
		{
			Point *p1=&data[i], *p2=&data[i+1];
			if (p1->x == p2->x)
			{
				int ymin = min(p1->y, p2->y), ymax = max(p1->y, p2->y), x = p1->x ;
				vertical_polyline_segments.push_back({&polyline, &data, p1, p2, ymin, ymax, x, VERTICAL});
			}
			if (p1->y == p2->y)
			{
				int xmin = min(p1->x, p2->x), xmax = max(p1->x, p2->x), y = p1->y ;
				horizontal_polyline_segments.push_back({&polyline, &data, p1, p2, xmin, xmax, y, HORIZONTAL});
			}
		}
	}
	
	vector<SegmentIntersection> intersections;
	
	for (PolylineSegment& hor_seg : horizontal_polyline_segments)
	{
		int xmin = hor_seg.min, xmax = hor_seg.max, y = hor_seg.value ;

		for (PolylineSegment& ver_seg : vertical_polyline_segments)
		{
			int ymin = ver_seg.min, ymax = ver_seg.max, x = ver_seg.value ;
			
			if (xmin < x && x < xmax && ymin < y && y < ymax)
			{
				intersections.push_back({ver_seg, hor_seg});
			}
		}
	}
	
	printf("%lu horizontal polyline segments\n", horizontal_polyline_segments.size());
	printf("%lu vertical polyline segments\n", vertical_polyline_segments.size());
	printf("intersection count: %lu\n", intersections.size());
	
	for (auto [ver_seg, hor_seg] : intersections)
	{
		printf("vertical segment (from %d, to %d, p1=(%d, %d) p2=(%d, %d)) intersects horizontal segment from (from %d, to %d, p1=(%d, %d) p2=(%d, %d))\n",
					ver_seg.polyline->from, ver_seg.polyline->to, ver_seg.p1->x, ver_seg.p1->y, ver_seg.p2->x, ver_seg.p2->y,
					hor_seg.polyline->from, hor_seg.polyline->to, hor_seg.p1->x, hor_seg.p1->y, hor_seg.p2->x, hor_seg.p2->y);		
	}

	return intersections;
}

void post_process_polylines(const vector<Rect>& rects, vector<Polyline> &polylines)
{
	vector<PointCollision> collisions = intersection_of_polyline_extremities(rects, polylines);
	
	for (auto [cp1, cp2] : collisions)
	{
		auto& [polyline1, p1, next1] = cp1;
		auto& [polyline2, p2, next2] = cp2;
		
		for (Direction dir : {HORIZONTAL, VERTICAL})
		{
			if ( (*p1)[dir] == (*(p1+next1))[dir] && (*p2)[dir] == (*(p2+next2))[dir])
			{
				for (Point *p : {p1, p1+next1})
					(*p)[dir] += TRANSLATION_ON_COLLISION;
				
				Point t = {0,0};
				t[dir] = TRANSLATION_ON_COLLISION;
				printf("translation (%d, %d) applied to polyline (from=%d, to=%d)\n", t.x, t.y, polyline1->from, polyline1->to);

				for (Point *p : {p2, p2+next2})
					(*p)[dir] -= TRANSLATION_ON_COLLISION;
				
				t[dir] = -TRANSLATION_ON_COLLISION;
				printf("translation (%d, %d) applied to polyline (from=%d, to=%d)\n", t.x, t.y, polyline2->from, polyline2->to);
			}
		}
	}
	
	vector<SegmentIntersection> intersections = intersection_of_polylines(polylines);
	
	for (auto [ver_seg, hor_seg] : intersections)
	{
		vector<SegmentIntersection> intersections_update ; 
		
		PolylineSegment seg2[2] = {ver_seg, hor_seg};
		for (int i=0; i < 2; i++)
		{
			auto& [/*Polyline* */polyline, /*vector<Point>* */ data, /*Point* */ p1, /*Point* */ p2, ymin, ymax, x, direction] = seg2[i];
			
			if (int ipred = distance(&(*data)[0], p1) - 1, d = seg2[1-i].value - (*p1)[direction]; ipred >= 0 && abs(d) < 10)
			{
				vector<Point> copy_of_data = * data ;
				const Rect& rfrom = rects[polyline->from] ;
				Span sfrom = rfrom[direction];
				bool start_docking_ko=false;
				(*p1)[direction] += 2 * d;
				Point *p0 = & (*data)[ipred] ;
				(*p0)[direction] += 2 * d;
				start_docking_ko = ipred==0 && ((*p0)[direction] <= sfrom.min || (*p0)[direction] >= sfrom.max);
			
				intersections_update = intersection_of_polylines(polylines);
				if (intersections_update.size() >= intersections.size() || start_docking_ko)
				{
					copy(begin(copy_of_data), end(copy_of_data), begin(*data));
				}
				else
				{
					Point translation;
					translation[direction] = 2 * d;
					auto& [tx, ty] = translation;
					printf("translation (%d, %d) applied to polyline (from=%d, to=%d)\n", tx, ty, polyline->from, polyline->to);
				}	
			}
			
			if (int inext = distance(&(*data)[0], p2) + 1, d = seg2[1-i].value - (*p2)[direction]; inext < (*data).size() && abs(d) < 10)
			{
				vector<Point> copy_of_data = * data ;
				const Rect& rto = rects[polyline->to] ;
				Span sto = rto[direction];
				bool end_docking_ko=false;
				(*p2)[direction] += 2 * d;
				Point *p3 = & (*data)[inext] ;
				(*p3)[direction] += 2 * d;
				end_docking_ko = inext+1==(*data).size() && ((*p3)[direction] <= sto.min || (*p3)[direction] >= sto.max);

				intersections_update = intersection_of_polylines(polylines);
				if (intersections_update.size() >= intersections.size() || end_docking_ko)
				{
					copy(begin(copy_of_data), end(copy_of_data), begin(*data));
				}
				else
				{
					Point translation;
					translation[direction] = 2 * d;
					auto& [tx, ty] = translation;
					printf("translation (%d, %d) applied to polyline (from=%d, to=%d)\n", tx, ty, polyline->from, polyline->to);
				}
			}				
		}
		
	}
}



void parse_command(const char* rectdim,
					const char* translations,
					const char* sframe,
					const char* slinks,
					Rect &frame,
					vector<Rect> &rects,
					vector<Link> &links)
{
	int pos;
	
	pos = 0;
	int width, height, x, y;
	while (sscanf(rectdim + pos, "%3x%3x", &width, &height) == 2 &&
	      sscanf(translations + pos, "%3x%3x", &x, &y) == 2)
	{
		rects.push_back({x, x + width, y, y + height});
		pos += 6;
	}
	
	pos = 0;
	int from, to;
	while (sscanf(slinks + pos, "%2x%2x", &from, &to) == 2)
	{
		links.push_back({from, to});
		pos += 4;
	}
	
	sscanf(sframe, "%4x%4x%4x%4x", &frame.left, &frame.right, &frame.top, &frame.bottom);
}

int main(int argc, char* argv[])
{
	Maille m1 = { VERTICAL, DECREASE,16,3 };
	uint64_t u = serialize(m1);
	Maille m2 = parse(u);
	assert(m1 == m2);

	Rect frame;
	vector<Rect> rects;
	vector<Link> links;
		
	if (argc == 1)
	{
		printf("testing bombix ...\n");
		
		for (const TestContext &ctx : contexts)
		{
			string json = diagdata(ctx);
			int i=distance(&contexts[0], &ctx);
			char file_name[40];
			sprintf(file_name, "regtest_%d_diagdata.json", i);
			FILE *f = fopen(file_name, "w");
			fprintf(f, "%s", json.c_str());
			fclose(f);
		}

		int nbOK=0;

	//TODO: use destructuring
		for (const TestContext &ctx : contexts)
		{
			high_resolution_clock::time_point t1 = high_resolution_clock::now();

			bool OK=true;
			
			vector<FaiceauOutput> faisceau_output;
			vector<Polyline> polylines;
			
			printf("testid=%d\n", ctx.testid);

			compute_polylines(ctx.rects, ctx.frame, ctx.links, faisceau_output, polylines);
			post_process_polylines(ctx.rects, polylines);
			
			string json = contexts_(ctx, polylines);
			int i=distance(&contexts[0], &ctx);
			char file_name[40];
			sprintf(file_name, "regtest_%d_contexts.json", i);
			FILE *f = fopen(file_name, "w");
			fprintf(f, "%s", json.c_str());
			fclose(f);
			
			string serialized;
			print(faisceau_output, serialized);

			printf("%s faisceaux.\n", faisceau_output == ctx.faisceau_output ? "OK":"KO");
			OK &= faisceau_output == ctx.faisceau_output;
			
			if (faisceau_output != ctx.faisceau_output)
			{
				printf("%s\n", serialized.c_str());
			}

			print(polylines, serialized);
			duration<double> time_span = high_resolution_clock::now() - t1;
			printf("%s polylines.\n", polylines == ctx.polylines ? "OK":"KO");
			OK &= polylines == ctx.polylines;
			
			if (polylines != ctx.polylines)
			{
				string json = polyline2json(polylines);
				printf("%s\n", json.c_str());
			}

			if (OK)
				nbOK++;
			
			printf("%f seconds elapsed.\n", time_span.count());
		}

		printf("bombix: %d/%ld tests successful.\n", nbOK, sizeof(contexts)/sizeof(TestContext));
		
		return 0;
	}
	else if (argc == 9)
	{
		const regex hexa("^[0-9a-z]*$");

			unordered_map<string, const char*> args={
				 {"--rectdim", 0},
				 {"--translations", 0},
				 {"--frame", 0},
				 {"--links", 0}
			};
		
		for (int i=1; i+1 < argc; i+=2)
		{
					if (args.count(argv[i]))
						 args[argv[i]] = argv[i + 1];
		}

		bool check = strlen(args["--rectdim"]) % 6 == 0 && regex_match(args["--rectdim"], hexa) && 
					 strlen(args["--translations"]) == strlen(args["--rectdim"]) &&regex_match(args["--translations"], hexa) &&
					 strlen(args["--links"]) % 4 == 0 && regex_match(args["--links"], hexa);
					 
		if (!check)
			return -1;
		
		parse_command(args["--recdim"], args["--translations"], args["--frame"], args["--links"], frame, rects, links);

		vector<FaiceauOutput> faiceau_output;
		vector<Polyline> polylines;
		
		compute_polylines(rects, frame, links, faiceau_output, polylines);
		string json = polyline2json(polylines);
		printf("%s", json.c_str());
	}
	else if (argc == 2 && strcmp(argv[1], "--help") == 0)
	{
		printf("example commmand\n");
		printf("--frame 000002c3000002e8 --rectdim 008d002800a200380040006800780078008d0038010b004800470088004e004800d3004800850058006a00680069007800390038003f0048009a00580062003800700038007000480032003800c40078 --translations 014900fa020b00eb0072002a0149014a01e9018a001600ba00da000a00d3012a00b7028a000a027a00b701fa023501ea020b008b0234028a0149007a000a021a00b1019a003b012a0057019a014901ea --links 010e020e020503040511060e0605070007100809090f0a120a100a090b0d0c0e0e0710031107130a13071308130b");
	}
}

//interface for emscripten wasm
extern "C" {
const char* bombix(const char *rectdim,
                        const char *translations,
                        const char *sframe,
                        const char *slinks)
{
        Rect frame;
        vector<Rect> rects;
        vector<Link> links;

        parse_command(rectdim, translations, sframe, slinks, frame, rects, links);

        vector<FaiceauOutput> faiceau_output;
        vector<Polyline> polylines;

        compute_polylines(rects, frame, links, faiceau_output, polylines);
		post_process_polylines(rects, polylines);

        string json = polyline2json(polylines);
        static char res[100000];
        std::copy(json.c_str(), json.c_str()+json.size(), res);
        res[json.size()]=0;
        return res;
}
}
/*
/var/www/projects/ludo$ emcc ~/linkedboxdraw/bombix.cpp -o bombix.html -s EXPORTED_FUNCTIONS='["_bombix"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -s ALLOW_MEMORY_GROWTH=1
puis dans https://dev.diskloud.fr/ludo/bombix.html

bombix=Module.cwrap("bombix","string",["string","string","string","string"])
exemple: 
2 rectangles taille (56,56)=("038,"038") en hexa. 
translations: (10,10) et (100,10) = ("00a","00a") et ("064","00a"). 
frame=(left,right,top,bottom):(0,200,0,100)=("0000","00c8","0000","0064").
links=(0,1):("00","01").
bombix("038038038038","00a00a06400a","000000c800000064","0001")
*/
