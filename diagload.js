var mycontexts;


var selectedSVGElement = 0;
var selectedElement = 0;
var selectedRectangleIndex = -1;
var selectedContextIndex = -1;
var currentX = 0;
var currentY = 0;
var currentTranslateX = 0;
var currentTranslateY = 0;


function selectElement(evt) 
{
	console.log("selectElement(evt)");
	let svg_elements = document.body.getElementsByTagName("svg");
  	for (let i=0; i < svg_elements.length; i++)
	{
    		let svg_element = svg_elements[i];
    		let r = svg_element.getBoundingClientRect();
	
		if (!(r.left <= evt.clientX && evt.clientY <= r.right && r.top <= evt.clientY && evt.clientY <= r.bottom))
			continue;

		console.log("svg=" + JSON.stringify({left:r.left, right:r.right, top:r.top, bottom:r.bottom}));

		selectedSVGElement = svg_element;
		group_elements = svg_element.getElementsByTagName("g");
    		for (let j=0; j < group_elements.length; j++)
		{
	  		let group = group_elements[j];
			let xForms = group.transform.baseVal;// an SVGTransformList
			let firstXForm = xForms.getItem(0); //an SVGTransform
			console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
			let translateX = firstXForm.matrix.e;
			let translateY = firstXForm.matrix.f;
			console.log("group=" + JSON.stringify({translateX, translateY}));
		
			let foreignObj = group.getElementsByTagName("foreignObject")[0];
			let width = foreignObj.width.baseVal.value;
			let height = foreignObj.height.baseVal.value;
			console.log(JSON.stringify({width,height}));
		
			let left = r.left + translateX;
			let right = left + width;
			let top = r.top + translateY;
			let bottom = top + height;
		
			if (left <= evt.clientX && evt.clientX <= right && top <= evt.clientY && evt.clientY <= bottom)
			{
				selectedContextIndex = i;
				selectedRectangleIndex = j;
				currentX = evt.clientX;
				currentY = evt.clientY;
				console.log(JSON.stringify({currentX, currentY}));
				selectedElement = group;
				selectedElement.setAttributeNS(null, "onmousemove", "moveElement(evt)");
				selectedElement.setAttributeNS(null, "onmouseout", "deselectElement(evt)");
				selectedElement.setAttributeNS(null, "onmouseup", "deselectElement(evt)");
				currentTranslateX = translateX;
				currentTranslateY = translateY;
				console.log("hit=" + JSON.stringify({left, right, top, bottom}));
				console.log("currentTranslateX=" + currentTranslateX);
				console.log("currentTranslateY=" + currentTranslateY);
				return;
			}		
    		}	
  	}
}
        
function moveElement(evt) {
	
	var dx = evt.clientX - currentX;
	var dy = evt.clientY - currentY;
	currentTranslateX += dx;
	currentTranslateY += dy;
      
	console.log("currentTranslateX=" + currentTranslateX);
	console.log("currentTranslateY=" + currentTranslateY);
	selectedElement.transform.baseVal.getItem(0).setTranslate(currentTranslateX, currentTranslateY);
	currentX = evt.clientX;
	currentY = evt.clientY;
}

        
function deselectElement(evt) 
{
	console.log("deselectElement(evt)");
	console.assert(selectedElement != 0, "no element selected!")

	let rectangles=[];
	
	let group_elements = selectedSVGElement.getElementsByTagName("g");

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
	}


	console.log("selectedContextIndex=" + selectedContextIndex)
	console.log("selectedRectangleIndex=" + selectedRectangleIndex);
	let reduced_edges = mycontexts.contexts[selectedContextIndex].reduced_edges;
	let frame = mycontexts.contexts[selectedContextIndex].frame;
	let data={rectangles,reduced_edges, frame}
	let url = 'http://localhost:8080/getReducedEdges?data=' + btoa(JSON.stringify(data));
	console.log("data=" + JSON.stringify(data));
	console.log(url);

        var xhr = new XMLHttpRequest();
    	xhr.open("GET", url, true);
    	xhr.onload = function(e)	{
        	if (xhr.readyState==4)	{
			if (xhr.status==200)	{
				console.log("attention!");
            			console.log("responseText=" + xhr.responseText);
				console.log("selectedContextIndex=" + selectedContextIndex);
				const ctx = mycontexts.contexts[selectedContextIndex];		
				ctx.links = JSON.parse(xhr.responseText);
				const translatedBox = ctx.translatedBoxes[selectedRectangleIndex];
				translatedBox.translation.x = currentTranslateX;
				translatedBox.translation.y = currentTranslateY;
				drawDiag();
				selectedElement.removeAttributeNS(null, "onmousemove");
				selectedElement.removeAttributeNS(null, "onmouseout");
				selectedElement.removeAttributeNS(null, "onmouseup");
				selectedElement = 0;
				selectedContextIndex = -1;
				selectedRectangleIndex = -1;
	selectedSVGElement = 0;
	        	} else	{
				console.error(xhr.statusText);
			}
		}
    	}

    	xhr.send(null);    
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
	alert(http_get_param);
	
	let source_boxes = boxes.map( () => [] );

	for (let link of links_)
	{
		source_boxes[link.to].push( boxes[link.from].title );
	}
	for (var i=0; i < source_boxes.length; i++)
		source_boxes[i].sort();
	alert(JSON.stringify(source_boxes));
	//TODO: deduplicate source_boxes[i]

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
	
	for (const {title, frame, translatedBoxes, links, reduced_edges} of mycontexts.contexts)
	{
		alert("context title=" + title);
		
		innerHTML += `<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="${width(frame)}" height="${height(frame)}" viewBox="0 0 ${width(frame)} ${height(frame)}" title="" >
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
			
			innerHTML += `<g class="draggable" transform="translate(${translation.x},${translation.y})" onmousedown="selectElement(evt);">
			<rect x="${rectangle.left}" y="${rectangle.top}" width="${width(rectangle)}" height="${height(rectangle)}" />
			<foreignObject id="box${id}" width="${width(rectangle)}" height="${height(rectangle)}">`;
			
			const toolbox = source_boxes[id].join(",");

			innerHTML += `<table id="${box.title}">`;
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
	
	alert(innerHTML);
	
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