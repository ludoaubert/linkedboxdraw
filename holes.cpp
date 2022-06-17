#include <vector>
#include <algorithm>
#include <ranges>
#include <stdio.h>
using namespace std;

const int FRAME_BORDER = 30 ;
const int RECT_BORDER = 20 ;

struct MyRect
{
	int16_t m_left=0, m_right=0, m_top=0, m_bottom=0 ;
} ;

struct Edge {
	int from;
	int to;
};

MyRect compute_frame(const vector<MyRect>& rectangles)
{
        MyRect frame ;

	if (rectangles.size()==0)
		return frame;
	
	return {
		ranges::min(rectangles | views::transform(&MyRect::m_left)),
		ranges::max(rectangles | views::transform(&MyRect::m_right)),
		ranges::min(rectangles | views::transform(&MyRect::m_top)),
		ranges::max(rectangles | views::transform(&MyRect::m_bottom))
	};
}


bool intersect_strict(const MyRect& r1, const MyRect& r2)
{
        return !(r1.m_left >= r2.m_right || r1.m_right <= r2.m_left || r1.m_top >= r2.m_bottom || r1.m_bottom <= r2.m_top) ;
}

//is r1 inside r2 ?
bool is_inside(const MyRect& r1, const MyRect& r2)
{
	return r1.m_right <= r2.m_right && r1.m_left >= r2.m_left && r1.m_top >= r2.m_top && r1.m_bottom <= r2.m_bottom ;
}

void main()
{

	struct TestContext {int testid; vector<MyRect> input_rectangles; vector<Edge> edges; vector<MyRect> expected_rectangles; };

	const TestContext test_contexts[1]={
	{
		.testid=3,
		.input_rectangles = {
			{.m_left=396-RECT_BORDER+FRAME_BORDER, .m_right=396+162+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+104+RECT_BORDER+FRAME_BORDER},//8
			{.m_left=320-RECT_BORDER+FRAME_BORDER, .m_right=320+182+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+72+RECT_BORDER+FRAME_BORDER},//9
			{.m_left=453-RECT_BORDER+FRAME_BORDER, .m_right=453+105+RECT_BORDER+FRAME_BORDER, .m_top=218-RECT_BORDER+FRAME_BORDER, .m_bottom=218+72+RECT_BORDER+FRAME_BORDER},//10
			{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=10-RECT_BORDER+FRAME_BORDER, .m_bottom=10+152+RECT_BORDER+FRAME_BORDER},//21
			{.m_left=598-RECT_BORDER+FRAME_BORDER, .m_right=598+126+RECT_BORDER+FRAME_BORDER, .m_top=202-RECT_BORDER+FRAME_BORDER, .m_bottom=202+88+RECT_BORDER+FRAME_BORDER},//24
			{.m_left=750-RECT_BORDER+FRAME_BORDER, .m_right=750+147+RECT_BORDER+FRAME_BORDER, .m_top=346-RECT_BORDER+FRAME_BORDER, .m_bottom=346+120+RECT_BORDER+FRAME_BORDER},//25
			{.m_left=273-RECT_BORDER+FRAME_BORDER, .m_right=273+140+RECT_BORDER+FRAME_BORDER, .m_top=154-RECT_BORDER+FRAME_BORDER, .m_bottom=154+120+RECT_BORDER+FRAME_BORDER},//26
			{.m_left=542-RECT_BORDER+FRAME_BORDER, .m_right=542+168+RECT_BORDER+FRAME_BORDER, .m_top=330-RECT_BORDER+FRAME_BORDER, .m_bottom=330+136+RECT_BORDER+FRAME_BORDER},//27
			{.m_left=335-RECT_BORDER+FRAME_BORDER, .m_right=335+168+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+120+RECT_BORDER+FRAME_BORDER},//28
			{.m_left=556-RECT_BORDER+FRAME_BORDER, .m_right=556+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+104+RECT_BORDER+FRAME_BORDER},//30
			{.m_left=764-RECT_BORDER+FRAME_BORDER, .m_right=764+133+RECT_BORDER+FRAME_BORDER, .m_top=186-RECT_BORDER+FRAME_BORDER, .m_bottom=186+120+RECT_BORDER+FRAME_BORDER},//32
			{.m_left=743-RECT_BORDER+FRAME_BORDER, .m_right=743+147+RECT_BORDER+FRAME_BORDER, .m_top=506-RECT_BORDER+FRAME_BORDER, .m_bottom=506+168+RECT_BORDER+FRAME_BORDER},//44
			{.m_left=93-RECT_BORDER+FRAME_BORDER, .m_right=93+140+RECT_BORDER+FRAME_BORDER, .m_top=153-RECT_BORDER+FRAME_BORDER, .m_bottom=153+88+RECT_BORDER+FRAME_BORDER},//48
			{.m_left=20-RECT_BORDER+FRAME_BORDER, .m_right=10+155+RECT_BORDER+FRAME_BORDER, .m_top=281-RECT_BORDER+FRAME_BORDER, .m_bottom=281+120+RECT_BORDER+FRAME_BORDER},//52
			{.m_left=21-RECT_BORDER+FRAME_BORDER, .m_right=21+175+RECT_BORDER+FRAME_BORDER, .m_top=441-RECT_BORDER+FRAME_BORDER, .m_bottom=441+136+RECT_BORDER+FRAME_BORDER}//53
		},

		.edges = {
			{.from=13,.to=12},
			{.from=13,.to=14},
			{.from=4,.to=7},
			{.from=3,.to=4},
			{.from=1,.to=7},
			{.from=5,.to=7},
			{.from=7,.to=11},
			{.from=6,.to=7},
			{.from=10,.to=7},
			{.from=9,.to=7},
			{.from=2,.to=7},
			{.from=8,.to=7},
			{.from=0,.to=3},
			{.from=12,.to=6}
		},

		.expected_rectangles = {
			{.m_left=396,.m_right=396+162,.m_top=10,.m_bottom=10+104},//8
			{.m_left=320,.m_right=320+182,.m_top=330,.m_bottom=330+72},//9
			{.m_left=453,.m_right=453+105,.m_top=218,.m_bottom=218+72},//10
			{.m_left=598,.m_right=598+126,.m_top=10,.m_bottom=10+152},//21
			{.m_left=598,.m_right=598+126,.m_top=202,.m_bottom=202+88},//24
			{.m_left=750,.m_right=750+147,.m_top=346,.m_bottom=346+120},//25
			{.m_left=273,.m_right=273+140,.m_top=154,.m_bottom=154+120},//26
			{.m_left=542,.m_right=542+168,.m_top=330,.m_bottom=330+136},//27
			{.m_left=335,.m_right=335+168,.m_top=506,.m_bottom=506+120},//28
			{.m_left=556,.m_right=556+147,.m_top=506,.m_bottom=506+104},//30
			{.m_left=764,.m_right=764+133,.m_top=186,.m_bottom=186+120},//32
			{.m_left=743,.m_right=743+147,.m_top=506,.m_bottom=506+168},//44
			{.m_left=93,.m_right=93+140,.m_top=153,.m_bottom=153+88},//48
			{.m_left=10,.m_right=10+155,.m_top=281,.m_bottom=281+120},//52
			{.m_left=11,.m_right=11+175,.m_top=441,.m_bottom=441+136}//53
		}
	}
	};

	for (const auto& [testid, input_rectangles, edges, expected_rectangles] : test_contexts)
	{
		const MyRect frame = compute_frame(input_rectangles);
		
		vector<MyRect> holes;

		for (const auto& [m_left, m_right, m_top, m_bottom] : input_rectangles)
		{
			const MyPoint pt4[4]={
						{.x=m_left, .y=m_top},
						{.x=m_left, .y=m_bottom},
						{.x=m_right, .y=m_top},
						{.y=m_right, .y=m_bottom}
						};

			const MyPoint directions[4][3]={
								{{.x=-1, .y=-1},{.x=+1, .y=-1},{.x=-1, .y=+1}},
								{{.x=-1, .y=+1},{.x=+1, .y=+1},{.x=-1, .y=-1}},
								{{.x=+1, .y=+1},{.x=+1, .y=-1},{.x=-1, .y=-1}},
								{{.x=-1, .y=+1},{.x=+1, .y=+1},{.x=+1, .y=-1}}
							};

			for (int corner=0; corner<4; corner++)
			{
				const MyPoint& pt = pt4[corner];
				for (const MyPoint& dir : directions[corner])
				{
					MyRect rec;
					int intervalle[2]={2, INT16_MAX};
					auto& [m, M] = intervalle;
					while (m != M)
					{
						int value = M==INT16_MAX ? 2*m : (m+M)/2 ;
						const auto [x1, y1] = pt;
						const auto [x2, y2] = pt + value*dir ;
						rec = {.m_left=min(x1,x2), .m_right=max(x1,x2), .m_top=min(y1,y2), .m_bottom = max(y1, y2)};
						auto rg = input_rectangles | views::filter([](const MyRect& r){return intersect_strict(rec,r) || is_inside(r, rec)};
						(rg.empty() && is_inside(rec,frame) ? m : M) = value;
						printf("[%d %d]\n", m, M);
					}
					holes.push_back(rec);
				}
			}
		}
		
		ranges::sort(holes, std::ranges::greater{}, [](const MyRect& r){return width(r);});
		for (const auto& [m_left, m_right, m_top, m_bottom] : holes | views::take(6))
		{
			printf("[.m_left=%d, .m_right=%d, .m_top=%d, .m_bottom=%d]\n", m_left, m_right, m_top, m_bottom);
		}
	}
}
