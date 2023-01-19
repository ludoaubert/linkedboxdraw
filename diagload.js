var mycontexts;
var mydata;

var currentX = 0;
var currentY = 0;
var g = 0;

// FRAME_MARGIN is duplicated in table_input.js, diagload.js and topo_space.js
const FRAME_MARGIN = 20;


function selectElement(elmnt,clr)
{
	elmnt.style.color = clr;

	switch (clr)
	{
	case 'red':{
		g = document.getElementById('g_' + elmnt.id);
		console.log(g.id);
		console.log(g.parentElement.tagName);
		break;}
	case 'green':{
		deselectElement();
		currentX=0;
		currentY=0;
		g=0;
		break;}
	}

}

/*
<svg id="0"
  <g id="g_0"
    <rect id="rect_0"
    <foreignObject id="box0"
      <table id="0"
*/

function translate_draggable(g, dx, dy)
{
	const xForms = g.transform.baseVal;// an SVGTransformList
	const firstXForm = xForms.getItem(0); //an SVGTransform
	console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
	const translateX = firstXForm.matrix.e;
	const translateY = firstXForm.matrix.f;

	g.transform.baseVal.getItem(0).setTranslate(translateX+dx, translateY+dy);
}


function moveElement(evt) {

	if (g == 0)
		return;

	console.assert(g.class == "draggable");

	if (currentX==0 && currentY==0)
	{
		currentX = evt.clientX;
		currentY = evt.clientY;
	}

	const dx = evt.clientX - currentX;
	const dy = evt.clientY - currentY;

	translate_draggable(g, dx, dy);

	currentX = evt.clientX;
	currentY = evt.clientY;
}

function enforce_bounding_rectangle(selectedContextIndex)
{
	let context = mycontexts.contexts[selectedContextIndex];

	const bounding_rectangle = {
		left:-FRAME_MARGIN/2 + Math.min(...Array.from(context.translatedBoxes, tB => mycontexts.rectangles[tB.id].left + tB.translation.x)),
		right:+FRAME_MARGIN/2 + Math.max(...Array.from(context.translatedBoxes, tB => mycontexts.rectangles[tB.id].right + tB.translation.x)),
		top:-FRAME_MARGIN/2 + Math.min(...Array.from(context.translatedBoxes, tB => mycontexts.rectangles[tB.id].top + tB.translation.y)),
		bottom:+FRAME_MARGIN/2 + Math.max(...Array.from(context.translatedBoxes, tB => mycontexts.rectangles[tB.id].bottom + tB.translation.y))
	}

	const dx = - bounding_rectangle.left;
	const dy = - bounding_rectangle.top;

	for (let {id,translation} of context.translatedBoxes)
	{
		translation.x += dx;
		translation.y += dy;
	}

	let matches = document.querySelectorAll(`svg[id="${selectedContextIndex}"] > g[id^="g_"].draggable`);
	for (let g of matches)
		translate_draggable(g, dx, dy);

	context.frame = {
			left:0,
			right: bounding_rectangle.right - bounding_rectangle.left,
			top:0,
			bottom: bounding_rectangle.bottom - bounding_rectangle.top
	};
}


function compute_links(selectedContextIndex)
{
	const frame = mycontexts.contexts[selectedContextIndex].frame;
	const rectangles = Array.from(mycontexts.contexts[selectedContextIndex].translatedBoxes, (tB, index) => ({
			id: index,
			name: mydata.boxes[tB.id].title, //of interest for test data investigations
			left: mycontexts.rectangles[tB.id].left + tB.translation.x,
			right: mycontexts.rectangles[tB.id].right + tB.translation.x,
			top: mycontexts.rectangles[tB.id].top + tB.translation.y,
			bottom: mycontexts.rectangles[tB.id].bottom + tB.translation.y
		})
	);

	const hex = (i,n) => i.toString(16).padStart(n,'0');

	const rectdim = rectangles.map(r => [r.right-r.left, r.bottom-r.top])
				.flat()
				.map(i => hex(i,3))
				.join('');

	const translations = rectangles.map(r => [r.left, r.top])
					.flat()
					.map(i => hex(i,3))
					.join('');

	const sframe = [frame.left, frame.right, frame.top, frame.bottom]
				.map(i => hex(i,4))
				.join('');
	console.log(sframe);

	const ids = mycontexts.contexts[selectedContextIndex].translatedBoxes
				.map(tB => tB.id);

	const slinks = mydata.links
				.filter(lk => lk.from != lk.to)
				.filter(lk => lk.category != "TR2")
				.map(lk => ({from:lk.from, to:lk.to}))
				.filter(lk => ids.indexOf(lk.from) != -1 && ids.indexOf(lk.to) != -1)
				.map(lk => [ids.indexOf(lk.from), ids.indexOf(lk.to)])
				.map(lk => JSON.stringify(lk))
				.filter(function(lk, pos, self){
									return self.indexOf(lk) == pos;}
					) //removing duplicates
				.map(lk => JSON.parse(lk))
				.flat()
				.map(i => hex(i,2))
				.join('');
	console.log(slinks);

//logging call input to produce test data for further investigations...
	console.log({rectangles, frame});
	bombix=Module.cwrap("bombix","string",["string","string","string","string"])
	const jsonResponse = bombix(rectdim, translations, sframe, slinks);
	const links = JSON.parse(jsonResponse)
						.map(({polyline, from, to}) => ({polyline, from:ids[from], to:ids[to]}));
	return links;
}


function deselectElement()
{
	console.assert(g.parentElement.tagName=='svg');
	const id = parseInt(g.id.substring('g_'.length));
	const selectedContextIndex = parseInt(g.parentElement.id);

	let tB = mycontexts.contexts[selectedContextIndex].translatedBoxes.find(tB => tB.id == id);

	const xForms = g.transform.baseVal;// an SVGTransformList
	const firstXForm = xForms.getItem(0); //an SVGTransform
	console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
	const translateX = firstXForm.matrix.e;
	const translateY = firstXForm.matrix.f;

	if (tB.translation.x == translateX && tB.translation.y == translateY)
		return;

	tB.translation = {"x": translateX, "y": translateY};

	console.log(JSON.stringify(mycontexts.contexts[selectedContextIndex].translatedBoxes));

	enforce_bounding_rectangle(selectedContextIndex);

	console.log(JSON.stringify(mycontexts.contexts[selectedContextIndex].translatedBoxes));

	const links = compute_links(selectedContextIndex);
	mycontexts.contexts[selectedContextIndex].links = links;
	document.getElementById(`links_${selectedContextIndex}`).innerHTML = drawLinks(links);
}


function zeroPad(num, places)
{
  const zero = places - num.toString().length + 1;
  return Array(+(zero > 0 && zero)).join("0") + num;
}

function width(rectangle)
{
  return rectangle.right - rectangle.left;
}

function height(rectangle)
{
  return rectangle.bottom - rectangle.top;
}

const ZERO_PADDING_SIZE = 4;
const RECT_STROKE_WIDTH = 6;


function drawLinks(links)
{
	var innerHTML = "";
	
	for (const {from, to, polyline} of links)
	{
		let points = [];
		for (var k=0; k < polyline.length; k++)
		{
			const point = polyline[k];
			if (k == 0)
				points[k] = "M";
			else
				points[k] = "L";
			points[k] += `${point.x},${point.y}`;
		}
		innerHTML += `<path id="${zeroPad(from,ZERO_PADDING_SIZE)}${zeroPad(to,ZERO_PADDING_SIZE)}" d="${points.join(" ")}" fill="none" stroke="black" stroke-width="100"  marker-end="url(#markerArrow)" />`;

		const p1 = polyline[0];
		const p2 = polyline[1];
		const p3 = polyline[polyline.length - 2];
		const p4 = polyline[polyline.length - 1];

		if (p1.y==p2.y && p1.x > p2.x)	// left
			innerHTML += `<text x="${p1.x-5}" y="${p1.y-5}"></text>`;
		else if (p1.y==p2.y && p1.x < p2.x) // right
			innerHTML += `<text x="${p1.x+5}" y="${p1.y-5}"></text>`;
		else if (p1.x==p2.x && p1.y > p2.y) // up
			innerHTML += `<text x="${p1.x}" y="${p1.y-5}"></text>`;
		else if (p1.x==p2.x && p1.y < p2.y) // down
			innerHTML += `<text x="${p1.x}" y="${p1.y+10+5}"></text>`;

		if (p4.y==p3.y && p4.x > p3.x)	// right
			innerHTML += `<text x="${p4.x-5}" y="${p4.y+10+5}"></text>`;
		else if (p4.y==p3.y && p4.x < p3.x) // left
			innerHTML += `<text x="${p4.x+5}" y="${p4.y+10+5}"></text>`;
		else if (p4.x==p3.x && p4.y > p3.y) // down
			innerHTML += `<text x="${p4.x+5}" y="${p4.y-5}"></text>`;
		else if (p4.x==p3.x && p4.y < p3.y) // up
			innerHTML += `<text x="${p4.x+5}" y="${p4.y+10+5}"></text>`;
	}

	return innerHTML;
}


function drawDiagram(drawBoxComponent) {

	const {rectangles} = mycontexts;

	var innerHTML = "";

	for (const [selectedContextIndex, {title, frame, translatedBoxes, links}] of mycontexts.contexts.entries())
	{

		innerHTML += `<svg id="${selectedContextIndex}" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="${width(frame)}" height="${height(frame)}" viewBox="0 0 ${width(frame)} ${height(frame)}" title="" >
      <defs>
		<marker id="markerArrow"
	viewBox="0 0 10 10" refX="${9+RECT_STROKE_WIDTH/2}" refY="3"
          markerUnits="strokeWidth"
          markerWidth="10" markerHeight="10"
          orient="auto">
          <path d="M 0 0 L 0 6 L 9 3 z" />
        </marker>
      </defs>`;

/*
Elements in an SVG document fragment have an implicit drawing order, with the first elements in the SVG document fragment getting "painted" first. 
Subsequent elements are painted on top of previously painted elements.
Links are drawn first, because of RECT_STOKE_WIDTH. Rectangle stroke is painted over a small part of the link (after the marker actually).
*/
	innerHTML += `<g id="links_${selectedContextIndex}">`;
	innerHTML += drawLinks(links);
	innerHTML += `</g>`;

		for (const {id, translation} of translatedBoxes)
		{
			const rectangle = rectangles[id];

			innerHTML += `<g id="g_${id}" class="draggable" transform="translate(${translation.x},${translation.y})">
			<rect id="rect_${id}" x="${rectangle.left}" y="${rectangle.top}" width="${width(rectangle)}" height="${height(rectangle)}" />
			<foreignObject id="box${id}" width="${width(rectangle)}" height="${height(rectangle)}">`;

			innerHTML += drawBoxComponent(id);

			innerHTML += `</foreignObject>
			</g>`;
		}

		innerHTML += `</svg>`;
	}

	return innerHTML;
}


function drawDiag()
{
	document.getElementById("repartitionc").innerHTML = drawRepartition(mydata, mycontexts);
	document.getElementById("diagram").innerHTML = drawDiagram(drawComponent);
	const styleMap = expressCutLinks(mydata, mycontexts);
	for (const [id, color] of styleMap)
	{
		document.getElementById(id).style.backgroundColor = color;
	}
}


function ApplyRepartition()
{
	const repartitionTable = document.getElementById("repartition");

	var repartition = [];

	for (let row of repartitionTable.rows)
	{
	//iterate through rows
	//rows would be accessed using the "row" variable assigned in the for loop
		const id = parseInt(row.cells[0].innerText);
		const n = parseInt(row.cells[2].innerText);
		repartition[id]=n;
	}

	const translatedBoxes = mycontexts.contexts.map(({frame,translatedBoxes,links})=>translatedBoxes)
											.flat();

	const nb = 1 + Math.max(...repartition);

	mycontexts.contexts = [...Array(nb).keys()].map(i => ({
			"frame":null,
			"translatedBoxes":translatedBoxes.filter(({id,translation}) => repartition[id]==i),
			"links":[]
			})
		);

// case when a new box was created. It has not been assigned to a context by the previous algorithm.
// Below is the code that will detect it and assign it to its context.

	const ids = mycontexts.contexts.map(context => context.translatedBoxes)
									.flat()
									.map(tB =>tB.id);

	console.log(ids);

	[...repartition.entries()]
		.filter( ([id,i]) => i!=-1 && !ids.includes(id) )
		.forEach( ([id,i]) => mycontexts.contexts[i].translatedBoxes.push({id,translation:{x:FRAME_MARGIN*1.5,y:FRAME_MARGIN*1.5}}) );

	console.log(JSON.stringify(mycontexts));

// if a context has become empty, remove it.
	mycontexts.contexts = mycontexts.contexts.filter(context => context.translatedBoxes.length != 0);

	mycontexts.rectangles = mydata.boxes.map(box => compute_box_rectangle(box));

	for (let [selectedContextIndex, context] of mycontexts.contexts.entries())
	{
		enforce_bounding_rectangle(selectedContextIndex);
		context.links = compute_links(selectedContextIndex);
	}

	console.log(JSON.stringify(mycontexts));
}


function drawRepartition(mydata, mycontexts){

	var innerHTML = `<table id="repartition">`;

	var repartitionEntries = [];

	for (const [id, box] of mydata.boxes.entries())
	{
		repartitionEntries[id] = {boxName:box.title, id, selectedContextIndex:-1};
	}

	for (const [selectedContextIndex, context] of mycontexts.contexts.entries())
	{
		for (const {id, translation} of context.translatedBoxes)
		{
			repartitionEntries[id].selectedContextIndex = selectedContextIndex;
		}
	}

	for (const {boxName, id, selectedContextIndex} of repartitionEntries.sort(
		function (a, b) {
			return a.boxName.localeCompare(b.boxName);
		}
		)
	)
	{
		innerHTML += `
			<tr>
			  <td>${id}</td>
              <td>${boxName}</td>
			  <td contenteditable="true">${selectedContextIndex}</td>
            </tr>
			`
	}

    innerHTML += `</table>
	  <button id="apply repartition" type="button" onclick="ApplyRepartition(); drawDiag();">Apply Repartition</button>
`;

	return innerHTML;
}
