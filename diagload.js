var mycontexts;
var mydata;

var currentX = 0;
var currentY = 0;
var g = 0;


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

        
function moveElement(evt) {
	
	if (g == 0)
		return;
	
	console.log('moveElement');
	if (currentX==0 && currentY==0)
	{
		currentX = evt.clientX;
		currentY = evt.clientY;
	}		
	
	const dx = evt.clientX - currentX;
	const dy = evt.clientY - currentY;

	const xForms = g.transform.baseVal;// an SVGTransformList
	const firstXForm = xForms.getItem(0); //an SVGTransform
	console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
	const translateX = firstXForm.matrix.e;
	const translateY = firstXForm.matrix.f;
  
	g.transform.baseVal.getItem(0).setTranslate(translateX+dx, translateY+dy);

	currentX = evt.clientX;
	currentY = evt.clientY;
}

function enforce_bounding_rectangle(context)
{
	const bounding_rectangle = {
		left:-FRAME_MARGIN/2 + Math.min(...Array.from(context.translatedBoxes, tB => mycontexts.rectangles[tB.id].left + tB.translation.x)),
		right:+FRAME_MARGIN/2 + Math.max(...Array.from(context.translatedBoxes, tB => mycontexts.rectangles[tB.id].right + tB.translation.x)),
		top:-FRAME_MARGIN/2 + Math.min(...Array.from(context.translatedBoxes, tB => mycontexts.rectangles[tB.id].top + tB.translation.y)),
		bottom:+FRAME_MARGIN/2 + Math.max(...Array.from(context.translatedBoxes, tB => mycontexts.rectangles[tB.id].bottom + tB.translation.y))
	}

	console.log(JSON.stringify(bounding_rectangle));

	for (let {id,translation} of context.translatedBoxes)
	{
		translation.x -= bounding_rectangle.left;
		translation.y -= bounding_rectangle.top;
	}
	
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
							.filter(lk => lk.Category != "TR2")
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
	const links = JSON.parse(jsonResponse);
	return links;	
}

        
function deselectElement() 
{	
	console.assert(g.parentElement.tagName=='svg');
	const id = parseInt(g.id.substring('g_'.length));
	console.log("id=" + id);
	const selectedContextIndex = g.parentElement.id;
	console.log("selectedContextIndex=" + selectedContextIndex);
	
	console.log(JSON.stringify(mycontexts.contexts[selectedContextIndex].translatedBoxes));
	
	let tB = mycontexts.contexts[selectedContextIndex].translatedBoxes.find(tB => tB.id == id);
	console.log("tB=" + JSON.stringify(tB));
	
	const xForms = g.transform.baseVal;// an SVGTransformList
	const firstXForm = xForms.getItem(0); //an SVGTransform
	console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
	const translateX = firstXForm.matrix.e;
	const translateY = firstXForm.matrix.f;
		
	tB.translation = {"x": translateX, "y": translateY};
	
	console.log(JSON.stringify(mycontexts.contexts[selectedContextIndex].translatedBoxes));
	
	enforce_bounding_rectangle(mycontexts.contexts[selectedContextIndex]);	
	
	console.log(JSON.stringify(mycontexts.contexts[selectedContextIndex].translatedBoxes));
	
	mycontexts.contexts[selectedContextIndex].links = compute_links(selectedContextIndex);		
	drawDiag();
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

function loadDiag(data, contexts) {
	mycontexts = JSON.parse(contexts);
	mydata = JSON.parse(data);
	drawDiag();
}


function drawDiag() {

	const {rectangles} = mycontexts;
	
	var innerHTML = drawRepartition(mydata, mycontexts);	
	
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
				innerHTML += `<text x="${p1.x-5}" y="${p1.y-5}" text-anchor="end">n</text>`;
			else if (p1.y==p2.y && p1.x < p2.x) // right
				innerHTML += `<text x="${p1.x+5}" y="${p1.y-5}" text-anchor="start">n</text>`;
			else if (p1.x==p2.x && p1.y > p2.y) // up
				innerHTML += `<text x="${p1.x}" y="${p1.y-5}" text-anchor="end">n</text>`;
			else if (p1.x==p2.x && p1.y < p2.y) // down
				innerHTML += `<text x="${p1.x}" y="${p1.y+10+5}" text-anchor="end">n</text>`;

			if (p4.y==p3.y && p4.x > p3.x)	// right
				innerHTML += `<text x="${p4.x-5}" y="${p4.y+10+5}" text-anchor="end">1</text>`;
			else if (p4.y==p3.y && p4.x < p3.x) // left
				innerHTML += `<text x="${p4.x+5}" y="${p4.y+10+5}" text-anchor="start">1</text>`;
			else if (p4.x==p3.x && p4.y > p3.y) // down
				innerHTML += `<text x="${p4.x+5}" y="${p4.y-5}" text-anchor="start">1</text>`;
			else if (p4.x==p3.x && p4.y < p3.y) // up
				innerHTML += `<text x="${p4.x+5}" y="${p4.y+10+5}" text-anchor="start">1</text>`;
		}
		
		for (const {id, translation} of translatedBoxes)
		{
			const rectangle = rectangles[id];
			
			innerHTML += `<g id="g_${id}" class="draggable" transform="translate(${translation.x},${translation.y})">
			<rect id="rect_${id}" x="${rectangle.left}" y="${rectangle.top}" width="${width(rectangle)}" height="${height(rectangle)}" />
			<foreignObject id="box${id}" width="${width(rectangle)}" height="${height(rectangle)}">`;
			
			innerHTML += drawComponent(id);
			
			innerHTML += `</foreignObject>
			</g>`;
		}
		
		innerHTML += `</svg>`;
	}
	
	innerHTML += drawiocomponent();

	document.getElementsByTagName("body")[0].innerHTML = innerHTML;
	
	expressCutLinks(mydata, mycontexts);
	
	var coll = document.getElementsByClassName("collapsible");

	for (i = 0; i < coll.length; i++) {
		coll[i].addEventListener("click", function() {
			this.classList.toggle("active");
			var content = this.nextElementSibling;
			if (content.style.display === "block") {
			  content.style.display = "none";
			} else {
			  content.style.display = "block";
			}
		});
	}
	
	setiohandlers();
}