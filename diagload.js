import sample_contexts from "./contexts.json" assert {type: "json"};

import {default as createMyModule} from "./latuile-origine.js";
import {init, mydata, data, resetData, displayCurrent} from "./table_edit.js";
import {getFileData} from "./iocomponent.js";

export {mycontexts, contexts, resetContexts, drawDiag};

var mycontexts = sample_contexts;

var contexts=null;

function resetContexts()
{
	contexts = null;
}

var currentX = 0;
var currentY = 0;
var g = 0;
var sizer = 0;

// FRAME_MARGIN is duplicated in table_input.js, diagload.js and topo_space.js
const FRAME_MARGIN = 20;
const RECT_BORDER = 20;


function selectElement(elmnt)
{
	g = elmnt;
}

function deselectElement(elmnt)
{
	handleDeselectElement();
	currentX=0;
	currentY=0;
	g=0;
}

function selectSizer(elmnt)
{
	sizer = elmnt;
}

function deselectSizer(elmnt)
{
	handleDeselectSizer();
	currentX=0;
	currentY=0;
	sizer = 0;
}

function moveSizer(evt)
{
	if (sizer == 0)
		return;

	if (currentX==0 && currentY==0)
	{
		currentX = evt.clientX;
		currentY = evt.clientY;
	}

	const dx = evt.clientX - currentX;
	const dy = evt.clientY - currentY;

	const i = sizer.id.substring("sizer_".length);

	let fO = document.querySelector(`foreignObject[id=box${i}]`);

	const width = parseInt(fO.getAttribute("width"));
	const height = parseInt(fO.getAttribute("height"));

	fO.setAttribute("width", `${width+dx}`);
	fO.setAttribute("height", `${height+dy}`);

	let rect = document.querySelector(`rect[id=rect_${i}]`);

	rect.setAttribute("width", `${width+dx}`);
	rect.setAttribute("height", `${height+dy}`);

	const x = parseInt(sizer.getAttribute("x"));
	const y = parseInt(sizer.getAttribute("y"));

	sizer.setAttribute("x", `${x+dx}`);
	sizer.setAttribute("y", `${y+dy}`);

	currentX = evt.clientX;
	currentY = evt.clientY;
}

/*
<svg id="0"
  <g id="g_0"
    <rect id="rect_0"
    <foreignObject id="box0"
      <table id="0"...>
	<rect id="sizer_0"...>
  </g>
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

function width(rectangle)
{
	return rectangle.right - rectangle.left;
}

function height(rectangle)
{
	return rectangle.bottom - rectangle.top;
}

function expand_by(rect, margin)
{
	return {
		left: rect.left - margin,
		right: rect.right + margin,
		top: rect.top - margin,
		bottom: rect.bottom + margin
	};
}

function enforce_bounding_rectangle(selectedContextIndex)
{
	let context = mycontexts.contexts[selectedContextIndex];

	const rectangles = context.translatedBoxes
				.map(tB => {
					const r = mycontexts.rectangles[tB.id];
					const {x, y} = tB.translation;
					return {
						left: r.left + x,
						right: r.right + x,
						top: r.top + y,
						bottom: r.bottom + y
					};
				});

	const frame_ = {
		left: Math.min(...rectangles.map(r => r.left)),
		right: Math.max(...rectangles.map(r => r.right)),
		top: Math.min(...rectangles.map(r => r.top)),
		bottom: Math.max(...rectangles.map(r => r.bottom))
	};

	const frame = expand_by(frame_, RECT_BORDER + FRAME_MARGIN/2);

	context.frame = frame;

	const width_ = width(frame);
	const height_ = height(frame);
	const x = frame.left;
	const y = frame.top;

	let svgElement = document.querySelector(`svg[id="${selectedContextIndex}"]`);
	svgElement.setAttribute("width", `${width_}`);
	svgElement.setAttribute("height", `${height_}`);
	svgElement.setAttribute("viewBox",`${x} ${y} ${width_} ${height_}`);
}


function compute_links(selectedContextIndex)
{
	let context = mycontexts.contexts[selectedContextIndex];

	const rectangles = context.translatedBoxes
				.map(tB => {
					const r = mycontexts.rectangles[tB.id];
					const {x, y} = tB.translation;
					return {
						left: r.left + x,
						right: r.right + x,
						top: r.top + y,
						bottom: r.bottom + y
					};
				});

	const frame = context.frame;

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

	const bombix = Module.cwrap("bombix","string",["string","string","string","string"])
	const jsonResponse = bombix(rectdim, translations, sframe, slinks);
	const links = JSON.parse(jsonResponse)
				.map(({polyline, from, to}) => ({polyline, from:ids[from], to:ids[to]}));
	return links;
}


function handleDeselectSizer()
{
	const i = parseInt(sizer.id.substring('sizer_'.length));
	const g = sizer.parentElement;
	const svg = g.parentElement;
	const selectedContextIndex = parseInt(svg.id);

	const fO = document.querySelector(`foreignObject[id=box${i}]`);

	const width_ = parseInt(fO.getAttribute("width"));
	const height_ = parseInt(fO.getAttribute("height"));

	const r = mycontexts.rectangles[i];

	if (width(r) == width_ && height(r) == height_)
		return;

	const dx = width_ - width(r);
	const dy = height_ - height(r);

	mycontexts.rectangles[i] = {
		left: r.left,
		right: r.right + dx,
		top: r.top,
		bottom: r.bottom + dy
	};

	enforce_bounding_rectangle(selectedContextIndex);

	const links = compute_links(selectedContextIndex);
	mycontexts.contexts[selectedContextIndex].links = links;
	document.getElementById(`links_${selectedContextIndex}`).innerHTML = drawLinks(links);
}


function handleDeselectElement()
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

	enforce_bounding_rectangle(selectedContextIndex);

	const links = compute_links(selectedContextIndex);
	mycontexts.contexts[selectedContextIndex].links = links;
	document.getElementById(`links_${selectedContextIndex}`).innerHTML = drawLinks(links);
}


function zeroPad(num, places)
{
	const zero = places - num.toString().length + 1;
	return Array(+(zero > 0 && zero)).join("0") + num;
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

			innerHTML += `<g id="g_${id}" transform="translate(${translation.x},${translation.y})">
			<rect id="rect_${id}" x="${rectangle.left}" y="${rectangle.top}" width="${width(rectangle)}" height="${height(rectangle)}" />
			<foreignObject id="box${id}" width="${width(rectangle)}" height="${height(rectangle)}">`;

			innerHTML += drawBoxComponent(id, mydata);

			innerHTML += `</foreignObject>`
			innerHTML += `<rect id="sizer_${id}" x="${rectangle.right-4}" y="${rectangle.bottom-4}" width="4" height="4" />`
			innerHTML += `</g>`;
		}

		innerHTML += `</svg>`;
	}

	return innerHTML;
}


function drawDiag()
{
	document.getElementById("repartitionc").innerHTML = drawRepartition(mydata, mycontexts);
	document.getElementById("diagram").innerHTML = drawDiagram(drawComponent);
	addEventListeners();
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

function addEventListeners()
{
	document.querySelectorAll("svg > g[id^=g_]")
		.forEach(g => {
			g.addEventListener("mousedown", (event) => selectElement(g));
			g.addEventListener("mouseup", (event) => deselectElement(g));
			g.addEventListener("mousemove", (event) => moveElement(event));
		});

	document.querySelectorAll("g > rect[id^=sizer_]")
		.forEach(sizer => {
			sizer.addEventListener("mousedown", (event) => selectSizer(sizer));
			sizer.addEventListener("mouseup", (event) => deselectSizer(sizer));
			sizer.addEventListener("mousemove", (event) => moveSizer(event));
		});
}


export var Module;

window.main = function main()
{
	let gfi = document.querySelector("input[id=gfi]");

	gfi.addEventListener("change", (event) => {
		getFileData(gfi).then(function(result){
			contexts = result;
			mycontexts = JSON.parse(result);
			if (data != null && contexts != null)
			{
				resetData();
				resetContexts();
				drawDiag();
			}
			currentBoxIndex = -1;
			displayCurrent();
		});
	});
	let gfo = document.querySelector("input[id=gfo]");
	gfo.addEventListener("click", (event) => download(gfo.previousElementSibling.value, mycontexts));

	createMyModule().then(function(mymod){
		Module = mymod;
	});
	drawDiag();
	init();
}
