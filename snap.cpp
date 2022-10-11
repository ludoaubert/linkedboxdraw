


void compact(Direction update_direction, const vector<RectLink>& rect_links, vector<MyRect>& rectangles)
{
	auto [minCompactRectDim, maxCompactRectDim] = rectDimRanges[update_direction];  //{LEFT, RIGHT} or {TOP, BOTTOM}
	
	vector<TranslationRangeItem> translation_ranges;

	//vector<MyRect> rectangles = input_rectangles;

	const int n = rectangles.size();

	for (int id=0; ; id++)
	{
		bitset<30> partition(n,0);
		
		auto rec_select_partition=[&](int ri, auto&& rec_select_partition){
	
			auto rg = ranges::equal_range(rect_links, ri, {}, &RectLink::i) 
						| views::filter([&](const RectLink& rl){return rectangles[ri][maxCompactRectDim] == rectangles[rl.j][minCompactRectDim];})
			ranges::for_each(rg, [&](const RectLink& rl){
					partition[rl.j]=1;
					rec_select_partition(rl.j, rec_select_partition);
			});

			}
		};
		
		auto rg = rectangles | views::transform([&](const MyRect& r){return r[minCompactRectDim];});
		const int frame_min = min(rg);
		const int next_min = min(rg | views::filter([&](int value){return value != frame_min;}));
		
		auto rg = views::iota(0,n) | views::filter([&](int i){return rectangles[i][minCompactRectDim]==frame_min;});
		
		for (int ri : rg)
		{
			partition[ri] = 1;
			rec_select_partition(ri, rec_select_partition);
		}

		auto rg = rect_links | views::filter([&](const RectLink& e){return partition[e.i] != partition[e.j];})
				| views::transform([&](const RectLink& e){return rectangles[e.j][minCompactRectDim]-rectangles[e.i][maxCompactRectDim];}) ;

		MyPoint tr={.x=0, .y=0};
		tr[update_direction] = min<int>(min(rg), next_min - frame_min);

		if (tr[update_direction] <= 0)
			break;

		auto rg2 = views::iota(0,n) | views::filter([&](int i){return partition[i]==1;})
					| views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i,.tr=tr};});

		ranges::copy(rg2, back_inserter(translation_ranges));
	}

	return translation_ranges;
}
