
import logical_graph from './logical_graph.json' assert {type: 'json'};
import holes from './holes.json' assert {type: 'json'};
import decision_tree from './decision_tree.json' assert {type: 'json'};


console.log(logical_graph);
console.log(holes);

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

	innerHTML += `<svg width="${frame.m_right-frame.m_left+100}" height="${frame.m_bottom-frame.m_top+100}">\n`;

	innerHTML += input_rectangles
			.map(({m_left, m_right, m_top, m_bottom}, index) => `<rect id="r-${index}" x="${m_left}" y="${m_top}" width="${m_right-m_left}" height="${m_bottom-m_top}" class=\"rect\" />\n`)
			.join('');

	innerHTML += input_rectangles
			.map((r, index) => `<text id="tr-${index}" x="${r.m_left}" y="${r.m_top}" fill="red">r-${index}</text>\n`)
			.join('');

	let windowed_index = [];
	windowed_index.length = input_rectangles.length;
	windowed_index.fill(1);

	innerHTML += logical_edges
			.map(({from, to}) => `<text id="le-${from}-${to}" x="${input_rectangles[from].m_left+8}" y="${input_rectangles[from].m_top+14*windowed_index[from]++}" class="logical_contact">r-${to}</text>\n`)
			.join('');

	windowed_index.fill(1);

	innerHTML += topological_edges
			.map(({from, to}) => `<text id="te-${from}-${to}" x="${input_rectangles[from].m_left+30}" y="${input_rectangles[from].m_top+14*windowed_index[from]++}" class="topological_contact">r-${to}</text>\n`)
			.join('');

	innerHTML += holes.holes
			.map((hole, index) => {	const {m_left, m_right, m_top, m_bottom}=hole.rec; return `<rect id="h-${index}" x="${m_left}" y="${m_top}" width="${m_right-m_left}" height="${m_bottom-m_top}" class="hole" />\n`;})
			.join('');

	innerHTML += holes.holes
			.map((hole, index) => `<text id="th-${index}" x="${hole.rec.m_left}" y="${hole.rec.m_top}" fill="black">hole-${index}</text>\n`)
			.join('');

	windowed_index.length = holes.holes.length;
	windowed_index.fill(1);

	innerHTML += holes.topological_contact
			.map(({hi, rj}) => `<text id="tc-${hi}-${rj}" x="${holes.holes[hi].rec.m_left + 8}" y="${holes.holes[hi].rec.m_top + 14*windowed_index[hi]++}" fill="black">r-${rj}</text>\n`)
			.join('');

	innerHTML += holes.holes
			.map(({ri, rec}, index) => `<text id="th-ri-${index}-${ri}" x="${rec.m_left+30}" y="${rec.m_top+14}" fill="black">ri=${ri}</text>\n`)
			.join('');

	innerHTML += "</svg>";

	return innerHTML;
}

var selected = null;

window.main = function main(){
	let div = document.getElementById("main_svg");
	const innerHTML = print_html();
	console.log(innerHTML);
	div.innerHTML = innerHTML;

	let dt = document.getElementById("decision_tree").getElementsByTagName('tbody')[0];
	dt.innerHTML = decision_tree
                        .map(({i, parent_index, depth, i_emplacement_source, i_emplacement_destination, match}) => `<tr><td>${i}</td><td>${parent_index}</td><td>${depth}</td><td>${i_emplacement_source}</td><td>${i_emplacement_destination}</td><td>${match}</td></tr>`)
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
`<tr><td>${i}</td><td>${parent_index}</td><td>${depth}</td><td>${i_emplacement_source}</td><td>${i_emplacement_destination}</td><td>${match}</td></tr>`)
			.join('\n');

     }
     else {
           tr.className='';
           selected = null;
     }
     return true
});
}
