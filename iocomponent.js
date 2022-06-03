

function drawiocomponent(){
	
	const innerHTML = `<h1>Geometry File Input</h1>

<ul>
<li>Load Geometry File</li>
<li><input type="file" accept=".json" value="Load" onchange="loadFile(this, handleReceiveMyContextsEvent2)"></li>
</ul>

<h1>Geometry File Output</h1>
<ul>
<li>
	<form onsubmit="return false">
	  <input type="text" name="name" value="test.txt">
	  <input type="submit" value="Save Geometry File As" onclick="download(this['name'].value)">
	</form>
</li>
</ul>

<h1>Data File Input</h1>
<ul>
<li>Load Data File</li>
<li><input type="file" accept=".json" value="Load" onchange="loadFile(this, handleReceiveMyDataEvent2)"></li>
</ul>`;

	return innerHTML;
}


function handleReceiveMyDataEvent(e) {
	
	mydata = JSON.parse(e.target.result);
	displayCurrent();
}

function handleReceiveMyContextsEvent2(e) {
	
	contexts = e.target.result;
	if (data != null && contexts != null)
		loadDiag(data, contexts);
}

function handleReceiveMyDataEvent2(e) {
	data = e.target.result;
	if (data != null && contexts != null)
		loadDiag(data, contexts);	
}


function loadFile(element, handleEvent) {
	
	if (element.files && element.files[0]) {

		var reader = new FileReader();
		
		reader.addEventListener('load', handleReceiveEvent);
		
		reader.readAsBinaryString(element.files[0]);
	}   
}


