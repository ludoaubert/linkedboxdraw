
import logical_graph from './logical_graph.json' assert {type: 'json'};
import holes from './holes.json' assert {type: 'json'};
import decision_tree from './decision_tree.json' assert {type: 'json'};
import translation_ranges from './translation_ranges.json' assert {type: 'json'};
import scores from './scores.json' assert {type: 'json'};


console.log(logical_graph);
console.log(holes);

// supposed to find the index of the first element that matches the predicate.
// expects that ALL elements after that also match!
function binarySearch(array, predicate) {
  let lo = 0, hi = array.length;
  while (lo < hi) {
    const mid = (lo + hi) >> 1;
    if (predicate(array[mid], mid, array)) {
      hi = mid;
    } else {
      lo = mid + 1;
    }
  }
  return lo;
}


function translation_range_print_html(id)
{
	const {input_rectangles, logical_edges, topological_edges}=logical_graph;

	// find the start of the range
	const start = binarySearch(translation_ranges, item => item.id >= id);
// find the end of the range
	const end = binarySearch(translation_ranges, item => item.id > id);

	const translations = translation_ranges.slice(start,end);

	const rectangles = input_rectangles.map((r, index) => {
			const tr=translations.find(tr => tr.ri==index);
			return tr==undefined ? r : {
				m_left : r.m_left + tr.x,
				m_right : r.m_right + tr.x,
				m_top : r.m_top + tr.y,
				m_bottom : r.m_bottom + tr.y
			};
		}
	);

	const frame = {
		m_left : Math.min(...rectangles.map(r => r.m_left)),
		m_right : Math.max(...rectangles.map(r => r.m_right)),
		m_top : Math.min(...rectangles.map(r => r.m_top)),
		m_bottom : Math.max(...rectangles.map(r => r.m_bottom))
	};

	return [`<svg width="${frame.m_right-frame.m_left+100}" height="${frame.m_bottom-frame.m_top+100}">`,
                rectangles.map(({m_left, m_right, m_top, m_bottom}, index) =>
                        [`<g id="g-${index}" transform="translate(${m_left} ${m_top})">`,
                        `<rect id="r-${index}" x="0" y="0" width="${m_right-m_left}" height="${m_bottom-m_top}" class=\"rect\" />`,
                        `<text id="tr-${index}" x="0" y="0" fill="red">r-${index}</text>`,
                        logical_edges.filter(({from, to}) => from==index)
			        .map(({from, to}, line) => `<text id="le-${from}-${to}" x="8" y="${14*(line+1)}" class="logical_contact">r-${to}</text>`),
			`</g>`]),
		"</svg>"].flat(3)
			.join('\n');
}

function print_html()
{
	var innerHTML = "";

	const {input_rectangles, logical_edges, topological_edges}=logical_graph;

	const frame = {
		m_left : Math.min(...input_rectangles.map(r => r.m_left)),
		m_right : Math.max(...input_rectangles.map(r => r.m_right)),
		m_top : Math.min(...input_rectangles.map(r => r.m_top)),
		m_bottom : Math.max(...input_rectangles.map(r => r.m_bottom))
	};

	return [`<svg width="${frame.m_right-frame.m_left+100}" height="${frame.m_bottom-frame.m_top+100}">`,
		input_rectangles.map(({m_left, m_right, m_top, m_bottom}, index) =>
			[`<g id="g-r-${index}" transform="translate(${m_left} ${m_top})">`,
			`<rect x="0" y="0" width="${m_right-m_left}" height="${m_bottom-m_top}" class=\"rect\" />`,
			`<text x="0" y="0" fill="red">r-${index}</text>`,
			logical_edges.filter(({from, to}) => from==index)
					.map(({from, to}, line) => `<text x="8" y="${14*(line+1)}" class="logical_contact">r-${to}</text>`),
			topological_edges.filter(({from, to}) => from==index)
					.map(({from, to}, line) => `<text x="30" y="${14*(line+1)}" class="topological_contact">r-${to}</text>`),
			`</g>`]),
		holes.holes.map(({rec, ri}, index) =>
			[`<g id="g-h-${index}" transform="translate(${rec.m_left} ${rec.m_top})">`,
			`<rect x="0" y="0" width="${rec.m_right-rec.m_left}" height="${rec.m_bottom-rec.m_top}" class="hole" />`,
			`<text x="0" y="0" fill="black">hole-${index}</text>`,
			holes.topological_contact.filter(({hi, rj}) => hi==index)
						.map(({hi, rj}, line) => `<text x="8" y="${14*(line+1)}" fill="black">r-${rj}</text>`),
			`<text x="30" y="14" fill="black">ri=${ri}</text>`,
			`</g>`]),
		"</svg>"].flat(3)
			.join('\n');
}


function print_emplacement(i_emplacement_destination)
{
	if (i_emplacement_destination >= logical_graph.input_rectangles.length)
		return `h${i_emplacement_destination - logical_graph.input_rectangles.length}`;
	else
		return `${i_emplacement_destination}`;
}

var selected = null;

window.main = function main(){
	let div = document.getElementById("main_svg");
	const innerHTML = print_html();
	console.log(innerHTML);
	div.innerHTML = innerHTML;

	let dt = document.getElementById("decision_tree").getElementsByTagName('tbody')[0];
	dt.innerHTML = decision_tree
                        .map(({i, parent_index, depth, i_emplacement_source, i_emplacement_destination, match}) =>
				[`<tr>`,[
					`${i}`, `${parent_index}`, `${depth}`, `${i_emplacement_source}`, `${print_emplacement(i_emplacement_destination)}`, `${match}`
					].map(s => `<td>${s}</td>`),
				`</tr>`])
			.flat(2)
			.join('\n');
			
	let sc = document.getElementById("scores").getElementsByTagName('tbody')[0];
	sc.innerHTML = scores
						.map(({id, sigma_edge_distance, width, height, total}) =>
				[`<tr>`,[
					`${id}`, `${sigma_edge_distance}`, `${width}`, `${height}`, `${total}`
					].map(s => `<td>${s}</td>`),
				`</tr>`])
			.flat(2)
			.join('\n');


	dt.addEventListener('mouseover', (event)=>{
	// 'highlight' color is set in tablelist.css
		   let tr = event.target.parentNode;
		   if ( tr.className === '') {
				tr.className='highlight';
		   }
		   return false
	});

	dt.addEventListener('mouseout', (event)=>{
		   let tr = event.target.parentNode;
		   if ( tr.className === 'highlight') {
				tr.className='';
		   }
		   return false
	});

	dt.addEventListener('mousedown', (event)=>{

		   let tr = event.target.parentNode;

		 if ( tr.className !== 'clicked' ) {
	// Clear previous selection
		if ( selected !== null ) {
			 selected.className='';
		}
	// Mark this row as selected
		tr.className='clicked';
		selected = tr;
		let i = parseInt(tr.cells[0].innerHTML,10);

		let div = document.getElementById("range_svg");
		div.innerHTML = translation_range_print_html(i);

		let chemin = [];
		while (i != -1)
		{
			chemin.push(decision_tree[i]);
			i = decision_tree[i].parent_index;
		}

		let cm = document.getElementById("chemin").getElementsByTagName("tbody")[0];
		cm.innerHTML = chemin
				.reverse()
				.map(({i, parent_index, depth, i_emplacement_source, i_emplacement_destination, match}) =>
                                ["<tr>",[
                                        `${i}`, `${parent_index}`, `${depth}`, `${i_emplacement_source}`, `${print_emplacement(i_emplacement_destination)}`, `${match}`
                                        ].map(s => `<td>${s}</td>`),
				"</tr>"]).flat(2)
                        .join('\n');

		document
			.querySelectorAll(`[id^="g-h-"]`)
			.forEach((element) => {element.style.visibility = "hidden";});

		const query = chemin
			.map(({i_emplacement_destination}) => i_emplacement_destination)
			.filter(i_emplacement_destination => i_emplacement_destination >= logical_graph.input_rectangles.length)
			.map(i_emplacement_destination => i_emplacement_destination - logical_graph.input_rectangles.length)
			.map(h => `[id^="g-h-${h}"]`)
			.join(', ');

		   document
			.querySelectorAll(query)
			.forEach((element) => {element.style.visibility = "visible";});
		 }
		 else {
			   tr.className='';
			   selected = null;
		 }
		 return true
	});

	const allTables = document.querySelectorAll('table');

	for (const table of allTables) {
	  const tBody = table.tBodies[0];
	  const rows = Array.from(tBody.rows);
	  const headerCells = table.tHead.rows[0].cells;

	  for (const th of headerCells) {
		const cellIndex = th.cellIndex;

		th.addEventListener('click', () => {
		  rows.sort((tr1, tr2) => {
			const tr1Text = tr1.cells[cellIndex].textContent;
			const tr2Text = tr2.cells[cellIndex].textContent;
			return tr1Text.localeCompare(tr2Text);
		  });

		  tBody.append(...rows);
		});
	  }
	}
}
