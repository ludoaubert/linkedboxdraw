// FRAME_MARGIN is duplicated in diagload.js
const FRAME_MARGIN = 20;

var mydata={documentTitle:"", boxes:[], values:[], boxComments:[], fieldComments:[], links:[]};
var currentBoxIndex = -1;
var currentFieldIndex = -1;


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

//sorting a combo
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

//empty a combo
function removeOptions(selElem) 
{
    while (selElem.options.length) 
	{
        selElem.remove(0);
    }
}

//copy combo content into another combo
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



function displayCurrent()
{
	if (currentBoxIndex == -1 && boxCombo.value != "")
		currentBoxIndex = mydata.boxes.findIndex(box => box.title == boxCombo.value);
	
	const innerHTML = mydata.boxes
							.sort((a, b) => a.title < b.title)
							.map(box => "<option>" + box.title + "</option>")
							.join('');
	
	console.log(innerHTML);
							
	if (boxCombo.innerHTML != innerHTML)
		boxCombo.innerHTML = innerHTML;
	
	if (fromBoxCombo.innerHTML != innerHTML)
		fromBoxCombo.innerHTML = innerHTML;	
	
	if (toBoxCombo.innerHTML != innerHTML)
		toBoxCombo.innerHTML = innerHTML;
	
	console.log(currentBoxIndex);
	if (currentBoxIndex != -1)
	{
		const {title, id, fields} = mydata.boxes[currentBoxIndex];
		boxCombo.value = title;
		
		const innerHTML = mydata.boxes[currentBoxIndex]
								.fields
								.sort((a, b) => a.name < b.name)
								.map(field => "<option>" + field.name + "</option>")
								.join('');
								
		if (fieldCombo.innerHTML != innerHTML)
			fieldCombo.innerHTML = innerHTML;
	}
	else
	{
		boxCombo.value = "";
	}
	
	if (currentBoxIndex != -1 && currentFieldIndex = -1 && fieldCombo.value != "")
	{
		currentFieldIndex = mydata.boxes[currentBoxIndex].fields.findIndex(field => field.name == fieldCombo.value);
		const {name, isPrimaryKey, isForeignKey} = mydata.boxes[currentBoxIndex].fields[currentFieldIndex];
		newFieldEditField.value = name;
		isPrimaryKeyCheckBox.checked = isPrimaryKey; 
		isForeignKeyCheckBox.checked = isForeignKey;
	}
}


function addNewBox()
{
	currentBoxIndex = mydata.boxes.length;
	currentFieldIndex = -1;
	
	mydata.boxes.push({title:newBoxEditField.value, id:currentBoxIndex, fields:[]});
	console.log(mydata.boxes);
	
	newBoxEditField.value = "";
	
	displayCurrent();
}

function selectBox(name)
{
	console.log(name);
	currentBoxIndex = mydata.boxes.findIndex(box => box.title==name);
	displayCurrent();
}

function dropBox()
{
	console.log('dropBox');
	currentBoxIndex = mydata.boxes.findIndex(box => box.title == document.getElementById("boxes").value);
	console.log(currentBoxIndex);
	
	mydata.boxes = mydata.boxes.filter(box => box.title != document.getElementById("boxes").value);
	mydata.links = mydata.links.filter(lk => lk.from != currentBoxIndex && lk.to != currentBoxIndex);
	
	for (let box of mydata.boxes)
	{
		box.id = box.id > currentBoxIndex ? box.id - 1 : box.id;
	}
	
	for (let lk of mydata.links)
	{
		lk.from = lk.from > currentBoxIndex ? lk.from - 1 : lk.from;
		lk.to = lk.to > currentBoxIndex ? lk.to - 1 : lk.to;
	}
	
	console.log(mydata);
	if (currentBoxIndex == mydata.boxes.length)
		currentBoxIndex = -1;
	displayCurrent();
}


function updateBox()
{

}


function updateFieldAttributes()
{	

}

function selectBox(boxCombo, fieldCombo)
{

}


function selectField()
{

}

function addNewFieldToBox()
{
	currentBoxIndex = mydata.boxes.findIndex(box => box.title == boxCombo.value);
	currentFieldIndex = mydata.boxes[currentBoxIndex].fields.length;
	
	mydata.boxes[currentBoxIndex].fields.push({
			name: newFieldEditField.value,
			isPrimaryKey: isPrimaryKeyCheckBox.checked, 
			isForeignKey: isForeignKeyCheckBox.checked
		}
	);
	console.log(mydata.boxes[currentBoxIndex].fields);
	
	newFieldEditField.value = "";
	isPrimaryKeyCheckBox.checked = false;
	isForeignKeyCheckBox.checked = false;
	
	displayCurrent();
}

function updateField()
{

}


function dropFieldFromBox()
{

}

function editValueFromField()
{

}

function addNewValueToField()
{

}

function updateValue()
{

}

function dropValueFromField()
{

}

function selectLink()
{

}

function linkComboOnClick()
{
	console.log("linkComboOnClick");
	const innerHTML = mydata.links
							.map(lk => "<option>" + mydata.boxes[lk.from].title + " => " + mydata.boxes[lk.to].title + "</option>")
							.join('');
	
	console.log(innerHTML);
							
	if (document.getElementById("links").innerHTML != innerHTML)
		document.getElementById("links").innerHTML = innerHTML;
}

function updateLink()
{
	const text = `${fromBoxCombo.value}.${fromFieldCombo.value}.${fromCardinalityCombo.value} \-> ${toBoxCombo.value}.${toFieldCombo.value}.${toCardinalityCombo.value}`;
	linkCombo.options[linkCombo.selectedIndex].innerHTML = text;
}

function addNewLink()
{
	const lk = {
		"from":mydata.boxes.find(box => box.title == document.getElementById("from boxes").value).id,
		"fromField":-1,
		"fromCardinality":"undefined",
		"to":mydata.boxes.find(box => box.title == document.getElementById("to boxes").value).id,
		"toField":-1,
		"toCardinality":"undefined",
		"category":""
	};
	
	console.log(lk);
	
	mydata.links.push(lk);
}

function dropLink()
{
	console.log(mydata.links);
	mydata.links = mydata.links.filter(lk => mydata.boxes[lk.from].title + " => " + mydata.boxes[lk.to].title != linkCombo.value);
	console.log(mydata.links);
	linkComboOnClick();
}



function dropBoxComment()
{

}

function updateBoxComment()
{

}

function dropFieldComment()
{

}

function updateFieldComment()
{

}

function addNewColor()
{

}

function updateColor()
{

}

function dropColor()
{

}




