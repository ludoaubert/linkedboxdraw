export {initClient};

var currentDigramIndex = -1;

var diagramCombo ;
var loadOnlineDocButton ;

var diagrams=null;

async function initClient() {
    const response = await fetch("http://127.0.0.1:3000/list_diagrams");
    diagrams = await response.json();
	
	diagramCombo = document.getElementById("online_docs");
	loadOnlineDocButton = document.getElementById("load_online_doc");
	
	diagramCombo.innerHTML = await diagrams.map(diagram => diagram.title)
											.map(title => `<option>$title</option>`)
											.join('\n');
}