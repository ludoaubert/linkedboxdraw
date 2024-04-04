/**
* \copyright Copyright (c) 2015-2024 Ludovic Aubert
*            All Rights Reserved
*            DO NOT ALTER OR REMOVE THIS COPYRIGHT NOTICE
*
* \file bombix-erdos.cpp
*
* - 11/23/2016 by Ludovic Aubert : creation
*/
#include <cstdint>
#include <vector>
#include <queue>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <assert.h>
#include <algorithm>
#include <ranges>
#include <execution>
//#include <format>
#include <initializer_list>
#include <numeric>
#include <iterator>
#include <chrono>
#include <regex>
#include <span>
#include <cstring>
using namespace std;
using namespace std::chrono;
using namespace std::literals;

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
/*
	auto && operator[](this auto && self, Direction direction)
	{
		switch (direction)
		{
		case HORIZONTAL:
			return self.j;
		case VERTICAL:
			return self.i;
		}
	}
*/
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
	int min=0, max=0;
};

int width(const Span span)
{
	return span.max - span.min;
}


struct DistanceInfo
{
	int distance = INT16_MAX ;
	Span largeur_chemin;
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

	bool operator==(const Range&) const = default;
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


struct Maille
{
	Direction direction;
	Way way;
	int16_t i, j;

/*
	auto && operator[](this auto && self, Direction input_direction)
	{
		switch (input_direction)
		{
		case HORIZONTAL:
			return self.j;
		case VERTICAL:
			return self.i;
		}
	}
*/

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

	bool operator==(const Maille&) const = default;

	operator Range() const
	{
		switch (direction)
		{
			case HORIZONTAL:
				return { direction, way, j, i, i };
			case VERTICAL:
				return { direction, way, i, j, j };
		}
	}
};


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

string print(const vector<DistanceInfo>& distance)
{
	char buffer[1024 * 1024];
	int pos = 0;
	
	for (uint64_t u=0; u < distance.size(); u++)
	{
		const DistanceInfo& di = distance[u];
		auto [min, max] = di.largeur_chemin;
		if (di.distance != INT16_MAX || min != 0 || max != 0)
		{
			Maille m = parse(u);
			pos += sprintf(buffer+pos, "{%s, %s, .i=%d, .j=%d}:{.distance=%d, .largeur_chemin={.min=%d,.max=%d}}\n",
				dir[m.direction], way_string[1+m.way], m.i, m.j,
				di.distance, min, max);
		}
	}
	
	return buffer;
}

struct Edge
{
	uint64_t u,v;
	int weight;
	
	friend auto operator<=>(const Edge&, const Edge&) = default;
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

	bool operator==(const Point&) const = default;
};


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
/*
	RectangleProjection operator[](this auto && self, Direction direction)
	{
		return RectangleProjection{direction, const_cast<Rect*>(&self)};
	}
*/
	RectangleProjection operator[](Direction direction)
	{
		return RectangleProjection{direction, this};
	}
	const RectangleProjection operator[](Direction direction) const
	{
		return (const RectangleProjection){direction, const_cast<Rect*>(this)};
	}
	friend bool operator==(const Rect&, const Rect&) = default;
	int left, right, top, bottom;
};


int width(const Rect& r){return r.right - r.left;}
int height(const Rect& r){return r.bottom - r.top;}


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

inline bool intersect_strict(const Rect& r1, const Rect& r2)
{
        return !(r1.left >= r2.right || r1.right <= r2.left || r1.top >= r2.bottom || r1.bottom <= r2.top) ;
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
	struct Dim
	{
		int n,m;
	};

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

/*
	auto && operator()(this auto && self, int i, int j)
	{
		assert(0 <= self.i);
		assert(i < self._n);
		assert(0 <= j);
		assert(j < self._m);
		return _data[i*self._m + j];
	}
*/

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

	Dim dim() const
	{
		return {_n,_m};
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

	bool operator==(const Target&) const = default;
};


struct Polyline
{
	int from;
	int to;
	vector<Point> data;

	bool operator==(const Polyline&) const = default;
};


struct Link
{
	int from, to;

	friend auto operator<=>(const Link&, const Link&) = default;
};


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

	bool operator==(const FaiceauOutput&) const = default;
};


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

/*
TODO:
void print(const vector<Polyline>& polylines, string& serialized)
AND
string polyline2json(const vector<Polyline>& polylines)
Are almost the same thing.
Both implementations should be replaced by standard C++ support for reflection and json.
*/
//TODO: this function should be replaced by support for reflection and json in C++ standard.
string polyline2json(const vector<Polyline>& polylines)
{
/*
 	const string buffer = views::concat(
		"["s,
		polylines | views::transform([](auto [from, to, data]){
			return views::concat(
				"{",
				R"("polyline":)",
				"[",
				data | views::transform([](auto [x, y]){return format(R"({{"x":{}, "y":{}}})", x, y);})
					| views::join_with(','),
				"],",
				format(R"("from":{},"to":{})", from, to),
				"}"
			) | views::join;
		}) | views::join_with(",\n"s),
		"]"
	) | views::join_with('\n') | ranges::to<string>();
*/
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

/*
	const string buffer = views::concat(
		".polylines={",
		polylines | views::transform([](auto [from, to, data]){
			return views::concat(
				"{",
				".data=",
				"{",
				data | views::transform([](auto [x, y]){return format("{{.x={}, .y={}}}", x, y);})
					| views::join_with(','),
				"},",
				format(".from={},.to={}", from, to),
				"}"
			) | views::join;
		}) | views::join_with(",\n"s),
		"}"
	) | views::join_with('\n') | ranges::to<string>();
*/
//TODO: this function should be replaced by support for reflection and json in C++ standard.
void print(const vector<Polyline>& polylines, string& serialized)
{
/*
	serialized = polyline2json(polylines);
	serialized = serialized | 
		views::split(R"(":)"s) | views::join_with('=') |
		views::split(R"(")"s) | views::join_with('.') |
		views::split('[') | views::join('{') |
		views::split(']') | views::join('}') |
		views::split("polyline"s) | views::join_with("data"s) |
		ranges::to<string>() ;
*/
	char buffer[100 * 1024];
	int pos = 0;

	pos += sprintf(buffer + pos, "\t\t.polylines= {\n");
	for (const auto& [from, to, data] : polylines)
	{
		pos += sprintf(buffer + pos, "\t\t\t{\n");
		pos += sprintf(buffer + pos, "\t\t\t\t.from=%d,\n", from);
		pos += sprintf(buffer + pos, "\t\t\t\t.to=%d,\n", to);
		pos += sprintf(buffer + pos, "\t\t\t\t.data={");

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

//TODO: this function should be replaced by support for reflection and json in C++ standard.
void print(const vector<FaiceauOutput>& faiceau_output, string& serialized)
{
	char buffer[100 * 1024];
	int pos = 0;

	pos += sprintf(buffer + pos, "\t\t.faisceau_output={\n");

	for (const /*FaiceauOutput*/auto& [targets, enlarged] : faiceau_output)
	{
		pos += sprintf(buffer + pos, "\t\t\t{\n");
		pos += sprintf(buffer + pos, "\t\t\t\t.targets={\n");

		for (const /*Target*/ auto& [from, to, expected_path] : targets)
		{
			pos += sprintf(buffer + pos, "\t\t\t\t\t{\n");
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t.from=%d,\n", from);
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t.to=%d,\n", to);
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t.expected_path={\n");

			for (const Maille& m : expected_path)
			{
				pos += sprintf(buffer + pos, "\t\t\t\t\t\t\t{%s, %s, %hu, %hu},\n", dir[m.direction], way_string[1+m.way], m.i, m.j);
			}
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t}\n");
			pos += sprintf(buffer + pos, "\t\t\t\t\t}\n");
		}

		pos += sprintf(buffer + pos, "\t\t\t\t},\n");

		pos += sprintf(buffer + pos, "\t\t\t\t.enlarged={\n");

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


string print_graph(const vector<Edge>& edges)
{
	char buffer[1024 * 1024];
	int pos = 0;
	
	for (const auto& [u, v, distance] : edges)
	{
		pos += sprintf(buffer+pos, "{.u=%d, .v=%d, .distance=%d}\n", u, v, distance);
	}
	
	return buffer;
}


vector<Edge> build_graph(const Matrix<bool>& definition_matrix,
						const Matrix<Span>(&range_matrix)[2],
						const vector<int>(&coords)[2])
{
//Erdos - Edge{int64_t u,v; int weight;}
//uint64_t serialize(const Maille& m)
//Maille parse(uint64_t u)
	const Way ways[2] = {DECREASE, INCREASE};
	const Direction directions[2] = {HORIZONTAL, VERTICAL};
	auto [n1, n2] = definition_matrix.dim();
	vector<Edge> edges;
//TODO: use for (auto [way, direction, i, j] : views::cartesian_product(ways, directions, views::iota(0, n1), views::iota(0, n2))){...}
	for (Way way : ways)
	{
		for (Direction direction : directions)
		{
			for (int i=0; i < n1; i++)
			{
				for (int j=0; j < n2; j++)
				{
					vector<Edge> adj;

					const int TURN_PENALTY = 1;
					const int WITHIN_RECTANGLE_PENALTY = 1000;

					Maille r{direction, way, i, j};
					uint64_t u = serialize(r);

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

					for (const Edge& e : adj)
						edges.push_back(e);
				}
			}
		}
	}
	
	ranges::sort(edges);
	return edges;
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

/*
bool valid = ranges::all_of(views::cartesian_product(
								views::iota(rec.left, rec.right+1),
								views::iota(rec.top, rec.bottom+1)
							), [](auto [j, i]){return definition_matrix(i, j);});
use valid instead of !detec. 
*/

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


Rect index(const vector<int>(&coords)[2], const Rect& r)
{
	auto binary_search=[&](Direction direction, int val){
		const vector<int>& v = coords[direction];
		return distance(begin(v), ranges::lower_bound(v, val));
	};

	return Rect{
		.left = binary_search(HORIZONTAL, r.left),
		.right = binary_search(HORIZONTAL, r.right) - 1,
		.top = binary_search(VERTICAL, r.top),
		.bottom = binary_search(VERTICAL, r.bottom) - 1
	};
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
		coords[HORIZONTAL].push_back(r.left + i * width(r) / nblink);
		coords[VERTICAL].push_back(r.top + i * height(r) / nblink);
	}
}


struct QueuedEdge
{
	int distance_v;
	uint64_t u,v;
	int weight;

	auto operator<=>(const QueuedEdge&) const = default;
};

/*
It is important to have a strong ordering here. (!(A < B) and !(B < A)) => A=B.
Before Mai 18th 2018, order used to be e1.distance_v > e2.distance_v, and so it was not deterministic
in case e1 != e2 having e1.distance_v = e2.distance_v. It led to some tricking testing issues. The tests were
all OK on 32 bit platforms but some were KO on 64 bit platforms.
*/


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



void dijkstra(const vector<Edge>& edges,
			const Matrix<Span>(&range_matrix)[2],
			const unordered_set<uint64_t>& source_nodes, 
			vector<DistanceInfo> &distance,
			vector<Edge> & predecessor)
{
	distance[0] = {.distance=0, .largeur_chemin={.min=0,.max=INT16_MAX}};

	priority_queue<QueuedEdge, vector<QueuedEdge>, greater<QueuedEdge> > Q;

	for (uint64_t u : source_nodes)
	{
		int weight = 0;
		Q.push({ weight, 0, u, weight });
	}
	
	const int MIN_CORRIDOR_WIDTH = 5;
	const int NARROW_CORRIDOR_PENALTY = 1000;	

	auto penalty=[](const Span& largeur_chemin){
		return width(largeur_chemin) < MIN_CORRIDOR_WIDTH ? NARROW_CORRIDOR_PENALTY : 0;
	};		

	while (!Q.empty())
	{
		QueuedEdge queued_edge = Q.top();
		Q.pop();
		
		uint64_t u = queued_edge.u, v = queued_edge.v;
		Maille mu = parse(u), mv = parse(v) ;
		
		const Span& sv = range_matrix[1-mv.direction](mv.i, mv.j);
		
		Span largeur_chemin_v = mu.direction == mv.direction ?
								intersection(sv, distance[u].largeur_chemin) :
								sv ;

		if (distance[u].distance + queued_edge.weight + penalty(largeur_chemin_v) < distance[v].distance)
		{
			predecessor[v] = { u, v, queued_edge.weight};
			distance[v] = {
				.distance = distance[u].distance + queued_edge.weight + penalty(largeur_chemin_v), 
				.largeur_chemin = largeur_chemin_v
			};

			for (const Edge& adj_edge : ranges::equal_range(edges, v, ranges::less {}, &Edge::u))
			{
				uint64_t u = adj_edge.u, v = adj_edge.v;
				Maille mu = parse(u), mv = parse(v) ;
				
				const Span& sv = range_matrix[1-mv.direction](mv.i, mv.j);
		
				Span largeur_chemin_v = mu.direction == mv.direction ?
										intersection(sv, distance[u].largeur_chemin) :
										sv ;
				int distance_v = distance[u].distance + adj_edge.weight;
				if (distance_v + penalty(largeur_chemin_v) + adj_edge.weight < distance[v].distance)
				{
					Q.push({ .distance_v=distance_v + penalty(largeur_chemin_v), .u=u, .v=v, .weight=adj_edge.weight });
				}
			}
		}
	}
}


template <typename GraphStruct, typename DistanceMap, typename PredecessorMap>
void dijkstra(const GraphStruct& graph, const unordered_map<uint64_t, int> &source_node_distance, 
			DistanceMap &distance, PredecessorMap & predecessor)
{
	static_assert(is_same<DistanceMap, vector<int> >::value || is_same<DistanceMap, unordered_map<uint64_t, Distance> >::value, "");
	static_assert(is_same<PredecessorMap, vector<Edge> >::value || is_same<PredecessorMap, unordered_map<uint64_t, Edge> >::value, "");

	distance[0] = 0;

	priority_queue<QueuedEdge, vector<QueuedEdge>, greater<QueuedEdge> > Q;

	for (const auto& [u, distance_v] : source_node_distance)
	{
		int weight = distance_v;
		Q.push({ distance_v, 0, u, weight });
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
					Q.push({ distance_v, adj_edge.u, adj_edge.v, adj_edge.weight });
				}
			}
		}
	}
}


void compute_target_candidates(const unordered_set<uint64_t> &source_nodes,
								const unordered_set<uint64_t> &target_nodes,
								const vector<DistanceInfo> &distance,
								const vector<Edge> &predecessor,
								vector<uint64_t> &target_candidates)
{
	uint64_t u = ranges::min(target_nodes, {}, [&](uint64_t u){return distance.at(u).distance;});
/*
	target_candidates = target_nodes | 
		views::filter([&](uint64_t v){return distance.at(v).distance == distance.at(u).distance;}) |
		ranges::to<vector>();
*/
	ranges::copy_if(target_nodes,
		back_inserter(target_candidates),
		[&](uint64_t v){
			return distance.at(v).distance == distance.at(u).distance;
		}
	);

	ranges::sort(target_candidates);
}


/*
use views::chunk_by
when it will be available. C++23 hopefully.
	path | views::chunk_by([](const Range& ri, const Range& rj){return ri.direction==rj.direction;}) |
		for_each([](const std::span<Range>& chunk){
			if (ranges::all_of(chunk, [&](Range r){

				r[way] += way;
				Coord c = r[way];

				return 0 <= r.min && r.max < m.dim(other(r.direction)) && m(c.i, c.j);
			}))
			{
				for (Range &r : chunk)
				{
					r[way] += way;
				}
			}
		});
*/
template <typename Pr>
vector<span<Range> > chunk_by(vector<Range>& path, const Pr& InSameChunk)
{
	vector<span<Range> > chunks;

	int i_prev=0;
	for (int i=1;i < path.size(); i++)
	{
		if (!InSameChunk(path[i-1], path[i]))
		{
			span<Range> chunk(&path[i_prev], i - i_prev);
			chunks.push_back(chunk);
			i_prev=i ;
		}
	}
	span<Range> chunk(&path[i_prev], path.size() - i_prev);
	chunks.push_back(chunk);

	return chunks;
}

template <typename Pr>
vector<span<Range const> > chunk_by(const vector<Range>& path, const Pr& InSameChunk)
{
        vector<span<Range const> > chunks;

        int i_prev=0;
        for (int i=1;i < path.size(); i++)
        {
                if (!InSameChunk(path[i-1], path[i]))
                {
                        span<Range const> chunk(&path[i_prev], i - i_prev);
                        chunks.push_back(chunk);
                        i_prev=i ;
                }
        }
        span<Range const> chunk(&path[i_prev], path.size() - i_prev);
        chunks.push_back(chunk);

        return chunks;
}


vector<Range> enlarge(const vector<Range>& input_path, const Matrix<bool>& m, const Rect& rfrom, const Rect& rto)
{
	printf("enter enlarge()\n");

	vector<Range> path = input_path;

	vector<span<Range> > chunks = chunk_by(path, [](const Range& ri, const Range& rj){return ri.direction==rj.direction;});

	for (span<Range>& chunk : chunks)
	{
		for (Way way : {DECREASE, INCREASE})
		{
			if (ranges::all_of(chunk, [&](Range r){
				r[way] += way;
				RangeExtremity e={.r=&r, .way=way};
				Coord c = e;
				return m.isdefined(c.i, c.j) && m(c.i, c.j)==true;
			}))
			{
				for (Range &r : chunk)
				{
					r[way] += way;
				}
			}
		}
	}

	for (Range &r : chunks[0])
	{
		Direction other_direction = other( r.direction );
		r[other_direction] = intersection(r[other_direction], rfrom[other_direction]);
	}

	for (Range &r : chunks.back())
	{
		Direction other_direction = other( r.direction );
		r[other_direction] = intersection(r[other_direction], rto[other_direction]);
	}

	printf("exit enlarge()\n");

	return path;
}


vector<Range> compute_inner_ranges(const InnerRangeGraph &graph)
{
	printf("enter compute_inner_ranges\n");

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
	int64_t u = *ranges::min_element(target_nodes, {}, [&](int64_t u){return distance.at(u).i; });
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
	ranges::reverse(inner_ranges);

	printf("exit compute_inner_ranges\n");
	return inner_ranges;
}

bool connect_outer_ranges(const OuterRangeGraph& graph)
{
	printf("enter connect_outer_ranges\n");

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

	printf("exit connect_outer_ranges\n");

	return !target_nodes.empty();
}

vector<Point> compute_polyline(const vector<int>(&coords)[2], const vector<Range>& path)
{
	printf("enter compute_polyline\n");

	vector<Point> polyline;

	if (!path.empty())
	{
		Point p;

		const Range &r = path.front();
		p[r.direction] = coords[r.direction][r.way == DECREASE ? r.value + 1 : r.value];

		vector<span<Range const> > chunks = chunk_by(path, [](const Range& ri, const Range& rj){return ri.direction==rj.direction;});
		for (span<Range const> &chunk : chunks)
		{
			const Range& r = chunk[0];
			Direction other_direction = other(r.direction);
			auto& tab = coords[other_direction];
			p[other_direction] = (tab[r.min] + tab[r.max + 1]) / 2;
			polyline.push_back(p);
		}

		const Range & rr = path.back();
		p[rr.direction] = coords[rr.direction][rr.way == DECREASE ? rr.value : rr.value + 1];
		polyline.push_back(p);
	}

	printf("exit compute_polyline\n");

	return polyline;
}

/*
    auto walk = [&](int64_t u)->generator<int64_t>{
        while (u != 0)
        {
            co_yield u;
            u = predecessor[u].u;
        }
    };
    
    auto v = adj_links |
        views::transform([&](const Link& l){return target_candidates.at(l.to);}) |
        views::join |
        views::transform(walk) |
        views::join |
        ranges::to<vector>() ;

    ranges::sort(v);

    auto rg = v |
        views::chunk_by(ranges::equal_to{}) |
        views::transform([](auto rg){return rg.size();}) |
        views::filter([](int n){return n>= 2;}) ;   

    int n = ranges::fold_left(rg, 0, std::plus<int>());
*/
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



//TODO: this function should be replaced by support for reflection and json in C++ standard.
string diagdata(const TestContext& ctx)
{
	const auto& [testid, rects, frame, links, faisceau_output, polylines] = ctx;
/*	
	const string buffer = views::concat(
		"{",
		format(R"("documentTitle":"reg-test-{}",)", testid),
		R"("boxes":[)",
		views::iota(0, (int)rects.size())
				| views::transform([](int i){return format(R"(\t{{"title":"rec-{}", "id":{}, "fields":[]}},\n)", i, i);})
				| views::join_with(",\n"s),
		R"("values":[],)",
		R"("boxComments":[],)",
		R"("fieldComments":[],)",
		R"("links":[)"s,
		links | views::transform([](const Link& e){return format(R"({{"from":{},"fromField":-1,"fromCardinality":"","to":{},"toField":-1,"toCardinality":"","Category":""}},\n)", e.from, e.to);})
				| views::join_with(",\n"s),
		"],"
		R"("fieldColors":[])",
		"}"
	) | views::join_with('\n') | ranges::to<string>();
*/
	char buffer[10 * 1024];
	int pos = 0;

	pos += sprintf(buffer + pos, "{\"documentTitle\":\"reg-test-%d\",\n\"boxes\":[\n", testid);
	for (int i=0; i < rects.size(); i++)
		pos += sprintf(buffer + pos, "\t{\"title\":\"rec-%d\", \"id\":%d, \"fields\":[]},\n", i, i);

	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}

	pos += sprintf(buffer + pos, R"(],
"values":[],
"boxComments":[],
"fieldComments":[],
"links":[
)");

	for (const auto& [from, to] : links)
	{
		pos += sprintf(buffer + pos, "{\"from\":%d,\"fromField\":-1,\"fromCardinality\":\"\",\"to\":%d,\"toField\":-1,\"toCardinality\":\"\",\"Category\":\"\"},\n", from, to);
	}

	if (buffer[pos-2]==',')
	{
		buffer[pos-2]='\n';
		pos--;
	}

	pos += sprintf(buffer + pos, R"(],
"fieldColors":[]
}
)");

	return buffer;
}

//TODO: this function should be replaced by support for reflection and json in C++ standard.
string contexts_(const TestContext& ctx, const vector<Polyline>& polylines)
{
	const auto& [testid, rects, frame, links, faisceau_output, polylines_] = ctx;
/*
	const string buffer = views::concat(
		"{",
		R"("contexts":[{{)",
		format(R"("frame":{"left":{},"right":{},"top":{},"bottom":{}},)", frame.left, frame.right, frame.top, frame.bottom),
		R"("translatedBoxes":[)",
		rects | views::enumerate
			| views::transform([](auto [i, r]){ return format(R"({{"id":{},"translation":{{"x":{},"y":{} }}}}")", i, r.left, r.top);})
			| views::join_with(",\n"s),
		"],",
		format(R"("links:{},")", polyline2json(polylines)),
		R"("rectangles":[)",
		rects | views::transform([](const Rect& r){return format(R"(\t{{"left":0,"right":{},"top":0,"bottom":{} }})", width(r), height(r));})
				| views::join_with(",\n"s),
		"]}]"
	) | views::join_with('\n') | ranges::to<string>();
*/
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
		pos += sprintf(buffer + pos, "{\"id\":%d,\"translation\":{\"x\":%hu,\"y\":%hu}},\n", i++, left, top);
	}

	buffer[pos-2]='\n';
	pos--;

	pos += sprintf(buffer + pos, R"(],
"links":%s
}
],
"rectangles":[
)", pjson.c_str());

	for (const Rect& r : rects)
	{
		pos += sprintf(buffer + pos, "\t{\"left\":%hu,\"right\":%hu,\"top\":%hu,\"bottom\":%hu},\n", 0, width(r), 0, height(r));
	}

	buffer[pos-2]='\n';
	pos--;

	pos += sprintf(buffer + pos, "]}\n");

	return buffer;
}


const vector<TestContext> contexts = {
{
	.testid=0,
	.rects={
/*0:*/ { .left=622, .right=790, .top=507, .bottom=739},
/*1:*/ { .left=434, .right=602, .top=523, .bottom=739},
/*2:*/ { .left=796, .right=1006, .top=187, .bottom=339},
/*3:*/ { .left=30, .right=212, .top=377, .bottom=449},
/*4:*/ { .left=611, .right=716, .top=321, .bottom=393},
/*5:*/ { .left=78, .right=253, .top=566, .bottom=702},
/*6:*/ { .left=811, .right=1001, .top=589, .bottom=741},
/*7:*/ { .left=610, .right=736, .top=398, .bottom=486},
/*8:*/ { .left=457, .right=604, .top=161, .bottom=281},
/*9:*/ { .left=358, .right=526, .top=310, .bottom=446},
/*10:*/ { .left=618, .right=786, .top=153, .bottom=273},
/*11:*/ { .left=51, .right=198, .top=455, .bottom=559},
/*12:*/ { .left=771, .right=897, .top=350, .bottom=470},
/*13:*/ { .left=49, .right=182, .top=249, .bottom=369},
/*14:*/ { .left=824, .right=999, .top=486, .bottom=574},
/*15:*/ { .left=414, .right=554, .top=30, .bottom=150},
/*16:*/ { .left=279, .right=426, .top=591, .bottom=759},
/*17:*/ { .left=32, .right=187, .top=123, .bottom=243},
/*18:*/ { .left=225, .right=400, .top=31, .bottom=167},
/*19:*/ { .left=244, .right=384, .top=201, .bottom=289}
	},
	.frame={.left= 0, .right= 1016, .top= 0, .bottom= 769},
	.links={
/*0*/ {.from= 0, .to= 0},
/*1*/ {.from= 0, .to= 9},
/*2*/ {.from= 1, .to= 0},
/*3*/ {.from= 1, .to= 9},
/*4*/ {.from= 2, .to= 9},
/*5*/ {.from= 3, .to= 9},
/*6*/ {.from= 4, .to= 9},
/*7*/ {.from= 5, .to= 9},
/*8*/ {.from= 6, .to= 0},
/*9*/ {.from= 7, .to= 0},
/*10*/ {.from= 7, .to= 9},
/*11*/ {.from= 8, .to= 9},
/*12*/ {.from= 9, .to= 16},
/*13*/ {.from= 10, .to= 9},
/*14*/ {.from= 11, .to= 9},
/*15*/ {.from= 12, .to= 0},
/*16*/ {.from= 13, .to= 9},
/*17*/ {.from= 14, .to= 0},
/*18*/ {.from= 15, .to= 9},
/*19*/ {.from= 17, .to= 18},
/*20*/ {.from= 17, .to= 19},
/*21*/ {.from= 19, .to= 15}
	},
	.faisceau_output={},  // left empty. Too bulky. Would make the file exceeds the size limit of some online compilers. 
	.polylines= {
		{
			.from=0,
			.to=0,
			.data={}
		},
		{
			.from=0,
			.to=9,
			.data={{622, 515},{500, 515},{500, 446}}
		},
		{
			.from=1,
			.to=0,
			.data={{602, 631},{622, 631}}
		},
		{
			.from=1,
			.to=9,
			.data={{454, 523},{454, 446}}
		},
		{
			.from=2,
			.to=9,
			.data={{796, 320},{526, 320}}
		},
		{
			.from=3,
			.to=9,
			.data={{212, 395},{358, 395}}
		},
		{
			.from=4,
			.to=9,
			.data={{611, 357},{526, 357}}
		},
		{
			.from=5,
			.to=9,
			.data={{248, 566},{248, 430},{358, 430}}
		},
		{
			.from=6,
			.to=0,
			.data={{811, 664},{790, 664}}
		},
		{
			.from=7,
			.to=0,
			.data={{679, 486},{679, 507}}
		},
		{
			.from=7,
			.to=9,
			.data={{610, 422},{526, 422}}
		},
		{
			.from=8,
			.to=9,
			.data={{491, 281},{491, 310}}
		},
		{
			.from=9,
			.to=16,
			.data={{398, 446},{398, 591}}
		},
		{
			.from=10,
			.to=9,
			.data={{702, 273},{702, 315},{526, 315}}
		},
		{
			.from=11,
			.to=9,
			.data={{198, 462},{364, 462},{364, 446}}
		},
		{
			.from=12,
			.to=0,
			.data={{780, 470},{780, 507}}
		},
		{
			.from=13,
			.to=9,
			.data={{182, 339},{358, 339}}
		},
		{
			.from=14,
			.to=0,
			.data={{824, 540},{790, 540}}
		},
		{
			.from=15,
			.to=9,
			.data={{439, 150},{439, 310}}
		},
		{
			.from=17,
			.to=18,
			.data={{187, 145},{225, 145}}
		},
		{
			.from=17,
			.to=19,
			.data={{187, 222},{244, 222}}
		},
		{
			.from=19,
			.to=15,
			.data={{384, 245},{418, 245},{418, 150}}
		},
	}

},

/*
				+-----+
				|  1  |
				+-----+

		   +-----+
		   |  0  |
		   +-----+
*/
{
    .testid=1,
    .rects={
        {.left=10,.right=30,.top=40,.bottom=60 },
        {.left=28,.right=50,.top=10,.bottom=30 }
    },
    .frame={ 0, 60, 0, 70 },
    .links={{.from=0,.to=1}},

	.faisceau_output={
			{
				.targets={
					{
						.from=0,
						.to=1,
						.expected_path={
							{VERTICAL, DECREASE, 2, 1},
							{VERTICAL, DECREASE, 1, 1},
							{HORIZONTAL, INCREASE, 1, 1}
						}
					}
				},
				.enlarged={
				}
			}
		},
		.polylines={
          	{
				.from=0,
				.to=1,
				.data={ { 19, 40 }, {19, 20}, {28, 20} }
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
    .testid=2,
    .rects={
        {.left=10,.right=30,.top=40,.bottom=60 },
        {.left=80,.right=100,.top=20,.bottom=30 },
        {.left=80,.right=100,.top=40,.bottom=60 },
        {.left=80,.right=100,.top=70,.bottom=80 }
    },
    .frame={ 0, 120, 0, 100 },
    .links={{.from=0,.to=1},{.from=0,.to=2},{.from=0,.to=3}},
    .faisceau_output={
			{
				.targets={
					{
						.from=0,
						.to=1,
						.expected_path={
							{VERTICAL, DECREASE, 2, 3},
							{VERTICAL, DECREASE, 1, 3},
							{HORIZONTAL, INCREASE, 1, 3},
							{HORIZONTAL, INCREASE, 1, 4}
						}
					},
					{
						.from=0,
						.to=2,
						.expected_path={
							{HORIZONTAL, INCREASE, 3, 4}
						}
					},
					{
						.from=0,
						.to=3,
						.expected_path={
							{VERTICAL, INCREASE, 6, 3},
							{VERTICAL, INCREASE, 7, 3},
							{HORIZONTAL, INCREASE, 7, 3},
							{HORIZONTAL, INCREASE, 7, 4}
						}
					}
				},
				.enlarged={
						{{VERTICAL,INCREASE,7,3},{VERTICAL,INCREASE,7,1,3}},
						{{VERTICAL,INCREASE,6,3},{VERTICAL,INCREASE,6,1,3}},
						{{HORIZONTAL,INCREASE,3,4},{HORIZONTAL,INCREASE,4,3,5}},
						{{VERTICAL,DECREASE,1,3},{VERTICAL,DECREASE,1,1,3}},
						{{VERTICAL,DECREASE,2,3},{VERTICAL,DECREASE,2,1,3}}
				}
			}
		},
		.polylines={
          	{
				.from=0,
				.to=1,
				.data={ { 20, 40 }, {20, 25}, {80, 25} }
			},
			{
				.from=0,
				.to=2,
				.data={{30, 50},{80,50 } }
			},
			{
				.from=0,
				.to=3,
				.data={{20,60},{20,75},{80,75}}
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
	.testid=3,
	.rects={
		{.left=25,.right=45,.top=15,.bottom=35 },
		{.left=25,.right=45,.top=45,.bottom=65 },
		{.left=10,.right=15,.top=25,.bottom=28 },
		{.left=4,.right=30,.top=70,.bottom=90 }
	},
	.frame={ 0, 55, 0, 100 },
	.links={{.from=0,.to=1},{.from=0,.to=3}},
	.faisceau_output={
			{
				.targets={
					{
						.from=0,
						.to=1,
						.expected_path={
							{ VERTICAL, INCREASE, 4, 4 }
						}
					},
					{
						.from=0,
						.to=3,
						.expected_path={
							{ HORIZONTAL, DECREASE, 3, 3},
							{ VERTICAL, INCREASE, 3, 3 },
							{ VERTICAL, INCREASE, 4, 3 },
							{ VERTICAL, INCREASE, 5, 3 },
							{ VERTICAL, INCREASE, 6, 3 }
						}
					}
				},
				.enlarged={
					{ { VERTICAL,INCREASE,6,3 },{ VERTICAL,INCREASE,6,1,3 } },
					{ { VERTICAL,INCREASE,5,3 },{ VERTICAL,INCREASE,5,1,3 } },
					{ { VERTICAL,INCREASE,4,3 },{ VERTICAL,INCREASE,4,1,3 } },
					{ { VERTICAL,INCREASE,3,3 },{ VERTICAL,INCREASE,3,1,3 } },
					{ { VERTICAL,INCREASE,4,4 },{ VERTICAL,INCREASE,4,4,6 } }
				}
			}
		},
       .polylines={
			{
				.from=0,
				.to=1,
				.data={ { 35, 35 },{ 35, 45} }
			},
			{
				.from=0,
				.to=3,
				.data={ { 25, 31 },{ 14, 31 },{ 14, 70 } }
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
	.testid=4,
	.rects={
		{.left=25,.right=45,.top=15,.bottom=35 },
		{.left=25,.right=45,.top=45,.bottom=65 },
		{.left=10,.right=15,.top=25,.bottom=28 },
		{.left=4,.right=30,.top=70,.bottom=90 }
	},
	.frame={ 0, 55, 0, 100 },
	.links={{.from=3,.to=0}},
	.faisceau_output={
			{
				.targets={
					{
						.from=0,
						.to=3,
						.expected_path={
							{ HORIZONTAL, DECREASE, 3, 3 },
							{ VERTICAL, INCREASE, 3, 3 },
							{ VERTICAL, INCREASE, 4, 3 },
							{ VERTICAL, INCREASE, 5, 3 },
							{ VERTICAL, INCREASE, 6, 3 }
						}
					}
				},
				.enlarged={
					{ { VERTICAL, INCREASE, 6, 3 },{ VERTICAL, INCREASE,6,1,3 } },
					{ { VERTICAL, INCREASE, 5, 3 },{ VERTICAL, INCREASE,5,1,3 } },
					{ { VERTICAL, INCREASE, 4, 3 },{ VERTICAL, INCREASE,4,1,3 } },
					{ { VERTICAL, INCREASE, 3, 3 },{ VERTICAL, INCREASE,3,1,3 } }
				}
			}
		},
		.polylines={
			{
				.from=3,
				.to=0,
				.data={ { 14, 70 },{ 14, 31 }, { 25, 31 } }
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
 .testid=5,
 .rects={
 /*0*/{.left=329,.right=141 + 329,.top=250,.bottom=40 + 250 },
 /*1*/{.left=523,.right=162 + 523,.top=235,.bottom=56 + 235 },
 /*2*/{.left=114,.right=64 + 114,.top=42,.bottom=104 + 42 },
 /*3*/{.left=329,.right=120 + 329,.top=330,.bottom=120 + 330 },
 /*4*/{.left=489,.right=141 + 489,.top=394,.bottom=56 + 394 },
 /*5*/{.left=22,.right=267 + 22,.top=186,.bottom=72 + 186 },
 /*6*/{.left=218,.right=71 + 218,.top=10,.bottom=136 + 10 },
 /*7*/{.left=211,.right=78 + 211,.top=298,.bottom=72 + 298 },
 /*8*/{.left=183,.right=211 + 183,.top=650,.bottom=72 + 650 },
 /*9*/{.left=10,.right=133 + 10,.top=634,.bottom=88 + 634 },
 /*10*/{.left=183,.right=106 + 183,.top=506,.bottom=104 + 506 },
 /*11*/{.left=565,.right=105 + 565,.top=490,.bottom=120 + 490 },
 /*12*/{.left=523,.right=57 + 523,.top=139,.bottom=56 + 139 },
 /*13*/{.left=564,.right=63 + 564,.top=650,.bottom=72 + 650 },
 /*14*/{.left=329,.right=154 + 329,.top=122,.bottom=88 + 122 },
 /*15*/{.left=10,.right=98 + 10,.top=538,.bottom=56 + 538 },
 /*16*/{.left=177,.right=112 + 177,.top=410,.bottom=56 + 410 },
 /*17*/{.left=59,.right=112 + 59,.top=298,.bottom=72 + 298 },
 /*18*/{.left=87,.right=50 + 87,.top=410,.bottom=56 + 410 },
 /*19*/{.left=329,.right=196 + 329,.top=490,.bottom=120 + 490 }
 },
 .frame={ 0, 707, 0, 744 },
 .links={
 {.from=1, .to=14 },
 {.from=2, .to=5 },
 {.from=2, .to=14 },
 {.from=3, .to=4 },
 {.from=5, .to=17 },
 {.from=6, .to=5 },
 {.from=6, .to=14 },
 {.from=7, .to=0 },
 {.from=7, .to=16 },
 {.from=8, .to=9 },
 {.from=9, .to=15 },
 {.from=10, .to=9 },
 {.from=10, .to=16 },
 {.from=10, .to=18 },
 {.from=11, .to=13 },
 {.from=12, .to=14 },
 {.from=14, .to=7 },
 {.from=16, .to=3 },
 {.from=17, .to=7 },
 {.from=19, .to=7 },
 {.from=19, .to=8 },
 {.from=19, .to=10 },
 {.from=19, .to=11 }
 },
.faisceau_output={}, // left empty. Too bulky. Would make the file exceeds the size limit of some online compilers. 
.polylines= {
	{
		.from=1,
		.to=14,
		.data={{523, 263},{476, 263},{476, 210}}
	},
	{
		.from=2,
		.to=5,
		.data={{142, 146},{142, 186}}
	},
	{
		.from=2,
		.to=14,
		.data={{150, 146},{150, 166},{329, 166}}
	},
	{
		.from=3,
		.to=4,
		.data={{449, 422},{489, 422}}
	},
	{
		.from=5,
		.to=17,
		.data={{115, 258},{115, 298}}
	},
	{
		.from=6,
		.to=5,
		.data={{253, 146},{253, 186}}
	},
	{
		.from=6,
		.to=14,
		.data={{289, 134},{329, 134}}
	},
	{
		.from=7,
		.to=0,
		.data={{250, 298},{250, 274},{329, 274}}
	},
	{
		.from=7,
		.to=16,
		.data={{250, 370},{250, 410}}
	},
	{
		.from=8,
		.to=9,
		.data={{183, 686},{143, 686}}
	},
	{
		.from=9,
		.to=15,
		.data={{59, 634},{59, 594}}
	},
	{
		.from=10,
		.to=9,
		.data={{236, 610},{236, 642},{143, 642}}
	},
	{
		.from=10,
		.to=16,
		.data={{236, 506},{236, 466}}
	},
	{
		.from=10,
		.to=18,
		.data={{183, 558},{122, 558},{122, 466}}
	},
	{
		.from=11,
		.to=13,
		.data={{596, 610},{596, 650}}
	},
	{
		.from=12,
		.to=14,
		.data={{523, 167},{483, 167}}
	},
	{
		.from=14,
		.to=7,
		.data={{329, 198},{309, 198},{309, 316},{289, 316}}
	},
	{
		.from=16,
		.to=3,
		.data={{289, 430},{329, 430}}
	},
	{
		.from=17,
		.to=7,
		.data={{171, 334},{211, 334}}
	},
	{
		.from=19,
		.to=7,
		.data={{329, 550},{309, 550},{309, 352},{289, 352}}
	},
	{
		.from=19,
		.to=8,
		.data={{361, 610},{361, 650}}
	},
	{
		.from=19,
		.to=10,
		.data={{329, 558},{289, 558}}
	},
	{
		.from=19,
		.to=11,
		.data={{525, 550},{565, 550}}
	}
}

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
        .testid=6,
        .rects={
            {.left=40,.right=80,.top=40,.bottom=80},
            {.left=120,.right=160,.top=60,.bottom=100},
            {.left=200,.right=240,.top=80,.bottom=120}
        },
        .frame={.left=0,.right=280,.top=0,.bottom=200},
        .links={{.from=0,.to=2}},
        .faisceau_output={
                       {
				.targets={
					{
						.from=0,
						.to=2,
						.expected_path={
							{VERTICAL, INCREASE, 3, 1},
							{VERTICAL, INCREASE, 4, 1},
							{HORIZONTAL, INCREASE, 4, 1},
							{HORIZONTAL, INCREASE, 4, 2},
							{HORIZONTAL, INCREASE, 4, 3},
							{HORIZONTAL, INCREASE, 4, 4}
						}
					}
				},
				.enlarged={
				}
			}
		},
		.polylines={
			{
				.from=0,
				.to=2,
				.data={{60,80},{60,110},{200,110}}
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
        .testid=7,
        .rects={
            {.left=80,.right=100,.top=40,.bottom=80},
            {.left=40,.right=140,.top=120,.bottom=160},
            {.left=60,.right=120,.top=200,.bottom=240}
        },
        .frame={.left=0,.right=200,.top=0,.bottom=300},
        .links={{.from=2,.to=0},{.from=2,.to=1}},
		.faisceau_output={
			{
				.targets={
					{
						.from=2,
						.to=0,
						.expected_path={
							{HORIZONTAL, DECREASE, 5, 1},
							{HORIZONTAL, DECREASE, 5, 0},
							{VERTICAL, DECREASE, 5, 0},
							{VERTICAL, DECREASE, 4, 0},
							{VERTICAL, DECREASE, 3, 0},
							{VERTICAL, DECREASE, 2, 0},
							{VERTICAL, DECREASE, 1, 0},
							{HORIZONTAL, INCREASE, 1, 0},
							{HORIZONTAL, INCREASE, 1, 1},
							{HORIZONTAL, INCREASE, 1, 2}
						}
					},
					{
						.from=2,
						.to=1,
						.expected_path={
							{ VERTICAL, DECREASE, 4, 2 },
						}
					}
				},
				.enlarged={
					{{HORIZONTAL,DECREASE,5,0},{HORIZONTAL,DECREASE,0,5,6}},
					{{HORIZONTAL,DECREASE,5,1},{HORIZONTAL,DECREASE,1,5,6}},
					{{VERTICAL,DECREASE,4,2},{VERTICAL,DECREASE,4,2,5}}
				}
			}
		},
        .polylines={
			{
				.from=2,
				.to=0,
				.data={{60,220},{20,220},{20,60},{80,60}}
			},
			{
				.from=2,
				.to=1,
				.data={{90,200},{90,160}}
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
        .testid=8,
        .rects={
            {.left=0,.right=2,.top=0,.bottom=2},
            {.left=4,.right=5,.top=4,.bottom=5}
        },
        .frame={.left=0,.right=5,.top=0,.bottom=5},
        .links={{.from=0,.to=1}},
        .faisceau_output={
			{
				.targets={
					{
						.from=0,
						.to=1,
						.expected_path={
							{VERTICAL,INCREASE,1,0},
							{VERTICAL,INCREASE,2,0},
							{HORIZONTAL,INCREASE,2,0},
							{HORIZONTAL,INCREASE,2,1}
						}
					}
				},
				.enlarged={}
			}
		},
		.polylines={
			{
				.from=0,
				.to=1,
				.data={{1,2},{1,4},{4,4}}
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
        .testid=9,
        .rects={
            { .left=2, .right=6, .top=2, .bottom=6 },
            { .left=2, .right=6, .top=20, .bottom=24 },
            { .left=36, .right=40, .top=2, .bottom=6 },
            { .left=36, .right=40, .top=20, .bottom=24 }
        },
        .frame={.left=0,.right=80,.top=0,.bottom=60},
        .links={{.from=0,.to=3}},
        .faisceau_output={
            {
				.targets={
					{
                        .from=0,
                        .to=3,
						.expected_path={
                            {HORIZONTAL,INCREASE,1,2},
                            {VERTICAL,INCREASE,1,2},
                            {VERTICAL,INCREASE,2,2},
                            {VERTICAL,INCREASE,3,2},
                            {HORIZONTAL,INCREASE,3,2}
                        }
					}
				},
                .enlarged={}
            }
        },
        .polylines={
            {
                .from=0,
                .to=3,
                .data={{6,4},{21,4},{21,22},{36,22}}
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
        .testid=10,
        .rects={
            {.left=20,.right=60,.top=20,.bottom=60 },
            {.left=20,.right=60,.top=200,.bottom=240 },
            {.left=360,.right=400,.top=20,.bottom=60 },
            {.left=360,.right=400,.top=200,.bottom=240 }
        },
        .frame={.left=0,.right=800,.top=0,.bottom=600},
        .links={{.from=3,.to=0},{.from=3,.to=1}},
		.faisceau_output={
			{
				.targets={
					{
						.from=3,
						.to=0,
						.expected_path={
							{VERTICAL,DECREASE,2,3},
							{HORIZONTAL,DECREASE,2,3},
							{HORIZONTAL,DECREASE,2,2},
							{HORIZONTAL,DECREASE,2,1},
							{VERTICAL,DECREASE,2,1}
						}
					},
					{
						.from=3,
						.to=1,
						.expected_path={
							{HORIZONTAL,DECREASE,4,2}
						}
					}
				},
				.enlarged={
						{{HORIZONTAL,DECREASE,4,2},{HORIZONTAL,DECREASE,2,3,4}},
						{{VERTICAL,DECREASE,2,3},{VERTICAL,DECREASE,2,3,4}}
				}
			}
		},
		.polylines={
			{
				.from=3,
				.to=0,
				.data={{380,200},{380,130},{40,130},{40,60}}
			},
			{
				.from=3,
				.to=1,
				.data={{360,220},{60,220}}
			}
		}
},

{
        .testid=11,
        .rects={
            {.left=10,.right=108,.top=10,.bottom=82 },
            {.left=10,.right=171,.top=10,.bottom=114 },
            {.left=10,.right=164,.top=10,.bottom=114 },
            {.left=10,.right=193,.top=10,.bottom=114 },
            {.left=10,.right=137,.top=10,.bottom=82 }
        },
        .frame={.left=0,.right=203,.top=0,.bottom=124},
        .links={{.from=1,.to=2},{.from=2,.to=0},{.from=3,.to=0},{.from=4,.to=0},{.from=4,.to=2}},
	.faisceau_output={
                        {
                                .targets={
                                        {
                                                .from=0,
                                                .to=2,
                                                .expected_path={}
                                        },
                                        {
                                                .from=0,
                                                .to=3,
                                                .expected_path={}
                                        },
                                        {
                                                .from=0,
                                                .to=4,
                                                .expected_path={}
                                        }
                                },
                                .enlarged={}
                        },
                        {
                                .targets={
                                        {
                                                .from=2,
                                                .to=1,
                                                .expected_path={}
                                        },
                                        {
                                                .from=2,
                                                .to=0,
                                                .expected_path={}
                                        },
                                        {
                                                .from=2,
                                                .to=4,
                                                .expected_path={}
                                        }
                                },
                                .enlarged={}
                        }
        },
	.polylines={
			{
				.from=1,
				.to=2,
				.data={}
			},
			{
				.from=2,
				.to=0,
				.data={}
			},
			{
				.from=3,
				.to=0,
				.data={}
			},
			{
				.from=4,
				.to=0,
				.data={}
			},
			{
				.from=4,
				.to=2,
				.data={}
			}
		}
}
};


struct PostProcessingTestContext
{
	int testid;
	vector<Rect> rects;
	Rect frame;
	vector<Polyline> polylines, expected_polylines;
};

const vector<PostProcessingTestContext> pp_contexts = {
{
        .testid=0,
        .rects={
            {.left=176,.right=176+154,.top=74,.bottom=74+200},
            {.left=397,.right=397+154,.top=375,.bottom=375+200},
            {.left=176,.right=176+154,.top=314,.bottom=314+200}
        },
        .frame={.left=0,.right=1106,.top=0,.bottom=588},
        .polylines={
			{
				.from=1,
				.to=0,
				.data={{397,475},{359,475},{359,242},{330,242}}
			},
			{
				.from=1,
				.to=2,
				.data={{397,444},{330,444}}
			}
		},
		.expected_polylines={
			{
					.from=1,
					.to=0,
					.data={{397,444},{359,444},{359,242},{330,242}}
			},
			{
					.from=1,
					.to=2,
					.data={{397,475},{330,475}}
			}
		}
},
{
		.testid=1,
		.rects={
			{.left=10,.right=10+77,.top=82,.bottom=82+72},
			{.left=248,.right=248+56,.top=10,.bottom=10+72},
			{.left=135,.right=135+42,.top=11,.bottom=11+72},
			{.left=191,.right=192+43,.top=26,.bottom=26+56}			
		},
		.frame={.left=0,.right=314,.top=0,.bottom=164},
		.polylines={
			{
				.from=0,
				.to=1,
				.data={{87,114},{276,114},{276,82}}
			},
			{
				.from=0,
				.to=2,
				.data={{87,122},{156,122},{156,83}}
			},
			{
				.from=0,
				.to=3,
				.data={{87,118},{212,118},{212,82}}
			}
		},
		.expected_polylines={
			{
				.from=0,
				.to=1,
				.data={{87,122},{276,122},{276,82}}
			},
			{
				.from=0,
				.to=2,
				.data={{87,114},{156,114},{156,83}}
			},
			{
				.from=0,
				.to=3,
				.data={{87,118},{212,118},{212,82}}
			}
		}
},
{
		.testid=2,
		.rects={
			{.left=10,.right=10+77,.top=43,.bottom=43+72},
			{.left=127,.right=127+56,.top=10,.bottom=10+72},
			{.left=287,.right=287+42,.top=63,.bottom=63+72},
			{.left=213,.right=213+43,.top=21,.bottom=21+56}
		},
		.frame={.left=0,.right=339,.top=0,.bottom=145},
		.polylines={
			{
				.from=0,
				.to=2,
				.data={{87,103},{287,103}}
			},
			{
				.from=0,
				.to=3,
				.data={{87,103},{234,103},{234,77}}
			}
		},
		.expected_polylines={
			{
				.from=0,
				.to=2,
				.data={{87,103+4},{287,103+4}}
			},
			{
				.from=0,
				.to=3,
				.data={{87,103-4},{234,103-4},{234,77}}
			}
		}
},
{
		.testid=3,
		.rects={
			{.left=459,.right=459+126,.top=10,.bottom=10+200},
			{.left=445,.right=445+203,.top=246,.bottom=246+200},
			{.left=10,.right=10+154,.top=13,.bottom=13+200},
			{.left=209,.right=209+196,.top=17,.bottom=17+200},
			{.left=146,.right=146+245,.top=404,.bottom=404+200},
			{.left=256,.right=256+126,.top=250,.bottom=250+120},
			{.left=13,.right=13+126,.top=251,.bottom=251+72}
		},
		.frame={.left=0,.right=658,.top=0,.bottom=614},
		.polylines={
			{.from=0,.to=1,.data={{522,210},{522,246}}},
			{.from=6,.to=2,.data={{76,251},{76,213}}},
			{.from=0,.to=3,.data={{459,65},{405,65}}},
			{.from=2,.to=3,.data={{164,115},{209,115}}},
			{.from=5,.to=3,.data={{281,250},{281,217}}},
			{.from=1,.to=4,.data={{445,425},{391,425}}},
			{.from=6,.to=5,.data={{139,287},{256,287}}},
			{.from=1,.to=5,.data={{445,310},{382,310}}},
			{.from=0,.to=5,.data={{459,161},{425,161},{425,310},{382,310}}},
			{.from=4,.to=5,.data={{319,404},{319,370}}}
		},
		.expected_polylines={
			{.from=0,.to=1,.data={{522,210},{522,246}}},
			{.from=6,.to=2,.data={{76,251},{76,213}}},
			{.from=0,.to=3,.data={{459,65},{405,65}}},
			{.from=2,.to=3,.data={{164,115},{209,115}}},
			{.from=5,.to=3,.data={{281,250},{281,217}}},
			{.from=1,.to=4,.data={{445,425},{391,425}}},
			{.from=6,.to=5,.data={{139,287},{256,287}}},
			{.from=1,.to=5,.data={{445,314},{382,314}}},
			{.from=0,.to=5,.data={{459,161},{425,161},{425,306},{382,306}}},
			{.from=4,.to=5,.data={{319,404},{319,370}}}
		}
}
};


string print_range_matrix(const Matrix<Span> (&range_matrix)[2])
{
	char buffer[1024 * 1024];
	int pos = 0;
	
	const Direction directions[2]={HORIZONTAL, VERTICAL};
	
	for (Direction direction : directions)
	{
		auto [n, m] = range_matrix[direction].dim();
		for (int i=0; i < n; i++)
		{
			for (int j=0; j < m; j++)
			{
				auto [min,max] = range_matrix[direction](i,j);
				pos += sprintf(buffer+pos, "rm[%s](%d, %d)={.min=%d, .max=%d}\n", dir[direction], i, j, min, max); 
			}
		}
	}
	
	return buffer;
}

/*
1/ Pour chaque rectangle, creer plusieurs entrees dans coords, suivant le nombre de liens connectes a ce rectangle.
Pour le calcul de coords, il faut passer un tableau nblink (nombre de liens par rectangle)
2/ Garder plusieurs meilleurs chemins, si il y en a plusieurs pour la meme distance.
3/ enlarge() elargit chaque path de 1 unit. Lorsqu'ils y a plusieurs chemins, certains sont elimins. Le meilleur reste.
*/


FaiceauOutput compute_faiceau(int testid,
							const vector<Link>& links,
							const vector<Edge>& edges,
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

	vector<DistanceInfo> distance(1000*1000);
	vector<Edge> predecessor(1000*1000);
	unordered_set<uint64_t> source_nodes = compute_nodes(coords, definition_matrix_, rects[from], OUTPUT);
	
	if (testid == 1 && from == 0)
	{
		char file_name[80];
		sprintf(file_name, "sources-nodes-reg-%d-from-%d.txt", testid, from);
		FILE *f = fopen(file_name, "w");
		for (uint64_t u : source_nodes)
			fprintf(f, "%lu,", u);
		fclose(f);	
	}
	
	for (uint64_t u : source_nodes)
	{
		distance[u] = DistanceInfo{
			.distance = INT16_MAX,
			.largeur_chemin = rects[from][Direction(1-parse(u).direction)]
		};
	}
	
	if (testid == 1 && from == 0)
	{
		string serialized_distance = print(distance);
		char file_name[80];
		sprintf(file_name, "distance-before-reg-%d-from-%d.txt", testid, from);
		FILE *f = fopen(file_name, "w");
		fprintf(f, "%s", serialized_distance.c_str());
		fclose(f);	
	}

	dijkstra(edges, range_matrix, source_nodes, distance, predecessor);
	
	if (testid == 1 && from == 0)
	{
		string serialized_distance = print(distance);
		char file_name[80];
		sprintf(file_name, "distance-reg-%d-from-%d.txt", testid, from);
		FILE *f = fopen(file_name, "w");
		fprintf(f, "%s", serialized_distance.c_str());
		fclose(f);	

		string serialized_graph = print_graph(edges);
		sprintf(file_name, "edges-reg-%d-from-%d.txt", testid, from);
		f = fopen(file_name, "w");
		fprintf(f, "%s", serialized_graph.c_str());
		fclose(f);

		string serialized_range_matrix = print_range_matrix(range_matrix);
		sprintf(file_name, "range-matrix-test-reg-%d-from-%d.txt", testid, from);
		f = fopen(file_name, "w");
		fprintf(f, "%s", serialized_range_matrix.c_str());
		fclose(f);
	}

	unordered_map<int, vector<uint64_t> > target_candidates_;
	unordered_map<int, uint64_t> best_target_candidate;

	for (const auto& [from, to] : adj_links)
	{
		unordered_set<uint64_t> target_nodes = compute_nodes(coords, definition_matrix_, rects[to], INPUT);
		compute_target_candidates(source_nodes, target_nodes, distance, predecessor, target_candidates_[to]);
	}

	printf("Selection of the best candidate\n");

	for (const Link& lk : adj_links)
	{
		vector<uint64_t> &candidates = target_candidates_[lk.to];
		uint64_t u = *ranges::min_element(candidates, {}, [&](uint64_t u) {
			unordered_map<int, vector<uint64_t> > target_candidates = target_candidates_;
			target_candidates[lk.to] = { u };
			int n = overlap(adj_links, target_candidates, predecessor);
			return n ;
		}
		);
		best_target_candidate[lk.to] = { u };
	}

	printf("enlarge the faiceau - BEGIN\n");

	unordered_map<Maille, Range> enlarged;

	while (true)
	{
		unordered_map<Maille, Range> enlarged_update = enlarged;

		for (const auto& [from, to] : adj_links)
		{
/*
    auto walk = [&](int to)->generator<int64_t>{
		for (uint64_t u = best_target_candidate[to]; u != 0; u = predecessor[u].u)
		{
			co_yield u;
		}
    };

        +   +   +   +      slide(3)
	  +   +   +   +   +    slide(2)
	1   2   3   4   5   6

	vector<Maille> result = walk(to) |
						views::slide(3) |
						views::transform([](auto rg){return rg[1];})
						views::transform(parse) |
						ranges::to<vector>();
	ranges::reverse(result);
*/
			vector<Maille> result;
			for (uint64_t u = best_target_candidate[to]; u != 0; u = predecessor[u].u)
			{
				result.push_back(parse(u));
			}

			printf("remove first (resp. last) node because it is inside the source (resp. target) rectangle.\n");

			if (result.size() >= 2)
			{
				result.pop_back();
				ranges::reverse(result);
				result.pop_back();
			}

			Target target = { from, to, result };

			if (ranges::find(targets, target) == end(targets))
				targets.push_back(target);
			Matrix<bool> definition_matrix = definition_matrix_;
			for (const Link& other_link : adj_links)
			{
				if (other_link.to == to)
					continue;
				for (uint64_t u = best_target_candidate[other_link.to]; u != 0; u = predecessor.at(u).u)
				{
					Maille m = parse(u);
					Range r = enlarged_update.count(m) ? enlarged_update[m] : Range(m);
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
				ranges.push_back(enlarged_update.count(m) ? enlarged_update[m] : Range(m));
			}
			ranges = enlarge(ranges, definition_matrix, index(coords, rects[from]), index(coords, rects[to]));

			if (!ranges.empty() && !connect_outer_ranges(OuterRangeGraph{ ranges, definition_matrix_, coords }))
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

	printf("enlarge the faiceau - END\n");

	printf("exit compute_faiceau() from=%d \n", from);

	return{ targets, enlarged };
}


void compute_range_matrix(const Matrix<bool> &definition_matrix, vector<int> (&coords)[2], Matrix<Span> (&range_matrix)[2])
{
	auto [n, m] = definition_matrix.dim();

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

			range_matrix[HORIZONTAL](i,j) = {coords[HORIZONTAL][jmin], coords[HORIZONTAL][jmax+1]};
			range_matrix[VERTICAL](i,j) = {coords[VERTICAL][imin], coords[VERTICAL][imax+1]};
		}
	}
}


void compute_polylines(int testid,
						const vector<Rect>& rects,
						const Rect& frame,
						const vector<Link>& links,
						Matrix<bool>& definition_matrix,
						Matrix<Span> (&range_matrix)[2],
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
		ranges::sort(coords_);
		auto ret = ranges::unique(coords_);
		coords_.erase( begin(ret), end(ret) );
	}

	definition_matrix = compute_definition_matrix(rects, coords);

	auto [n1, n2] = definition_matrix.dim();

	range_matrix[0] = range_matrix[1] = Matrix<Span>(n1,n2);
	compute_range_matrix(definition_matrix, coords, range_matrix);

	vector<const Link*> link_pointers;
	ranges::transform(links,
			std::back_inserter(link_pointers),
			[](const Link& lk){return &lk;}
			);

	vector<int> origins;
	while (int &nn = *ranges::max_element(nblinks))
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
	
	const vector<Edge> edges = build_graph(definition_matrix,
											range_matrix,
											coords);

	transform(execution::par_unseq, begin(origins), end(origins), begin(faiceau_output), [&](int from){
		return compute_faiceau(testid, links, edges, definition_matrix, range_matrix, coords, rects, from);
	});

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
				mypath.push_back( enlarged.count(maille) ? enlarged[maille] : Range(maille) );
			}
			ranges::reverse(mypath);
			for (Range &r : mypath)
				reverse(r.way);
		}
		else if(!faiceau_paths.count({to,from}))
		{
			auto& [enlarged, path] = faiceau_paths[{from,to}];
			for (Maille &maille : path)
			{
				mypath.push_back( enlarged.count(maille) ? enlarged[maille] : Range(maille) );
			}
		}
		else
		{
			unordered_map<Range, int> entgegen_ranges;

			auto& [r_enlarged, r_path] = faiceau_paths[{to,from}];
			for (Maille& maille : r_path)
			{
				Range range = r_enlarged.count(maille) ? r_enlarged[maille] : Range(maille);
				reverse(range.way);
				entgegen_ranges[range] = distance(&r_path[0], &maille);
			}

			int i = -1;

			auto& [enlarged, path] = faiceau_paths[{from,to}];
			for (const Maille& maille : path)
			{
				Range range = enlarged.count(maille) ? enlarged[maille] : Range(maille) ;
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
				Range range = r_enlarged.count(maille) ? r_enlarged[maille] : Range(maille) ;
				reverse(range.way);
				mypath.push_back(range);
			}
		}

		polylines.push_back({from, to, compute_polyline(coords, mypath)});
	}
}

vector<int> shared_values(1000);
int index_available=0;
unordered_map<int*, Span> dock_range;

struct SharedValuePoint
{
	int &x, &y;
};

bool operator==(const SharedValuePoint& p1, const SharedValuePoint& p2)
{
	return p1.x == p2.x && p1.y == p2.y ;
}


vector<Point> owned_value(const vector<SharedValuePoint>& svpolyline)
{
	vector<Point> polyline ;
	for (const auto& [x, y] : svpolyline)
	{
		polyline.push_back({x, y});
	}
	return polyline;
}


vector<SharedValuePoint> shared_value(vector<Point>& polyline, const Rect& rfrom, const Rect& rto)
{
	vector<SharedValuePoint> result;

	for (int i=0; i < polyline.size(); i++)
	{
		Point& p = polyline[i] ;
		int *pvalue=0;
		Direction pvalue_direction;

		if (i==0)
		{
			int & x = shared_values[index_available++];
			int & y = shared_values[index_available++] ;
			x = p.x;
			y = p.y;
			result.push_back({x, y});
		}
		else
		{
			SharedValuePoint previous = result[i-1];
			if (previous.x == p.x)
			{
				int & y = shared_values[index_available++] ;
				y = p.y;
				result.push_back({previous.x, y});
				pvalue = & previous.x ;
				pvalue_direction = HORIZONTAL;
			}
			else
			{
				int & x = shared_values[index_available++] ;
				x = p.x;
				result.push_back({x, previous.y});
				pvalue = & previous.y ;
				pvalue_direction = VERTICAL;
			}

			if (i==1 && i+1==polyline.size())
			{
				Span  s1 = rfrom[pvalue_direction], s2 = rto[pvalue_direction] ;
				auto [m1, M1]=s1;
				auto [m2, M2]=s2;
				assert(!(m1 > M2) && !(m2 > M1));
				dock_range[ pvalue ] = {std::max(m1,m2), std::min(M1,M2)};
			}
			else if (i==1)
			{
				dock_range[ pvalue ] = rfrom[pvalue_direction];
			}
			else if (i+1==polyline.size())
			{
				dock_range[ pvalue ] = rto[pvalue_direction];
			}
		}
	}

	return result;
}

struct PolylineSegment
{
	SharedValuePoint p1;
	SharedValuePoint p2;
};

struct SegmentIntersection
{
	PolylineSegment vertical_segment, horizontal_segment;
	SharedValuePoint p;
};

/*
	auto rg = polylines |
		views::transform([](const auto& polyline){
			return polyline |
				views::adjacent<2> |
				views::transform([](const auto& [p1, p2]){
					auto [mx, Mx] = minmax(p1.x, p2.x);
					auto [my, My] = minmax(p1.y, p2.y);
					return Rect{.left=mx, .right=Mx, .top=my, .bottom=My};
				});
		) |
		views::join ;
		
	auto rng = views::cartesian_product(rg, rects) |
		views::filter([](auto [r1, r2]){return intersect_strict(r1,r2);});
	return rng.size();
*/
int intersection_polylines_rectangles(const vector<vector<SharedValuePoint> > &polylines, const vector<Rect> &rects)
{
	int n = 0;

	for (const vector<SharedValuePoint>& polyline : polylines)
	{
		for (int i=0; i+1 < polyline.size(); i++)
		{
			const auto &[x1, y1] = polyline[i];
			const auto &[x2, y2] = polyline[i+1];
			auto [mx, Mx] = minmax(x1, x2);
			auto [my, My] = minmax(y1, y2);
			Rect rr={.left=mx, .right=Mx, .top=my, .bottom=My};

			for (const Rect& r : rects)
			{
				if (intersect_strict(rr, r))
					n++;
			}
		}
	}

	printf("polyline rectangle intersection count : %d\n", n);

	return n;
}

/*
	auto rg = polylines |
		views::transform([](const auto& polyline){return polyline | views::adjacent<2>; } | 
		views::join ;

	vector<SegmentIntersection> intersections =
		views::cartesian_product( rg | views::filter([](const auto& [p1, p2]){return p1.y==p2.y;}), 
								rg | views::filter([](const auto& [p1, p2]){return p1.x==p2.x;}) ) |
		views::filter([](const auto& [[p1, p2], [p3,p4]]){
			auto [xmin, xmax] = minmax(p1.x, p2.x);
			int &y = p1.y ;
			auto [ymin, ymax] = minmax(p3.y, p4.y);
			int& x = p3.x;
			return (xmin < x && x < xmax && ymin < y && y < ymax);
		}) |
		views::transform([](const auto& [[p1, p2], [p3,p4]]){return {{p3, p4}, {p1, p2}, {p3.x,p1.y}};}) |
		ranges::to<vector>();
		
*/

vector<SegmentIntersection> intersection_of_polylines(vector<vector<SharedValuePoint> > &polylines)
{
	vector<PolylineSegment> horizontal_segments, vertical_segments;

	for (vector<SharedValuePoint>& polyline : polylines)
	{
		for (int i=0; i+1 < polyline.size(); i++)
		{
			auto &p1 = polyline[i];
			auto &p2 = polyline[i+1];
			if (p1.x == p2.x)
			{
				vertical_segments.push_back( {p1, p2} );
			}
			if (p1.y == p2.y)
			{
				horizontal_segments.push_back( {p1, p2} );
			}
		}
	}

	vector<SegmentIntersection> intersections;

	for (auto& [p1, p2] : horizontal_segments)
	{
		auto [xmin, xmax] = minmax(p1.x, p2.x);
		int &y = p1.y ;

		for (auto& [p3, p4] : vertical_segments)
		{
			auto [ymin, ymax] = minmax(p3.y, p4.y);
			int& x = p3.x;

			if (xmin < x && x < xmax && ymin < y && y < ymax)
			{
				intersections.push_back({{p3, p4}, {p1, p2}, {x,y}});
			}
		}
	}

	printf("%lu horizontal polyline segments\n", horizontal_segments.size());
	printf("%lu vertical polyline segments\n", vertical_segments.size());
	printf("intersection count: %lu\n", intersections.size());

	for (auto [ver_seg, hor_seg, p] : intersections)
	{
		auto& [p1, p2] = hor_seg;
		auto& [p3, p4] = ver_seg;
		printf("horizontal segment [p1=(%d, %d) p2=(%d, %d)] intersects vertical segment [p3=(%d, %d) p4=(%d, %d)]\n",
					p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y);
	}

	return intersections;
}

struct PointCollision
{
	SharedValuePoint p1, p2;
};

const int TRANSLATION_ON_COLLISION = 4;

/*
	auto extremities = polylines |
		views::transform([](const auto& polyline){
			return polyline | views::stride(polyline.size()-1); 
		}) |
		views::join |
		views::enumerate ;
	
	vector collisions = views::cartesian_product(extremities, extremities) |
		views::filter([](auto [i, pi, j, pj]){return i+2 <= j && pi == pj;}) |
		views::transform([](auto [i, pi, j, pj]){return PointCollision{.p1=pi, .p2=pj};) |
		ranges::to<vector>();
*/

vector<PointCollision> intersection_of_polyline_extremities(vector<vector<SharedValuePoint> > &polylines)
{
	vector<SharedValuePoint> points;
	for (vector<SharedValuePoint>& polyline : polylines)
	{
		if (polyline.size() < 2)
			continue;
		for (auto& p : {polyline[0], polyline.back()})
		{
			points.push_back(p);
        }
	}

	vector<PointCollision> collisions;

	for (int i=0; i < points.size(); i++)
	{
		SharedValuePoint& pi = points[i];
		for (int j=i+2; j < points.size(); j++)
		{
			SharedValuePoint& pj = points[j];
			if (pi == pj)
			{
				collisions.push_back({pi,pj});
			}
		}
	}

	return collisions;
}


bool inside_range(const Span& s, int value)
{
	auto [m, M] = s;
	return m < value && value < M;
};


void post_process_polylines(const vector<Rect>& rects, vector<Polyline> &polylines)
{
	index_available=0;
	dock_range.clear();

	vector<vector<SharedValuePoint> > svpolylines;
	for (auto &[from, to, data] : polylines)
	{
		svpolylines.push_back(shared_value(data, rects[from], rects[to]));
	}

	for (auto& polyline : svpolylines)
	{
		printf("shared points polyline size : %ld\n", polyline.size());
	}
	for (auto& [pvalue, s] : dock_range)
	{
		bool b = ranges::any_of(svpolylines | views::join, [=](SharedValuePoint& p){return &p.x == pvalue;});
		char c = b ? 'x' : 'y' ;
		auto [m, M] = s;
		printf("dock range for %c=%d : [%d, %d]\n", c, *pvalue, m, M);
	}

	vector<SegmentIntersection> intersections = intersection_of_polylines(svpolylines);

	int ni = intersections.size();

	vector<PointCollision> collisions = intersection_of_polyline_extremities(svpolylines);

	for (auto& [p1, p2] : collisions)
	{
		printf("collision detected at (%d, %d)\n", p1.x, p1.y);
	}

	for (auto &[p1, p2] : collisions)
	{
		struct Coll{int* pvalue1; int tr1; int* pvalue2; int tr2;};
		Coll tab[4]={
			{&p1.x, +TRANSLATION_ON_COLLISION, &p2.x, -TRANSLATION_ON_COLLISION},
			{&p1.x, -TRANSLATION_ON_COLLISION, &p2.x, +TRANSLATION_ON_COLLISION},
			{&p1.y, +TRANSLATION_ON_COLLISION, &p2.y, -TRANSLATION_ON_COLLISION},
			{&p1.y, -TRANSLATION_ON_COLLISION, &p2.y, +TRANSLATION_ON_COLLISION},
		};

		for (int i=0; i<4; i++)
		{
			auto& [pvalue1, tr1, pvalue2, tr2] = tab[i];

			if (dock_range.contains(pvalue1) && dock_range.contains(pvalue2) && inside_range(dock_range[pvalue1],*pvalue1+tr1) && inside_range(dock_range[pvalue2],*pvalue2+tr2))
			{
				int value1 = *pvalue1;
				int value2 = *pvalue2;
				assert(value1 == value2);
				*pvalue1 += tr1;
				*pvalue2 += tr2;

 				char c = i < 2 ? 'x' : 'y' ;
                		printf("evaluation of collision spread (%c=%d) => (%c=%d, %c=%d)\n", c, value1, c, *pvalue1, c, *pvalue2);

				vector<SegmentIntersection> intersections_update = intersection_of_polylines(svpolylines);
				int ni_pr = intersection_polylines_rectangles(svpolylines, rects);

				if (intersections_update.size() + ni_pr > ni)
				{
					*pvalue1 = value1;
					*pvalue2 = value2;
					printf("rolling back spread\n");
				}
				else
				{
					intersections = std::move(intersections_update);
					ni = intersections.size();
					printf("collision spread (%c=%d) => (%c=%d, %c=%d) applied\n", c, value1, c, *pvalue1, c, *pvalue2);
					break;
				}
        		}
		}
	}


	for (auto& [ver_seg, hor_seg, p] : intersections)
	{
		auto& [p1, p2] = hor_seg;
		auto& [p3, p4] = ver_seg;

		auto& [x1, y1] = p1;
		auto& [x2, y2] = p2;
		auto& [x3, y3] = p3;
		auto& [x4, y4] = p4;
		auto& [x , y] = p;

		assert(x3 == x4);
		assert(x3 == x);
		assert(y1 == y2);
		assert(y1 == y);

		int* mat[6][2]= {{ &x1, &x}, {&x1,&x2}, {&x,&x2}, { &y3, &y}, {&y3, &y4}, {&y4, &y}};

		for (int i=0; i<6; i++)
		{
			int *pvalue1 = mat[i][0];
			int *pvalue2 = mat[i][1];

			if (dock_range.contains(pvalue1) && dock_range.contains(pvalue2) && inside_range(dock_range[pvalue1], *pvalue2) && inside_range(dock_range[pvalue2], *pvalue1))
			{
				int value1 = *pvalue1;
				int value2 = *pvalue2;
				*pvalue1 = value2;
				*pvalue2 = value1;

 				char c = i < 3 ? 'x' : 'y' ;
                                printf("evaluation of value swap (%c=%d, %c=%d)\n", c, value1, c, value2);

				vector<SegmentIntersection> intersections_update = intersection_of_polylines(svpolylines);
				int ni_pr = intersection_polylines_rectangles(svpolylines, rects);

				if (intersections_update.size() + ni_pr >= ni)
				{
					*pvalue1 = value1;
					*pvalue2 = value2;
					printf("rolling back swap\n");
				}
				else
				{
					ni = intersections_update.size();
					printf("value swap (%c=%d, %c=%d) applied\n", c, value1, c, value2);
				}
			}
		}

		int* mat2[4][2] = {{ &x1, &x}, {&x2,&x}, { &y3, &y}, {&y4, &y}};

		for (int i=0; i<4; i++)
		{
			int *pvalue1 = mat2[i][0];
			int value2 = * mat2[i][1];
			int tr = value2 - *pvalue1;

			if (dock_range.contains(pvalue1) && inside_range(dock_range[pvalue1], *pvalue1 + 2*tr))
			{
				int value1 = *pvalue1;
				*pvalue1 += 2 * tr;

				char c = i < 2 ? 'x' : 'y' ;
				printf("evaluation of translation (%c=%d) applied to (%c=%d)\n", c, 2*tr, c, value1);

				vector<SegmentIntersection> intersections_update = intersection_of_polylines(svpolylines);
				int ni_pr = intersection_polylines_rectangles(svpolylines, rects);

				if (intersections_update.size() + ni_pr >= ni)
				{
					*pvalue1 = value1;
					printf("rolling back translation\n");
				}
				else
				{
					ni = intersections_update.size();
					printf("translation (%c=%d) applied to (%c=%d)\n", c, 2*tr, c, value1);
				}
			}
		}
	}

	for (int i=0; i < svpolylines.size(); i++)
	{
        polylines[i].data = owned_value(svpolylines[i]);
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

	ranges::sort(links);

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
		
		vector<bool> pp_test_results(pp_contexts.size());
		vector<bool> test_results(contexts.size());

		for (const auto& [testid, rects, frame, polylines, expected_polylines] : pp_contexts)
		{
			vector<Polyline> polylines_ = polylines ;
			printf("pp testid=%d\n", testid);
			post_process_polylines(rects, polylines_);
			printf("test %s.\n", polylines_ == expected_polylines ? "OK":"KO");
			bool OK = polylines_ == expected_polylines;
			pp_test_results[testid] = OK;
		}

		for (const TestContext &ctx : contexts)
		{
			high_resolution_clock::time_point t1 = high_resolution_clock::now();

			bool OK=true;

			vector<FaiceauOutput> faisceau_output;
			vector<Polyline> polylines;
			Matrix<bool> definition_matrix;
			Matrix<Span> range_matrix[2];

			printf("testid=%d\n", ctx.testid);

			compute_polylines(ctx.testid, ctx.rects, ctx.frame, ctx.links, definition_matrix, range_matrix, faisceau_output, polylines);

			post_process_polylines(ctx.rects, polylines);

			const string json_data = diagdata(ctx);
			const string json_contexts = contexts_(ctx, polylines);
			char file_name[40];
			sprintf(file_name, "test-reg-%d.json", ctx.testid);
			FILE *f = fopen(file_name, "w");
			fprintf(f, "{\"data\":%s,\"contexts\":%s}", json_data.c_str(), json_contexts.c_str());
			fclose(f);
/*
			printf("%s faisceaux.\n", faisceau_output == ctx.faisceau_output ? "OK":"KO");
			OK &= faisceau_output == ctx.faisceau_output;
*/
			duration<double> time_span = high_resolution_clock::now() - t1;
			printf("%s polylines.\n", polylines == ctx.polylines ? "OK":"KO");
			OK = polylines == ctx.polylines;

			if (OK == false)
			{
				string faisceau_serialized, polylines_serialized;
				print(faisceau_output, faisceau_serialized);
				print(polylines, polylines_serialized);
				char file_name[40];
				sprintf(file_name, "test-reg-%d.cpp", ctx.testid);
				FILE *f = fopen(file_name, "w");
				fprintf(f, "%s,\n%s", faisceau_serialized.c_str(), polylines_serialized.c_str());
				fclose(f);				
			}

			test_results[ctx.testid] = OK;

			printf("%f seconds elapsed.\n", time_span.count());
		}
		
		for (int testid=0; testid < pp_test_results.size(); testid++)
		{
			printf("pp test %d : %s\n", testid, pp_test_results[testid] ? "OK" : "KO");
		}
		
		printf("\nbombix: %ld/%ld pp tests successful.\n", ranges::count(pp_test_results, true), pp_test_results.size());
		
		for (int testid=0; testid < test_results.size(); testid++)
		{
			printf("test %d : %s\n", testid, test_results[testid] ? "OK" : "KO");
		}

		printf("\nbombix: %ld/%ld tests successful.\n", ranges::count(test_results, true), test_results.size());

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

		const int testid = -1;
		vector<FaiceauOutput> faiceau_output;
		vector<Polyline> polylines;
		Matrix<bool> definition_matrix;
		Matrix<Span> range_matrix[2];

		compute_polylines(testid, rects, frame, links, definition_matrix, range_matrix, faiceau_output, polylines);
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

		const int testid = -1;
        vector<FaiceauOutput> faiceau_output;
        vector<Polyline> polylines;
		Matrix<bool> definition_matrix;
		Matrix<Span> range_matrix[2];

        compute_polylines(testid, rects, frame, links, definition_matrix, range_matrix, faiceau_output, polylines);
		post_process_polylines(rects, polylines);

        string json = polyline2json(polylines);
        static char res[100000];
		sprintf(res, "%s", json.c_str());
        return res;
}
}
/*
/var/www/projects/ludo$ emcc ~/linkedboxdraw/bombix-origine.cpp -o bombix-origine.html -s EXPORTED_FUNCTIONS='["_bombix"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_ES6=1 -sMODULARIZE -s EXPORT_NAME="createMyModule"
puis dans https://dev.diskloud.fr/ludo/bombix.html

bombix=Module.cwrap("bombix","string",["string","string","string","string"])
exemple:
2 rectangles taille (56,56)=("038,"038") en hexa.
translations: (10,10) et (100,10) = ("00a","00a") et ("064","00a").
frame=(left,right,top,bottom):(0,200,0,100)=("0000","00c8","0000","0064").
links=(0,1):("00","01").
bombix("038038038038","00a00a06400a","000000c800000064","0001")
*/
