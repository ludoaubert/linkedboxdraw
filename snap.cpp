

const unsigned NR_JOB_PIPELINES2=2;

const Job pipelines2[NR_JOB_PIPELINES2][2]={
	{
		{.algo=COMPACT, .update_direction=EAST_WEST},
		{.algo=COMPACT, .update_direction=NORTH_SOUTH}
	},
	{
		{.algo=COMPACT, .update_direction=NORTH_SOUTH},
		{.algo=COMPACT, .update_direction=EAST_WEST}
	}
};


//cartesian product with NR_MIRRORING_OPTIONS

struct ProcessSelector2
{
	unsigned pipeline, mirroring;
};

// TODO: use upcoming C++23 views::cartesian_product()
vector<ProcessSelector2> cartesian_product2()
{
	vector<ProcessSelector2> result;

	for (int pipeline=0; pipeline < NR_JOB_PIPELINES2; pipeline++)
		for (int mirroring=0; mirroring < NR_MIRRORING_OPTIONS; mirroring++)
			result.push_back({pipeline, mirroring});

	return result;
}


const vector<ProcessSelector> process_selectors = cartesian_product();


//TODO: use views::chunk_by() C++23

vector<TranslationRangeItem> compute_decision_tree_translations2(const vector<DecisionTreeNode>& decision_tree,
													const vector<TranslationRangeItem>& translation_ranges,
													const vector<MyRect>& input_rectangles,
													const vector<LogicalEdge>& logical_edges)
{
	int n = input_rectangles.size();
	
	vector<MyRect> rectangles(n), rectangles2(n);
	
	vector<TranslationRangeItem>& translation_ranges2;
	
	auto tf=[&](int id, unsigned pipeline, unsigned mirroring){

		D(printf("calling tf(id=%d, pipeline=%u, mirroring=%u)\n", id, pipeline, mirroring));
		
		ranges::copy(rectangles, rectangles2);

		for (const Mirror& mirror : mirrors[mirroring])
			apply_mirror(mirror, rectangles2);
		for (const Job& job : pipelines2[pipeline])
			apply_job(job, rectangles2);
		for (const Mirror& mirror : mirrors[mirroring])
			apply_mirror(mirror, rectangles2);
	};

	
	for (int id=0; id < decision_tree.size(); id++)
	{
		ranges::copy(input_rectangles, rectangles);
		
		vector<TranslationRangeItem> ts;

		ranges::set_union(
			ranges::equal_range(translation_ranges, id, {}, &TranslationRangeItem::id),
			views::iota(0,n) | views::transform([&](int i){return TranslationRangeItem{.id=id,.ri=i, .tr={.x=0,.y=0}}; }),
			back_inserter(ts),
			{},
			&TranslationRangeItem::ri,
			&TranslationRangeItem::ri);
			
		auto rg = views::iota(0,n) | views::transform([&](int i){return input_rectangles[i]+ts[i].tr;});
		ranges::copy(rg, rectangles);
		
		const auto [pipeline, mirroring] = ranges::min(process_selectors2, {}, [&](const ProcessSelector2& ps){
			D(printf("pipeline=%u\n", ps.pipeline));
			D(printf("MirroringStrings[mirroring]=%s\n", MirroringStrings[ps.mirroring]));

			tf(id, ps.pipeline, ps.mirroring);

			auto rg1 = logical_edges |
				views::transform([&](const auto& le){ return rectangle_distance(rectangles2[le.from],rectangles2[le.to]);	});

			auto rg2 = views::iota(0,n) |
				views::transform([&](int i)->TranslationRangeItem{
					const MyRect &ir = rectangles[i], &r = rectangles2[i];
					MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
					return {id, i, tr};}) |
				views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};}) |
				views::transform([&](const TranslationRangeItem& item){const auto [id,i,tr]=item; return abs(tr.x) + abs(tr.y);});

			const int sigma_edge_distance = accumulate(ranges::begin(rg1), ranges::end(rg1),0);
			const int sigma_translation = accumulate(ranges::begin(rg2), ranges::end(rg2),0);
			const auto [width, height] = dimensions(compute_frame(rectangles));

			D(printf("sigma_edge_distance = %d\n", sigma_edge_distance));
			D(printf("sigma_translation = %d\n", sigma_translation));
			D(printf("[.width=%d, .height=%d]\n", width, height));

			int cost = width + height + sigma_edge_distance + sigma_translation ;

			D(printf("cost=%d\n", cost));
			return cost;
		});

		D(printf("selection[id=%d] = {pipeline=%u, mirroring=%u}\n", id, pipeline, mirroring));

		D(printf("MirroringStrings[mirroring]=%s\n", MirroringStrings[mirroring]));

		tf(id, pipeline, mirroring, match_corner);

		auto rg = views::iota(0,n) |
					views::transform([&](int i)->TranslationRangeItem{
                                        const MyRect &ir = rectangles[i], &r = rectangles2[i];
                                        MyPoint tr={.x=r.m_left - ir.m_left, .y=r.m_top - ir.m_top};
                                        return {id, i, tr};}) |
					views::filter([](const TranslationRangeItem& item){return item.tr != MyPoint{0,0};});

		for (TranslationRangeItem item : rg)
		{
			translation_ranges2.push_back(item);
		}
	}
	
{
	FILE *f=fopen("translation_ranges2.json", "w");
	fprintf(f, "[\n");
	for (int i=0; i < translation_ranges2.size(); i++)
	{
		const auto [id, ri, tr] = translation_ranges2[i];
		fprintf(f, "{\"id\":%d, \"ri\":%d, \"x\":%d, \"y\":%d}%s\n", id, ri, tr.x, tr.y,
			i+1 == translation_ranges2.size() ? "": ",");
	}
	fprintf(f, "]\n");
	fclose(f);
}
	return translation_ranges2;
}