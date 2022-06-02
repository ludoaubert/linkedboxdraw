

function switchCollapsible(this) {
	
	this.classList.toggle("active");
	var content = this.nextElementSibling;
	if (content.style.display === "block") {
		content.style.display = "none";
	} 
	else {
		content.style.display = "block";
	}
}