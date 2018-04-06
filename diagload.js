
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
	alert("belles fesses!");
	const {title, boxes, values, boxComments, fieldComments, links:links_, rectangles, http_get_param} = JSON.parse(data);
	alert(http_get_param);
	let mycontexts = JSON.parse(contexts);
	
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
	
	var innerHTML = `<ul>
      <li>
        <svg width="80" height="70">

          <a xlink:href="http://www.scmlite.com" title="StructureXpress" />
          <polygon points="10,30 50,30 50,60 10,60" />
          <line x1="50" y1="45" x2="60" y2="45" />
          <polygon points="60,50 70,45 60,40" />
          <line x1="30" y1="30" x2="30" y2="20" />
          <polygon points="25,20 35,20 30,10" />
          </a>

        </svg>
      </li>

      <li>
        <a href = "centered_rectangles.html">connected rectangles</a>
      </li>
      <li>
        <a href = "http://localhost:8080/fan_in.html">fan in</a>
      </li>
      <li>
        <a href = "http://localhost:8080/fan_out.html">fan out</a>
      </li>
      <li>
        <a href = "http://localhost:8080/rectangle_filter.html">filter rectangles</a>
      </li>
    </ul>`;
	
	for (const {title, frame, translatedBoxes, links} of mycontexts.contexts)
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
			var points = [];
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
	let contexts = [];

	const svgTags = document.getElementsByTagName("svg");
	
	//start loop at 1 instead of 0 to skip the icon.
	for (let i=1; i < svgTags.length; i++)
	{
		const svgTag = svgTags[i];
		const title = svgTag.getAttribute("title");
		alert(title);
		const frame = {left:0, right:svgTag.width.baseVal.value, top:0, bottom:svgTag.height.baseVal.value};
		let translatedBoxes = [];
		
		const gTags = svgTag.getElementsByTagName("g");
		for (let j=0; j < gTags.length; j++)
		{
			const gTag = gTags[j];
			const xforms = gTag.transform.baseVal; // an SVGTransformList
			const firstXForm = xforms.getItem(0);	// an SVGTransform
			const foreignObject = gTag.getElementsByTagName("foreignObject")[0];
			translatedBoxes.push({id:parseInt(foreignObject.id.substr("box".length)), translation:{ x:firstXForm.matrix.e, y:firstXForm.matrix.f}});
		}
		
		let links = [];
		const pathTags = svgTag.getElementsByTagName("path");
		for (const pathTag of pathTags)
		{
			const id = pathTag.id;
			if (id.length != 2*ZERO_PADDING_SIZE)
				continue;
			const segments = pathTag.getAttribute("d") ;
			let polyline = [];
			const re = /(M|L)(\d+),(\d+)/g;
			do{
				m = re.exec(segments);
				if (m){
					polyline.push({x:parseInt(m[2]), y:parseInt(m[3])});
				}
			} while (m);

			links.push({polyline, from:parseInt(id.substring(0,ZERO_PADDING_SIZE)), to:parseInt(id.substring(ZERO_PADDING_SIZE, 2*ZERO_PADDING_SIZE))});
		}
		
		contexts.push({title, frame, translatedBoxes, links})
	}

	json_io = document.getElementById("json_input_output");
	json_io.value = JSON.stringify({contexts}/*, null, 4*/); // Indented 4 spaces
}