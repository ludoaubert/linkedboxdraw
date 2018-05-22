

var selectedSVGElement = 0;
var selectedElement = 0;
var selectedRectangleIndex = -1;
var currentX = 0;
var currentY = 0;
var currentTranslateX = 0;
var currentTranslateY = 0;



function selectElement(evt) {
  console.log("selectElement(evt)");
  var svg_elements = document.body.getElementsByTagName("svg");
  for (var i=0; i < svg_elements.length; i++){
    var svg_element = svg_elements[i];
    var r = svg_element.getBoundingClientRect();
	console.log("svg=" + JSON.stringify({left:r.left, top:r.top}));
	if (!(r.left <= evt.clientX && evt.clientY <= r.right && r.top <= evt.clientY && evt.clientY <= r.bottom))
		continue;
	selectedSVGElement = svg_element;
	group_elements = svg_element.getElementsByTagName("g");
    for (var j=0; j < group_elements.length; j++){
	  	var group = group_elements[j];
		var xForms = group.transform.baseVal;// an SVGTransformList
		var firstXForm = xForms.getItem(0); //an SVGTransform
//		assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
		var translateX = firstXForm.matrix.e;
		var translateY = firstXForm.matrix.f;
		console.log("group=" + JSON.stringify({translateX, translateY}));
		
		var foreignObj = group.getElementsByTagName("foreignObject")[0];
		var width = foreignObj.width.baseVal.value;
		var height = foreignObj.height.baseVal.value;
		console.log(JSON.stringify({width,height}));
		
		var left = r.left + translateX;
		var right = left + width;
		var top = r.top + translateY;
		var bottom = top + height;
		
		if (left <= evt.clientX && evt.clientX <= right && top <= evt.clientY && evt.clientY <= bottom)
		{
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
//  console.log("moveElement(evt)");
//  console.log("svg=" + JSON.stringify({left:selectedR.left, top:selectedR.top}));
	
  var dx = evt.clientX - currentX;
  var dy = evt.clientY - currentY;
  currentTranslateX += dx;
  currentTranslateY += dy;
      
  selectedElement.transform.baseVal.getItem(0).setTranslate(currentTranslateX, currentTranslateY);
  currentX = evt.clientX;
  currentY = evt.clientY;
}

function getRectangleNameFromIndex(selectedSVGElement, rectangleIndex)
{
	var foreignObject = selectedSVGElement.getElementsByTagName("foreignObject")[rectangleIndex];
	if (foreignObject == null)
		return "";
	var table = foreignObject.getElementsByTagName("table")[0];
	var tbody = table.getElementsByTagName("tbody")[0];
	var tr = tbody.getElementsByTagName("tr")[0];
	var th = tr.getElementsByTagName("th")[0];
	var rectangleName = th.innerHTML;
	return rectangleName;
}
        
function deselectElement(evt) 
{
	console.log("deselectElement(evt)");
	console.assert(selectedElement != 0, "no element selected!")

	var rectangle_names = [];
	var rectangles=[];
	
	var group_elements = selectedSVGElement.getElementsByTagName("g");

	for (var i=0; i < group_elements.length; i++)
	{
		var group = group_elements[i];
		var xForms = group.transform.baseVal;// an SVGTransformList
		var firstXForm = xForms.getItem(0); //an SVGTransform
//		assert (firstXForm.type == SVGTransform.SVG_TRANSFORM_TRANSLATE);
		var translateX = firstXForm.matrix.e;
		var translateY = firstXForm.matrix.f;
		
		foreignObj = group.getElementsByTagName("foreignObject")[0];
		var width = foreignObj.width.baseVal.value;
		var height = foreignObj.height.baseVal.value;
		
		var left = translateX;
		var right = left + width;
		var top = translateY;
		var bottom = top + height;
		rectangles.push({left, right, top, bottom});
	}
	
	for (var i=0; i < rectangles.length; i++)
	{
		rectangle_names.push(getRectangleNameFromIndex(selectedSVGElement, i));
	}

	console.log("rectangle_names=" + JSON.stringify(rectangle_names));
	console.log("rectangles=" + JSON.stringify(rectangles));
	console.log("selectedRectangleIndex=" + selectedRectangleIndex);
	console.log("selectedRectangleName=" + getRectangleNameFromIndex(selectedSVGElement, selectedRectangleIndex));
	
	var path_elements = selectedSVGElement.getElementsByTagName("path");
	for (var i=0; i<path_elements.length; i++)
	{
		var pathElement = path_elements[i];
		var edge = pathElement.id;
		rectangleFromIndex = parseInt(edge.substr(0,4));
		rectangleToIndex = parseInt(edge.substr(4,4));
		console.log("rectangleFromIndex=" + rectangleFromIndex);
		console.log("rectangleFromName=" + getRectangleNameFromIndex(selectedSVGElement, rectangleFromIndex));
		console.log("rectangleToIndex=" + rectangleToIndex);
		console.log("RectangleToName=" + getRectangleNameFromIndex(selectedSVGElement, rectangleToIndex));
	  
		if (selectedRectangleIndex == rectangleFromIndex || selectedRectangleIndex == rectangleToIndex)
		{
			var polyline = compute_polyline(rectangle_names, rectangles, rectangleFromIndex, rectangleToIndex);
			
			pathElement.setAttribute("d", encode_polyline(polyline));
		}
	}

	selectedElement.removeAttributeNS(null, "onmousemove");
	selectedElement.removeAttributeNS(null, "onmouseout");
	selectedElement.removeAttributeNS(null, "onmouseup");
	selectedElement = 0;
	selectedRectangleIndex = -1;
	selectedSVGElement = 0;
}
