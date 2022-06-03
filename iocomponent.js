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


function loadFile(element, handleReceiveEvent) {
	
	if (element.files && element.files[0]) {

		var reader = new FileReader();
		
		reader.addEventListener('load', handleReceiveEvent);
		
		reader.readAsBinaryString(element.files[0]);
	}   
}


