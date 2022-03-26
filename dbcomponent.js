// FRAME_MARGIN is duplicated in table_input.js
const FRAME_MARGIN = 20;


function download(filename) {
  var element = document.createElement('a');
  const Json = prettyContexts(JSON.stringify(mycontexts));
  element.setAttribute('href', 'data:text/plain;charset=utf-8,' + Json);
  element.setAttribute('download', filename);
  element.style.display = 'none';
  document.body.appendChild(element);
  element.click();
  document.body.removeChild(element);
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


function drawComponent(id) {

	const {documentTitle, boxes, values, boxComments, fieldComments, links, fieldColors} = mydata;
	
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
	
	const box = boxes[id];
	const key_distrib = compute_key_distrib(box.fields);

	const titleAttribute = box.title in box2comment ? `title="${box2comment[box.title]}"` : '';

	innerHTML = `<table id="${id}" ${titleAttribute} onmousedown="selectElement(this,'red')" onmouseup="selectElement(this,'green')" onmousemove="moveElement(event)">`;
	innerHTML += `<thead>
					<tr>
						<th>${box.title}</th>
					</tr>
				  </thead>
				  <tbody>
				  `;
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
		
		const link = links.find( link => link.from == id && link.fromField == i );
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
		
		const titleAttribute= tooltip.length!=0 ? `title="${tooltip.join('\n')}"` : '';
		innerHTML += `<tr id="${field.name}"><td ${font_weight} ${titleAttribute}>${leading_blanks}${prefix}${open_link}${field.name}${close_link}</td></tr>`;
	}

	innerHTML += `</tbody></table>`;
	
	return innerHTML;
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
	
	//make a deep copy of mycontexts
	const mycontexts_ = JSON.parse(JSON.stringify(mycontexts));

	const nb = 1 + Math.max(...repartition);
	
	mycontexts.contexts = [];
	
	for (let i=0; i <nb; i++)
	{
		mycontexts.contexts[i] = new Object({
			"title":"",
			"frame":null,
			"translatedBoxes":[],
			"links":[]
			});
	}
	
	for (const context_ of mycontexts_.contexts)
	{
		for (const {id,translation} of context_.translatedBoxes)
		{
			const i = repartition[id];
			mycontexts.contexts[i].translatedBoxes.push({id,translation});
		}
	}
	
	for (let [selectedContextIndex, context] of mycontexts.contexts.entries())
	{
		enforce_bounding_rectangle(context);	
		context.links = compute_links(selectedContextIndex);
	}
	
	console.log(JSON.stringify(mycontexts));
	
// case when a new box was created. It has not been assigned to a context by the previous algorithm.
// Below is the code that will detect it and assign it to its context.

	const ids = Array.from(mycontexts.contexts, context => context.translatedBoxes).flat().map(tB => tB.id);
	console.log(ids);
	
	[...repartition.entries()]
		.filter( ([id,i]) => i!=-1 && !ids.includes(id) )
		.forEach( ([id,i]) => mycontexts.contexts[i].translatedBoxes.push({id,translation:{x:FRAME_MARGIN*1.5,y:FRAME_MARGIN*1.5}}) );

	console.log(JSON.stringify(mycontexts));
	
// if a context has become empty, remove it.
	mycontexts.contexts = mycontexts.contexts.filter(context => context.translatedBoxes.length != 0);
	
	for (let context of mycontexts.contexts)
	{
		enforce_bounding_rectangle(context);
	}
	
	console.log(JSON.stringify(mycontexts));
}


function drawRepartition(mydata, mycontexts){
	
	var innerHTML = `<button type="button" class="collapsible">Repartition</button>
<div class="content">
      <table id="repartition">`;
	  
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
	  </div>
`;

	return innerHTML;
}

function expressCutLinks(mydata, mycontexts){
	
	const {documentTitle, boxes, values, boxComments, fieldComments, links, fieldColors} = mydata;
	
// listing unexpressed links - beginning

	let repartition=[];
	
	for (let id=0; id < mycontexts.rectangles.length; id++)
	{
		repartition[id] = -1;
	}
	
	for (const [i, context] of mycontexts.contexts.entries())
	{
		for (const {id,translation} of context.translatedBoxes)
		{
			repartition[id]=i;
		}
	}
	console.log(repartition);
	
	document.title = documentTitle;
	
	const cut_links = links.filter(link => link.Category=="TR2" || repartition[link.from] != repartition[link.to])
									.filter(link => link.fromField!=-1 && link.toField!=-1);
	console.log(cut_links);
	
// listing unexpressed link targets - beginning
	const cut_link_targets = [... new Set(cut_links.map( link => `${link.to}.${link.toField}`))];
	console.log(cut_link_targets);
//https://www.w3.org/wiki/CSS/Properties/color/keywords
	const cut_link_colors = ['lime','fuchsia','teal','aqua','aquamarine','coral','cornflowerblue','darkgray','darkkhaki']
	
	const colormap = new Map(
		[...cut_link_targets.entries()]
								.map(([i, to_toField]) => ([to_toField, cut_link_colors[i % cut_link_colors.length]]))
	);
	console.log(colormap);

// listing unexpressed link targets - end
	
	var sheet = document.createElement('style')
	
	const style = [...fieldColors
						.map( ({index,box,field,color})=>({index, field, color}) ),
				   ...cut_links
						.map( ({from,fromField,fromCardinality,to,toField,toCardinality}) => ([
																			{index:from, field:`${boxes[from].fields[fromField].name}`, color:colormap.get(`${to}.${toField}`)},
																			{index:to, field:`${boxes[to].fields[toField].name}`, color:colormap.get(`${to}.${toField}`)}
																							  ])
							)
				  ]
		.flat()
		.map(({index, field, color}) => `foreignObject#box${index} > table > tbody > tr#${field}{background-color: ${color};}`)
		.join('\n');

	console.log(style);
	sheet.innerHTML = style;
	document.body.appendChild(sheet);
}
