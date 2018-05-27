

var selectedSVGElement = 0;
var selectedElement = 0;
var selectedRectangleIndex = -1;
var selectedContextIndex = -1;
var currentX = 0;
var currentY = 0;
var currentTranslateX = 0;
var currentTranslateY = 0;


function selectElement(evt) 
{
	console.log("selectElement(evt)");
	let svg_elements = document.body.getElementsByTagName("svg");
  	for (let i=0; i < svg_elements.length; i++)
	{
    		let svg_element = svg_elements[i];
    		let r = svg_element.getBoundingClientRect();
	
		if (!(r.left <= evt.clientX && evt.clientY <= r.right && r.top <= evt.clientY && evt.clientY <= r.bottom))
			continue;

		console.log("svg=" + JSON.stringify({left:r.left, right:r.right, top:r.top, bottom:r.bottom}));

		selectedSVGElement = svg_element;
		group_elements = svg_element.getElementsByTagName("g");
    		for (let j=0; j < group_elements.length; j++)
		{
	  		let group = group_elements[j];
			let xForms = group.transform.baseVal;// an SVGTransformList
			let firstXForm = xForms.getItem(0); //an SVGTransform
			console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
			let translateX = firstXForm.matrix.e;
			let translateY = firstXForm.matrix.f;
			console.log("group=" + JSON.stringify({translateX, translateY}));
		
			let foreignObj = group.getElementsByTagName("foreignObject")[0];
			let width = foreignObj.width.baseVal.value;
			let height = foreignObj.height.baseVal.value;
			console.log(JSON.stringify({width,height}));
		
			let left = r.left + translateX;
			let right = left + width;
			let top = r.top + translateY;
			let bottom = top + height;
		
			if (left <= evt.clientX && evt.clientX <= right && top <= evt.clientY && evt.clientY <= bottom)
			{
				selectedContextIndex = i;
				selectedRectangleIndex = j;
				currentX = evt.clientX;
				currentY = evt.clientY;
				console.log(JSON.stringify({currentX, currentY}));
				selectedElement = group;
				selectedElement.setAttributeNS(null, "onmousemove", "moveElement(evt)");
				selectedElement.setAttributeNS(null, "onmouseout", "deselectElement(evt)");
				selectedElement.setAttributeNS(null, "onmouseup", "deselectElement(evt)");
				currentTranslateX = translateX;
				currentTranslateY = translateY;
				console.log("hit=" + JSON.stringify({left, right, top, bottom}));
				console.log("currentTranslateX=" + currentTranslateX);
				console.log("currentTranslateY=" + currentTranslateY);
				return;
			}		
    		}	
  	}
}
        
function moveElement(evt) {
	
	var dx = evt.clientX - currentX;
	var dy = evt.clientY - currentY;
	currentTranslateX += dx;
	currentTranslateY += dy;
      
	console.log("currentTranslateX=" + currentTranslateX);
	console.log("currentTranslateY=" + currentTranslateY);
	selectedElement.transform.baseVal.getItem(0).setTranslate(currentTranslateX, currentTranslateY);
	currentX = evt.clientX;
	currentY = evt.clientY;
}

        
function deselectElement(evt) 
{
	console.log("deselectElement(evt)");
	console.assert(selectedElement != 0, "no element selected!")

	let rectangles=[];
	
	let group_elements = selectedSVGElement.getElementsByTagName("g");

	for (let i=0; i < group_elements.length; i++)
	{
		let group = group_elements[i];
		let xForms = group.transform.baseVal;// an SVGTransformList
		let firstXForm = xForms.getItem(0); //an SVGTransform
		console.assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
		let translateX = firstXForm.matrix.e;
		let translateY = firstXForm.matrix.f;
		
		foreignObj = group.getElementsByTagName("foreignObject")[0];
		let width = foreignObj.width.baseVal.value;
		let height = foreignObj.height.baseVal.value;
		
		let left = translateX;
		let right = left + width;
		let top = translateY;
		let bottom = top + height;
		rectangles.push({left, right, top, bottom});
	}


	console.log("selectedContextIndex=" + selectedContextIndex)
	console.log("selectedRectangleIndex=" + selectedRectangleIndex);
	let mycontexts = JSON.parse(contexts);
	let reduced_edges = mycontexts.contexts[selectedContextIndex].reduced_edges;
	let frame = mycontexts.contexts[selectedContextIndex].frame;
	let data={rectangles,reduced_edges, frame}
	let url = 'http://localhost:8080/getReducedEdges?data=' + btoa(JSON.stringify(data));
	console.log("data=" + JSON.stringify(data));
	console.log(url);

        var xhr = new XMLHttpRequest();
    	xhr.open("GET", url, true);
    	xhr.onload = function(e)	{
        	if (xhr.readyState==4)	{
			if (xhr.status==200)	{
            			console.log("response=" + xhr.responseText);
	        	} else	{
				console.error(xhr.statusText);
			}
		}
    	}

    	xhr.send(null);    

	selectedElement.removeAttributeNS(null, "onmousemove");
	selectedElement.removeAttributeNS(null, "onmouseout");
	selectedElement.removeAttributeNS(null, "onmouseup");
	selectedElement = 0;
	selectedContextIndex = -1;
	selectedRectangleIndex = -1;
	selectedSVGElement = 0;
}