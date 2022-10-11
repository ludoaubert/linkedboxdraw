

	vector<TranslationRangeItem> translation_ranges;

	vector<MyRect> rectangles = input_rectangles;

	const int n = rectangles.size();

	for (int id=0; ; id++)
	{
		bitset<30> partition(n,0);
		
		auto rec_select_partition=[&](int ri, auto&& rec_select_partition){
			//TODO: avoid naming adj_list ?
			span adj_list = ranges::equal_range(rect_links, ri, {}, &RectLink::i);
			auto rg = adj_list | views::filter([&](const RectLink& rl){return rectangles[ri][maxCompactRectDim] == rectangles[rl.j][minCompactRectDim];})
			ranges::for_each(rg, [&](const RectLink& rl){
					partition[rl.j]=1;
					rec_select_partition(rl.j, rec_select_partition);
			});

			}
		};
		
		const int frame_left = min(rectangles | views::transform(&MyRect::m_left));
		
		auto rg = views::iota(0,n) | views::filter([&](int i){return rectangles[i].m_left==frame_left;});
		
		for (int ri : rg)
		{
			rec_select_partition(ri, rec_select_partition);
		}

		auto rg = rect_links | views::filter([&](const RectLink& e){return partition[e.i] != partition[e.j];})
				| views::transform([&](const RectLink& e){return rectangles[e.j].m_left-rectangles[e.i].m_right;}) ;

		int tr = min(rg);

		if (tr <= 0)
			break;

		auto rg2 = views::iota(0,n) | views::filter([&](int i){return rectangles[i].m_left==frame_left;})
					| views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i,.tr=tr};});

		ranges::copy(rg2, back_inserter(translation_ranges));
	}

	return translation_ranges;
