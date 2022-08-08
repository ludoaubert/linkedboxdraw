
import logical_graph from './logical_graph.json' assert {type: 'json'};
import holes from './holes.json' assert {type: 'json'};


console.log(logical_graph);
console.log(holes);

function print_html()
{
	var innerHTML = "";

	const {input_rectangles, logical_edges, topological_edges}=logical_graph;

	innerHTML += input_rectangles
			.map(({m_left, m_right, m_top, m_bottom}) => `<rect x="${m_left}" y="${m_top}" width="${m_right-m_left}" height="${m_bottom-m_top}" class=\"rect\" />`)
			.join('\n');

	innerHTML += input_rectangles
			.entries()
			.map(({index, r}) => `<text x="${m_left}" y="${m_top}" fill="red">r-${index}</text>`)
			.join('\n');

	let windowed_index = [];
	windowed_index.length = input_rectangles.length;
	windowed_index.fill(0);

	innerHTML += logical_edges
			.map(({from, to}) => `<text x="${input_rectangles[from].m_left+8}" y="${input_rectangles[from].m_top+14*windowed_index[from]++}" class="logical_contact">r-${to}</text>`)
			.join('\n');

	windowed_index.fill(0);

	innerHTML += topological_edges
			.map(({from, to}) => `<text x="${input_rectangles[from].m_left+30}" y="${input_rectangles[from].m_top+14*windowed_index[from]++}" class="topological_contact">r-${to}</text>`;)
			.join('\n');

	innerHTML += holes.holes
			.map(hole => {	const {m_left, m_right, m_top, m_bottom}=hole.rec; return `<rect x="${m_left}" y="${m_top}" width="${m_right-m_left}" height="${m_bottom-m_top}" class="hole" />`;})
			.join('\n');

	innerHTML += holes.holes
			.entries()
			.map(({index, hole}) => `<text x="${hole.rec.m_left}" y="${hole.rec.m_top}" fill="black">hole-${index}</text>`)
			.join('\n');

	windowed_index.length = holes.holes.length;
	windewed_index.fill(0);

	innerHTML += holes.topological_contact
			.map(({hi, rj}) => `<text x="${holes.holes[hi].rec.m_left + 8}" y="${holes.holes[hi].rec.m_top + 14*window_index[hi]++}" fill="black">r-${rj}</text>`)
			.join('\n');

	innerHTML += holes.holes
			.map(({ri, rec}) => `<text x="${rec.m_left+30}" y="${rec.m_top+14}" fill="black">ri=${ri}</text>`)
			.join('\n');

	return innerHTML;
}

function main(){
	document.body.innerHtml = `<svg id="main_svg" width="959" height="704">` + print_html() + `</svg>`;
}

window.addEventListener('load', main)
