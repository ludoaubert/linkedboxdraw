
import logical_graph0 from './logical_graph0.json' assert {type: 'json'};
import holes0 from './holes0.json' assert {type: 'json'};
import decision_tree0 from './decision_tree0.json' assert {type: 'json'};
import translation_ranges_0 from './translation_ranges_0.json' assert {type: 'json'};
import translation_ranges2_0 from './translation_ranges2_0.json' assert {type: 'json'};
import scores0 from './scores0.json' assert {type: 'json'};

import logical_graph1 from './logical_graph1.json' assert {type: 'json'};
import holes1 from './holes1.json' assert {type: 'json'};
import decision_tree1 from './decision_tree1.json' assert {type: 'json'};
import translation_ranges_1 from './translation_ranges_1.json' assert {type: 'json'};
import translation_ranges2_1 from './translation_ranges2_1.json' assert {type: 'json'};
import scores1 from './scores1.json' assert {type: 'json'};

import logical_graph2 from './logical_graph2.json' assert {type: 'json'};
import holes2 from './holes2.json' assert {type: 'json'};
import decision_tree2 from './decision_tree2.json' assert {type: 'json'};
import translation_ranges_2 from './translation_ranges_2.json' assert {type: 'json'};
import translation_ranges2_2 from './translation_ranges2_2.json' assert {type: 'json'};
import scores2 from './scores2.json' assert {type: 'json'};

import logical_graph3 from './logical_graph3.json' assert {type: 'json'};
import holes3 from './holes3.json' assert {type: 'json'};
import decision_tree3 from './decision_tree3.json' assert {type: 'json'};
import translation_ranges_3 from './translation_ranges_3.json' assert {type: 'json'};
import translation_ranges2_3 from './translation_ranges2_3.json' assert {type: 'json'};
import scores3 from './scores3.json' assert {type: 'json'};

import logical_graph4 from './logical_graph4.json' assert {type: 'json'};
import holes4 from './holes4.json' assert {type: 'json'};
import decision_tree4 from './decision_tree4.json' assert {type: 'json'};
import translation_ranges_4 from './translation_ranges_4.json' assert {type: 'json'};
import translation_ranges2_4 from './translation_ranges2_4.json' assert {type: 'json'};
import scores4 from './scores4.json' assert {type: 'json'};

import logical_graph5 from './logical_graph5.json' assert {type: 'json'};
import holes5 from './holes5.json' assert {type: 'json'};
import decision_tree5 from './decision_tree5.json' assert {type: 'json'};
import translation_ranges_5 from './translation_ranges_5.json' assert {type: 'json'};
import translation_ranges2_5 from './translation_ranges2_5.json' assert {type: 'json'};
import scores5 from './scores5.json' assert {type: 'json'};

var testIndex=0;

const tests = [
	{logical_graph:logical_graph0, holes:holes0, decision_tree:decision_tree0, translation_ranges:translation_ranges_0, translation_ranges2:translation_ranges2_0, scores:scores0},
	{logical_graph:logical_graph1, holes:holes1, decision_tree:decision_tree1, translation_ranges:translation_ranges_1, translation_ranges2:translation_ranges2_1, scores:scores1},
        {logical_graph:logical_graph2, holes:holes2, decision_tree:decision_tree2, translation_ranges:translation_ranges_2, translation_ranges2:translation_ranges2_2, scores:scores2},
	{logical_graph:logical_graph3, holes:holes3, decision_tree:decision_tree3, translation_ranges:translation_ranges_3, translation_ranges2:translation_ranges2_3, scores:scores3},
        {logical_graph:logical_graph4, holes:holes4, decision_tree:decision_tree4, translation_ranges:translation_ranges_4, translation_ranges2:translation_ranges2_4, scores:scores4},
        {logical_graph:logical_graph5, holes:holes5, decision_tree:decision_tree5, translation_ranges:translation_ranges_5, translation_ranges2:translation_ranges2_5, scores:scores5}
];


export function PreviousTest()
{
	if (testIndex > 0)
		testIndex--;
	window.main();
}
window.PreviousTest = PreviousTest;

export function NextTest()
{
	if (testIndex + 1 < tests.length)
		testIndex++;
	window.main();
}

window.NextTest = NextTest;

// FRAME_MARGIN is duplicated in table_input.js, diagload.js and topo_space.js
const FRAME_MARGIN = 20;

function compute_frame(rectangles)
{
	const frame = {
		m_left : -FRAME_MARGIN/2 + Math.min(...rectangles.map(r => r.m_left)),
		m_right : +FRAME_MARGIN/2 + Math.max(...rectangles.map(r => r.m_right)),
		m_top : -FRAME_MARGIN/2 + Math.min(...rectangles.map(r => r.m_top)),
		m_bottom : +FRAME_MARGIN/2 + Math.max(...rectangles.map(r => r.m_bottom))
	};
	return frame;
}

function compute_center_frame_translation(rectangles)
{
	const frame = compute_frame(rectangles);
	const translation = {
		x: -frame.m_left,
		y: -frame.m_top
	};
	return translation;
}

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


const equal_range = (translation_ranges, id) => {
	const start = binarySearch(translation_ranges, item => item.id >= id);
	const end = binarySearch(translation_ranges, item => item.id > id);
	const translations = translation_ranges.slice(start,end);
	return translations;
};


function translated_rectangle(r, tr)
{
	return {
		m_left : r.m_left + tr.x,
		m_right : r.m_right + tr.x,
		m_top : r.m_top + tr.y,
		m_bottom : r.m_bottom + tr.y
	};
}

function resized_rectangle(r, tr)
{
	return {
		m_left : r.m_left,
		m_right : r.m_right + tr.x,
		m_top : r.m_top,
		m_bottom : r.m_bottom + tr.y
	};
}

const width = (r) => r.m_right-r.m_left;
const height = (r) => r.m_bottom-r.m_top;

function translation_range_print_html(id)
{
	const {logical_graph, holes, decision_tree, translation_ranges, translation_ranges2, scores} = tests[testIndex];
	const {input_rectangles, logical_edges, topological_edges}=logical_graph;
	const {input_holes, topological_contact}=holes;

	const translations = equal_range(translation_ranges, id);
	const rectangles_ = input_rectangles.map((r, index) => {
				const tr=translations.find(tr => tr.ri==index);
				return tr==undefined ? r : translated_rectangle(r, tr);});
	const cftr = compute_center_frame_translation(rectangles_);
	const rectangles = rectangles_.map(r => translated_rectangle(r, cftr));
	const frame = compute_frame(rectangles);
	const n = input_rectangles.length;
	const distinctHoleIndices = [...new Set(translations.map(tr => tr.ri).filter(ri => ri>=n))];
	const selected_holes = distinctHoleIndices.map(i => {
				const r = input_holes[i-n];
				const tr=translations.find(tr => tr.ri==i && tr.tt==0);
                		const r2 = tr==undefined ? r : translated_rectangle(r, tr);
				const rs=translations.find(rs => rs.ri==i && rs.tt==1);
                		const r3 = rs==undefined ? r2 : resized_rectangle(r2, rs);
				const r4 = translated_rectangle(r3, cftr);
				return {...r4, ri:i-n};
		     	      });

	const translations2 = equal_range(translation_ranges2, id);
        const rectangles2_ = rectangles.map((r, index) => {
                                const tr=translations2.find(tr => tr.ri==index);
                                return tr==undefined ? r : translated_rectangle(r, tr);});
        const cftr2 = compute_center_frame_translation(rectangles2_);
        const rectangles2 = rectangles2_.map(r => translated_rectangle(r, cftr2));
	const frame2 = compute_frame(rectangles2);

	const svg = (frame, rectangles, holes) => {
		return [`<svg width="${width(frame)}" height="${height(frame)}">`,
                	rectangles.map((r, index) =>
                        	[`<g transform="translate(${r.m_left} ${r.m_top})">`,
                        	`<rect x="0" y="0" width="${width(r)}" height="${height(r)}" class=\"rect\" />`,
                        	`<text x="0" y="0" fill="red">r-${index}</text>`,
                        	logical_edges.filter(({from, to}) => from==index)
			        	     .map(({from, to}, line) => `<text x="8" y="${14*(line+1)}" class="logical_contact">r-${to}</text>`),
				`</g>`]),
			holes.map(r =>
                        	[`<g transform="translate(${r.m_left} ${r.m_top})">`,
                        	`<rect x="0" y="0" width="${width(r)}" height="${height(r)}" class=\"hole\" />`,
                        	`<text x="0" y="0" fill="red">h-${r.ri}</text>`,
                        	`</g>`]),
			"</svg>"].flat(3)
			.join('\n');
	};

	return [svg(frame, rectangles, selected_holes), svg(frame2, rectangles2, [])].join('\n');
}

function print_html()
{
	var innerHTML = "";

	const {logical_graph, holes, decision_tree, translation_ranges, translation_ranges2, scores} = tests[testIndex];
	const {input_rectangles, logical_edges, topological_edges}=logical_graph;
	const {holes:input_holes, topological_contact}=holes;

	const tr = compute_center_frame_translation(input_rectangles);

	const rectangles = input_rectangles.map(r => translated_rectangle(r, tr));

	const frame = compute_frame(rectangles);

	const width = (r) => r.m_right-r.m_left;
	const height = (r) => r.m_bottom-r.m_top;

	return [`<svg width="${width(frame)}" height="${height(frame)}">`,
		rectangles.map((r, index) =>
			[`<g id="g-r-${index}" transform="translate(${r.m_left} ${r.m_top})">`,
			`<rect x="0" y="0" width="${width(r)}" height="${height(r)}" class=\"rect\" />`,
			`<text x="0" y="0" fill="red">r-${index}</text>`,
			logical_edges.filter(({from, to}) => from==index)
					.map(({from, to}, line) => `<text x="8" y="${14*(line+1)}" class="logical_contact">r-${to}</text>`),
			topological_edges.filter(({from, to}) => from==index)
					.map(({from, to}, line) => `<text x="30" y="${14*(line+1)}" class="topological_contact">r-${to}</text>`),
			`</g>`]),
		input_holes.map(r => translated_rectangle(r, tr))
				.map((r, index) =>
				[`<g id="g-h-${index}" transform="translate(${r.m_left} ${r.m_top})">`,
				`<rect x="0" y="0" width="${r.m_right-r.m_left}" height="${r.m_bottom-r.m_top}" class="hole" />`,
				`<text x="0" y="0" fill="black">hole-${index}</text>`,
				topological_contact.filter(({hi, rj}) => hi==index)
						.map(({hi, rj}, line) => `<text x="8" y="${14*(line+1)}" fill="black">r-${rj}</text>`),
				`</g>`]),
		"</svg>"].flat(3)
			.join('\n');
}


function print_emplacement(i_emplacement_destination)
{
	const {logical_graph, holes, decision_tree, translation_ranges, translation_ranges2, scores} = tests[testIndex];
	if (i_emplacement_destination >= logical_graph.input_rectangles.length)
		return `h${i_emplacement_destination - logical_graph.input_rectangles.length}`;
	else
		return `${i_emplacement_destination}`;
}

var selected = null;

function select_id(event)
{
	let tr = event.target.parentNode;
	const {logical_graph, holes, decision_tree, translation_ranges, translation_ranges2, scores} = tests[testIndex];

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
			.map(item => {
					let deep_copy=JSON.parse(JSON.stringify(item));
					deep_copy.i_emplacement_destination = print_emplacement(item.i_emplacement_destination);
					return deep_copy;
				}
			)
			.map(item => ["<tr>",
					Object.values(item)
					.map(i => `<td>${i}</td>`),
				      "</tr>"])
			.flat(2)
			.join('\n');
	document.getElementById("chemin").addEventListener('mousedown', select_id);

	document
		.querySelectorAll(`[id^="g-h-"]`)
		.forEach((element) => {element.style.visibility = "hidden";});

	const query = chemin
		.map(({i_emplacement_destination}) => i_emplacement_destination)
		.filter(i_emplacement_destination => i_emplacement_destination >= logical_graph.input_rectangles.length)
		.map(i_emplacement_destination => i_emplacement_destination - logical_graph.input_rectangles.length)
		.map(h => `[id="g-h-${h}"]`)
		.join(', ');

	   document
		.querySelectorAll(query)
		.forEach((element) => {element.style.visibility = "visible";});
	 }
	 else {
		   tr.className='';
		   selected = null;
		   document
			.querySelectorAll(`[id^="g-h-"]`)
			.forEach((element) => {element.style.visibility = "visible";});
		   document.getElementById("range_svg").innerHTML = "";
		   document.getElementById("chemin").getElementsByTagName("tbody")[0].innerHTML = "";
	 }
	 return true;
}


window.main = function main(){
	const {logical_graph, holes, decision_tree, translation_ranges, translation_ranges2, scores} = tests[testIndex];
	document.getElementById("range_svg").innerHTML = "";
	document.getElementById("chemin").getElementsByTagName("tbody")[0].innerHTML = "";
	document.getElementById("main_svg").innerHTML = print_html();

	for (const [json_source, id] of [[decision_tree, "decision_tree"], [scores, "scores"]])
	{
		let div = document.getElementById(id).getElementsByTagName('tbody')[0];

		div.innerHTML = json_source
				.map(item => {
								let deep_copy=JSON.parse(JSON.stringify(item));
								if ('i_emplacement_destination' in deep_copy)
									deep_copy.i_emplacement_destination = print_emplacement(item.i_emplacement_destination);
								return deep_copy;
							}
				)
				.map(item => [`<tr>`,
					      Object.values(item)
					      .map(i => `<td>${i}</td>`),
					     `</tr>`])
                        	.flat(2)
                        	.join('\n');

		div.addEventListener('mouseover', (event)=>{
		// 'highlight' color is set in tablelist.css
		   let tr = event.target.parentNode;
		   if ( tr.className === '') {
				tr.className='highlight';
		   }
		   return false
		});

		div.addEventListener('mouseout', (event)=>{
		   let tr = event.target.parentNode;
		   if ( tr.className === 'highlight') {
				tr.className='';
		   }
		   return false
		});

		div.addEventListener('mousedown', select_id);
	}

	const allTables = document.querySelectorAll('table');

	for (const table of allTables) {
	  const headerCells = table.tHead.rows[0].cells;

	  for (const th of headerCells) {
		const cellIndex = th.cellIndex;

		th.addEventListener('click', () => {
			const tBody = table.tBodies[0];
			const rows = Array.from(tBody.rows);
	  
			rows.sort((tr1, tr2) => {
				const tr1Text = tr1.cells[cellIndex].textContent;
				const tr2Text = tr2.cells[cellIndex].textContent;
				const tr1Int = parseInt(tr1Text);
				const tr2Int = parseInt(tr2Text);
				if (tr1Int < tr2Int)
					return -1;
				else if (tr1Int == tr2Int)
					return 0;
				else
					return +1;
			});

			tBody.append(...rows);
		});
	  }
	}
}
