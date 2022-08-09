
import logical_graph from './logical_graph.json' assert {type: 'json'};
import holes from './holes.json' assert {type: 'json'};


console.log(logical_graph);
console.log(holes);

function print_html()
{
	var innerHTML = "";
        innerHTML += '\n';

	const {input_rectangles, logical_edges, topological_edges}=logical_graph;

	innerHTML += input_rectangles
			.map(({m_left, m_right, m_top, m_bottom}) => `<rect x="${m_left}" y="${m_top}" width="${m_right-m_left}" height="${m_bottom-m_top}" class=\"rect\" />`)
			.join('\n');

	innerHTML += '\n';

	innerHTML += input_rectangles
			.map((r, index) => `<text x="${r.m_left}" y="${r.m_top}" fill="red">r-${index}</text>`)
			.join('\n');

        innerHTML += '\n';

	let windowed_index = [];
	windowed_index.length = input_rectangles.length;
	windowed_index.fill(1);

	innerHTML += logical_edges
			.map(({from, to}) => `<text x="${input_rectangles[from].m_left+8}" y="${input_rectangles[from].m_top+14*windowed_index[from]++}" class="logical_contact">r-${to}</text>`)
			.join('\n');

        innerHTML += '\n';

	windowed_index.fill(1);

	innerHTML += topological_edges
			.map(({from, to}) => `<text x="${input_rectangles[from].m_left+30}" y="${input_rectangles[from].m_top+14*windowed_index[from]++}" class="topological_contact">r-${to}</text>`)
			.join('\n');

        innerHTML += '\n';

	innerHTML += holes.holes
			.map(hole => {	const {m_left, m_right, m_top, m_bottom}=hole.rec; return `<rect x="${m_left}" y="${m_top}" width="${m_right-m_left}" height="${m_bottom-m_top}" class="hole" />`;})
			.join('\n');

        innerHTML += '\n';

	innerHTML += holes.holes
			.map((hole, index) => `<text x="${hole.rec.m_left}" y="${hole.rec.m_top}" fill="black">hole-${index}</text>`)
			.join('\n');

        innerHTML += '\n';

	windowed_index.length = holes.holes.length;
	windowed_index.fill(1);

	innerHTML += holes.topological_contact
			.map(({hi, rj}) => `<text x="${holes.holes[hi].rec.m_left + 8}" y="${holes.holes[hi].rec.m_top + 14*windowed_index[hi]++}" fill="black">r-${rj}</text>`)
			.join('\n');

        innerHTML += '\n';

	innerHTML += holes.holes
			.map(({ri, rec}) => `<text x="${rec.m_left+30}" y="${rec.m_top+14}" fill="black">ri=${ri}</text>`)
			.join('\n');

        innerHTML += '\n';

	return innerHTML;
}

window.main = function main(){
	let div = document.getElementById("main_svg");
	const innerHTML = print_html();
	console.log(innerHTML);
	div.innerHTML = innerHTML;
}
