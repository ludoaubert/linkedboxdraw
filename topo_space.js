
import logical_graph from './logical_graph.json' assert {type: 'json'};
import holes from './holes.json' assert {type: 'json'};


console.log(logical_graph);
console.log(holes);

function print_html()
{
	var innerHTML = "";

	innerHTML += logical_graph
			.input_rectangles
			.map(({m_left, m_right, m_top, m_bottom}) => `<rect x="${m_left}" y="${m_top}" width="${m_right-m_left}" height="${m_bottom-m_top}" class=\"rect\" />`)
			.join('\n');

	return innerHTML;
/*
	MyRect frame={
		.m_left=ranges::min(input_rectangles | views::transform(&MyRect::m_left)),
 		.m_right=ranges::max(input_rectangles | views::transform(&MyRect::m_right)),
		.m_top=ranges::min(input_rectangles | views::transform(&MyRect::m_top)),
 		.m_bottom=ranges::max(input_rectangles | views::transform(&MyRect::m_bottom))
	};

	pos+= sprintf(buffer+pos, "<html>\n<head>\n<link rel=\"stylesheet\" href=\"topo_space.css\">\n</head>\n<body>\n");
	pos+= sprintf(buffer+pos, "<svg width=\"%d\" height=\"%d\">\n", width(frame)+100, height(frame));
	for (int ri=0; ri < input_rectangles.size(); ri++)
	{
		const MyRect& r = input_rectangles[ri];
		const auto& [m_left, m_right, m_top, m_bottom, no_sequence, i, selected] = r;
		pos += sprintf(buffer+pos, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" class=\"rect\" />\n",m_left, m_top, width(r), height(r));
		pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"red\">r-%d</text>\n", m_left, m_top, ri);

		int dy = 0;

		auto contacts = logical_edges |
			views::filter([&](const LogicalEdge& e){return e.from==ri;}) |
			views::transform(&LogicalEdge::to);

		for (int rj : contacts)
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" class=\"logical_contact\">r-%d</text>\n", m_left + 8, m_top + dy, rj);
		}

		dy = 0;
		for (int rj : views::iota(0, n) | views::filter([&](int rj){return ri != rj && edge_overlap(input_rectangles[ri], input_rectangles[rj]);}))
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" class=\"topological_contact\">r-%d</text>\n", r.m_left + 30, r.m_top + dy, rj);
		}
	}

	for (int hi=0; hi < holes.size(); hi++)
	{
		const auto& [ri, RectCorner, direction, value, rec] = holes[hi];

		pos += sprintf(buffer+pos, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" class=\"hole\" />\n",
					rec.m_left, rec.m_top, width(rec), height(rec));
		pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">hole-%d</text>\n", rec.m_left, rec.m_top, hi);

		int dy = 0;
		for (int rj : views::iota(0, n) | views::filter([&](int rj){return edge_overlap(rec, input_rectangles[rj]);}))
		{
			dy += 14;
			pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">r-%d</text>\n", rec.m_left + 8, rec.m_top + dy, rj);
		}
		pos += sprintf(buffer+pos, "<text x=\"%d\" y=\"%d\" fill=\"black\">ri=%d</text>\n", rec.m_left + 30, rec.m_top + 1*14, ri);
	}

	pos += sprintf(buffer+pos, "</svg>\n</html>");
	buffer[pos]=0;
	return buffer;
*/
}

document.body.innerHtml = `<svg id="main_svg" width="959" height="704">` + print_html() + `</svg>`;
