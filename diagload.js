import sample_contexts from "./contexts.json" with {type: "json"};

import {default as createMyModule} from "./latuile-origine.js";
import {init, mydata, data, resetData, displayCurrent, createMutationObserver} from "./table_edit.js";
import {initClient} from "./client.js";
import {getFileData, download} from "./iocomponent.js";
import {compute_box_rectangle} from "./compute_box_rectangles.js"

export {mycontexts, contexts, resetContexts, setContexts, drawDiag, compute_links, ApplyRepartition, enforce_bounding_rectangle, data2contexts};

var Module;

var mycontexts = sample_contexts;

function setContexts(mycontexts_)
{
	mycontexts = mycontexts_;
	contexts = JSON.stringify(mycontexts);
}

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


function hex(i,n) {
	console.assert(i >= 0, "formatting negative number as hex"); 
//encoding positive number
	return i.toString(16).padStart(n,'0');
}


async function data2contexts(mydata) {

	const {boxes, links} = mydata;

	const rectangles = boxes.map(box => compute_box_rectangle(box));

	const rectdim = rectangles.map(r => hex(r.right-r.left,3)+hex(r.bottom-r.top,3));
	console.log(rectdim);

	const slinks = links.filter(lk => lk.from != lk.to)
						.filter(lk => lk.category != "TR2")
						.map(lk => [lk.from, lk.to])
						.map(lk => JSON.stringify(lk))
						.filter(function(lk, pos, self){
									return self.indexOf(lk) == pos;}
						) //removing duplicates
						.map(lk => JSON.parse(lk))
						.flat()
						.map(i => hex(i,3))
						.join('');
	console.log(slinks);

	const bombix = Module.cwrap("bombix","string",["string","string","string","string"]);
	const latuile = Module.cwrap("latuile","string",["string","string"]);

	const jsonResponse = latuile(rectdim.join(''), slinks);
	console.log(jsonResponse);

	mycontexts = JSON.parse(jsonResponse);
	mycontexts.rectangles = rectangles;
	
	for (const [selectedContextIndex, context] of mycontexts.contexts.entries())
		context.links = await compute_links(selectedContextIndex);
	
	return mycontexts;
}


function selectElement(elmnt)
{
	if (sizer != 0)
		return;
	console.log("selectElement()");
	g = elmnt;
}

function deselectElement(elmnt)
{
	if (g == 0)
		return;
	console.log("deselectElement()");
	handleDeselectElement();
	currentX=0;
	currentY=0;
	g=0;
}

function selectSizer(elmnt)
{
	if (g != 0)
		return;
	console.log("selectSizer()");
	sizer = elmnt;
}

function deselectSizer(elmnt)
{
	if (sizer == 0)
		return;
	console.log("deselectSizer()");
	handleDeselectSizer();
	currentX=0;
	currentY=0;
	sizer = 0;
}

const MOVE_RANGE = 20;

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
	
	if (dx == 0 && dy == 0)
		return;
	
	console.log(`moveSizer() dx=${dx} dy=${dy}`);

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

	const g = sizer.parentElement;
	const svg = g.parentElement;
	const selectedContextIndex = parseInt(svg.id);	
	
	const xForms = g.transform.baseVal;// an SVGTransformList
	const firstXForm = xForms.getItem(0); //an SVGTransform
	console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
	const translateX = firstXForm.matrix.e;
	const translateY = firstXForm.matrix.f;
	
	const r = {
		left: translateX - MOVE_RANGE,
		right: translateX + width + dx + MOVE_RANGE,
		top: translateY - MOVE_RANGE,
		bottom: translateY + height + dy + MOVE_RANGE
	};
	
	enforce_bounding_rectangle(selectedContextIndex, r);

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

	console.log(`moveElement() dx=${dx} dy=${dy}`);

	translate_draggable(g, dx, dy);

	const selectedContextIndex = parseInt(g.parentElement.id);

	const i = g.id.substring("g_".length);

	let fO = document.querySelector(`foreignObject[id=box${i}]`);

	const width = parseInt(fO.getAttribute("width"));
	const height = parseInt(fO.getAttribute("height"));

	const xForms = g.transform.baseVal;// an SVGTransformList
	const firstXForm = xForms.getItem(0); //an SVGTransform
	console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
	const translateX = firstXForm.matrix.e;
	const translateY = firstXForm.matrix.f;
	
	const r = {
		left: translateX - MOVE_RANGE,
		right: translateX + width + MOVE_RANGE,
		top: translateY - MOVE_RANGE,
		bottom: translateY + height + MOVE_RANGE
	};

	enforce_bounding_rectangle(selectedContextIndex, r);

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

function enforce_bounding_rectangle(selectedContextIndex, r=null)
{
	let context = mycontexts.contexts[selectedContextIndex];

	const rectangles = context.translatedBoxes
				.map(tB => {
					const rect = document.querySelector(`rect[id=rect_${tB.id}]`);

					const width = parseInt(rect.getAttribute("width"));
					const height = parseInt(rect.getAttribute("height"));
					
					const g = document.querySelector(`g[id=g_${tB.id}]`);
					
					const xForms = g.transform.baseVal;// an SVGTransformList
					const firstXForm = xForms.getItem(0); //an SVGTransform
					console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
					const translateX = firstXForm.matrix.e;
					const translateY = firstXForm.matrix.f;
					
					return {
						left: translateX,
						right: translateX + width,
						top: translateY,
						bottom: translateY + height
					};
				});
				
	const compute_frame = rects => {
		const frame = {
			left: Math.min(...rects.map(r => r.left)),
			right: Math.max(...rects.map(r => r.right)),
			top: Math.min(...rects.map(r => r.top)),
			bottom: Math.max(...rects.map(r => r.bottom))
		};
		return expand_by(frame, RECT_BORDER + FRAME_MARGIN/2);
	};
	
	const frame = compute_frame(rectangles);
	
	let svgElement = document.querySelector(`svg[id="${selectedContextIndex}"]`);

	const [x, y, w, h] = svgElement.getAttribute("viewBox")
								.split(' ')
								.map(num => parseInt(num));
								
	const viewBox = {left:x, right:x+w, top:y, bottom:y+h};
	
//test if frame is contained inside viewBox.

	if (!(viewBox.left <= frame.left && frame.right <= viewBox.right && viewBox.top <= frame.top && frame.bottom <= viewBox.bottom))
	{
//if it is not contained, use the envelop of frame and r if r is not null.
		const _frame = compute_frame(r == null ? rectangles : [...rectangles, r]);

		const width_ = width(_frame);
		const height_ = height(_frame);
		const x = _frame.left;
		const y = _frame.top;
		
		console.log(`updating viewBox to ${x} ${y} ${width_} ${height_}`);

		svgElement.setAttribute("width", `${width_}`);
		svgElement.setAttribute("height", `${height_}`);
		svgElement.setAttribute("viewBox",`${x} ${y} ${width_} ${height_}`);
	}
	
	if (r == null)
	{
		const width_ = width(frame);
		const height_ = height(frame);
		const x = frame.left;
		const y = frame.top;

		console.log(`updating viewBox to ${x} ${y} ${width_} ${height_}`);
		
		svgElement.setAttribute("width", `${width_}`);
		svgElement.setAttribute("height", `${height_}`);
		svgElement.setAttribute("viewBox",`${x} ${y} ${width_} ${height_}`);		
		context.frame = frame;
	}
}


async function compute_links(selectedContextIndex)
{
	const {frame, translatedBoxes, links_} = mycontexts.contexts[selectedContextIndex];
	
// goal is to avoid negative number.
	const XY_TR = 1000;

	const rectangles = translatedBoxes
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

	const rectdim = rectangles.map(r => [r.right-r.left, r.bottom-r.top])
				.flat()
				.map(i => hex(i,3))
				.join('');

	const translations = rectangles.map(r => [r.left, r.top])
					.flat()
					.map(i => i + XY_TR)	//protection against negative numbers
					.map(i => hex(i,3))
					.join('');

	const sframe = [frame.left, frame.right, frame.top, frame.bottom]
				.map(i => i + XY_TR)		//protection against negative numbers
				.map(i => hex(i,4))
				.join('');
	console.log(sframe);

	const ids = translatedBoxes
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
	const jsonResponse = await bombix(rectdim, translations, sframe, slinks);
	const links = await JSON.parse(jsonResponse)
				.map(({polyline, from, to}) => ({
					polyline: polyline.map(({x,y}) => ({x:x-XY_TR, y:y-XY_TR})), 
					from:ids[from], 
					to:ids[to]
					}));
	return links;
}


async function handleDeselectSizer()
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

	const links = await compute_links(selectedContextIndex);
	mycontexts.contexts[selectedContextIndex].links = await links;
	document.getElementById(`links_${selectedContextIndex}`).innerHTML = await drawLinks(links);
}


async function handleDeselectElement()
{
	console.assert(g.parentNode.tagName=='svg');
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

	const links = await compute_links(selectedContextIndex);
	mycontexts.contexts[selectedContextIndex].links = await links;
	document.getElementById(`links_${selectedContextIndex}`).innerHTML = await drawLinks(links);
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
		const points = polyline.map((point, index) => `${index==0 ? "M":"L"}${point.x},${point.y}`);

		innerHTML += `<path id="${zeroPad(from,ZERO_PADDING_SIZE)}${zeroPad(to,ZERO_PADDING_SIZE)}" d="${points.join(" ")}" fill="none" stroke="black" stroke-width="100"  marker-end="url(#markerArrow)" />`;

		if (polyline.length >= 2)
		{
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
	}

	return innerHTML;
}


function drawDiagram(drawBoxComponent) {

	const {rectangles} = mycontexts;

	var innerHTML = "";

	for (const [selectedContextIndex, {title, frame, translatedBoxes, links}] of mycontexts.contexts.entries())
	{

		innerHTML += `<svg id="${selectedContextIndex}" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="${width(frame)}" height="${height(frame)}" viewBox="${frame.left} ${frame.top} ${width(frame)} ${height(frame)}" title="" >
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


async function drawDiag()
{
	document.getElementById("repartition").innerHTML = drawRepartition(mydata, mycontexts);
	document.getElementById("diagram").innerHTML = await drawDiagram(drawComponent);
	addEventListeners();
	const styleMap = expressCutLinks(mydata, mycontexts);
	for (const [id, color] of styleMap)
	{
		document.getElementById(id).style.backgroundColor = color;
	}
}


async function ApplyRepartition()
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

	const ids = mycontexts.contexts.map((context, i) => context.translatedBoxes.filter(({id, translation})=>repartition[id]==i))
									.flat()
									.map(tB =>tB.id);

	[...repartition.entries()]
			.filter( ([id,i]) => i!=-1 && !ids.includes(id) )
			.forEach( ([id,i]) => mycontexts.contexts[i].translatedBoxes.push({id,translation:{x:FRAME_MARGIN*1.5,y:FRAME_MARGIN*1.5}}) );

// if a context has become empty, remove it.
	mycontexts.contexts = mycontexts.contexts.filter(context => context.translatedBoxes.length != 0);

	mycontexts.rectangles = mydata.boxes.map(box => compute_box_rectangle(box));

	for (let [selectedContextIndex, context] of mycontexts.contexts.entries())
	{
		enforce_bounding_rectangle(selectedContextIndex);
		context.links = await compute_links(selectedContextIndex);
	}

	console.log(JSON.stringify(mycontexts));
}


function drawRepartition(mydata, mycontexts){

	const repartitionEntries = mycontexts.contexts.map((context, selectedContextIndex) => context.translatedBoxes.map(({id, translation}) => ({boxName:mydata.boxes[id].title,id,selectedContextIndex})))
												.flat(1);

	const innerHTML = repartitionEntries.sort((a, b)=>{return a.boxName < b.boxName ? -1 : a.boxName > b.boxName ? 1 : 0;})
					.map(({boxName, id, selectedContextIndex}) => `
			<tr>
			  <td>${id}</td>
              <td>${boxName}</td>
			  <td contenteditable="true">${selectedContextIndex}</td>
            </tr>
			`)
					.join('');

	return innerHTML;
}

function addEventListeners()
{
	document.querySelectorAll("svg > g[id^=g_]")
		.forEach(g => g.addEventListener("mousedown", event => selectElement(g)));
		
	document.querySelectorAll("svg")
		.forEach(svg => {
			svg.addEventListener("mousemove", event => {moveElement(event); moveSizer(event);});
			svg.addEventListener("mouseup", event => {deselectElement(g); deselectSizer(sizer);});
		});

	document.querySelectorAll("g > rect[id^=sizer_]")
		.forEach(sizer => sizer.addEventListener("mousedown", event => selectSizer(sizer)));
}


window.main = async function main()
{
	createMyModule().then(function(mymod){
		Module = mymod;
	});
	await drawDiag();
	init();
	initClient();
	createMutationObserver();
	
//making sure svg viewBox is computed in a unified way
	
	for (const [selectedContextIndex, context] of mycontexts.contexts.entries())
	{		
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
					
		const compute_frame = rects => {
			const frame = {
				left: Math.min(...rects.map(r => r.left)),
				right: Math.max(...rects.map(r => r.right)),
				top: Math.min(...rects.map(r => r.top)),
				bottom: Math.max(...rects.map(r => r.bottom))
			};
			return expand_by(frame, RECT_BORDER + FRAME_MARGIN/2);
		};
		
		const frame = compute_frame(rectangles);
		context.frame = frame;

		const width_ = width(context.frame);
		const height_ = height(context.frame);
		const x = context.frame.left;
		const y = context.frame.top;

		let svgElement = document.querySelector(`svg[id="${selectedContextIndex}"]`);
		svgElement.setAttribute("width", `${width_}`);
		svgElement.setAttribute("height", `${height_}`);
		svgElement.setAttribute("viewBox",`${x} ${y} ${width_} ${height_}`);		
	}
}
