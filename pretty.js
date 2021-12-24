

function prettyContexts(json) {
	return json
			.replaceAll('}}]', '}}\n]')
			.replaceAll('}]}', '}\n]}')
			.replaceAll('{"title"', '\n{"title"')
			.replaceAll('"frame"', '\n"frame"')
			.replaceAll('{"polyline"','\n{"polyline"')
			.replaceAll('{"id"', '\n{"id"')
			.replaceAll('"reduced_edges"','\n"reduced_edges"')
			.replaceAll('"translatedBoxes"', '\n"translatedBoxes"')
			.replaceAll('"links"', '\n"links"'); 
}


function prettyData(json) {
	return json
			.replaceAll('}}]', '}}\n]')
			.replaceAll('}]}', '}\n]}')
			.replaceAll('{"title"','\n{"title"')
			.replaceAll('"id"', '\n"id"')
			.replaceAll('"fields"', '\n"fields"')
			.replaceAll('{"name"','\n\t{"name"')
			.replaceAll('{"from"','\n{"from"')
			.replaceAll('{"left"','\n{"left"')
			.replaceAll('"links"','\n"links"')
			.replaceAll('"fieldColors"','\n"fieldColors"')
			.replaceAll('"rectangles"','\n"rectangles"')
			.replaceAll('"values"', '\n"values"')
			.replaceAll('{"box"', '\n{"box"')
			.replaceAll('"fieldComments"', '\n"fieldComments"')
			.replaceAll('"boxComments"', '\n"boxComments"')
			.replaceAll('{"index"', '\n{"index"'); 	
}