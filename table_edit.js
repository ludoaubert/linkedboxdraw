
var currentBoxIndex = -1;
var currentFieldIndex = -1;

var currentFromBoxIndex = -1;
var currentFromFieldIndex = -1;

var currentToBoxIndex = -1;
var currentToFieldIndex = -1;

var currentColorBoxIndex = -1;
var currentColorFieldIndex = -1;


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
var categoryCombo ;
var newValueEditField ;
var colorBoxCombo ;
var colorFieldCombo ;
var colorCombo ;
var colorsCombo ;

function newDiagram() {

	mydata={documentTitle:"", boxes:[], values:[], boxComments:[], fieldComments:[], links:[], fieldColors:[]};
	mycontexts={contexts:[], rectangles:[]};

	currentBoxIndex = -1;
	currentFieldIndex = -1;

	currentFromBoxIndex = -1;
	currentFromFieldIndex = -1;

	currentToBoxIndex = -1;
	currentToFieldIndex = -1;

	currentColorBoxIndex = -1;
	currentColorFieldIndex = -1;
}


//TODO: refactor with compute_links(selectedContextIndex)

function download2(filename) {
	var element = document.createElement('a');

	const {boxes, links} = mydata;

	const rectangles = boxes.map(box => compute_box_rectangles(box));

	const hex = (i,n) => i.toString(16).padStart(n,'0');

	const rectdim = rectangles.map(r => hex(r.right-r.left,3)+hex(r.bottom-r.top,3));
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

	mycontexts = JSON.parse(jsonResponse);
	mycontexts.rectangles = rectangles;
	mycontexts.contexts = mycontexts.contexts.map(
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
			const links_ = links.filter(lk => lk.from != lk.to)
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

	download(filename, mycontexts);
}


function init() {

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
	categoryCombo = document.getElementById("category");
	newValueEditField = document.getElementById("new value");
	colorBoxCombo = document.getElementById("color boxes");
	colorFieldCombo = document.getElementById("color fields");
	colorCombo = document.getElementById("color");
	colorsCombo = document.getElementById("colors");

	const innerHTML = ["","0","1","n","0,1","0,n","1,n"].map(c => '<option>' + c + '</option>')
															.join('');
	fromCardinalityCombo.innerHTML = innerHTML;
	toCardinalityCombo.innerHTML = innerHTML;


	const colors=['yellow','pink','hotpink','palegreen','red','orange','skyblue','olive','grey','darkviolet'];
	colorCombo.innerHTML = colors.map(color => '<option>' + color + '</option>')
								.join('');
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
								.map(box => "<option>" + box.title + "</option>")
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
										.map(value => '<option>' + value + '</option>')
										.join('');

	if (valueCombo.innerHTML != valueComboInnerHTML)
		valueCombo.innerHTML = valueComboInnerHTML;

	const currentBoxCommentIndex = mydata.boxComments.findIndex(({box, comment}) => box == boxCombo.value);

	const boxComment = mydata.boxComments[currentBoxCommentIndex]?.comment || "" ;
	if (boxComment != boxCommentTextArea.value)
	{
		boxCommentTextArea.value = boxComment ;
	}

	const currentFieldCommentIndex = mydata.fieldComments.findIndex(({box, field, comment}) => box == boxCombo.value && field == fieldCombo.value);

	const fieldComment = mydata.fieldComments[currentFieldCommentIndex]?.comment || "" ;
	if (fieldComment != fieldCommentTextArea.value)
	{
		fieldCommentTextArea.value = fieldComment ;
	}
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

	currentBoxIndex = -1;

	displayCurrent();

        mycontexts.rectangles.splice(currentBoxIndex, 1);

        drawDiag();
}


function updateBox()
{
	currentBoxIndex = mydata.boxes.findIndex(box => box.title == boxCombo.value);
	mydata.boxes[currentBoxIndex].title = newBoxEditField.value;
	displayCurrent();

        const rec = compute_box_rectangle(mydata.boxes[currentBoxIndex]);
        mycontexts.rectangles[currentBoxIndex] = rec;

        drawDiag();
}


function updateFieldAttributes()
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

	displayCurrent();

        const rec = compute_box_rectangle(mydata.boxes[currentBoxIndex]);
        mycontexts.rectangles[currentBoxIndex] = rec;

        drawDiag();
}


function dropFieldFromBox()
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

function linkComboOnClick()
{
	const innerHTML = mydata.links
				.map(({from, fromField, to, toField}) => {
					const fromBox = mydata.boxes[from] ;
					const fromFieldName = fromField != -1 ? fromBox.fields[fromField].name : "";
					const toBox = mydata.boxes[to] ;
					const toFieldName = toField != -1 ? toBox.fields[toField].name : "";
			return `<option>${fromBox.title}.${fromFieldName} ${toBox.title}.${toFieldName}</option>`;
				})
				.join('');

	if (linkCombo.innerHTML != innerHTML)
		linkCombo.innerHTML = innerHTML;
}

function updateLink()
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

	console.log(lk);

	mydata.links[linkCombo.selectedIndex] = lk;

        drawDiag();
}

function addNewLink()
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

	console.log(lk);

	mydata.links.push(lk);

        drawDiag();
}

function dropLink()
{
	const lk = mydata.links[ linkCombo.selectedIndex ];
	console.log({lk});
	mydata.links = mydata.links.filter((_, index) => index != linkCombo.selectedIndex);
	linkComboOnClick();

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
	const boxComment = {box: boxCombo.value, comment: boxCommentTextArea.value} ;

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

function updateFieldComment()
{
	const currentFieldCommentIndex = mydata.fieldComments.findIndex(({box, field, comment}) => box == boxCombo.value && field == fieldCombo.value);
	const fieldComment = {box: boxCombo.value, field: fieldCombo.value, comment: fieldCommentTextArea.value};

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
