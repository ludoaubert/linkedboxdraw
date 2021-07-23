

function prettyContexts(json) {
	return json
			.replaceAll('{"frame"', '\n{"frame"')
			.replaceAll('{"polyline"','\n{"polyline"')
			.replaceAll('{"id"', '\n{"id"')
			.replaceAll('"reduced_edges"','\n"reduced_edges"')
			.replaceAll('"translatedBoxes"', '\n"translatedBoxes"')
			.replaceAll('"links"', '\n"links"'); 
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