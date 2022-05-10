


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

	let innerHTML = `<table id="${id}" ${titleAttribute} onmousedown="selectElement(this,'red')" onmouseup="selectElement(this,'green')" onmousemove="moveElement(event)">`;
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
	
	const cut_links = links.filter(link => link.category=="TR2" || repartition[link.from] != repartition[link.to])
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
