
//TODO: friend auto operator<=>(const TranslationRangeItem&, const TranslationRangeItem&) = default;

//TODO: use C++23 chunk_by() and views::set_union(). views::zip_transform(), to<vector>().

struct DiagramScore{
	int id, sigma_edge_distance, width, height, total;
};

auto rg = views::iota(0, decision_tree.size()) |
			views::transform([&](int id)->DiagramScore{
				
				vector<TranslationRangeItem> ts;

				ranges::set_union(
					ranges::equal_range(translation_ranges, id, {}, &TranslationRangeItem::id),
					views::iota(0,n) | views::transform([](int i){return TranslationRangeItem{.id=0,.ri=i, .tr={.x=0,.y=0}}; }),
					back_inserter(ts),
					{},
					&TranslationRangeItem::ri,
					&TranslationRangeItem::ri);
					
				auto rg = views::iota(0,n) | views::transform([](int i){return input_rectangles[i]+ts[i];});
				vector<MyRect> rectangles(ranges::begin(rg), ranges::end(rg));
				
				auto rg1 = logical_edges |
								views::transform([&](const auto& le){ return rectangle_distance(rectangles[le.from],rectangles[le.to]); });
				const int sigma_edge_distance = accumulate(ranges::begin(rg1), ranges::end(rg1),0);
				const auto [width, height] = dimensions(compute_frame(rectangles));

				return {
					.id=id,
					.sigma_edge_distance=sigma_edge_distance,
					.width=width,
					.height=height,
					.total= width + height + sigma_edge_distance
				};
			});

//TODO: use C++23 views::join_with(",") and avoid allocation of 'vector<DiagramScore> scores'
{		
	vector<DiagramScore> scores(ranges::begin(rg), ranges::end(rg));
	FILE *f=fopen("scores.json", "w");
	fprintf(f, "[\n");
	for (int i=0; i < scores.size(); i++)
	{
		const auto [id, sigma_edge_distance, width, height, total] = scores[i];
		fprintf(f, "{\"id\":%d, \"sigma_edge_distance\":%d, \"width\":%d, \"height\":%d, \"total\":%d}%s\n", 
			id, sigma_edge_distance, width, height, total,
			i+1 == translation_ranges.size() ? "": ",");
	}
	fprintf(f, "]\n");
	fclose(f);	
}