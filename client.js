export {initClient};

var currentDigramIndex = -1;

var diagrams=null;

async function initClient() {
    const response = await fetch("http://127.0.0.1:3000/list_diagrams");
    diagrams = await response.json();
}