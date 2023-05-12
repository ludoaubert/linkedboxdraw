export {initClient};
import {mydata, setData, displayCurrent} from "./table_edit.js";
import {mycontexts, setContexts, drawDiag} from "./diagload.js";

var currentDiagramIndex = -1;

var diagramCombo ;
var downloadOnlineDocButton ;
var uploadOnlineDocButton ;

var diagrams=null;

async function initClient() {
    const response = await fetch("http://127.0.0.1:3000/list_diagrams");
    diagrams = await response.json();
	
	diagramCombo = document.getElementById("online_docs");
	downloadOnlineDocButton = document.getElementById("download_online_doc");
	uploadOnlineDocButton = document.getElementById("upload_online_doc");
	
	diagramCombo.innerHTML = await diagrams.map(diagram => diagram.title)
											.map(title => `<option>${title}</option>`)
											.join('\n');
	currentDiagramIndex = await diagrams.length==0 ? -1 : 0;
											
	diagramCombo.addEventListener("click", ()=>{
		currentDiagramIndex = diagramCombo.selectedIndex;
		const titi="titi";
	});
	diagramCombo.addEventListener("change", ()=>{
		currentDiagramIndex = diagramCombo.selectedIndex;
		const titi="titi";
	});
											
	downloadOnlineDocButton.addEventListener("click", async () => {
		const guid = currentDiagramIndex==-1 ? 'a8828ddfef224d36935a1c66ae86ebb3' : diagrams[currentDiagramIndex].guid;
		const response = await fetch(`http://127.0.0.1:3000/get_document?guid=${guid}` , {
			method: 'GET',
			headers: {
				'Accept': 'application/json',
                'Content-Type': 'application/json',
				'Access-Control-Allow-Origin': '*'				
			}
		});
		const json = await response.json();
		setData(json.data);
		setContexts(json.contexts);
		drawDiag();
		displayCurrent();
	});

	uploadOnlineDocButton.addEventListener("click", async () => {
		
		const response = await fetch("http://127.0.0.1:3000/set_document", {
            method: 'POST',
            headers: {
				'Accept': 'application/json',
                'Content-Type': 'application/json',
				'Access-Control-Allow-Origin': '*'
            },
            body: JSON.stringify({data:mydata, contexts:mycontexts})
        });

		const json = await response.json();
	});
}