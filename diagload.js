var mycontexts;


var currentX = 0;
var currentY = 0;
var g = 0;


function myFunction(elmnt,clr) 
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

        
function deselectElement() 
{
	let rectangles=[];
	let translatedBoxes=[];
	
	console.assert(g.parentElement.tagName=='svg')
	console.log(g.parentElement.id);
	
	let group_elements = g.parentElement.getElementsByTagName("g");

	for (let i=0; i < group_elements.length; i++)
	{
		let group = group_elements[i];
		let xForms = group.transform.baseVal;// an SVGTransformList
		let firstXForm = xForms.getItem(0); //an SVGTransform
		console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
		let translateX = firstXForm.matrix.e;
		let translateY = firstXForm.matrix.f;
		
		foreignObj = group.getElementsByTagName("foreignObject")[0];
		let width = foreignObj.width.baseVal.value;
		let height = foreignObj.height.baseVal.value;
		
		let left = translateX;
		let right = left + width;
		let top = translateY;
		let bottom = top + height;
		rectangles.push({left, right, top, bottom});
		translatedBoxes.push({"id": group.id.substring(/*'g_'.length()*/2), "translation": {"x": translateX, "y": translateY}});
	}
	
	console.log(JSON.stringify(translatedBoxes));

	const selectedContextIndex = g.parentElement.id;
	console.log("selectedContextIndex=" + selectedContextIndex)
	const reduced_edges = mycontexts.contexts[selectedContextIndex].reduced_edges;
	const frame = mycontexts.contexts[selectedContextIndex].frame;
	const data={rectangles,reduced_edges, frame}
	var url = 'http://localhost:8080/getReducedEdges?data=' + btoa(JSON.stringify(data));
	url = url.replace('localhost', '192.168.0.27');
	console.log("data=" + JSON.stringify(data));
	console.log(url);
	
	var Http = new XMLHttpRequest();
	Http.onreadystatechange = (e) => {
		console.log("Http.response received");
		console.log(`response size = ${Http.responseText.length}`);
		const n = Http.responseText.length;
		if (n==0)
			return;
		console.log(Http.responseText);
		mycontexts.contexts[selectedContextIndex].links = JSON.parse(Http.responseText);
//il faut mettre translatedBoxes a jour si on veux que le deplacement de la boite ne soit pas resette lors de l'appel de drawDiag
		mycontexts.contexts[selectedContextIndex].translatedBoxes = translatedBoxes;
	
		drawDiag();
	}
	Http.open("GET", url);
	Http.send();
}


/*TODO: this function is present also in tableinput.html */
function compute_key_distrib(fields)
{
	var key_distrib = {"PK":0,"FK":0,"PKFK":0};
	
	for (let field of fields)
	{
		if (field.isPrimaryKey && field.isForeignKey)
			key_distrib["PKFK"]++;
		else if (field.isPrimaryKey)
			key_distrib["PK"]++;
		else if (field.isForeignKey)
			key_distrib["FK"]++;
	}

	return key_distrib;
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

function loadDiag() {
	mycontexts = JSON.parse(contexts);
	drawDiag();
}

function drawDiag() {

	const {title, boxes, values, boxComments, fieldComments, links:links_, rectangles, http_get_param} = JSON.parse(data);
	
	let source_boxes = boxes.map( () => [] );

	for (let link of links_)
	{
		source_boxes[link.to].push( boxes[link.from].title );
	}
	for (var i=0; i < source_boxes.length; i++)
		source_boxes[i].sort();

	let field2values = {};
	for (let {box, field, value} of values)
	{
		if (!(`${box}.${field}` in field2values))
			field2values[`${box}.${field}`] = [];
		field2values[`${box}.${field}`].push(value);
	}
	for (let boxField in field2values)
	{
		field2values[boxField].sort();
	}
	
	let box2comment = {};
	for (let {box, comment} of boxComments)
	{
		box2comment[box] = comment;
	}
	
	let field2comment = {};
	for (let {box, field, comment} of fieldComments)
	{
		field2comment[`${box}.${field}`] = comment;
	}
	
	var innerHTML = '';
	
	var selectedContextIndex=0;
	
	for (const {title, frame, translatedBoxes, links, reduced_edges} of mycontexts.contexts)
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
	  
		selectedContextIndex++;
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
				innerHTML += `<text x="${p1.x-5}" y="${p1.y-5}" text-anchor="end">1</text>`;
			else if (p1.y==p2.y && p1.x < p2.x) // right
				innerHTML += `<text x="${p1.x+5}" y="${p1.y-5}" text-anchor="start">1</text>`;
			else if (p1.x==p2.x && p1.y > p2.y) // up
				innerHTML += `<text x="${p1.x}" y="${p1.y-5}" text-anchor="end">1</text>`;
			else if (p1.x==p2.x && p1.y < p2.y) // down
				innerHTML += `<text x="${p1.x}" y="${p1.y+10+5}" text-anchor="end">1</text>`;

			if (p4.y==p3.y && p4.x > p3.x)	// right
				innerHTML += `<text x="${p4.x-5}" y="${p4.y+10+5}" text-anchor="end">n</text>`;
			else if (p4.y==p3.y && p4.x < p3.x) // left
				innerHTML += `<text x="${p4.x+5}" y="${p4.y+10+5}" text-anchor="start">n</text>`;
			else if (p4.x==p3.x && p4.y > p3.y) // down
				innerHTML += `<text x="${p4.x+5}" y="${p4.y-5}" text-anchor="start">n</text>`;
			else if (p4.x==p3.x && p4.y < p3.y) // up
				innerHTML += `<text x="${p4.x+5}" y="${p4.y+10+5}" text-anchor="start">n</text>`;
		}
		
		for (const {id, translation} of translatedBoxes)
		{
			const rectangle = rectangles[id];
			const box = boxes[id];
			const key_distrib = compute_key_distrib(box.fields);
			
			innerHTML += `<g id="g_${id}" class="draggable" transform="translate(${translation.x},${translation.y})">
			<rect id="rect_${id}" x="${rectangle.left}" y="${rectangle.top}" width="${width(rectangle)}" height="${height(rectangle)}" />
			<foreignObject id="box${id}" width="${width(rectangle)}" height="${height(rectangle)}">`;
			
			const toolbox = source_boxes[id].join(",");

			innerHTML += `<table id="${id}" onmousedown="myFunction(this,'red')" onmouseup="myFunction(this,'green')" onmousemove="moveElement(event)">`;
			innerHTML += `<tr><th contextmenu="menu${id}" title="${toolbox}">${box.title}</th></tr>`;
			for (var i=0; i < box.fields.length; i++)
			{
				const field = box.fields[i];

				var prefix = "";
				
				if (key_distrib["PKFK"])
				{
			//at least one 'PK FK' is present
					var pk = {"false":"&#160;&#160;", "true":"PK"}[field.isPrimaryKey.toString()];
					var fk = {"false":"&#160;&#160;", "true":"FK"}[field.isForeignKey.toString()];
					prefix = `${pk}&#160;${fk}&#160;`;
				}
				else if (key_distrib["PK"] || key_distrib["FK"])
				{
			//no 'PK FK' is present, but at least one PK|FK is present.
					var s;
					if (field.isPrimaryKey)
						s = "PK";
					else if (field.isForeignKey)
						s = "FK";
					else
						s = "&#160;&#160;";
					prefix = `${s}&#160;`;
				}
				else
				{
			//no 'PK FK' is present. no PK|FK either.
					prefix = `` ;
				}
				
				let leading_blanks = "&#160;&#160;&#160;" ;
				if (prefix.indexOf(leading_blanks) != 0)
					leading_blanks = "" ;

				let open_link = "";
				let close_link = "";
				
				const link = links_.find( link => link.from == id && link.fromField == i );
				if (link !== undefined)
				{
					const {to} = link;
					open_link = `<a href="#${boxes[to].title}">`;
					close_link = "</a>";
				}

				let font_weight="";
				let tooltip = [];
				
				if (`${box.title}.${field.name}` in field2comment)
				{
					tooltip.push(field2comment[`${box.title}.${field.name}`]); 
				}
				
				if (`${box.title}.${field.name}` in field2values)
				{
					font_weight = `style="font-weight: bold;"`;
					tooltip.push(field2values[`${box.title}.${field.name}`].join("\n"));
				}

				prefix = prefix.substring(leading_blanks.length);
				
				innerHTML += `<tr id="${field.name}"><td ${font_weight} title="${tooltip.join('\n')}">${leading_blanks}${prefix}${open_link}${field.name}${close_link}</td></tr>`;
			}

			innerHTML += `</table>`;
			
			innerHTML += `<menu type="popup" id="menu${box.title}">` ;
			innerHTML += `<menuitem label="centered_diagram" onclick="window.open(urlEncode('{id}'));"></menuitem>` ;
			for (let source_box of source_boxes[id])
			{
				innerHTML += `<menuitem label="${source_box}" onclick="window.location='#${source_box}';"></menuitem>` ;
			}
			innerHTML += `</menu>` ;
			
			innerHTML += `</foreignObject>
			</g>`;
		}
		
		innerHTML += `</svg>`;
	}
	
	innerHTML += `<h1>JSON Output</h1>

<ul>
<li><button type="button" onclick="refreshJsonFromEditData()">Refresh JSON From Edit Data</button></li>
<li><button type="button" onclick="refreshEditDataFromJson()">Refesh Edit Data From JSON</button></li>
<li><textarea rows="30" cols="180" id="json_input_output"></textarea></li>
</ul>`;
	
	document.title = title;
	document.getElementsByTagName("body")[0].innerHTML = innerHTML;
}


function refreshJsonFromEditData()
{
	alert("refreshJsonFromEditData");
	let json_io = document.getElementById("json_input_output");
	json_io.value = JSON.stringify({mycontexts}/*, null, 4*/); // Indented 4 spaces
}

function refreshEditDataFromJson()
{
	alert("refreshEditDataFromJson");
	let json_io = document.getElementById("json_input_output");
	context = json_io.value;
	loadDiag();
}