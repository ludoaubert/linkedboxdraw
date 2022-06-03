var data = null;
var contexts = null;

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


function loadFile(element, handleReceiveEvent) {
	
	if (element.files && element.files[0]) {

		var reader = new FileReader();
		
		reader.addEventListener('load', handleReceiveEvent);
		
		reader.readAsBinaryString(element.files[0]);
	}   
}


