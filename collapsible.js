

function switchCollapsible(element) {
	
	element.classList.toggle("active");
	var content = element.nextElementSibling;
	
	const cssObj = window.getComputedStyle(content);
	let display = cssObj.getPropertyValue("display");
	
	if (content.style.display === "block") {
		content.style.display = "none";
	} 
	else {
		content.style.display = "block";
	}
}