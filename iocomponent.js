

function handleReceiveMyDataEvent(e) {
	
	mydata = JSON.parse(e.target.result);
	displayCurrent();
}

function handleReceiveMyContextsEvent2(e) {
	
	mycontexts = JSON.parse(e.target.result);
	if (mydata != null && mycontexts != null)
		drawDiag();
}

function handleReceiveMyDataEvent2(e) {
	mydata = JSON.parse(e.target.result);
	if (mydata != null && mycontexts != null)
		drawDiag();	
}


function handleReceiveMyDataEventTi(e) {
	mydata = JSON.parse(e.target.result);
	if (mydata != null && mycontexts != null)
		drawDiag();	
	displayCurrent();
}


function loadFile(element, handleReceiveEvent) {
	
	if (element.files && element.files[0]) {

		var reader = new FileReader();
		
		reader.addEventListener('load', handleReceiveEvent);
		
		reader.readAsBinaryString(element.files[0]);
	}   
}


