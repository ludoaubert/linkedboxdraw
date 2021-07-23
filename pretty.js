

function prettyContexts(json) {
	return json
			.replaceAll('{"polyline"','\n{"polyline"')
			.replaceAll('{"id"', '\n{"id"')
			.replace('"reduced_edges"','\n"reduced_edges"')
			.replace('"translatedBoxes"', '\n"translatedBoxes"')
			.replace('"links"', '\n"links"'); 
}


function prettyData(json) {
	return json
			.replaceAll('{"title"','\n{"title"')
			.replaceAll('{"name"','\n\t{"name"')
			.replaceAll('{"from"','\n{"from"')
			.replaceAll('{"left"','\n{"left"')
			.replace('"links"','\n"links"')
			.replace('"fieldColors"','\n"fieldColors"')
			.replace('"rectangles"','\n"rectangles"'); 	
}