

function pretty(json) {
	return json
			.replaceAll('}}]', '}}\n]')
			.replaceAll('}]}', '}\n]}')
			.replaceAll('{"title"', '\n{"title"')
			
			.replaceAll('"frame"', '\n"frame"')
			.replaceAll('{"polyline"','\n{"polyline"')
			.replaceAll('{"id"', '\n{"id"')
			.replaceAll('"translatedBoxes"', '\n"translatedBoxes"')
			.replaceAll('"rectangles"','\n"rectangles"')
			.replaceAll('{"left"','\n{"left"')
			
			.replaceAll(/,"id":(\d+)/g, ',\n"id":$1')
			.replaceAll('"fields"', '\n"fields"')
			.replaceAll('{"name"','\n\t{"name"')
			.replaceAll('{"from"','\n{"from"')
			.replaceAll('"links"','\n"links"')
			.replaceAll('"fieldColors"','\n"fieldColors"')
			.replaceAll('"pictures"', '\n"pictures"')
			.replaceAll('"values"', '\n"values"')
			.replaceAll('{"box"', '\n{"box"')
			.replaceAll('"fieldComments"', '\n"fieldComments"')
			.replaceAll('"boxComments"', '\n"boxComments"')
			.replaceAll('{"index"', '\n{"index"');
}
