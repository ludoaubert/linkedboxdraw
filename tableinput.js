// FRAME_MARGIN is duplicated in table_input.js, diagload.js and topo_space.js
const FRAME_MARGIN = 20;

var myBoxes = [];
var box2fields = {};
var field2values = {};
var box2comment = {};
var field2comment = {};
var field2color = {};


var input ;
var editTitle ;
var boxCombo ;
var fieldCombo ;
var valueCombo ;
var boxCommentTextArea ;	
var fieldCommentTextArea ;
var isPrimaryKeyCheckBox ;
var isForeignKeyCheckBox ;
var linkCombo ;
var newBoxEditField ;
var newFieldEditField ;
var fromBoxCombo ;
var fromFieldCombo ;
var fromCardinalityCombo ;
var toBoxCombo ;
var toFieldCombo ;
var toCardinalityCombo ;
var category ;
var newValueEditField ;
var colorBoxCombo ;
var colorFieldCombo ;
var colorCombo ;
var colorsCombo ;


function download(filename) {
  var element = document.createElement('a');
  const Json = refreshJsonFromEditData();
  const jsons = prettyData(JSON.stringify(Json));
  element.setAttribute('href', 'data:text/plain;charset=utf-8,' + jsons);
  element.setAttribute('download', filename);
  element.style.display = 'none';
  document.body.appendChild(element);
  element.click();
  document.body.removeChild(element);
}


function download2(filename) {
	var element = document.createElement('a');
	const Json = refreshJsonFromEditData();
	const {boxes, links} = Json;
	
	const rectangles = compute_box_rectangles(boxes);
	
	const hex = (i,n) => i.toString(16).padStart(n,'0');
	
	const rectdim = rectangles
						.map(r => hex(r.right-r.left,3)+hex(r.bottom-r.top,3));
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
	
	latuile = Module.cwrap("latuile","string",["string","string"]);
	bombix = Module.cwrap("bombix","string",["string","string","string","string"]);	


	const jsonResponse = latuile(rectdim.join(''), slinks);
	console.log(jsonResponse);
	
	data = JSON.parse(jsonResponse);
	
	data.rectangles = rectangles;
	
	data.contexts = data.contexts.map(
		({frame, translatedBoxes}) => {
			const {left,right,top,bottom} = frame;
			const sframe = [left, right, top, bottom]
							.map(i => hex(i,4))
							.join('');
			console.log(sframe);
			
			const translations = translatedBoxes
							.map(({id,translation})=>[translation.x,translation.y])
							.flat()
							.map(i => hex(i,3))
							.join('');
			console.log(translations);
			
			const ids = translatedBoxes.map(tB => tB.id);
			
			const rectdim_ = translatedBoxes
							.map(({id}) => rectdim[id])
							.join('');
			console.log(rectdim_);
			console.assert(rectdim_.size == translations.size);
			console.log(links);
			const links_ = links
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
			console.log(links_);
			const json2 = bombix(rectdim_, translations, sframe, links_);
			console.log(json2);
			const polylines = JSON.parse(json2);
			const polylines2 = polylines.map(({polyline,from,to})=>({polyline, from:ids[from], to:ids[to]}));
			console.log(polylines2);
			
			return {frame, translatedBoxes, links:polylines2};
		}
	);
	
	const jsonCompletedResponse = prettyContexts(JSON.stringify(data));
	element.setAttribute('href', 'data:text/plain;charset=utf-8,' + jsonCompletedResponse);
	element.setAttribute('download', filename);
	element.style.display = 'none';
	document.body.appendChild(element);
	element.click();
	document.body.removeChild(element);
}


function init(e) {
	
	input = document.getElementById("myFile");
	editTitle = document.getElementById("title");
	boxCombo = document.getElementById("boxes");
	fieldCombo = document.getElementById("fields");
	valueCombo = document.getElementById("values");
	boxCommentTextArea = document.getElementById("box comment");	
	fieldCommentTextArea = document.getElementById("field comment");
	isPrimaryKeyCheckBox = document.getElementById("PK");
	isForeignKeyCheckBox = document.getElementById("FK");
	linkCombo = document.getElementById("links");
	newBoxEditField = document.getElementById("new box");
	newFieldEditField = document.getElementById("new field");
	fromBoxCombo = document.getElementById("from boxes");
	fromFieldCombo = document.getElementById("from fields");
	fromCardinalityCombo = document.getElementById("from cardinality");
	toBoxCombo = document.getElementById("to boxes");
	toFieldCombo = document.getElementById("to fields");
	toCardinalityCombo = document.getElementById("to cardinality");
	category = document.getElementById("category");
	newValueEditField = document.getElementById("new value");
	colorBoxCombo = document.getElementById("color boxes");
	colorFieldCombo = document.getElementById("color fields");
	colorCombo = document.getElementById("color");
	colorsCombo = document.getElementById("colors");

	for (let cardinality of ["NULL","0","1","N","0,1","0,N","1,N"])
	{
		fromCardinalityCombo.add(new Option(cardinality,cardinality));
		toCardinalityCombo.add(new Option(cardinality,cardinality));
	}

	const colors=['yellow','pink','hotpink','palegreen','red','orange','skyblue','olive','grey','darkviolet'];

	colors.forEach(color => colorCombo.add(new Option(color, color)));

	input.addEventListener("change", function () {
	  if (this.files && this.files[0]) {
		var myFile = this.files[0];
		var reader = new FileReader();
		
		reader.addEventListener('load', function (e) {
		  refreshEditDataFromJson(e.target.result);
		});
		
		reader.readAsBinaryString(myFile);
	  }   
	});
	
	setCollapsibleHandler();
}

document.addEventListener('DOMContentLoaded', init, false);


function sortSelect(selElem) 
{
    let tmpAry = [];
    for (let {text, value} of selElem.options) 
	{
        tmpAry.push([text, value]);
    }
    tmpAry.sort();
	removeOptions(selElem);
   
    for (let [text, value] of tmpAry) 
	{
        selElem.add(new Option(text, value));
    }
}

function removeOptions(selElem) 
{
    while (selElem.options.length) 
	{
        selElem.remove(0);
    }
}

function copyOptions(sourceElem, targetElem)
{
	removeOptions(targetElem);
	for (let {text, value} of sourceElem.options)
	{
		targetElem.add(new Option(text, value));
    }
	sortSelect(targetElem);
}

function selectCascadeBox()
{
	copyOptions(boxCombo, fromBoxCombo);
	selectBox(fromBoxCombo, fromFieldCombo);
	copyOptions(boxCombo, toBoxCombo);
	selectBox(toBoxCombo, toFieldCombo);
	selectBox(boxCombo, fieldCombo);
	updateFieldAttributes();
	copyOptions(boxCombo, colorBoxCombo);
	selectBox(colorBoxCombo, colorFieldCombo);
	selectField();	
}

function addNewBox()
{
	const text = newBoxEditField.value;
	boxCombo.add(new Option(text,text));
	sortSelect(boxCombo);
	boxCombo.value = newBoxEditField.value;
	myBoxes.push(newBoxEditField.value);
	box2fields[newBoxEditField.value] = [];
	newBoxEditField.value='';
	boxCommentTextArea.value='';
	selectCascadeBox();
}

function dropBox()
{
	const box = boxCombo.value;
	let index = myBoxes.indexOf(box);
	myBoxes.splice(index,1);
	for (const {name, isPrimaryKey, isForeignKey} in box2fields[box])
	{
		const field = name;
		delete field2values[`${box}.${field}`];
		delete field2comment[`${box}.${field}`];
	}
	delete box2fields[box];
	boxCombo.remove(boxCombo.selectedIndex);
	
	selectCascadeBox();
	
	alert(JSON.stringify(box2fields));
	
	for (let i=linkCombo.options.length-1; i >= 0; i--) 
	{
	//Split a string with multiple parameters: Pass in a regexp as the parameter.
		let [fromBoxTitle, , ,toBoxTitle, ,] = linkCombo.options[i].text.split(/ -> |\./);
		if (fromBoxTitle == box || toBoxTitle == box)
		{
			alert ("Cascade dropping link: " + linkCombo.options[i].text);
			linkCombo.remove(i);
		}
	}		
}


function updateBox()
{
	//alert("updateBox");
	const box = boxCombo.value;
	
	boxCombo.remove(boxCombo.selectedIndex);
	
	//alert(JSON.stringify(box2fields[box]));
	
	let index = myBoxes.indexOf(box);
	myBoxes[index] = newBoxEditField.value;
		
	box2fields[newBoxEditField.value] = box2fields[box];
	delete box2fields[box];
	
	for (const {name, isPrimaryKey, isForeignKey} of box2fields[newBoxEditField.value])
	{
		const field = name;
		//alert(field);
		if (`${box}.${field}` in field2values)
		{
			//alert("Cascade updating values: " + JSON.stringify(field2values[`${box}.${field}`]));
			field2values[`${newBoxEditField.value}.${field}`] = field2values[`${box}.${field}`];
			delete field2values[`${box}.${field}`];
		}
	

		if (`${box}.${field}` in field2comment)
		{
			//alert("Cascade updating comment: " + field2comment[`${box}.${field}`]);
			field2comment[`${newBoxEditField.value}.${field}`] = field2comment[`${box}.${field}`];
			delete field2comment[`${box}.${field}`];
		}
		
		if (`${box}.${field}` in field2color)
		{
			//alert("Cascade updating comment: " + field2color[`${box}.${field}`]);
			field2color[`${newBoxEditField.value}.${field}`] = field2color[`${box}.${field}`];
			delete field2color[`${box}.${field}`];
		}
	}
	
	for (let option of linkCombo.options)
	{
	//Split a string with multiple parameters: Pass in a regexp as the parameter.
		let [fromBoxTitle, fromFieldName, fromCardinality, toBoxTitle, toFieldName, toCardinality] = option.text.split(/ -> |\./);
		let replace = false;
		if (fromBoxTitle == box)
		{
			replace = true;
			fromBoxTitle = newBoxEditField.value;
		}
		if (toBoxTitle == box)
		{
			replace = true;
			toBoxTitle = newBoxEditField.value;
		}
		if (replace)
		{
			//alert ("Cascade updating link: " + option.text);
			option.text = `${fromBoxTitle}.${fromFieldName}.${fromCardinality} -> ${toBoxTitle}.${toFieldName}.${toCardinality}`;
		}
	}	

	sortSelect(linkCombo);	
	
	const text = newBoxEditField.value
	boxCombo.add(new Option(text,text));
	boxCombo.value = newBoxEditField.value;
	newBoxEditField.value = '';
	
	copyOptions(boxCombo, fromBoxCombo);
	copyOptions(boxCombo, toBoxCombo);	
	
	//alert(JSON.stringify(box2fields));
}


function updateFieldAttributes()
{	
	if (fieldCombo.selectedIndex != -1)
	{
		const box = boxCombo.value;
		const fields = box2fields[box];
		
		isPrimaryKeyCheckBox.checked = false;
		isForeignKeyCheckBox.checked = false ;
		
		const {isPrimaryKey, isForeignKey} = fields.find( f => f.name == fieldCombo.value );

		isPrimaryKeyCheckBox.checked = isPrimaryKey;
		isForeignKeyCheckBox.checked = isForeignKey;
	}
	
	selectField();
}

function selectBox(boxCombo, fieldCombo)
{
	if (boxCombo.selectedIndex != -1)
	{
		removeOptions(fieldCombo);
		var box = boxCombo.value;
		var fields = box2fields[box];

		for (let {name} of fields)
		{
			fieldCombo.add(new Option(name,name));
		}
		sortSelect(fieldCombo);
	}
}


function selectField()
{
	if (boxCombo.selectedIndex != -1 && fieldCombo.selectedIndex != -1)
	{	
		removeOptions(valueCombo);
		var box = boxCombo.value;
		var field = fieldCombo.value;
		var values = [];
		if (`${box}.${field}` in field2values)
			values = field2values[`${box}.${field}`];

		for (let value in values)
		{
			valueCombo.add(new Option(value,value));
		}
		
		sortSelect(valueCombo);
		
		boxCommentTextArea.value = "";
		if (box in box2comment)
		{
			boxCommentTextArea.value = box2comment[box];
		}
		
		fieldCommentTextArea.value = "";
		if (`${box}.${field}` in field2comment)
		{
			fieldCommentTextArea.value = field2comment[`${box}.${field}`];
		}
	}
}

function addNewFieldToBox()
{
	const box = boxCombo.value;
	box2fields[box].push({"name":newFieldEditField.value,"isPrimaryKey":isPrimaryKeyCheckBox.checked,"isForeignKey":isForeignKeyCheckBox.checked});
	const field = newFieldEditField.value;
	fieldCombo.add(new Option(field,field));
	sortSelect(fieldCombo);
	fieldCombo.value = newFieldEditField.value;
	newFieldEditField.value = '';
	selectBox(fromBoxCombo, fromFieldCombo);
	selectBox(toBoxCombo, toFieldCombo);
	selectBox(colorBoxCombo, colorFieldCombo);
	updateFieldAttributes();
	selectField();
}

function updateField()
{
	//alert("updateField");
	const box = boxCombo.value;
	const field = fieldCombo.value;
	Object.assign(box2fields[box].find( f => f.name == field ), {
		"name":newFieldEditField.value.length!=0 ? newFieldEditField.value : field,
		"isPrimaryKey":isPrimaryKeyCheckBox.checked,
		"isForeignKey":isForeignKeyCheckBox.checked
	});

	fieldCombo.remove(fieldCombo.selectedIndex);
	
	//alert(JSON.stringify(box2fields[box]));
	if (`${box}.${field}` in field2values)
	{
		//alert("Cascade dropping values: " + JSON.stringify(field2values[`${box}.${field}`]));
		field2values[`${box}.${newFieldEditField.value}`] = field2values[`${box}.${field}`];
		delete field2values[`${box}.${field}`];
	}
	if (`${box}.${field}` in field2comment)
	{
		//alert("Cascade dropping comment: " + field2comment[`${box}.${field}`]);
		field2comment[`${box}.${newFieldEditField.value}`] = field2comment[`${box}.${field}`];
		delete field2comment[`${box}.${field}`];
	}
	selectField();
	
	for (let option of linkCombo.options)
	{
	//Split a string with multiple parameters: Pass in a regexp as the parameter.
		let [fromBoxTitle, fromFieldName, fromCardinality, toBoxTitle, toFieldName, toCardinality] = option.text.split(/ -> |\./);
		let replace = false;
		if (fromBoxTitle == box && fromFieldName == field)
		{
			replace = true;
			fromFieldName = newFieldEditField.value;
		}
		if (toBoxTitle == box && toFieldName == field)
		{
			replace = true;
			toFieldName = newFieldEditField.value;
		}
		if (replace)
		{
			//alert ("Cascade dropping link: " + option.text);
			option.text = `${fromBoxTitle}.${fromFieldName}.${fromCardinality} -> ${toBoxTitle}.${toFieldName}.${toCardinality}`;
		}
	}	

	sortSelect(linkCombo);	

	const text = newFieldEditField.value;
	fieldCombo.add(new Option(text,text));
	sortSelect(fieldCombo);
	fieldCombo.value = newFieldEditField.value;
	newFieldEditField.value = '';
	//alert(JSON.stringify(box2fields));
	selectBox(fromBoxCombo, fromFieldCombo);
	selectBox(toBoxCombo, toFieldCombo);
	selectBox(boxCombo,fieldCombo);
	selectBox(colorBoxCombo, colorFieldCombo);
	updateFieldAttributes();
	selectField();
}


function dropFieldFromBox()
{
	//alert("dropFieldFromBox");
	var box = boxCombo.value;
	var field = fieldCombo.value;
	let index = box2fields[box].findIndex(f => f.name == field);
	box2fields[box].splice(index,1);
	fieldCombo.remove(fieldCombo.selectedIndex);
	updateFieldAttributes();
	//alert(JSON.stringify(box2fields[box]));
	if (`${box}.${field}` in field2values)
	{
		//alert("Cascade dropping values: " + JSON.stringify(field2values[`${box}.${field}`]));
		delete field2values[`${box}.${field}`];
	}
	if (`${box}.${field}` in field2comment)
	{
		//alert("Cascade dropping comment: " + field2comment[`${box}.${field}`]);
		delete field2comment[`${box}.${field}`];
	}
	if (`${box}.${field}` in field2color)
	{
		//alert("Cascade dropping comment: " + field2color[`${box}.${field}`]);
		delete field2color[`${box}.${field}`];
	}	
	selectField();
	
	for (let i=linkCombo.options.length-1; i >= 0; i--) 
	{
	//Split a string with multiple parameters: Pass in a regexp as the parameter.
		let [fromBoxTitle, fromFieldName, fromCardinality, toBoxTitle, toFieldName, toCardinality] = linkCombo.options[i].text.split(/ -> |\./);
		if ((fromBoxTitle == box && fromFieldName == field) || (toBoxTitle == box && toFieldName == field))
		{
			//alert ("Cascade dropping link: " + linkCombo.options[i].text);
			linkCombo.remove(i);
		}
	}		
}

function editValueFromField()
{
	//alert("editValueFromField");
	newValueEditField.value = valueCombo.value;
}

function addNewValueToField()
{
	//alert("addNewValueToField");
	const box = boxCombo.value;
	const field = fieldCombo.value;

	if (!(`${box}.${field}` in field2values))
		field2values[`${box}.${field}`] = [];
	field2values[`${box}.${field}`].push(newValueEditField.value);
	field2values[`${box}.${field}`].sort();
	const text = newValueEditField.value;
	valueCombo.add(new Option(text,text));
	sortSelect(valueCombo);
	valueCombo.value = newValueEditField.value;
	newValueEditField.value = '';
	//alert(JSON.stringify(field2values));
}

function updateValue()
{
	//alert("updateValue");
	dropValueFromField();
	addNewValueToField();
}

function dropValueFromField()
{
	//alert("dropValueFromField");
	const box = boxCombo.value;
	const field = fieldCombo.value;
	const value = valueCombo.value;
	valueCombo.remove(valueCombo.selectedIndex);
	let values = field2values[`${box}.${field}`];
	const index = values.indexOf(value);
	values.splice(index,1);
	selectField();
}

function selectLink()
{
	const myRegexp = /([^\.]+)\.([^\.]+)\.([^\s]+) \-\> ([^\.]+)\.([^\.]+)\.([^\s]+).([^\s]+)/g;
	const match = myRegexp.exec(linkCombo.value);
	fromBoxCombo.value = match[1];
	selectBox(fromBoxCombo, fromFieldCombo);
	fromFieldCombo.value = match[2];
	fromCardinalityCombo.value = match[3];
	toBoxCombo.value = match[4];
	selectBox(toBoxCombo, toFieldCombo);
	toFieldCombo.value = match[5];
	toCardinalityCombo.value = match[6];
	category.value = match[7];
}

function updateLink()
{
	const text = `${fromBoxCombo.value}.${fromFieldCombo.value}.${fromCardinalityCombo.value} \-> ${toBoxCombo.value}.${toFieldCombo.value}.${toCardinalityCombo.value}.${category.value}`;
	linkCombo.options[linkCombo.selectedIndex].innerHTML = text;
}

function addNewLink()
{
	const text = `${fromBoxCombo.value}.${fromFieldCombo.value}.${fromCardinalityCombo.value} \-> ${toBoxCombo.value}.${toFieldCombo.value}.${toCardinalityCombo.value}.${category.value}`;					
	linkCombo.add(new Option(text, text));
	sortSelect(linkCombo);
	linkCombo.value = text;
}

function dropLink()
{
	if (linkCombo.selectedIndex != -1)
		linkCombo.remove(linkCombo.selectedIndex);
}



function dropBoxComment()
{
	const box = boxCombo.value;
	delete box2comment[box];
	boxCommentTextArea.value = "";
}

function updateBoxComment()
{
	const box = boxCombo.value;
	box2comment[box] = boxCommentTextArea.value;
}

function dropFieldComment()
{
	const box = boxCombo.value;
	const field = fieldCombo.value;
	delete field2comment[`${box}.${field}`];
	fieldCommentTextArea.value = "";
}

function updateFieldComment()
{
	const box = boxCombo.value;
	const field = fieldCombo.value;
	field2comment[`${box}.${field}`] = fieldCommentTextArea.value;
}

function addNewColor()
{
	const box = colorBoxCombo.value;
	const field = colorFieldCombo.value;
	const color = colorCombo.value;
	field2color[`${box}.${field}`] = color;
	
	const text = `${box}.${field}=>${color}`;
	colorsCombo.add(new Option(text, text));
	sortSelect(colorsCombo);
	colorsCombo.value = text;
}

function updateColor()
{
	const box = colorBoxCombo.value;
	const field = colorFieldCombo.value;
	const color = colorCombo.value;
	field2color[`${box}.${field}`] = color;
	
	const text = `${box}.${field}=>${color}`;
	colorsCombo.options[colorsCombo.selectedIndex].innerHTML = text;
}

function dropColor()
{
	if (colorsCombo.selectedIndex != -1)
		colorsCombo.remove(colorsCombo.selectedIndex);
}

function refreshJsonFromEditData()
{	
	var boxes = [];

	for (let [id, box] of myBoxes.entries())
	{
		boxes.push({"title":box, "id":id, "fields":box2fields[box]})
	}
	
	let values = [];
	for (let boxfield in field2values)
	{
		[box, field] = boxfield.split(".");
		for (let value of field2values[boxfield])
		{
			values.push({box, field, value});
		}
	}
	
	let boxComments = [];
	for (let box in box2comment)
	{
		comment = box2comment[box];
		boxComments.push({box, comment});
	}
	
	let fieldComments = [];
	for (let boxfield in field2comment)
	{
		comment = field2comment[boxfield];
		[box, field] = boxfield.split(".");
		fieldComments.push({box, field, comment});
	}
	
	let fieldColors = [];
	for (let boxfield in field2color)
	{
		const color = field2color[boxfield];
		[box, field] = boxfield.split(".");
		let index = myBoxes.indexOf(box);
		fieldColors.push({index, box, field, color});
	}

	let links = [];

	let i=0;
	for (let option of linkCombo.options)
	{
	//Split a string with multiple parameters: Pass in a regexp as the parameter.
		const [fromBoxTitle, fromFieldName, fromCardinality, toBoxTitle, toFieldName, toCardinality, category] = option.text.split(/ -> |\./);
		console.log({fromBoxTitle, fromFieldName, toBoxTitle, toFieldName, category});
		const fromBoxIndex = boxes.findIndex( box => box.title == fromBoxTitle );
		if (fromBoxIndex == -1)
			alert(`link ${i}: No Box named ${fromBoxTitle}!`);
		const fromFieldIndex = boxes[fromBoxIndex].fields.findIndex( field => field.name == fromFieldName );
		if (fromFieldIndex == -1)
			alert(`link ${i}: Box ${fromBoxTitle} has no field named ${fromFieldName}!`);
		const toBoxIndex = boxes.findIndex( box => box.title == toBoxTitle );
		if (toBoxIndex == -1)
			alert(`link ${i}: No Box named ${toBoxTitle}`);
		const toFieldIndex = boxes[toBoxIndex].fields.findIndex( field => field.name == toFieldName );
		if (toFieldIndex == -1)
			alert(`link ${i}: Box ${toBoxTitle} has no field named ${toFieldName}!`);
		links.push(
			{
			"from":fromBoxIndex, 
			"fromField":fromFieldIndex,
			"fromCardinality":fromCardinality,
			"to":toBoxIndex,
			"toField":toFieldIndex,
			"toCardinality":toCardinality,
			"Category":category
			}
		);
		i++;
    }
	
	const documentTitle = editTitle.value;

	const json = {documentTitle, boxes, values, boxComments, fieldComments, links, fieldColors};
	return json;
}


function refreshEditDataFromJson(Json)
{
	const {documentTitle, boxes, values, boxComments, fieldComments, links, fieldColors, rectangles, http_get_param, http_get_request} = JSON.parse(Json);
	
	editTitle.value = documentTitle;
	
	removeOptions(boxCombo);
	removeOptions(fieldCombo);
	removeOptions(fromBoxCombo);
	removeOptions(toBoxCombo);
	removeOptions(linkCombo);
	removeOptions(colorBoxCombo);
	removeOptions(colorFieldCombo);
	removeOptions(colorsCombo);
	
	myBoxes = [];
	for (const {title, id, fields} of boxes)
	{
		myBoxes.push(title);
	}
	
	console.log(myBoxes);
	
	for (const {from,fromField,fromCardinality,to,toField,toCardinality,category} of links)
	{
		console.log({from,fromField,fromCardinality,to,toField,toCardinality,category});
		console.assert(from < boxes.length);
		console.assert(fromField < boxes[from].fields.length);
		console.assert(to < boxes.length);
		console.assert(toField < boxes[to].fields.length);
		
		let text = boxes[from].title +
					"." +
					(fromField != -1 ? boxes[from].fields[fromField].name : '') +
					"." +
					fromCardinality +
					" -> " + 
					boxes[to].title +
					"." +
					(toField != -1 ? boxes[to].fields[toField].name : '') +
					"." +
					toCardinality +
					"." +
					category;
					
		linkCombo.add(new Option(text, text));
    }
	sortSelect(linkCombo);

	for (const box of boxes) 
	{
		boxCombo.add(new Option(box.title, box.title));
	}
	sortSelect(boxCombo);
	
	copyOptions(boxCombo, fromBoxCombo);
	copyOptions(boxCombo, toBoxCombo);
	copyOptions(boxCombo, colorBoxCombo);
	
	box2fields = {};
	for (const box of boxes)
		box2fields[box.title] = box.fields;
	
	field2values = {};
	for (const {box,field,value} of values)
	{
		if (!(`${box}.${field}` in field2values))
			field2values[`${box}.${field}`] = [];
		field2values[`${box}.${field}`].push(value);
	}
	
	box2comment = {};
	for (let {box,comment} of boxComments)
	{
		box2comment[box] = comment;
	}
	
	field2comment = {};
	for (let {box,field,comment} of fieldComments)
	{
		field2comment[`${box}.${field}`] = comment;
	}
	
	field2color = {};
	for (let {index,box,field,color} of fieldColors)
	{
		field2color[`${box}.${field}`] = color;
	}
	
	for (const {index,box,field,color} of fieldColors)
	{
		let text = `${box}.${field}=>${color}`;
		colorsCombo.add(new Option(text, text));
    }
	sortSelect(colorsCombo);
	
	selectBox(boxCombo, fieldCombo);
	updateFieldAttributes();
	selectBox(fromBoxCombo, fromFieldCombo);
	selectBox(toBoxCombo, toFieldCombo);
	selectBox(colorBoxCombo, colorFieldCombo);
	selectField();
}


function enable_disable()
{
	const box = boxCombo.value;
	const field = fieldCombo.value;
	const boxComment = boxCommentTextArea.value;
	const fieldComment = fieldCommentTextArea.value;
	var fieldIndex = -1;
	var name = newFieldEditField.value;
	var isPrimaryKey = isPrimaryKeyCheckBox.checked;
	var isForeignKey = isForeignKeyCheckBox.checked;
/*
	if (box in box2fields)
	{
		let myfield = box2fields[box].find( f => f.name == field );
		if (myfield !== undefined)
		{
			({isPrimaryKey, isForeignKey} = myfield);
			console.log(myfield);
		}
	}
*/	
	document.getElementById("add box").disabled = newBoxEditField.value.length == 0 || myBoxes.indexOf(newBoxEditField.value) != -1;
	document.getElementById("drop box").disabled = boxCombo.selectedIndex == -1 ||
												newBoxEditField.value.length != 0;
	document.getElementById("update box").disabled = boxCombo.selectedIndex == -1 ||
														newBoxEditField.value.length == 0 ||
														boxCombo.value == newBoxEditField.value ||
														myBoxes.indexOf(newBoxEditField.value) != -1 ;
	
	document.getElementById("add field").disabled = newFieldEditField.value.length == 0 || box2fields[box].findIndex(f => f.name == newFieldEditField.value) != -1;
	document.getElementById("drop field").disabled = fieldCombo.selectedIndex == -1 ||
													newFieldEditField.value.length != 0;
	
	document.getElementById("update field").disabled = fieldCombo.selectedIndex == -1 ||
														//newFieldEditField.value.length == 0 ||
														(newFieldEditField.value.length != 0 && newFieldEditField.value == fieldCombo.value) ||
														(isPrimaryKey == isPrimaryKeyCheckBox.checked && isForeignKey == isForeignKeyCheckBox.checked) ||
														box2fields[box].findIndex(f => f.name == newFieldEditField.value) != -1;	

	
	document.getElementById("edit value").disabled = valueCombo.selectedIndex == -1;

	const isExistingValue = `${box}.${field}` in field2values && field2values[`${box}.${field}`].indexOf(newValueEditField.value) != -1;
	document.getElementById("add value").disabled = newValueEditField.value.length == 0 || isExistingValue;
	document.getElementById("drop value").disabled = valueCombo.selectedIndex == -1 ||
													newValueEditField.value.length != 0;
	document.getElementById("update value").disabled = (valueCombo.selectedIndex == -1 || newValueEditField.value.length == 0) || isExistingValue;
	
	document.getElementById("drop link").disabled = linkCombo.selectedIndex == -1;

	document.getElementById("update box comment").disabled = boxComment.length == 0 || (box in box2comment && box2comment[box] == boxComment);
	document.getElementById("drop box comment").disabled = !(box in box2comment);
	document.getElementById("update field comment").disabled = fieldComment.length == 0 || (`${box}.${field}` in field2comment && field2comment[`${box}.${field}`] == fieldComment);
	document.getElementById("drop field comment").disabled = !(`${box}.${field}` in field2comment);	

	document.getElementById("add link").disabled = fromBoxCombo.selectedIndex == -1 ||
													toBoxCombo.selectedIndex == -1;
}

setInterval("enable_disable()", 100);
