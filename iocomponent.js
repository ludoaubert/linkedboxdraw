import {displayCurrent, mydata} from "./table_edit.js"
import {mycontexts, drawDiag} from "./diagload.js"

export {handleReceiveMyDataEvent, handleReceiveMyDataEvent2, loadFile, download};
export {getFileData};


function handleReceiveMyDataEvent(e) {

	data = e.target.result;
	mydata = JSON.parse(e.target.result);
	currentBoxIndex = -1;
	displayCurrent();
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
		});
	}
}
