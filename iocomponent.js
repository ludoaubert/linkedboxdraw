

function drawiocomponent(){
	
	const innerHTML = `<h1>Geometry File Input</h1>

<ul>
<li>Load Geometry File</li>
<li><input type="file" accept=".json" id="myFile" value="Load"></li>
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
<li><input type="file" accept=".json" id="myDataFile" value="Load"></li>
</ul>`;

	return innerHTML;
}


function setiohandlers(){
	
	var input = document.getElementById("myFile");
	
	var data = null;
	var contexts = null;

	input.addEventListener("change", function () {
	  if (this.files && this.files[0]) {
		var myFile = this.files[0];
		var reader = new FileReader();
		
		reader.addEventListener('load', function (e) {
		  contexts = e.target.result;
		  if (data != null && contexts != null)
			loadDiag(data, contexts);
		});
		
		reader.readAsBinaryString(myFile);
	  }   
	});
	
	var dataInput = document.getElementById("myDataFile");

	dataInput.addEventListener("change", function () {
	  if (this.files && this.files[0]) {
		var myDataFile = this.files[0];
		var reader = new FileReader();
		
		reader.addEventListener('load', function (e) {
		  data = e.target.result;
		  if (data != null && contexts != null)
			loadDiag(data, contexts);
		});
		
		reader.readAsBinaryString(myDataFile);
	  }   
	});	
}