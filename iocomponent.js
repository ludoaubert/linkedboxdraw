import {displayCurrent} from "./table_edit.js"
import {mycontexts, mydata, drawDiag} from "./diagload.js"

export {handleReceiveMyDataEvent, handleReceiveMyContextsEvent2, handleReceiveMyDataEvent2, handleReceiveMyDataEventTi, handleReceiveMyPictureEvent, loadFile, download};

var data=null;
var contexts=null;


function handleReceiveMyDataEvent(e) {

	data = e.target.result;
	mydata = JSON.parse(e.target.result);
	currentBoxIndex = -1;
	displayCurrent();
}

function handleReceiveMyContextsEvent2(e)
{
	contexts = e.target.result;
	mycontexts = JSON.parse(e.target.result);
	if (mydata != null && mycontexts != null)
	{
		data=null;
		contexts=null;
		drawDiag();
	}
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

	const name = document.getElementById("add pic").value;
	const base64 = btoa(e.target.result);

	const pic = {name, base64};

	mydata.pictures.push(pic);

	document.getElementById("cid").src = "data:image/jpg;base64, " + pic.base64;

	const pictureComboInnerHTML = mydata.pictures
					.sort((a, b) => a.name.localeCompare(b.name))
					.map(pic => "<option>" + pic.name + "</option>")
					.join('');

	document.getElementById("pictures").innerHTML = pictureComboInnerHTML;
	document.getElementById("pictures").value = name;
}


function loadFile(element, handleReceiveEvent)
{

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

function getFileData(element)
{
	if (element.files && element.files[0])
	{
		return new Promise((resolve) => {
			var reader = new FileReader();
			reader.addEventListener('load', (e) => {
				resolve(e.target.result);
			})
			reader.readAsBinaryString(element.files[0]);
		}
	}	
}