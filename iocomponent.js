export {download};
export {getFileData};


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
