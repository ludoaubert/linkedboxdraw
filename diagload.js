var mycontexts;
var mydata;

var currentX = 0;
var currentY = 0;
var g = 0;

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

	const reduced_edges = mycontexts.contexts[selectedContextIndex].reduced_edges;
	const frame = mycontexts.contexts[selectedContextIndex].frame;
	const rectangles = Array.from(mycontexts.contexts[selectedContextIndex].translatedBoxes, tB => ({
			left: mydata.rectangles[parseInt(tB.id)].left + tB.translation.x,
			right: mydata.rectangles[parseInt(tB.id)].right + tB.translation.x,
			top: mydata.rectangles[parseInt(tB.id)].top + tB.translation.y,
			bottom: mydata.rectangles[parseInt(tB.id)].bottom + tB.translation.y
		})
	);
	const data={rectangles,reduced_edges, frame}
	
//	Following lines will be usefull when wasm import will be available in the future:
//	const rectdim = rectangles.map(rec => [rec.right-rec.left, rec.bottom-rec.top]).flat().map(i => i.toString().padStart(3, "0")).join('');
//	const translations = rectangles.map(rec => [rec.left, rec.top]).flat().map(i => i.toString().padStart(3, "0")).join('');
//	const links = reduced_edges.map(edge => [edge.from, edge.to]).flat().map(i => i.toString().padStart(2, "0")).join('');
//	const sframe = [frame.left, frame.right, frame.top, frame.bottom].map(i => i.toString().padStart(4, "0")).join('');
//	(export "_Z6bombixRKNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEES7_S7_S7_" (func $_Z6bombixRKNSt3__112basic_stringIcNS_11char_traitsIcEENS_9allocatorIcEEEES7_S7_S7_))
//	WebAssembly.instantiateStreaming(fetch('bombix.wasm'), importObject)
//		.then(results => {
//		// Do something with the results!
//		const json = bombix(rectdim, translations, links, sframe);
//	});	
	var url = 'http://localhost:8080/getReducedEdges?data=' + btoa(JSON.stringify(data));
	url = url.replace('localhost', '192.168.0.27');
	console.log("data=" + JSON.stringify(data));
	console.log(url);
	
	var Http = new XMLHttpRequest();
	Http.onreadystatechange = (e) => {
		if (Http.readyState==4 && Http.status==200){
			console.log("Http.response received");
			console.log(`response size = ${Http.responseText.length}`);
			const n = Http.responseText.length;
			if (n==0)
			{
	//			drawDiag();
				return;
			}
			console.log(Http.responseText);
			mycontexts.contexts[selectedContextIndex].links = JSON.parse(Http.responseText);			
			drawDiag();
		}
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
	mydata = JSON.parse(data);
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
	
	var innerHTML = `<div><h1>Repartition</h1>
      <table id="repartition">`;
	  
	var repartitionEntries = [];
	  
	for (const [selectedContextIndex, context] of mycontexts.contexts.entries())
	{
		for (const {id, translation} of context.translatedBoxes)
		{
			repartitionEntries.push({boxName:boxes[id].title, id, selectedContextIndex});
		}
	}
	
// in case a new box has been added:

	for (var id=repartitionEntries.length; id < mydata.boxes.length; id++)
	{
		repartitionEntries.push({boxName:mydata.boxes[id].title, id, selectedContextIndex:-1});
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
	  <button id="apply repartition" type="button" onclick="ApplyRepartition()">Apply Repartition</button>
	  </div>
`;
	
	
	for (const [selectedContextIndex, {title, frame, translatedBoxes, links, reduced_edges}] of mycontexts.contexts.entries())
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
			const box = boxes[id];
			const key_distrib = compute_key_distrib(box.fields);
			
			innerHTML += `<g id="g_${id}" class="draggable" transform="translate(${translation.x},${translation.y})">
			<rect id="rect_${id}" x="${rectangle.left}" y="${rectangle.top}" width="${width(rectangle)}" height="${height(rectangle)}" />
			<foreignObject id="box${id}" width="${width(rectangle)}" height="${height(rectangle)}">`;
			
			const toolbox = source_boxes[id].join(",");

			innerHTML += `<table id="${id}" onmousedown="selectElement(this,'red')" onmouseup="selectElement(this,'green')" onmousemove="moveElement(event)">`;
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
<li>Load File</li>
<li><input type="file" id="myFile" value="Load"></li>
<li>
	<form onsubmit="return false">
	  <input type="text" name="name" value="test.txt">
	  <input type="submit" value="Save As" onclick="download(this['name'].value)">
	</form>
</li>
</ul>`;
	
	document.title = title;
	document.getElementsByTagName("body")[0].innerHTML = innerHTML;
	
	var input = document.getElementById("myFile");

	input.addEventListener("change", function () {
	  if (this.files && this.files[0]) {
		var myFile = this.files[0];
		var reader = new FileReader();
		
		alert("belles fesses!");
		
		reader.addEventListener('load', function (e) {
		  const buffer = e.target.result;
		  console.assert(buffer.slice(0,"contexts='".length)=="contexts='");
		  console.assert(buffer.slice(-"';".length)==-"';");
		  const Json = buffer.slice("contexts='".length, -"';".length);
		  refreshEditDataFromJson(Json);
		});
		
		reader.readAsBinaryString(myFile);
	  }   
	});
}

function download(filename) {
  var element = document.createElement('a');
  const Json = refreshJsonFromEditData();
  const Js = `data='${Json}';`  
  element.setAttribute('href', 'data:text/plain;charset=utf-8,' + Js);
  element.setAttribute('download', filename);
  element.style.display = 'none';
  document.body.appendChild(element);
  element.click();
  document.body.removeChild(element);
}


function refreshJsonFromEditData()
{
	alert("refreshJsonFromEditData");
	return JSON.stringify({mycontexts}/*, null, 4*/); // Indented 4 spaces
}

function refreshEditDataFromJson(Json)
{
	alert("refreshEditDataFromJson");
	context = Json;
	loadDiag();
}

function ApplyRepartition()
{
	alert("ApplyRepartition");

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
	console.log(JSON.stringify(repartition));
	
	const nb = 1 + Math.max(...repartition);
	var new_contexts = {contexts:[]};
	for (let i=0; i < nb; i++)
	{
		new_contexts.contexts.push({
			"title":"",
			"frame":{"left":0,"right":1921,"top":0,"bottom":1488},
			"translatedBoxes":[],
			"reduced_edges":[],
			"links":[]
			});
	}
	console.log(JSON.stringify(new_contexts));

// recopier les frame, les links et redispatcher les translatedBoxes.
	for (const [i, context] of mycontexts.contexts.entries())
	{
		new_contexts.contexts[i].frame = mycontexts.contexts[i].frame;
		new_contexts.contexts[i].links = mycontexts.contexts[i].links;
		for (const {id,translation} of context.translatedBoxes)
		{
			new_contexts.contexts[ repartition[id] ].translatedBoxes.push({id, translation});
		}
	}
	console.log(JSON.stringify(new_contexts));
	
// case when a new box was created. It has not been assigned to a context by the previous algorithm.
// Below is the code that will detect it and assign it to its context.

	const ids = Array.from(new_contexts.contexts, context => context.translatedBoxes).flat().map(tB => parseInt(tB.id));
	console.log(JSON.stringify(ids));
	
	[...repartition.entries()]
		.filter( ([id,i]) => i!=-1 && !ids.includes(id) )
		.forEach( ([id,i]) => new_contexts.contexts[i].translatedBoxes.push({id:`${id}`,translation:{x:FRAME_MARGIN*1.5,y:FRAME_MARGIN*1.5}}) );

	console.log(JSON.stringify(new_contexts));
	
// recalculer ensuite reduced_edges.
	console.log(JSON.stringify(mydata.links));
	for (let context of new_contexts.contexts)
	{
		const ids = Array.from(context.translatedBoxes, tB => parseInt(tB.id));
		console.log(JSON.stringify(ids));
		for (const link of mydata.links)
		{
			const index_from = ids.indexOf(parseInt(link.from));
			const index_to = ids.indexOf(parseInt(link.to));
			if (index_from != -1 && index_to != -1)
			{
				context.reduced_edges.push({from:index_from,to:index_to});
			}
		}	
	}

	
	for (let context of new_contexts.contexts)
	{
		enforce_bounding_rectangle(context);
	}
	
	console.log(JSON.stringify(new_contexts));
	mycontexts = new_contexts;
	drawDiag();
}

function enforce_bounding_rectangle(context)
{
	const bounding_rectangle = {
		left:-FRAME_MARGIN + Math.min(...Array.from(context.translatedBoxes, tB => mydata.rectangles[parseInt(tB.id)].left + tB.translation.x)),
		right:+FRAME_MARGIN + Math.max(...Array.from(context.translatedBoxes, tB => mydata.rectangles[parseInt(tB.id)].right + tB.translation.x)),
		top:-FRAME_MARGIN + Math.min(...Array.from(context.translatedBoxes, tB => mydata.rectangles[parseInt(tB.id)].top + tB.translation.y)),
		bottom:+FRAME_MARGIN + Math.max(...Array.from(context.translatedBoxes, tB => mydata.rectangles[parseInt(tB.id)].bottom + tB.translation.y))
	}				
	console.log(JSON.stringify(bounding_rectangle));

	for (let {id,translation} of context.translatedBoxes)
	{
		translation.x -= bounding_rectangle.left - FRAME_MARGIN/2;
		translation.y -= bounding_rectangle.top - FRAME_MARGIN/2;
	}
	
	context.frame = {
			left:0, 
			right: bounding_rectangle.right - bounding_rectangle.left,
			top:0,
			bottom: bounding_rectangle.bottom - bounding_rectangle.top
	};
}