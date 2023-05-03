export {initClient};

var currentDigramIndex = -1;

var diagrams=null;

function initClient() {
    const response = await fetch("http://127.0.0.1/list_diagrams");
    const jsonData = await response.json();
	diagrams = JSON.parse(jsonData);
}