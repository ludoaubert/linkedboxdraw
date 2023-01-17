var data=null;
var contexts=null;


function handleReceiveMyDataEvent(e) {

	data = e.target.result;
	mydata = JSON.parse(e.target.result);
	currentBoxIndex = -1;
	displayCurrent();
}

function handleReceiveMyContextsEvent2(e) {
	
	contexts = e.target.result;
	mycontexts = JSON.parse(e.target.result);
	if (mydata != null && mycontexts != null)
		drawDiag();
}

function handleReceiveMyDataEvent2(e) {
	data = e.target.result;
	mydata = JSON.parse(e.target.result);
	if (data != null && contexts != null)
	{
		data=null;
		contexts=null;
		drawDiag();
	}		
}


function handleReceiveMyDataEventTi(e) {
	data = e.target.result;
	mydata = JSON.parse(e.target.result);
	if (data != null && contexts != null)
	{
		data=null;
		contexts=null;
		drawDiag();
	}
	currentBoxIndex = -1;	
	displayCurrent();
}


function handleReceiveMyPictureEvent(e)
{
	data = e.target.result;
	currentPictureIndex = mydata.pictures.length;

	const pic = {
		name: newPictureNameEditField.value,
		source: 'titi',
		base64: btoa(e.target.result)
	};

	mydata.pictures.push(pic);

	newPictureNameEditField.value = "";
	
	document.getElementById("cid").src = "data:image/jpg;base64, " + pic.base64;

	displayCurrent();
}


function loadFile(element, handleReceiveEvent) {
	
	if (element.files && element.files[0]) {

		var reader = new FileReader();
		
		reader.addEventListener('load', handleReceiveEvent);
		
		reader.readAsBinaryString(element.files[0]);
	}   
}


function download(filename, jsonData) {
  var element = document.createElement('a');
  const jsons = pretty(JSON.stringify(jsonData));
  element.setAttribute('href', 'data:text/plain;charset=utf-8,' + jsons);
  element.setAttribute('download', filename);
  element.style.display = 'none';
  document.body.appendChild(element);
  element.click();
  document.body.removeChild(element);
}

