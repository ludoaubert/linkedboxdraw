import sample_diagdata from "./diagdata.json" with {type: "json"};

import {mycontexts, contexts, resetContexts, setContexts, drawDiag, compute_links, ApplyRepartition, enforce_bounding_rectangle, data2contexts} from "./diagload.js";
import {download} from "./iocomponent.js";
import {getFileData} from "./iocomponent.js";
import {compute_box_rectangle} from "./compute_box_rectangles.js"

export {init, mydata, data, resetData, setData, displayCurrent, createMutationObserver};

var mydata = sample_diagdata;

function setData(mydata_)
{
	mydata = mydata_;
	data = JSON.stringify(mydata);
}

var data=null;
function resetData()
{
	data=null;
}

var currentBoxIndex = -1;
var currentFieldIndex = -1;

var currentFromBoxIndex = -1;
var currentFromFieldIndex = -1;

var currentToBoxIndex = -1;
var currentToFieldIndex = -1;

var currentColorBoxIndex = -1;
var currentColorFieldIndex = -1;

var currentPictureIndex = -1;

var input ;
var editTitle ;
var newDiagramButton ;
var boxCombo ;
var addBoxButton ;
var dropBoxButton ;
var updateBoxButton ;
var fieldCombo ;
var addFieldButton ;
var dropFieldButton ;
var updateFieldButton ;
var addPicToBox2Button ;
var valueCombo ;
var editValueButton;
var addValueButton;
var dropValueButton;
var updateValueButton;
var boxCommentTextArea ;
var updateBoxCommentButton ;
var dropBoxCommentButton ;
var fieldCommentTextArea ;
var updateFieldCommentButton;
var dropFieldCommentButton;
var isPrimaryKeyCheckBox ;
var isForeignKeyCheckBox ;
var linkCombo ;
var dropLinkButton ;
var addLinkButton ;
var updateLinkButton ;
var newBoxEditField ;
var newFieldEditField ;
var fromBoxCombo ;
var fromFieldCombo ;
var fromCardinalityCombo ;
var toBoxCombo ;
var toFieldCombo ;
var toCardinalityCombo ;
var categoryCombo ;
var newValueEditField ;
var colorBoxCombo ;
var colorFieldCombo ;
var colorCombo ;
var colorsCombo ;
var dropColorButton;
var addColorButton;
var updateColorButton;
var picturesCombo ;
var pictureZoom ;
var currentImageDisplay ;
var applyRepartitionButton;


function newDiagram() {

	mydata={documentTitle:"", boxes:[], values:[], boxComments:[], fieldComments:[], links:[], fieldColors:[], pictures:[]};
	const mycontexts_={
		contexts:[{frame:{left:0,right:1197,top:0,bottom:507}, translatedBoxes:[], links:[]}],
		rectangles:[]
	};
	
	setContexts(mycontexts_);

	currentBoxIndex = -1;
	currentFieldIndex = -1;

	currentFromBoxIndex = -1;
	currentFromFieldIndex = -1;

	currentToBoxIndex = -1;
	currentToFieldIndex = -1;

	currentColorBoxIndex = -1;
	currentColorFieldIndex = -1;

	currentPictureIndex = -1;
}

function addPicture(base64)
{
	currentPictureIndex = mydata.pictures.length;

	const name = document.getElementById("add_pic").value
						.replace(/\\/g, '/')
						.replace("C:/fakepath/", "");
	const pic = {name, base64, width:currentImageDisplay.width, height:currentImageDisplay.height, zoomPercentage:null};
	mydata.pictures.push(pic);

	const pictureComboInnerHTML = mydata.pictures
				.map(pic => `<option>${pic.name}</option>`)
				.join('');

	document.getElementById("pictures").innerHTML = pictureComboInnerHTML;
	document.getElementById("pictures").value = name;	
}

function loadPicture(blob)
{	
	return new Promise((resolve) => {
		const base64 = btoa(blob);
		//const base64 = Buffer.from(blob, 'binary').toString('base64');
		currentImageDisplay.onload = () => {
			resolve(base64);
		};
		currentImageDisplay.src = "data:image/jpg;base64, " + base64;
	});
}

function init() {

	editTitle = document.getElementById("title");
	newDiagramButton = document.getElementById("new diagram");
	boxCombo = document.getElementById("boxes");
	addBoxButton = document.getElementById("add box");
	dropBoxButton = document.getElementById("drop box");
	updateBoxButton = document.getElementById("update box");
	fieldCombo = document.getElementById("fields");
	addFieldButton = document.getElementById("add field");
	dropFieldButton = document.getElementById("drop field");
	updateFieldButton = document.getElementById("update field");;
	addPicToBox2Button = document.getElementById("add pic to box 2");;
	valueCombo = document.getElementById("values");
	editValueButton = document.getElementById("edit value");
	addValueButton = document.getElementById("add value");
	dropValueButton = document.getElementById("drop value");
	updateValueButton = document.getElementById("update value");
	boxCommentTextArea = document.getElementById("box comment");
	updateBoxCommentButton = document.getElementById("update box comment");
	dropBoxCommentButton = document.getElementById("drop box comment");
	fieldCommentTextArea = document.getElementById("field comment");
	updateFieldCommentButton = document.getElementById("update field comment");
	dropFieldCommentButton = document.getElementById("drop field comment");
	isPrimaryKeyCheckBox = document.getElementById("PK");
	isForeignKeyCheckBox = document.getElementById("FK");
	linkCombo = document.getElementById("links");
	dropLinkButton = document.getElementById("drop link");
	addLinkButton = document.getElementById("add link");
	newBoxEditField = document.getElementById("new box");
	newFieldEditField = document.getElementById("new field");
	fromBoxCombo = document.getElementById("from boxes");
	fromFieldCombo = document.getElementById("from fields");
	fromCardinalityCombo = document.getElementById("from cardinality");
	toBoxCombo = document.getElementById("to boxes");
	toFieldCombo = document.getElementById("to fields");
	toCardinalityCombo = document.getElementById("to cardinality");
	categoryCombo = document.getElementById("category");
	newValueEditField = document.getElementById("new value");
	colorBoxCombo = document.getElementById("color boxes");
	colorFieldCombo = document.getElementById("color fields");
	colorCombo = document.getElementById("color");
	colorsCombo = document.getElementById("colors");
	dropColorButton = document.getElementById("drop color");
	addColorButton = document.getElementById("add color");
	updateColorButton = document.getElementById("update color");
	picturesCombo = document.getElementById("pictures");
	pictureZoom = document.getElementById("pic_zoom");
	currentImageDisplay = document.getElementById("cid");
	applyRepartitionButton = document.getElementById("apply repartition");

	const innerHTML = ["","0","1","n","0,1","0,n","1,n"].map(c => '<option>' + c + '</option>')
							.join('');
	fromCardinalityCombo.innerHTML = innerHTML;
	toCardinalityCombo.innerHTML = innerHTML;

	document.querySelectorAll("button.collapsible")
			.forEach(button => {
				button.addEventListener("click", (event) => switchCollapsible(button));
			});
	
	let fi = document.querySelector("input[id=fi]");

	fi.addEventListener("change", (event) => {
		getFileData(fi).then(function(result){
			const json = JSON.parse(result);
			if ("data" in json && "contexts" in json)
			{
				setData(json.data);
				setContexts(json.contexts);
				drawDiag();
				currentBoxIndex = -1;
				displayCurrent();
			}
			else if ("documentTitle" in json && "boxes" in json && "links" in json)
			{
				setData(json);
				setContexts(data2contexts(mydata));
				drawDiag();
				currentBoxIndex = -1;
				displayCurrent();
			}
	})});

	let fo = document.querySelector("input[id=fo]");
	fo.addEventListener("click", () => download(fo.previousElementSibling.value, {data:mydata, contexts:mycontexts}));
	

	picturesCombo.addEventListener("change", () => {currentPictureIndex = -1; displayCurrent();});
	pictureZoom.addEventListener("change", () => {
			mydata.pictures[currentPictureIndex].zoomPercentage = isNaN(pictureZoom.valueAsNumber) ? null : pictureZoom.valueAsNumber;
			drawDiag();
		}
	);
	let add_pic = document.querySelector("input[id=add_pic]");
	add_pic.addEventListener("change", () => getFileData(add_pic).then(loadPicture).then(addPicture));
	let drop_pic = document.querySelector("button[id=drop_pic]");
	drop_pic.addEventListener("click", () => dropPicture());
	let add_pic_to_box = document.querySelector("button[id=add_pic_to_box]");
	add_pic_to_box.addEventListener("click", () => addSelectedPictureToSelectedBox());

	editTitle.addEventListener("change", () => updateTitle());
	newDiagramButton.addEventListener("click", () => {newDiagram(); displayCurrent(); drawDiag();});
	boxCombo.addEventListener("change", () => {currentBoxIndex = -1; displayCurrent();});
	addBoxButton.addEventListener("click", () => addNewBox());
	dropBoxButton.addEventListener("click", () => dropBox());
	updateBoxButton.addEventListener("click", () => updateBox());
	updateBoxCommentButton.addEventListener("click", () => updateBoxComment()) ;
	dropBoxCommentButton.addEventListener("click", () => dropBoxComment()) ;
	fieldCombo.addEventListener("change", () => {currentFieldIndex = -1; displayCurrent();});
	addFieldButton.addEventListener("click", () => addNewFieldToBox()) ;
	dropFieldButton.addEventListener("click", () => dropFieldFromBox()) ;
	updateFieldButton.addEventListener("click", () => updateField()) ;
	addPicToBox2Button.addEventListener("click", () => addSelectedPictureToSelectedBox()) ;
	updateFieldCommentButton.addEventListener("click", () => updateFieldComment());
	dropFieldCommentButton.addEventListener("click", () => dropFieldComment());
	valueCombo.addEventListener("change", () => updateValueAttributes());
	editValueButton.addEventListener("click", () => editValueFromField());
	addValueButton.addEventListener("click", () => addNewValueToField());
	dropValueButton.addEventListener("click", () => dropValueFromField());
	updateValueButton.addEventListener("click", () => updateValue());
	linkCombo.addEventListener("click", () => linkComboOnClick());
	dropLinkButton.addEventListener("click", () => {linkComboOnClick(); dropLink();});
	fromBoxCombo.addEventListener("change", () => {currentFromBoxIndex = -1; displayCurrent();});
	fromFieldCombo.addEventListener("change", () => {currentFromFieldIndex = -1; displayCurrent();});
	toBoxCombo.addEventListener("change", () => {currentToBoxIndex = -1; displayCurrent();});
	toFieldCombo.addEventListener("change", () => {currentToFieldIndex = -1; displayCurrent();});
	addLinkButton.addEventListener("click", () => addNewLink()) ;
	colorsCombo.addEventListener("click", () => colorsComboOnClick());
	dropColorButton.addEventListener("click", () => dropColor());
	colorBoxCombo.addEventListener("change", () => {currentColorBoxIndex = -1; displayCurrent();});
	colorFieldCombo.addEventListener("change", () => {currentColorFieldIndex = -1; displayCurrent();});
	addColorButton.addEventListener("click", () => addNewColor());
	updateColorButton.addEventListener("click", () => updateColor());
	applyRepartitionButton.addEventListener("click", async () => {await ApplyRepartition(); drawDiag();});
	newFieldEditField.addEventListener("keypress", () => {onNewFieldUpdate();});
	newFieldEditField.addEventListener("paste", () => {onNewFieldUpdate();});

//avoid duplicate entries
	newBoxEditField.addEventListener("change", () => {
		addBoxButton.disabled = (newBoxEditField.value == '' || mydata.boxes.find(box => box.title == newBoxEditField.value)) ? true : false;
	});
	newFieldEditField.addEventListener("change", () => {
		currentBoxIndex = mydata.boxes.findIndex(box => box.title == boxCombo.value);
		if (currentBoxIndex == -1)
			addFieldButton.disabled = true;
		else
			addFieldButton.disabled = (newFieldEditField.value == '' || mydata.boxes[currentBoxIndex].fields.find(f => f.name == newFieldEditField.value)) ? true : false;
	});

	const colors=['yellow','pink','hotpink','palegreen','red','orange','skyblue','olive','grey','darkviolet'];
	colorCombo.innerHTML = colors.map(color => '<option>' + color + '</option>')
				.join('');

	displayCurrent();
}


function createMutationObserver()
{
	var fnCallback = function (mutations)
	{
		mutations.forEach(function (mutation) {
			const tagName = mutation.target.parentElement.tagName;
			const id = mutation.target.parentElement.id;
			const data = mutation.target.data;
			if (tagName=="TD")
			{
				const regexpId = /b([0-9]+)f([0-9]+)/;
				const match = id.match(regexpId);
				console.log(`box: ${match[1]} / field: ${match[2]}.`);
				const boxIndex = parseInt(match[1]);
				const fieldIndex = parseInt(match[2]);
				mydata.boxes[boxIndex].fields[fieldIndex].name = data;
			}
			if (tagName=="TH")
			{
				const regexpId = /b([0-9]+)/;
				const match = id.match(regexpId);
				console.log(`box: ${match[1]}.`);
				const boxIndex = parseInt(match[1]);
				mydata.boxes[boxIndex].title = data;				
			}
		});
	};

	const elTarget = document.querySelector("div#diagram.content");

	var observer = new MutationObserver(fnCallback);

	const objConfig = {
		childList: false,
		subtree : true,
		attributes: true, 
		characterData : true,
		attributeFilter : ['style', 'id'],
		attributeOldValue : false
	};

    observer.observe(elTarget, objConfig);
}


function displayCurrent()
{
	if (editTitle.value != mydata.documentTitle)
		editTitle.value = mydata.documentTitle;

	let contexts = [
		{boxCombo_:boxCombo, fieldCombo_:fieldCombo, currentBoxIndex_:currentBoxIndex, currentFieldIndex_:currentFieldIndex},
		{boxCombo_:fromBoxCombo, fieldCombo_:fromFieldCombo, currentBoxIndex_:currentFromBoxIndex, currentFieldIndex_:currentFromFieldIndex},
		{boxCombo_:toBoxCombo, fieldCombo_:toFieldCombo, currentBoxIndex_:currentToBoxIndex, currentFieldIndex_:currentToFieldIndex},
		{boxCombo_:colorBoxCombo, fieldCombo_:colorFieldCombo, currentBoxIndex_:currentColorBoxIndex, currentFieldIndex_:currentColorFieldIndex},		
	];

	let index = 0;
	for (let {boxCombo_, fieldCombo_, currentBoxIndex_, currentFieldIndex_} of contexts)
	{
		if (currentBoxIndex_ == -1 && boxCombo_.value != "")
			currentBoxIndex_ = mydata.boxes.findIndex(box => box.title == boxCombo_.value);

		const boxComboInnerHTML = mydata.boxes
								.concat()	//shallow copy
								.sort((a, b) => a.title.localeCompare(b.title))
								.map(box => `<option>${box.title}</option>`)
								.join('');

		if (boxCombo_.innerHTML != boxComboInnerHTML)
		{
			boxCombo_.innerHTML = boxComboInnerHTML;
			if (currentBoxIndex_ == -1)
				currentBoxIndex_ = mydata.boxes.length > 0 ? 0 : -1;
		}

		boxCombo_.value = mydata.boxes[currentBoxIndex_]?.title || "";

		const fieldComboInnerHTML = mydata.boxes[currentBoxIndex_]
									?.fields
									?.concat() //shallow copy
									?.sort((a, b) => a.name.localeCompare(b.name))
									?.map(field => "<option>" + field.name + "</option>")
									?.join('') || "";

		if (fieldCombo_.innerHTML != fieldComboInnerHTML)
		{
			fieldCombo_.innerHTML = fieldComboInnerHTML;
			if (currentFieldIndex_ == -1)
				currentFieldIndex_ = mydata.boxes[currentBoxIndex_]?.fields?.length > 0 ? 0 : -1;
		}

		currentFieldIndex_ = mydata.boxes[currentBoxIndex_]?.fields?.findIndex(field => field.name == fieldCombo_.value) || -1;

		contexts[index] = {boxCombo_, fieldCombo_, currentBoxIndex_, currentFieldIndex_};
		index++;
	}

	currentBoxIndex = contexts[0].currentBoxIndex_;
	currentFieldIndex = contexts[0].currentFieldIndex_;
	currentFromBoxIndex = contexts[1].currentBoxIndex_;
	currentFromFieldIndex = contexts[1].currentFieldIndex_;
	currentToBoxIndex = contexts[2].currentBoxIndex_;
	currentToFieldIndex = contexts[2].currentFieldIndex_;
	currentColorBoxIndex = contexts[3].currentBoxIndex_;
	currentColorFieldIndex = contexts[3].currentFieldIndex_;

	const isPrimaryKey = mydata.boxes[currentBoxIndex]?.fields[currentFieldIndex]?.isPrimaryKey;
	const isForeignKey = mydata.boxes[currentBoxIndex]?.fields[currentFieldIndex]?.isForeignKey;
	isPrimaryKeyCheckBox.checked = isPrimaryKey || false;
	isForeignKeyCheckBox.checked = isForeignKey || false;

	const boxTitle = mydata.boxes[currentBoxIndex]?.title;
	const fieldName = mydata.boxes[currentBoxIndex]?.fields[currentFieldIndex]?.name;

	const valueComboInnerHTML = mydata.values.filter(({box, field, value}) => box == boxTitle && field == fieldName)
										.map(({box, field, value}) => value)
										.sort()
										.map(value => `<option>${value}</option>`)
										.join('');

	if (valueCombo.innerHTML != valueComboInnerHTML)
		valueCombo.innerHTML = valueComboInnerHTML;

	const currentBoxCommentIndex = mydata.boxComments.findIndex(({box, comment}) => box == boxCombo.value);

	const boxComment = mydata.boxComments[currentBoxCommentIndex]?.comment || "" ;
	const reversedBoxComment = reverseJsonSafe(boxComment);
	if (reversedBoxComment != boxCommentTextArea.value)
	{
		boxCommentTextArea.value = reversedBoxComment ;
	}

	const currentFieldCommentIndex = mydata.fieldComments.findIndex(({box, field, comment}) => box == boxCombo.value && field == fieldCombo.value);

	const fieldComment = mydata.fieldComments[currentFieldCommentIndex]?.comment || "" ;
	const reversedFieldComment = reverseJsonSafe(fieldComment);

	if (reversedFieldComment != fieldCommentTextArea.value)
	{
		fieldCommentTextArea.value = reversedFieldComment ;
	}

	const picturesComboInnerHTML = mydata?.pictures
									?.map(pic => `<option>${pic.name}</option>`)
									?.join('') || "";

	if (picturesCombo.innerHTML != picturesComboInnerHTML)
	{
		picturesCombo.innerHTML = picturesComboInnerHTML;
	}

	currentPictureIndex = picturesCombo.selectedIndex;

	displaySelectedPicture();

}


function updateTitle()
{
	mydata.documentTitle=editTitle.value;
}


function addNewBox()
{
	currentBoxIndex = mydata.boxes.length;
	currentFieldIndex = -1;

	const box = {title:newBoxEditField.value, id:currentBoxIndex, fields:[]};
	mydata.boxes.push(box);

	newBoxEditField.value = "";

	displayCurrent();

	const rec = compute_box_rectangle(box);
	const id = mycontexts.rectangles.length;
	mycontexts.rectangles.push(rec);
	mycontexts.contexts[0]?.translatedBoxes?.push({id, translation:{x:0,y:0}});

	drawDiag();
}


function dropBox()
{
	console.log('dropBox');
	currentBoxIndex = mydata.boxes.findIndex(box => box.title == boxCombo.value);
	console.log(currentBoxIndex);

	mydata.boxes = mydata.boxes.filter(box => box.title != boxCombo.value);
	mydata.links = mydata.links.filter(lk => lk.from != currentBoxIndex && lk.to != currentBoxIndex);
	mydata.values = mydata.values.filter(({box, field, value}) => box != boxCombo.value);
	mydata.boxComments = mydata.boxComments.filter(({box, comment}) => box != boxCombo.value);
	mydata.fieldComments = mydata.fieldComments.filter(({box, field, comment}) => box != boxCombo.value);
	mydata.fieldColors = mydata.fieldColors.filter(({box, field, color}) => box != boxCombo.value);

	for (let box of mydata.boxes)
	{
		if (box.id > currentBoxIndex)
			box.id--;
	}

	for (let lk of mydata.links)
	{
		if (lk.from > currentBoxIndex)
			lk.from--;
		if (lk.to > currentBoxIndex)
			lk.to--;
	}

	for (let fc of mydata.fieldColors)
	{
		if (fc.index > currentBoxIndex)
			fc.index--;
	}

	mycontexts.rectangles.splice(currentBoxIndex, 1);
	for (let context of mycontexts.contexts)
	{
		context.translatedBoxes = context.translatedBoxes.filter(tB => tB.id != currentBoxIndex)
														.map(({id, translation}) => ({id: id > currentBoxIndex ? id-1 : id, translation}));

		context.links = context.links.filter(({polyline, from, to}) => from != currentBoxIndex && to != currentBoxIndex)
									.map(({polyline, from, to}) => ({polyline, from: from > currentBoxIndex ? from-1 : from, to: to > currentBoxIndex ? to-1 : to}));
	}

	currentBoxIndex = -1;

	displayCurrent();
	drawDiag();
}


function updateBox()
{
	currentBoxIndex = mydata.boxes.findIndex(box => box.title == boxCombo.value);
	mydata.boxes[currentBoxIndex].title = newBoxEditField.value;
	
	mydata.values = mydata.values.map( ({box, field, value}) => ({box: box == boxCombo.value ? newBoxEditField.value : box, field, value}) );
	mydata.boxComments = mydata.boxComments.map( ({box, comment}) => ({box: box == boxCombo.value ? newBoxEditField.value : box, comment}) );
	mydata.fieldComments = mydata.fieldComments.map( ({box, field, comment}) => ({box: box == boxCombo.value ? newBoxEditField.value : box, field, comment}) );
	mydata.fieldColors = mydata.fieldColors.map( ({box, field, color}) => ({box: box == boxCombo.value ? newBoxEditField.value : box, field,color}) );
	
	displayCurrent();

    const rec = compute_box_rectangle(mydata.boxes[currentBoxIndex]);
    mycontexts.rectangles[currentBoxIndex] = rec;

	drawDiag();
}


function updateFieldAttributes()
{

}


function onNewFieldUpdate()
{
	if (newFieldEditField.value.length == 0)
	{
		isPrimaryKeyCheckBox.checked = false;
		isForeignKeyCheckBox.checked = false;
	}
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

	const rec = compute_box_rectangle(mydata.boxes[currentBoxIndex]);
	mycontexts.rectangles[currentBoxIndex] = rec;

	drawDiag();
}

function updateField()
{
	currentBoxIndex = mydata.boxes.findIndex(box => box.title == boxCombo.value);
	currentFieldIndex = mydata.boxes[currentBoxIndex].fields.findIndex(field => field.name == fieldCombo.value);

	mydata.boxes[currentBoxIndex].fields[currentFieldIndex] = {
		name: fieldCombo.value,
		isPrimaryKey: isPrimaryKeyCheckBox.checked,
		isForeignKey: isForeignKeyCheckBox.checked
	} ;
	
	mydata.values = mydata.values.map( ({box, field, value}) => ({box, field: box == boxCombo.value && field == fieldCombo.value ? newFieldEditField.value : field, value}) );
	mydata.fieldComments = mydata.fieldComments.map( ({box, field, comment}) => ({box, field: box == boxCombo.value && field == fieldCombo.value ? newFieldEditField.value : field, comment}) );
	mydata.fieldColors = mydata.fieldColors.map( ({box, field, color}) => ({box, field: box == boxCombo.value && field == fieldCombo.value ? newFieldEditField.value : field, color}) );

	displayCurrent();

	const rec = compute_box_rectangle(mydata.boxes[currentBoxIndex]);
	mycontexts.rectangles[currentBoxIndex] = rec;

	drawDiag();
}


async function dropFieldFromBox()
{
	currentBoxIndex = mydata.boxes.findIndex(box => box.title == boxCombo.value);

	var fields = mydata.boxes[currentBoxIndex].fields ;
	currentFieldIndex = fields.findIndex(field => field.name == fieldCombo.value);
	mydata.boxes[currentBoxIndex].fields = fields.filter(field => field.name != fieldCombo.value);


	mydata.links = mydata.links.filter(({from, fromField, to, toField}) => !(from==currentBoxIndex && fromField==currentFieldIndex) && !(to==currentBoxIndex && toField==currentFieldIndex));
	mydata.values = mydata.values.filter(({box, field, value}) => !(box == boxCombo.value && field == fieldCombo.value));
	mydata.fieldComments = mydata.fieldComments.filter(({box, field, comment}) => !(box == boxCombo.value && field == fieldCombo.value));
	mydata.fieldColors = mydata.fieldColors.filter(({box, field, color}) => !(box == boxCombo.value && field == fieldCombo.value));

	for (let lk of mydata.links)
	{
		if (lk.from == currentBoxIndex && lk.fromField > currentFieldIndex)
			lk.fromField--;
		if (lk.to == currentBoxIndex && lk.toField > currentFieldIndex)
			lk.toField--;
	}

	currentFieldIndex = -1;

	displayCurrent();

    const rec = compute_box_rectangle(mydata.boxes[currentBoxIndex]);
    mycontexts.rectangles[currentBoxIndex] = rec;

	const selectedContextIndex = mycontexts.contexts
					.map(({frame, translatedBoxes, links}) => translatedBoxes.map(({id, translation}) => id))
					.findIndex(ids => ids.includes(currentBoxIndex) );
	mycontexts.contexts[selectedContextIndex].links = await compute_links(selectedContextIndex);
	
	enforce_bounding_rectangle(selectedContextIndex);

	drawDiag();
}

function editValueFromField()
{

}

function addNewValueToField()
{
	mydata.values.push({
		box: boxCombo.value,
		field: fieldCombo.value,
		value: newValueEditField.value
	});

	newValueEditField.value = "";

	displayCurrent();
}

function updateValue()
{
	const currentValueIndex = mydata.values.findIndex(({box, field, value}) => box ==  boxCombo.value && field == fieldCombo.value);
	mydata.values[ currentValueIndex ] = {box, field, value: newValueEditField.value};

	displayCurrent();
}

function dropValueFromField()
{
	mydata.values = mydata.values.filter(({box, field, value}) => !(box ==  boxCombo.value && field == fieldCombo.value && value == valueCombo.value));

	displayCurrent();
}

function selectLink()
{

}

function produce_options(links)
{
	const options = links.map((lk, position) => {
			const {from, fromField, to, toField} = lk;
			const fromBox = mydata.boxes[from] ;
			const fromFieldName = fromField != -1 ? fromBox.fields[fromField].name : "";
			const toBox = mydata.boxes[to] ;
			const toFieldName = toField != -1 ? toBox.fields[toField].name : "";
			return {option:`${fromBox.title}.${fromFieldName} ${toBox.title}.${toFieldName}`, position};
		})
		.sort((a, b) => a.option<b.option ? -1 : a.option > b.option ? 1 : 0);

	return options;
}

function linkComboOnClick()
{
	const options = produce_options(mydata.links)
	
	const innerHTML = options
				.map(({option, position}) => option) 
				.map(option => `<option>${option}</option>`)
				.join('');

	if (linkCombo.innerHTML != innerHTML)
		linkCombo.innerHTML = innerHTML;
}


async function addNewLink()
{
	currentFromBoxIndex = mydata.boxes.findIndex(box => box.title == fromBoxCombo.value);
	currentFromFieldIndex = mydata.boxes[currentFromBoxIndex].fields.findIndex(field => field.name == fromFieldCombo.value);
	currentToBoxIndex = mydata.boxes.findIndex(box => box.title == toBoxCombo.value);
	currentToFieldIndex = mydata.boxes[currentToBoxIndex].fields.findIndex(field => field.name == toFieldCombo.value);

	const lk = {
		from: currentFromBoxIndex,
		fromField: currentFromFieldIndex,
		fromCardinality: fromCardinalityCombo.value,
		to: currentToBoxIndex,
		toField: currentToFieldIndex,
		toCardinality: toCardinalityCombo.value,
		category:categoryCombo.value
	};

	mydata.links.push(lk);

	for (let [selectedContextIndex, context] of mycontexts.contexts.entries())
	{
		let {translatedBoxes, links} = context ;
		const ids = translatedBoxes.map(({id,translation}) => id);
		if (ids.includes(lk.from) && ids.includes(lk.to))
		{
			links.push({polyline:[], from:lk.from, to:lk.to}); 
			context.links = await compute_links(selectedContextIndex);
		}
	}

	drawDiag();
}

async function dropLink()
{
	const {option, position} = produce_options(mydata.links)[linkCombo.selectedIndex];
	
	const lk = mydata.links[ position ];
	console.log({lk});
	mydata.links = mydata.links.filter((_, index) => index != position);
	linkComboOnClick();

	for (let [selectedContextIndex, context] of mycontexts.contexts.entries())
	{
		let {translatedBoxes, links} = context ;
		const ids = translatedBoxes.map(({id,translation}) => id);
		if (ids.includes(lk.from) && ids.includes(lk.to))
		{
			context.links = context.links.filter(link => !(link.to==lk.to && link.from==lk.from));
			context.links = await compute_links(selectedContextIndex);
		}
	}

	drawDiag();
}



function dropBoxComment()
{
	const currentCommentIndex = mydata.boxComments.findIndex(({box, comment}) => box == boxCombo.value);
	mydata.boxComments = mydata.boxComments.filter((_, index) => index != currentCommentIndex );
	displayCurrent();
	drawDiag();
}

function updateBoxComment()
{
	const currentBoxCommentIndex = mydata.boxComments.findIndex(({box, comment}) => box == boxCombo.value);
	const boxComment = {box: boxCombo.value, comment: jsonSafe(boxCommentTextArea.value)} ;

	if (currentBoxCommentIndex != -1)
		mydata.boxComments[ currentBoxCommentIndex ] = boxComment;
	else
		mydata.boxComments.push(boxComment);

	displayCurrent();
	drawDiag();
}

function dropFieldComment()
{
	const currentFieldCommentIndex = mydata.fieldComments.findIndex(({box, field, comment}) => box == boxCombo.value && field == fieldCombo.value);
	mydata.fieldComments = mydata.fieldComments.filter((_, index) => index != currentFieldCommentIndex );
	displayCurrent();
	drawDiag();
}

function jsonSafe(text)
{
	return text.replace(/\\n/g, "\\n")
			  .replace(/\\'/g, "\\'")
			  .replace(/\\"/g, '\\"')
			  .replace(/\\&/g, "\\&")
			  .replace(/\\r/g, "\\r")
			  .replace(/\\t/g, "\\t")
			  .replace(/\\b/g, "\\b")
			  .replace(/\\f/g, "\\f");
}

function reverseJsonSafe(text)
{
	return text.replaceAll('\\n', '\n')
				.replaceAll("\\'", "'")
				.replaceAll('\\"', '"')
				.replaceAll("\\&", '\&')
				.replaceAll("\\r", '\r')
				.replaceAll("\\t", '\t')
				.replaceAll("\\b", '\b')
				.replaceAll("\\f", '\f');
}

function updateFieldComment()
{
	const currentFieldCommentIndex = mydata.fieldComments.findIndex(({box, field, comment}) => box == boxCombo.value && field == fieldCombo.value);
	const fieldComment = {box: boxCombo.value, field: fieldCombo.value, comment: jsonSafe(fieldCommentTextArea.value)};

	if (currentFieldCommentIndex != -1)
		mydata.fieldComments[ currentBoxCommentIndex ] = fieldComment;
	else
		mydata.fieldComments.push(fieldComment);

	displayCurrent();
	drawDiag();
}

function colorsComboOnClick()
{
	console.log("colorsComboOnClick");
	const innerHTML = mydata.fieldColors
				.map(({index, box, field, color}) => `<option>${box}.${field}.${color}</option>`)
				.join('');
	console.log(innerHTML);

	if (colorsCombo.innerHTML != innerHTML)
		colorsCombo.innerHTML = innerHTML;
}


function addNewColor()
{
	currentColorBoxIndex = mydata.boxes.findIndex(box => box.title == colorBoxCombo.value);
	const box = mydata.boxes[currentColorBoxIndex];

	currentColorFieldIndex = box.fields.findIndex(field => field.name == colorFieldCombo.value);
	const field = box.fields[currentColorFieldIndex];

	mydata.fieldColors.push({
		index: currentColorBoxIndex,
		box: box.title,
		field: field.name,
		color: colorCombo.value
	})

	console.log(mydata.fieldColors);
	colorsComboOnClick();

	drawDiag();
}

function updateColor()
{
	const fieldColorIndex = mydata.fieldColors.findIndex(({box, field, color}) => box == boxCombo.value && field == fieldCombo.value);
	const fc = mydata.fieldColors[fieldColorIndex];
	mydata.fieldColors[ fieldColorIndex ] = {
		index: fc.index,
		box: boxCombo.value,
		field: fieldCombo.value,
		color: colorCombo.value
	};
	console.log(mydata.fieldColors);
	colorsComboOnClick();
	drawDiag();
}

function dropColor()
{
	console.log(mydata.fieldColors);
	mydata.fieldColors = mydata.fieldColors.filter((_, index) => index != colorsCombo.selectedIndex );
	console.log(mydata.fieldColors);
	colorsComboOnClick();
	drawDiag();
}


function dropPicture()
{
	currentPictureIndex = picturesCombo.selectedIndex;
	mydata.pictures.splice(currentPictureIndex, 1);
	if (currentPictureIndex >= mydata.pictures.length)
		currentPictureIndex--;
	const pictureComboInnerHTML = mydata.pictures
					.map(pic => `<option>${pic.name}</option>"`)
					.join('');

	picturesCombo.innerHTML = pictureComboInnerHTML;
	const base64 = mydata?.pictures?.[currentPictureIndex]?.base64 ;
	currentImageDisplay.src = base64==undefined ? "data:text/plain," : "data:image/jpg;base64, " + base64 ;
}

function displaySelectedPicture()
{
	currentPictureIndex = picturesCombo.selectedIndex ;
	const base64 = mydata?.pictures?.[currentPictureIndex]?.base64 ;
	currentImageDisplay.src = base64==undefined ? "data:text/plain," : "data:image/jpg;base64, " + base64 ;
}


function addSelectedPictureToSelectedBox()
{
	currentPictureIndex = picturesCombo.selectedIndex;
	mydata.boxes[currentBoxIndex].fields.push(
		{
			name: mydata.pictures[currentPictureIndex].name,
			isPrimaryKey: false,
			isForeignKey: false,
			type:"image"
		}
	);

	displayCurrent();

	const rec = compute_box_rectangle(mydata.boxes[currentBoxIndex]);
	mycontexts.rectangles[currentBoxIndex] = rec;

	drawDiag();
}
