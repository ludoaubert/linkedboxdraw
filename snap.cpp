
struct FreeRectangleRangeItem
{
        int id;
        int ri;
        MyRect r;

        friend bool operator==(const FreeRectangleRangeItem&, const FreeRectangleRangeItem&) = default;
};


vector<FreeRectangleRangeItem> free_rectangle_ranges;

/*
TODO: utiliser C++23 deducing this
*/

	auto rec_list_nodes = [&](int id, auto&& rec_list_nodes)->vector<int>{
		vector<int> nodes;
		const int parent_id = decision_tree[id].parent_id;
		if (parent_id != -1)
		{
			vector<int> parent_nodes = rec_list_nodes(parent_id, rec_list_nodes);
			nodes = move(parent_nodes);
		}
		nodes.push_back(id);
		return nodes;
	};
	
	auto rg1 = rec_list_nodes(id) | views::transform([&](int id){	return decision_tree[i].recmap.i_emplacement_source;	})
								| views::filter([&](int i){return i < n;});
	
	auto rg2 = rec_list_nodes(id) | views::transform([&](int id){	return decision_tree[i].recmap.i_emplacement_destination;	})
								| views::filter([&](int i){return i < n});
	
	vector<int> emplacements_sources(ranges::begin(rg1), ranges::end(rg1)),
				emplacements_destinations(ranges::begin(rg2), ranges::end(rg2));
	ranges::sort(emplacements_sources);
	ranges::sort(emplacements_destinations);
	vector<int> emplacements_liberes;
	ranges::set_difference(emplacements_sources, emplacements_destinations, back_inserter(emplacements_liberes));
	
	auto rg = emplacements_liberes | views::transform([&](int i)->FreeRectangleRangeItem{return {.id=id, .ri=i, .r=emplacements[i+n+m]};})
									| views::transform([&](const FreeRectangleRangeItem& i)->FreeRectangleRangeItem{
											return {.id=i.id, .ri=i.ri, .r=trimmed(i.r, rectangles)};});
											
	for (const FreeRectangleRangeItem& i : rg)
		free_rectangle_ranges.push_back(i);
	
	
	FILE* f=fopen("free_rectangle_ranges.json", "w");
	
	const int size = free_rectangle_ranges.size();
	fwrite(f, "[\n");
	for (int i=0; i < size; i++)
	{
		const auto [id, ri, r] = free_rectangle_ranges[i];
		fwrite(f, "\t{\"id\":%d, \"ri\":%d, \"r\":{\"m_left\":%d, \"m_right\":%d, \"m_top\":%d, \"m_bottom\":%d}}%s\n",
			id, ri, r.m_left, r.m_right, r.m_top, r.m_bottom, i+1<n ? "," : "");
	}
	fwrite(f, "]");

	fclose(f);