export {initClient};
import {mydata, setData, displayCurrent} from "./table_edit.js";
import {mycontexts, setContexts, drawDiag} from "./diagload.js";

var currentDigramIndex = -1;

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
											
	downloadOnlineDocButton.addEventListener("click", async () => {
		const response = await fetch("http://127.0.0.1:3000/get_document");
		const json = await response.json();
		setData(json.data);
		setContexts(json.contexts);
		drawDiag();
		displayCurrent();
	});

	uploadOnlineDocButton.addEventListener("click", async () => {
		
		const response = await fetch("http://127.0.0.1:3000/set_document", {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify({data:mydata, contexts:mycontexts}),
        });

		const json = await response.json();
	});
}