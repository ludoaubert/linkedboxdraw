

	vector<TranslationRangeItem> translation_ranges;

	vector<MyRect> rectangles = input_rectangles;

        const int n = rectangles.size();

	for (int id=0; ; id++)
	{
		const int frame_left = min(rectangles | views::transform(&MyRect::m_left));

		auto rg = rect_links | views::filter([&](const RectLink& e){return rectangles[e.i].m_left==frame_left;})
				| views::transform([&](const RectLink& e){return rectangles[e.j].m_left-rectangles[e.i].m_right;}) ;

		int tr = min(rg);

		if (tr <= 0)
			break;

		auto rg2 = views::iota(0,n) | views::filter([&](int i){return rectangles[i].m_left==frame_left;})
					| views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i,.tr=tr};});

		ranges::copy(rg2, back_inserter(translation_ranges));
	}

	return translation_ranges;
