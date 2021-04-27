/**
* \copyright Copyright (c) 2015-2017 Ludovic Aubert
*            All Rights Reserved
*            DO NOT ALTER OR REMOVE THIS COPYRIGHT NOTICE
*
* \file bombix.cpp
*
* - 11/23/2016 by Ludovic Aubert : creation
*/
#define WASM

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
using namespace std;
using namespace std::chrono;


enum Direction : uint16_t
{
	HORIZONTAL,
	VERTICAL
};

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


struct Range
{
	Direction direction;
	Way way;
	int16_t value, min, max;
	
	int16_t operator[](Way w) const
	{
		switch (w)
		{
		case INCREASE:
			return max;
		case DECREASE:
			return min;
		}
	}
};

//TODO: le compilateur peut-il generer une implementation par default pour tout les operateurs == sans qu'il y ai besoin d'ecrire
// le code ? C++17 ? C++20 ?
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
	int16_t value, other;
};


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
	assert(m.value <= UINT8_MAX);
	assert(m.other <= UINT8_MAX);
	uint64_t u = 1 + (m.value << 1) + (m.other << (8 + 1)) + (m.direction << (8 + 8 + 1)) + ((m.way + 1) << (8 + 8 + 1 + 1));
	assert(u < 1000 * 1000);
	return u;
}

Maille parse(uint64_t u)
{
	u -= 1;
	u >>= 1;
	Maille m;
	m.value = u & 0xFF;
	assert(m.value <= UINT8_MAX);
	u >>= 8;
	m.other = u & 0xFF;
	assert(m.other <= UINT8_MAX);
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

bool operator==(const Point& p1, const Point& p2)
{
	return memcmp(&p1, &p2, sizeof(Point))==0;
}

bool operator!=(const Point& p1, const Point& p2)
{
	return memcmp(&p1, &p2, sizeof(Point)) != 0;
}

struct Rect
{
	int left, right, top, bottom;
};

struct RectBand
{
	Direction direction;
	int min, max;
};

RectBand rectband(const Rect& r, Direction direction)
{
	switch (direction)
	{
	case HORIZONTAL:
		return { direction, r.top, r.bottom };
	case VERTICAL:
		return { direction, r.left, r.right };
	}
}

Range intersection(const Range& r, const RectBand& band)
{
	assert(r.direction == band.direction);
	Range rg = r;
	rg.min = max<int>(rg.min, band.min);
	rg.max = min<int>(rg.max, band.max);
	return rg;
}

//TODO: mauvais nommage
Rect intersection(const Rect& r, const RectBand& band)
{
	Rect rec = r;
	switch (band.direction)
	{
	case HORIZONTAL:
		//assert(rec.top <= band.min); FAUX
		//assert(rec.bottom >= band.max); FAUX
		rec.top = band.min;
		rec.bottom = band.max;
		break;
	case VERTICAL:
		//assert(rec.left <= band.min); FAUX
		//assert(rec.right >= band.max); FAUX
		rec.left = band.min;
		rec.right = band.max;
		break;
	}
	return rec;
}

template <typename T>
struct Matrix
{
	Matrix() {}
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
	
//suppose implicitement que T=bool.	
	T operator()(int i, int j) const
	{
		if (i < 0)
			return false;
		if (i >= _n)
			return false;
		if (j < 0)
			return false;
		if (j >= _m)
			return false;
		return _data[i*_m + j];
	}
	
	T operator()(const Maille& m) const
	{
		switch (m.direction)
		{
		case HORIZONTAL:
			return (*this)(m.value, m.other);
		case VERTICAL:
			return (*this)(m.other, m.value);
		}
	}
	
	T& operator()(const Maille& m)
	{
		switch (m.direction)
		{
		case HORIZONTAL:
			return (*this)(m.value, m.other);
		case VERTICAL:
			return (*this)(m.other, m.value);
		}
	}
	
	int _n=0, _m=0;
	T *_data = nullptr;
};


struct Graph
{
	const Matrix<bool> &definition_matrix;
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

bool operator==(const Link& lk1, const Link& lk2)
{
	return memcmp(&lk1, &lk2, sizeof(Link)) == 0;
}

//TODO: could use a default compiler generated implementation ? C++17 ? C++20 ?
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
}



struct FaiceauOutput
{
	vector<Target> targets;
	unordered_map<Maille, Range> enlarged;
};

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
	for (const Polyline& polyline : polylines)
	{
		pos += sprintf(buffer + pos, "\t\t\t{\n");
		pos += sprintf(buffer + pos, "\t\t\t\t/*from*/%d,\n", polyline.from);
		pos += sprintf(buffer + pos, "\t\t\t\t/*to*/%d,\n", polyline.to);
		pos += sprintf(buffer + pos, "\t\t\t\t/*data*/{");
		
		for (const Point& p : polyline.data)
		{
			pos += sprintf(buffer + pos, "{%d, %d},", p.x, p.y);
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
	const char* dir[2] = { "HORIZONTAL", "VERTICAL"};
	const char* way[3] = {"DECREASE",0,"INCREASE"};
	
	pos += sprintf(buffer + pos, "\t\t/*faiceau output*/{\n");
	
	for (const FaiceauOutput& faiceau : faiceau_output)
	{
		pos += sprintf(buffer + pos, "\t\t\t{\n");
		pos += sprintf(buffer + pos, "\t\t\t\t/*targets*/{\n");
		
		for (const Target& target : faiceau.targets)
		{
			pos += sprintf(buffer + pos, "\t\t\t\t\t{\n");
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t/*from*/%d,\n", target.from);
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t/*to*/%d,\n", target.to);
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t/*expected path*/{\n");
			
			for (const Maille& m : target.expected_path)
			{
				pos += sprintf(buffer + pos, "\t\t\t\t\t\t\t{%s, %s, %d, %d},\n", dir[m.direction], way[1+m.way], m.value, m.other);
			}
			pos += sprintf(buffer + pos, "\t\t\t\t\t\t}\n");
			pos += sprintf(buffer + pos, "\t\t\t\t\t}\n");
		}
		
		pos += sprintf(buffer + pos, "\t\t\t\t},\n");
		
		pos += sprintf(buffer + pos, "\t\t\t\t/*enlarged*/{\n");
	//TODO: use destructuring
		for (const pair<Maille, Range>& p : faiceau.enlarged)
		{
			Maille m;
			Range r;
			tie(m, r) = p;
			pos += sprintf(buffer + pos, "\t\t\t\t\t{{%s,%s,%d,%d},{%s,%s,%d,%d,%d}},\n", dir[m.direction], way[1+m.way], m.value, m.other, dir[r.direction], way[1+r.way], r.value, r.min, r.max);
		}
		pos += sprintf(buffer + pos, "\t\t\t\t},\n");
		
		pos += sprintf(buffer + pos, "\t\t\t},\n");
	}
	
	pos += sprintf(buffer + pos, "\t\t}\n");
	
	serialized = buffer;
}

//TODO: use destructuring when it is available

vector<Edge> adj_list(const Graph& graph, uint64_t u)
{
	vector<Edge> adj;

	const int TURN_PENALTY = 1;
	const Matrix<bool> & definition_matrix = graph.definition_matrix;
	const vector<int> (&coords)[2] = graph.coords;
	
	Maille r = parse(u);
	
	size_t size = 0;
	
	Maille next = r;
	next.value += next.way;
	
	if (definition_matrix(next))
	{
		uint64_t v = serialize(next);
		int distance = 0;
		for (Maille* m : {&r, &next})
		{
			auto& tab = coords[m->direction];
			distance += tab[m->value+1] - tab[m->value];
		}
		adj.push_back({u, v, distance});
	}
	
	for (Way way : { DECREASE, INCREASE})
	{
		Maille next = r;
		next.direction = other(r.direction);
		next.way = way;
		swap(next.value, next.other);
		
		if (definition_matrix(next))
		{
			uint64_t v = serialize(next);
			adj.push_back({ u, v, TURN_PENALTY });
		}
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

template <typename Graph>
vector<Edge> adj_list(const Graph& graph, uint64_t u)
{
	static_assert(is_same<Graph, InnerRangeGraph>::value || is_same<Graph, OuterRangeGraph>::value, "");
	
//TODO: use destructuring
	const Matrix<bool>& definition_matrix = graph.definition_matrix;
	const vector<Range>& path = graph.path;
	const vector<int> (&coords)[2] = graph.coords;
	
	Rect rec = {0,0,0,0};
	
	InnerRange ir = parse_ir(u);
	
	vector<Edge> adj;
	
	if (ir.range_index + 1 == path.size())
		return adj;
	
	if (path[ir.range_index + 1].direction == path[ir.range_index].direction)
	{
		InnerRange next = ir;
		next.range_index++;
		uint64_t v = serialize(next);
		adj.push_back({ u, v, compute_distance(next, path, coords)});
	}
	else
	{
		const Range& r = path[ir.range_index];
		
		rec = intersection(rec, RectBand{ r.direction, ir.min, ir.max });
		
		const Range& next_r = path[ir.range_index+1];
		
		struct Bound
		{
			int16_t min, max;
		};
		
		vector<Bound> bounds;
		
	//TODO: use C++17 constexpr if
		if (is_same<Graph, InnerRangeGraph>::value)
		{
			for (int16_t min = next_r.min; min <= next_r.max; min++)
			{
				for (int16_t max = min; max <= next_r.max; max++)
				{
					bounds.push_back({ min, max});
				}
			}
		}
		if (is_same<Graph, OuterRangeGraph>::value)
		{
			bounds.push_back({ next_r.min, next_r.max });
		}
	
		for (const Bound& bound : bounds)
		{	
			rec = intersection(rec, RectBand{ next_r.direction, bound.min, bound.max});
				
			bool detect = false;
			for (int i = rec.left; i <= rec.right; i++)
			{
				for (int j = rec.top; j <= rec.bottom; j++)
				{
				//detect si n'est pas definie
					detect |= !definition_matrix(i, j);
				}
			}
			if (!detect)
			{
				InnerRange next = { bound.min, bound.max, ir.range_index + 1 };
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


//TODO: use destructuring
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
	Matrix<bool> m(coords[0].size() - 1, coords[1].size() - 1, true);
	for (const Rect& r : rects)
	{
//TODO: use destructuring
		Rect ir = index(coords, r);
		int left = ir.left, right = ir.right, top = ir.top, bottom = ir.bottom;
		
		for (int i = left; i <= right; i++)
		{
			for (int j = top; j <= bottom; j++)
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
	
//TODO: use destructuring
	Rect ir = index(coords, r);
	int16_t left = ir.left, right = ir.right, top = ir.top, bottom = ir.bottom;
	
	for (int16_t i = left; i <= right; i++)
	{
		if (top > 1)
			result.insert({VERTICAL, input_output(ioswitch, DECREASE), int16_t(top-1), i});
		if (bottom + 1 < coords[VERTICAL].size())
			result.insert({VERTICAL, input_output(ioswitch, INCREASE), int16_t(bottom+1), i});
	}
	
	for (int16_t j = top; j <= bottom; j++)
	{
		if (left > 1)
			result.insert({HORIZONTAL, input_output(ioswitch, DECREASE), int16_t(left-1),j});
		if (right+1 < coords[HORIZONTAL].size())
			result.insert({HORIZONTAL, input_output(ioswitch, INCREASE), int16_t(right+1), j});
	}
	
	unordered_set<uint64_t> defined;
	
	for (const Maille& m : result)
	{
		if (definition_matrix(m))
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
	
	//TODO: use C++17 structured binding
	for (const auto& kv : source_node_distance)
	{
		uint64_t u = kv.first;
		int distance_v = kv.second;
		int weight = distance_v;
		Q.push({ 0, u, weight, distance_v});
	}
	
	while (!Q.empty())
	{
	//TODO: use C++17 structured binding
		QueuedEdge queued_edge = Q.top();
		Q.pop();
		
		if (queued_edge.distance_v < distance[queued_edge.v])
		{
			predecessor[queued_edge.v] = { queued_edge.u, queued_edge.v, queued_edge.weight};
			distance[queued_edge.v] = queued_edge.distance_v;
			
			for (const Edge& adj_edge : adj_list(graph, queued_edge.v))
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
			int other;
			if (all_of(begin(index_range), end(index_range), [&](int k){
				
				other = path[k][way] + way;
				
				switch (path[i].direction)
				{
				case HORIZONTAL:
					return 0 <= path[k].min + way && path[k].max + way < m._m && m(path[k].value, other);
				case VERTICAL:
					return 0 <= path[k].min + way && path[k].max + way < m._n && m(other, path[k].value);
				}
			}))
			{
				for (Range &r : ranges)
				{
					(way > 0 ? r.max : r.min) = other;
				}
			}
		}
		
		if (i == 0)
		{
			for (Range &r : ranges)
			{
				r = intersection(r, rectband(rfrom, path[i].direction));
			}
		}
		
		if (j == path.size())
		{
			for (Range &r : ranges)
			{
				r = intersection(r, rectband(rto, path[i].direction));
			}
		}
		
		for (Range &r : ranges)
			result.push_back(r);
		
		i = j;
	}
	
	assert(path.size() == result.size());
	return result;
}

Range range(const Maille& m)
{
	return { m.direction, m.way, m.value, m.other, m.other};
}


vector<Range> compute_inner_ranges(const InnerRangeGraph &graph)
{
	const vector<Range> &ranges = graph.path;
	const Matrix<bool> &definition_matrix = graph.definition_matrix;
	const vector<int>(&coords)[2] = graph.coords;
	
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
	//TODO: use destructuring
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
	for (const Link& link : adj_links)
	{
		for (uint64_t u : target_candidates.at(link.to))
		{
			while (u != 0)
			{
				hit_count[u]++;
				u = predecessor.at(u).u;
			}
		}
	}
	
	int n=0;
//TODO: use C++17 destructuring
	for (auto p : hit_count)
	{
		int c = p.second;
		if (c >= 2)
			n += c;
	}
	
	return n;
}


string polyline2json(const vector<Polyline>& polylines)
{
	char buffer[10 * 1024];
	int pos = 0;
	
	pos += sprintf(buffer + pos, "[");
	
//TODO: use C++17 destructuring
	for (const Polyline& polyline : polylines)
	{
		pos += sprintf(buffer + pos, "{\"polyline\":[");
//TODO: use C++17 destructuring
		for (const Point& p : polyline.data)
		{
			pos += sprintf(buffer + pos, "{\"x\":%d,\"y\":%d},", p.x, p.y);
		}
		if (buffer[pos-1]==',')
			pos--;
		pos += sprintf(buffer + pos, "],\"from\":%d,\"to\":%d},", polyline.from, polyline.to);
	}
	
	if (buffer[pos-1]==',')
		pos--;
	pos += sprintf(buffer + pos, "]");
	
	return buffer;
}


/*
1/ Pour chaque rectangle, creer plusieurs entrees dans coords, suivant le nombre de liens connectes a ce rectangle.
Pour le calcul de coords, il faut passer un tableau nblink (nombre de liens par rectangle)
2/ Garder plusieurs meilleurs chemins, si il y en a plusieurs pour la meme distance.
3/ enlarge() elargit chaque path de 1 unit. Lorsqu'ils y a plusieurs chemins, certains sont elimins. Le meilleur reste.
*/


FaiceauOutput compute_faiceau(const vector<Link>& links, 
				const Matrix<bool>& definition_matrix_, 
				const vector<int>(&coords)[2], 
				const vector<Rect>& rects,
				int from)
{
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

	dijkstra(Graph{ definition_matrix_, coords }, source_node_distance, distance, predecessor);

	unordered_map<int, vector<uint64_t> > target_candidates_;
	unordered_map<int, uint64_t> best_target_candidate;

	//TODO: use destructuring
	for (const Link& link : adj_links)
	{
		unordered_set<uint64_t> target_nodes = compute_nodes(coords, definition_matrix_, rects[link.to], INPUT);
		compute_target_candidates(source_nodes, target_nodes, distance, predecessor, target_candidates_[link.to]);
	}

	//Selection of the best candidate
	for (const Link& link : adj_links)
	{
		vector<uint64_t> &candidates = target_candidates_[link.to];
		uint64_t u = *min_element(begin(candidates), end(candidates), [&](uint64_t u, uint64_t v) {
			unordered_map<int, vector<uint64_t> > target_candidates = target_candidates_;
			target_candidates[link.to] = { u };
			int n1 = overlap(adj_links, target_candidates, predecessor);
			target_candidates[link.to] = { v };
			int n2 = overlap(adj_links, target_candidates, predecessor);
			return n1 < n2;
		}
		);
		best_target_candidate[link.to] = { u };
	}

	//enlarge the faiceau - BEGIN

	unordered_map<Maille, Range> enlarged;

	while (true)
	{
		unordered_map<Maille, Range> enlarged_update = enlarged;

		for (const Link& link : adj_links)
		{
			vector<Maille> result;
			for (uint64_t u = best_target_candidate[link.to]; u != 0; u = predecessor.at(u).u)
			{
				result.push_back(parse(u));
			}
			reverse(begin(result), end(result));

			Target target = { link.from, link.to, result };

			if (find(begin(targets), end(targets), target) == end(targets))
				targets.push_back(target);
			Matrix<bool> definition_matrix = definition_matrix_;
			for (const Link& other_link : adj_links)
			{
				if (other_link.to == link.to)
					continue;
				for (uint64_t u = best_target_candidate[other_link.to]; u != 0; u = predecessor.at(u).u)
				{
					Maille m = parse(u);
					//TODO: use destructuring
					Direction direction = m.direction;
					Way way = m.way;
					int16_t value = m.value;
					int16_t other = m.other;
					Range r = enlarged_update.count(m) ? enlarged_update[m] : Range{ direction, way, value, other, other };
					for (int16_t other = r.min; other <= r.max; other++)
					{
						definition_matrix(Maille{ m.direction, m.way, m.value, other }) = false;
					}
				}
			}
			vector<Range> ranges;
			for (Maille& m : result)
			{
				ranges.push_back(enlarged_update.count(m) ? enlarged_update[m] : Range{ m.direction, m.way,m.value,m.other,m.other });
			}
			ranges = enlarge(ranges, definition_matrix, index(coords, rects[link.from]), index(coords, rects[link.to]));

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

	return{ targets, enlarged };
}

void compute_polylines(const vector<Rect>& rects,
						const Rect& frame,
						const vector<Link>& links,
						vector<FaiceauOutput>& faiceau_output,
						vector<Polyline>& polylines)
{
	int n = rects.size();
	vector<int> nblinks(n,0 );
	
	//TODO: use destructuring
	for (const Link& link : links)
	{
		int from = link.from, to = link.to;
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

	vector<int> origins;
	while (int &nn = *max_element(begin(nblinks), end(nblinks)))
	{
		int from = distance(&nblinks[0], &nn);
		origins.push_back(from);

		vector<Link> adj_links;
		for (const Link& link : links)
		{
			if (link.from == from)
				adj_links.push_back({ link.from, link.to });
			else if (link.to == from)
				adj_links.push_back({ link.to, link.from });
		}

		nblinks[from] = 0;
		for (const Link& link : adj_links)
		{
			for (int n : {link.from, link.to})
			{
				if (nblinks[n] == 1)
					nblinks[n] = 0;
			}
		}
	}

	faiceau_output.resize(origins.size());

//	#pragma omp parallel for
	for (int i = 0; i < origins.size(); i++)
	{
		faiceau_output[i] = compute_faiceau(links, definition_matrix_, coords, rects, origins[i]);
	}
	
	unordered_map<Link, FaiceauPath> faiceau_paths;
	for (FaiceauOutput & faiceau : faiceau_output)
	{
	//TODO: use destructuring
		for (Target &target : faiceau.targets)
		{
			faiceau_paths[{target.from, target.to}] = {faiceau.enlarged, target.expected_path};
		}
	}
	
	//TODO: use destructuring
	for (const Link& link : links)
	{
		int from = link.from, to = link.to;
		
		vector<Range> mypath;
		
		if (!faiceau_paths.count({from,to}))
		{
			FaiceauPath &fp = faiceau_paths[{to,from}];
			unordered_map<Maille,Range>& enlarged = fp.enlarged;
			for (Maille &maille : fp.path)
			{
			//TODO: use destructuring
				Direction direction = maille.direction;
				Way way = maille.way;
				int16_t value = maille.value;
				int16_t other = maille.other;
				mypath.push_back({enlarged.count(maille) ? enlarged[maille] : Range{direction, way, value, other, other}});
			}
			reverse(begin(mypath),end(mypath));
			for (Range &r : mypath)
				reverse(r.way);
		}
		else if(!faiceau_paths.count({to,from}))
		{
			FaiceauPath &fp = faiceau_paths[{from,to}];
			unordered_map<Maille,Range>& enlarged = fp.enlarged;
			for (Maille &maille : fp.path)
			{
				//TODO: destructuring
				Direction direction = maille.direction;
				Way way = maille.way;
				int16_t value = maille.value;
				int16_t other = maille.other;
				mypath.push_back(enlarged.count(maille) ? enlarged[maille] : Range{direction, way, value, other, other});
			}
		}
		else
		{
			unordered_map<Range, int> entgegen_ranges;
			
			//TODO: use destructuring
			FaiceauPath &rfp = faiceau_paths[{to,from}];
			for (Maille &maille : rfp.path)
			{
				//TODO: use destructuring
				Direction direction = maille.direction;
				Way way = maille.way;
				int16_t value = maille.value;
				int16_t other = maille.other;
				Range range = rfp.enlarged.count(maille) ? rfp.enlarged[maille] : Range{direction, way, value, other, other};
				reverse(range.way);
				entgegen_ranges[range] = distance(&rfp.path[0], &maille);
			}
			
			int i = -1;
			
			FaiceauPath &fp = faiceau_paths[{from,to}];
			for (Maille &maille : fp.path)
			{
				//TODO: use destructuring
				Direction direction = maille.direction;
				Way way = maille.way;
				int16_t value = maille.value;
				int16_t other = maille.other;
				Range range = fp.enlarged.count(maille) ? fp.enlarged[maille] : Range{direction, way, value, other, other};
				mypath.push_back(range);
				if (entgegen_ranges.count(range))
				{
					i = entgegen_ranges[range] - 1;
					break;
				}
			}
			
			for (; i >= 0; i--)
			{
				Maille &maille = rfp.path[i];
				//TODO: use destructuring
				Direction direction = maille.direction;
				Way way = maille.way;
				int16_t value = maille.value;
				int16_t other = maille.other;
				Range range = rfp.enlarged.count(maille) ? rfp.enlarged[maille] : Range{direction, way, value, other, other};
				reverse(range.way);
				mypath.push_back(range);
			}
		}
		
		polylines.push_back({from, to, compute_polyline(coords, mypath)});
	}
	
	assert(nblinks == vector<int>(n,0));
}



string bombix(const string& rectdim, const string& translations, const string& slinks, const string& sframe)
{
	Rect frame;
	vector<Rect> rects;
	vector<Link> links;
	
	int pos;
	
	pos = 0;
	int width, height, x, y;
	while (sscanf(rectdim.c_str() + pos, "%3x%3x", &width, &height) == 2 &&
	      sscanf(translations.c_str() + pos, "%3x%3x", &x, &y) == 2)
	{
		rects.push_back({x, x + width, y, y + height});
		pos += 6;
	}
	
	pos = 0;
	int from, to;
	while (sscanf(slinks.c_str() + pos, "%2x%2x", &from, &to) == 2)
	{
		links.push_back({from, to});
		pos += 4;
	}
	
	sscanf(sframe.c_str(), "%4x%4x%4x%4x", &frame.left, &frame.right, &frame.top, &frame.bottom);
	
	vector<FaiceauOutput> faiceau_output;
	vector<Polyline> polylines;
		
	compute_polylines(rects, frame, links, faiceau_output, polylines);
	string json = polyline2json(polylines);
	return json;
}
