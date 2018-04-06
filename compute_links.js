
function handleLoad(e) 
{
	console.log('Loaded import: ' + e.target.href) ;
	var doc= document.querySelector('link[rel="import"]').import;
	for (var i=0; i < 1000; i++)
	{
		var group = document.getElementById('box'+i) ;
		if (group != null)
		{
			group.appendChild(doc.getElementById(i)) ;
		}
	}
}
function handleError(e)
{
	console.log('Error loading import: ' + e.target.href) ;
}

var selectedSVGElement = 0;
var selectedElement = 0;
var selectedRectangleIndex = -1;
var currentX = 0;
var currentY = 0;
var currentTranslateX = 0;
var currentTranslateY = 0;

/*
	<g class="draggable" transform="translate(768,90)" onmousedown="selectElement(evt);">
	<rect x="0" y="0" width="155" height="56" />
	<foreignObject id="box5" width="155" height="56">
	</foreignObject>
	</g>
*/

/*
<foreignobject id="box1" width="190" height="56">
	<div id="1">
		<table>
			<tbody>
				<tr>
					<th contextmenu="menu1" id="MDInboundConnectionHandler">MDInboundConnectionHandler</th>
				</tr>
				<tr><td title="">PK&nbsp;id</td></tr>
				<tr><td title="PacketizedSocketReadingStrategy"><a href="http://localhost:8080/getFilter?0120850480be03809b0280b00380e103809b0380a20380e103807103807803804d04809403809303807803807f03806a0480930580c408801300000800000d00100700300500400300500100600a00700600800900900200a00c00a01100b00400d00e00e00200f01101001101100001100dffff3#PacketizedSocketReadingStrategy">FK&nbsp;strategy</a></td></tr>
			</tbody>
		</table>
		<menu type="popup" id="menu1">
			<menuitem label="centered_diagram" onclick="window.open(urlEncode(&#39;1&#39;));"></menuitem>
			<menuitem label="MulticastLineListener" onclick="window.location=&#39;#MulticastLineListener&#39;;"></menuitem>
		</menu>
	</div>
</foreignobject>
*/

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


function MyPartitioning(){
  this.positions = [0];
}

function MyPartitioning(startPos){
  this.positions = [startPos];
}

function MyPartitioning(tab, N)
{
  this.positions = [];
  for (var i=0; i < N; i++)
    positions.push(tab[i]) ;
}

MyPartitioning.prototype = {
  Clear: function() {
    this.positions = [0] ;
  },

  GetPartitionCount: function() {
    return this.positions.length - 1;
  },

  GetSize: function() {
    return this.positions[this.positions.length-1] ;
  },

  //endPos is past the end: it is the beginPos of the next partition if there is one.
  InsertPartition: function(endPos) {
    this.positions.push(endPos) ;
  },

  PopPartition: function(){
    this.positions.pop() ;
  },

  InsertSplitPosition: function(pos){
    var p = this.PartitionFromPosition(pos) ;
//  this.positions.insert(m_positions.begin()+p+1, pos) ;
  },

  InsertPartitions: function(partitions){
    var offset = this.positions[this.positions.length-1] ;
    for (var i=0; i<partitions.GetPartitionCount(); i++)
      this.positions.push(partitions.endPosFromPartition(i) + offset) ;
  },

  startPosFromPartition: function(partition){
    var pos = this.positions[partition] ;
    return pos;
  },

  endPosFromPartition: function(partition){
    var pos = this.positions[partition+1] ;
    return pos;
  },

  PartitionSize: function(partition){
    return this.positions[partition+1] - this.positions[partition] ;
  },

  ResizePartition: function(p, new_size){
    var old_size = this.positions[p+1] - this.positions[p] ;
    for (var i=p+1; i < this.positions.size(); i++)
      this.positions[i] += new_size - old_size ;
  },

  PartitionMiddle: function(partition){
    return (this.positions[partition+1] + this.positions[partition]) / 2 ;
  },

  PartitionFromPosition: function(pos)
  {
    if (this.positions.length < 2)
      return -1 ;
    if (pos == this.positions[this.position.length-1])
      return this.positions.length - 2 ;
    var lower = 0;
    var upper = this.positions.length-1;
    do 
    {
      var middle = Math.floor((upper + lower + 1) / 2); 	// Round high
      var posMiddle = this.positions[middle];

      if (pos < posMiddle) 
      {
        upper = middle - 1;
      } 
      else 
      {
        lower = middle;
      }
    } 
    while (lower < upper);
    //assert(lower < m_positions.size()-1) ;
    //assert(0 <= lower) ;
    return lower;
  }
};

function BinaryHeap(scoreFunction){
  this.content = [];
  this.scoreFunction = scoreFunction;
}

BinaryHeap.prototype = {
  push: function(element) {
    // Add the new element to the end of the array.
    this.content.push(element);
    // Allow it to bubble up.
    this.bubbleUp(this.content.length - 1);
  },

  pop: function() {
    // Store the first element so we can return it later.
    var result = this.content[0];
    // Get the element at the end of the array.
    var end = this.content.pop();
    // If there are any elements left, put the end element at the
    // start, and let it sink down.
    if (this.content.length > 0) {
      this.content[0] = end;
      this.sinkDown(0);
    }
    return result;
  },

  remove: function(node) {
    var length = this.content.length;
    // To remove a value, we must search through the array to find
    // it.
    for (var i = 0; i < length; i++) {
      if (this.content[i] != node) continue;
      // When it is found, the process seen in 'pop' is repeated
      // to fill up the hole.
      var end = this.content.pop();
      // If the element we popped was the one we needed to remove,
      // we're done.
      if (i == length - 1) break;
      // Otherwise, we replace the removed element with the popped
      // one, and allow it to float up or sink down as appropriate.
      this.content[i] = end;
      this.bubbleUp(i);
      this.sinkDown(i);
      break;
    }
  },

  size: function() {
    return this.content.length;
  },

  bubbleUp: function(n) {
    // Fetch the element that has to be moved.
    var element = this.content[n], score = this.scoreFunction(element);
    // When at 0, an element can not go up any further.
    while (n > 0) {
      // Compute the parent element's index, and fetch it.
      var parentN = Math.floor((n + 1) / 2) - 1,
      parent = this.content[parentN];
      // If the parent has a lesser score, things are in order and we
      // are done.
      if (score >= this.scoreFunction(parent))
        break;

      // Otherwise, swap the parent with the current element and
      // continue.
      this.content[parentN] = element;
      this.content[n] = parent;
      n = parentN;
    }
  },

  sinkDown: function(n) {
    // Look up the target element and its score.
    var length = this.content.length,
    element = this.content[n],
    elemScore = this.scoreFunction(element);

    while(true) {
      // Compute the indices of the child elements.
      var child2N = (n + 1) * 2, child1N = child2N - 1;
      // This is used to store the new position of the element,
      // if any.
      var swap = null;
      // If the first child exists (is inside the array)...
      if (child1N < length) {
        // Look it up and compute its score.
        var child1 = this.content[child1N],
        child1Score = this.scoreFunction(child1);
        // If the score is less than our element's, we need to swap.
        if (child1Score < elemScore)
          swap = child1N;
      }
      // Do the same checks for the other child.
      if (child2N < length) {
        var child2 = this.content[child2N],
        child2Score = this.scoreFunction(child2);
        if (child2Score < (swap == null ? elemScore : child1Score))
          swap = child2N;
      }

      // No need to swap further, we are done.
      if (swap == null) break;

      // Otherwise, swap and continue.
      this.content[n] = this.content[swap];
      this.content[swap] = element;
      n = swap;
    }
  }
};
/*
var heap = new BinaryHeap(function(x){return x;});
forEach([10, 3, 4, 8, 2, 9, 7, 1, 2, 6, 5],
        method(heap, "push"));

heap.remove(2);
while (heap.size() > 0)
  print(heap.pop());
*/

var RECT_BORDER = 20 ;
var FRAME_BORDER = 30 ;
var penalty_on_turn=1;
var penalty_on_crossing=1;


function compute_polyline(rectangle_names,
						  rectangles,
						  rectangleFromIndex, 
						  rectangleToIndex)
{
	var expanded_rectangles = JSON.parse(JSON.stringify(rectangles));
	for (var i=0; i < expanded_rectangles.length; i++) {
		expandBy(expanded_rectangles[i], RECT_BORDER);
	}

	var frame = compute_frame(expanded_rectangles);
	var expanded_frame = JSON.parse(JSON.stringify(frame));
	expandBy(expanded_frame, FRAME_BORDER);
 
	var excluded_partition = 0;
	var polylines_data=[];
	var polylines_partition = new MyPartitioning();
	var partition = [new MyPartitioning(),new MyPartitioning()] ;
	var troncons=[];
	var polylines_partition = new MyPartitioning();
	compute_arc_troncons(rectangle_names,
						 rectangles,
						  expanded_rectangles,
						  expanded_frame,
						  polylines_data,
						  polylines_partition,
						  excluded_partition,
						  rectangleFromIndex,
						  rectangleToIndex,
						  penalty_on_turn,
						  penalty_on_crossing,
						  troncons);
//	console.log("polylines_data=" + JSON.stringify(polylines_data));
//	console.log("polylines_partition=" + JSON.stringify(polylines_partition));
	console.log("troncons=" + JSON.stringify(troncons));
	
	var polyline = compute_polyline_from_troncons(troncons);
	console.log("polyline=" + JSON.stringify(polyline));
	
	return polyline;
}


function compute_polyline_from_troncons(troncons)
{
	var polyline = [] ;
	
	if (troncons.length == 0)
		return polyline;

	var tr = troncons[0] ;

/*
pour chaque troncon, on utilise une variable aleatoire.
plus tard, il pourrait etre interessant de faire des permutations
sur ces variables et de recompter le nombre d'intersections.
permutation sur les variables lorsque les troncons s'intersectent.
*/
	var p = {x:0, y:0} ;
	p[tr.direction==0 ? "x" : "y"] = start_value(tr) ;
	
	console.log("p=" + JSON.stringify(p));

	for (let tr of troncons)
	{
		d = transpose(tr.direction) ;
		var x = Math.floor( min(tr.rect, d) + dimension(tr.rect, d) * Math.random());//tr.rd / RAND_MAX ;
		p[d == 0 ? "x" : "y"] = x ;
		console.log("p=" + JSON.stringify(p));
		polyline.push(JSON.parse(JSON.stringify(p))) ;
		console.log("polyline=" + JSON.stringify(polyline));
	}

	var last_tr = troncons[troncons.length-1];
	p[last_tr.direction == 0 ? "x" : "y"] = last_value(last_tr) ;
	console.log("p=" + JSON.stringify(p));
	polyline.push(JSON.parse(JSON.stringify(p))) ;
	console.log("polyline=" + JSON.stringify(polyline));
		
	return polyline ;
}

function encode_polyline(polyline)
{
	var encoding = "";
	for (var i=0; i < polyline.length; i++)
	{
		p = polyline[i];
		if (i != 0)
			encoding += " ";
		encoding += i==0 ? "M" : "L";
		encoding += p.x + "," + p.y;
	}
	console.log("encoding=" + encoding);
	return encoding;
}

function width(r)
{
	return r.right - r.left ;
}

function height(r)
{
	return r.bottom - r.top ;
}

function dimension(r, direction)
{
	switch (direction)
	{
	case 0://Troncon::EAST_WEST:
		return width(r) ;
	case 1://Troncon::NORTH_SOUTH:
		return height(r) ;
	}
}


function start_value(tr)
{
	switch (tr.sens)
	{
	case +1://Troncon::INCREASE:
		return min(tr.rect, tr.direction) ;
	case -1://Troncon::DECREASE:
		return max(tr.rect, tr.direction) ;
	}
}

function last_value(tr)
{
	switch (tr.sens)
	{
	case +1://Troncon::INCREASE:
		return max(tr.rect, tr.direction) ;
	case -1://Troncon::DECREASE:
		return min(tr.rect, tr.direction) ;
	}
}


function compute_frame(rectangles)
{
	var frame = {
		left: Number.MAX_SAFE_INTEGER,
		right: Number.MIN_SAFE_INTEGER,
		top: Number.MAX_SAFE_INTEGER,
		bottom: Number.MIN_SAFE_INTEGER
	};
	
	for (let r of rectangles)
	{
		frame.left = Math.min(frame.left, r.left) ;
		frame.right = Math.max(frame.right, r.right) ;
		frame.top = Math.min(frame.top, r.top) ;
		frame.bottom = Math.max(frame.bottom, r.bottom) ;
	}
	
	return frame ;
}

function expandBy(rect, border) 
{
	rect.left -= border ;
	rect.right += border ;
	rect.top -= border ;
	rect.bottom += border ;
}

//O:Troncon::EAST_WEST, 1:Troncon::NORTH_SOUTH
function min(rect, direction) 
{
	switch(direction)
	{
	case 0:
		return rect.left;
	case 1:
		return rect.top;
	}
}

//O:Troncon::EAST_WEST, 1:Troncon::NORTH_SOUTH
function max(rect, direction) 
{
	switch(direction)
	{
	case 0:
		return rect.right;
	case 1:
		return rect.bottom;
	}
}


function sort_unique(arr) {
    arr = arr.sort(function (a, b) { return a - b; });
    var ret = [arr[0]];
    for (var i = 1; i < arr.length; i++) { // start loop at 1 as element 0 can never be a duplicate
        if (arr[i-1] !== arr[i]) {
            ret.push(arr[i]);
        }
    }
    return ret;
}

function load_graph_data(rectangles,
					     expanded_rectangles,
					     frame,
					     i,
					     j,
					     partition)
{
	for (var direction in [0, 1])
	{
		var coord = [];

		coord.push(min(frame, direction*1)-FRAME_BORDER) ;
		coord.push(min(frame, direction*1)) ;
		coord.push(max(frame, direction*1)) ;
		coord.push(max(frame, direction*1)+FRAME_BORDER) ;

		for (var k=0; k < expanded_rectangles.length; k++)
		{
			var r = (k==i || k==j) ? rectangles[k] : expanded_rectangles[k] ;
			coord.push(min(r, direction*1)) ;
			coord.push(max(r, direction*1)) ;
		}

		partition[direction].positions = sort_unique(coord) ;
	}
}

//O:Troncon::EAST_WEST, 1:Troncon::NORTH_SOUTH
function compute_rectangle_nodes(partition, 
								 r,
								 nodes)
{
	var M = partition[0/*Troncon::EAST_WEST*/].GetPartitionCount() ;

	var x_min = partition[0/*Troncon::EAST_WEST*/].positions.indexOf(r.left) ;
	var x_max = partition[0/*Troncon::EAST_WEST*/].positions.indexOf(r.right) ;
	var y_min = partition[1/*Troncon::NORTH_SOUTH*/].positions.indexOf(r.top) ;
	var y_max = partition[1/*Troncon::NORTH_SOUTH*/].positions.indexOf(r.bottom) ;

	//console.log(JSON.stringify(x_min));
	//console.log(JSON.stringify(x_max));
	//console.log(JSON.stringify(y_min));
	//console.log(JSON.stringify(y_max));
	
	for (var x=x_min; x < x_max; x++)
	{
		for (var y=y_min; y < y_max; y++)
		{
			nodes.add(y*M+x) ;
		}
	}
	
	return {x_min, x_max, y_min, y_max};
}


function coordinate(u, M, direction)
{
	switch (direction)
	{
	case 0/*Troncon::EAST_WEST*/:
		return u % M ;
	case 1/*Troncon::NORTH_SOUTH*/:
		return Math.floor(u / M) ;
	}
}

function can_shift(u, M, N, direction, sens)
{
	var y = Math.floor(u / M);
	var x = u % M ;
	
	switch (direction)
	{
	case 0/*Troncon::EAST_WEST*/:
		switch (sens)
		{
		case -1/*Troncon::DECREASE*/: //west bound
			return x > 0 ;
		case +1/*Troncon::INCREASE*/: //east bound
			return x+1 < M ;
		}
	case 1/*Troncon::NORTH_SOUTH*/:
		switch (sens)
		{
		case -1/*Troncon::DECREASE*/: //north bound
			return y > 0 ;
		case +1/*Troncon::INCREASE*/: //south bound
			return y+1 < N ;
		}
	}
}

function shift(u, M, N, direction, sens)
{
	var x = u % M;
	var y = Math.floor(u / M) ;
	var v = 0 ;

	switch (direction)
	{
	case 0/*Troncon::EAST_WEST*/:
		switch (sens)
		{
		case -1/*Troncon::DECREASE*/: //west bound
			v = M * y + (x-1) ;
			return v ;
		case +1/*Troncon::INCREASE*/: //east bound
			v = M * y + (x+1) ;
			return v ;
		}
	case 1/*Troncon::NORTH_SOUTH*/:
		switch (sens)
		{
		case -1/*Troncon::DECREASE*/: //north bound
			v = M * (y-1) + x ;
			return v ;
		case +1/*Troncon::INCREASE*/: //south bound
			v = M * (y+1) + x ;
			return v ;
		}
	}
}

function adjacency_list(u, N, M)
{
	var edges = [];

	for (var direction=0; direction<=1; direction++)
	{
		for (var sens=-1; sens <=1; sens+=2/*Troncon::DECREASE, Troncon::INCREASE*/)
		{
			if (can_shift(u, M, N, direction, sens))
			{
				var v = shift(u, M, N, direction, sens) ;
				edges.push({u,v,distance_v:-1,direction,sens}) ;
			}
		}
	}

	return edges ;
}

function edge_cost(edge,
				   partition,
				   nr_troncons,
				   direction_from_predecessor,
				   penalty_on_turn,
				   penalty_on_crossing)
{
	var N = partition[1/*Troncon::NORTH_SOUTH*/].GetPartitionCount(); 
	var M = partition[0/*Troncon::EAST_WEST*/].GetPartitionCount() ;
	var u = edge.u ;
	var v = edge.v ;
	var direction = edge.direction ;
	var sens = edge.sens ;

	var my_penalty_on_crossing = (nr_troncons[1-direction][u] + 
		nr_troncons[1-direction][v])*
		penalty_on_crossing ;

	//assert(predecessor[u] != -1) ;
	//assert(direction_from_predecessor[u] != Troncon::UNKNOWN_DIRECTION) ;
	var my_penalty_on_turn = direction_from_predecessor[u]==direction ? 0 : penalty_on_turn ;

	var k = coordinate(u, M, direction) ;
	var cost = Math.abs( partition[direction].PartitionMiddle(k+sens) - partition[direction].PartitionMiddle(k) ) ;
	
	return cost + my_penalty_on_turn + my_penalty_on_crossing ;
}

function dijkstra(sourceNodes,
				  targetNodes,
				  forbiddenNodes,
				  partition,
				  nr_troncons,
				  penalty_on_turn,
				  penalty_on_crossing,
				  optimal_path,
				  selected_source,
				  selected_target)
{
	var N = partition[1/*Troncon::NORTH_SOUTH*/].GetPartitionCount(); 
	var M = partition[0/*Troncon::EAST_WEST*/].GetPartitionCount() ;

	var distance = new Array(N*M);
	var predecessor = new Array(N*M);
	var direction_from_predecessor = new Array(N*M);
	
	for (var i=0; i < N*M; i++){
		distance[i] = Number.POSITIVE_INFINITY;
		predecessor[i] = -1;
		direction_from_predecessor[i]=-1;// Troncon::UNKNOWN_DIRECTION) ;
	}
	for (let s of sourceNodes)
		distance[s] = 0;

	Q = new BinaryHeap(function(edge){return edge.distance_v;});

	for (let s of sourceNodes)
	{
		adj = adjacency_list(s, N, M);
		for (let e of adj) {
			v = e.v ;
			e.distance_v = distance[s] + edge_cost(e, partition, nr_troncons, direction_from_predecessor, penalty_on_turn, penalty_on_crossing) ;
			if (e.distance_v < distance[v])
				Q.push(e) ;
		}
	}

	while (Q.size() != 0)
	{
		var e = Q.pop() ;
		var u = e.u ;
		var v = e.v ;
		var direction = e.direction ;
		var sens = e.sens ;
		
		console.log(JSON.stringify(e));

		if (forbiddenNodes.has(v))
			continue ;

		if (e.distance_v < distance[v])
		{
			distance[v] = e.distance_v ;
			console.assert(distance[v] == e.distance_v, "distance[" + v + "]=" + distance[v] + "\ne=" + JSON.stringify(e));
			
			predecessor[v] = u ;
			direction_from_predecessor[v] = direction ;

			var adj = adjacency_list(v, N, M);
			for (let edge of adj)
			{
				console.assert(distance[v] == e.distance_v, "distance[" + v +"]=" + distance[v] + "\ne=" + JSON.stringify(e));
				edge.distance_v = distance[v] + edge_cost(edge, partition, nr_troncons, direction_from_predecessor, penalty_on_turn, penalty_on_crossing) ;
				console.assert(edge.distance_v >= e.distance_v, "violation of priority Queue order." + "\nedge=" + JSON.stringify(edge) + "\ne=" + JSON.stringify(e) +
					"\ndistance[" + v + "]=" + distance[v]);
				if (edge.distance_v < distance[edge.v])
				{
					Q.push(edge) ;
				}
			}
		}
	}
	
	console.log("main dijkstra loop done !");

	if (targetNodes.size == 0)
		return ;

	var vv = -1;
	var min_distance = Number.POSITIVE_INFINITY;
	for (let u of targetNodes){
		if (distance[u] < min_distance) {
			min_distance = distance[u];
			vv = u;
		}
	}
	
	console.log("hit: vv=" + vv);

	u = vv ;
	while (u != -1)
	{
		if (targetNodes.has(u) && !targetNodes.has(predecessor[u]))
			selected_target = u ;
		if (sourceNodes.has(predecessor[u]) && !sourceNodes.has(u))
			selected_source = predecessor[u] ;
		if (!sourceNodes.has(u) && !targetNodes.has(u))
			optimal_path.push(u) ;
		u = predecessor[u] ;
	}
	optimal_path.reverse() ;
	
	console.log("optimal_path=");
	for (let u of optimal_path)
	{
		console.log(JSON.stringify({x: u%M, y:Math.floor(u/M)}));
	}
	
	return [selected_source, selected_target];
}


function compute_direction(u, v, M)
{
	if (u == v)
		return 3;//Troncon::UNKNOWN_DIRECTION ;
	if (u % M == v % M)
		return 1;//Troncon::NORTH_SOUTH ;
	else if (Math.floor(u / M) == Math.floor(v / M))
		return 0;//Troncon::EAST_WEST ;
}

function compute_sens(u, v, M)
{
	var direction = compute_direction(u, v, M) ;
	switch (direction)
	{
	case 0://Troncon::EAST_WEST:
		if (v == u+1)
			return 1;//Troncon::INCREASE ;
		else if (v == u-1)
			return -1;//Troncon::DECREASE ;
		else
			return 0;//Troncon::UNDEFINED_SENS ;

	case 1://Troncon::NORTH_SOUTH:
		if (v == u+M)
			return 1;//Troncon::INCREASE ;
		else if (v == u-M)
			return -1;//Troncon::DECREASE ;
		else
			return 0;//Troncon::UNDEFINED_SENS ;
	}
}

function compute_rectangle(partition, u)
{
	var M = partition[0/*Troncon::EAST_WEST*/].GetPartitionCount() ;
	var y_u = Math.floor(u/M);
	var x_u = u%M ;
	
	console.log(JSON.stringify({x_u, y_u, M}));
	console.log("partition[1].GetPartitionCount()=" + partition[1].GetPartitionCount());

	return {
		left: partition[0/*Troncon::EAST_WEST*/].startPosFromPartition(x_u),
		right: partition[0/*Troncon::EAST_WEST*/].endPosFromPartition(x_u),
		top: partition[1/*Troncon::NORTH_SOUTH*/].startPosFromPartition(y_u),
		bottom: partition[1/*Troncon::NORTH_SOUTH*/].endPosFromPartition(y_u)
	} ;
}


function enveloppe(r1, r2)
{
	return {
		left : Math.min(r1.left, r2.left),
		right : Math.max(r1.right, r2.right),
		top : Math.min(r1.top, r2.top),
		bottom : Math.max(r1.bottom, r2.bottom)	
	} ;
}


function compute_troncons(selected_source,
						  selected_target,
						  optimal_path, 
						  partition, 
						  troncons)
{
	if (optimal_path.length == 0)
		return ;

	var M = partition[0/*Troncon::EAST_WEST*/].GetPartitionCount() ;

	var source_direction = compute_direction(selected_source, optimal_path[0], M) ;
	var source_sens = compute_sens(selected_source, optimal_path[0], M) ;
	var target_direction = compute_direction(optimal_path[optimal_path.length-1], selected_target, M) ;
	var target_sens = compute_sens(optimal_path[optimal_path.length-1], selected_target, M) ;

	console.assert(optimal_path.length != 0, "optimal_path is empty!") ;

	troncons.push(
		{ 
			rect: compute_rectangle(partition, optimal_path[0]), 
			direction: source_direction, 
			sens: source_sens
		}
	) ;

	for (var i=1; i < optimal_path.length; i++)
	{
		var v = optimal_path[i-1];
		var u = optimal_path[i] ;
		var ru = compute_rectangle(partition, u) ;
		var rv = compute_rectangle(partition, v) ;
		var direction = u != v ?
			compute_direction(v, u, M) :
			transpose(tr.direction) ;

		var sens = compute_sens(v, u, M) ;

		if (troncons[troncons.length-1].direction == direction)
		{
			tr = troncons[troncons.length-1];
			tr.rect = enveloppe(tr.rect, ru) ;
		}
		else
		{
			var rv = compute_rectangle(partition, v) ;
			troncons.push(
				{
					rect: enveloppe(rv, ru), 
					direction: direction, 
					sens: sens	
				}
			);
		}
	}

	if (troncons[troncons.length-1].direction != target_direction)
	{
		troncons.push_back({rect: compute_rectangle(partition, optimal_path[optimal_path.length-1]), direction: target_direction, sens: target_sens});
	}
}

function intersect_strict(r1, r2)
{
	return !(r1.left >= r2.right || r1.right <= r2.left || r1.top >= r2.bottom || r1.bottom <= r2.top) ;
}


function transpose(direction)
{
	switch (direction)
	{
	case 0://Troncon::EAST_WEST:
		return 1;//Troncon::NORTH_SOUTH ;
	case 1://Troncon::NORTH_SOUTH:
		return 0;//Troncon::EAST_WEST ;
	}
}


function rectangle_dimension(direction, sens)
{
	switch (direction)
	{
	case 0://Troncon::EAST_WEST
		switch (sens)
		{
		case +1://Troncon::INCREASE
			return "right";
		case -1://Troncon::DECREASE
			return "left";
		}
	case 1://Troncon::NORTH_SOUTH
		switch (sens)
		{
		case +1://Troncon::INCREASE
			return "bottom";
		case -1://Troncon::DECREASE
			return "top";
		}
	}
}


function troncons_dilatation(troncons,
							 ri,
							 rj,
							 rectangles,
							 frame)
{
	var k=-1 ;
	for (let tr of troncons)
	{
		k++ ;
		var band = JSON.parse(JSON.stringify(tr.rect)) ;
		var dilatation_direction = transpose(tr.direction) ;
		band[dilatation_direction==0 ? "left":"top"] = frame[dilatation_direction==0 ? "left":"top"] + FRAME_BORDER ;
		band[dilatation_direction==0 ? "right":"bottom"] = frame[dilatation_direction==0 ? "right":"bottom"] - FRAME_BORDER ;

console.log("band=" + JSON.stringify(band));
//min de left parmi ceux dont left > tr.m_rect.right
//max de right parmi ceux dont right < tr.m_rect.left
		for (let r of rectangles)
		{
			if (!intersect_strict(r, band))
				continue ;
			if (min(r,dilatation_direction) >= max(tr.rect,dilatation_direction))
			{
				band[dilatation_direction==0?"right":"bottom"] = Math.min(max(band, dilatation_direction), min(r,dilatation_direction)) ;
				console.log(k);
				console.log("band=" + JSON.stringify(band));
			}
			if (max(r,dilatation_direction) <= min(tr.rect,dilatation_direction))
			{
				band[dilatation_direction==0?"left":"top"] = Math.max(min(band, dilatation_direction), max(r,dilatation_direction)) ;
				console.log(k);
				console.log("band=" + JSON.stringify(band));
			}
		}
		tr.rect = band ;

		if (k==0)
		{
			tr.rect[dilatation_direction==0 ? "left":"top"] = Math.max(min(tr.rect, dilatation_direction), min(ri, dilatation_direction)) ;
			tr.rect[dilatation_direction==0 ? "right":"bottom"] = Math.min(max(tr.rect, dilatation_direction), max(ri, dilatation_direction)) ;
			console.log(k);
			console.log("tr.rect=" + JSON.stringify(tr.rect));
		}
		if (k+1 == troncons.length)
		{
			tr.rect[dilatation_direction==0 ? "left":"top"] = Math.max(min(tr.rect, dilatation_direction), min(rj, dilatation_direction)) ;
			tr.rect[dilatation_direction==0 ? "right":"bottom"] = Math.min(max(tr.rect, dilatation_direction), max(rj, dilatation_direction)) ;
			console.log(k);
			console.log("tr.rect=" + JSON.stringify(tr.rect));
		}

		if (k > 0)
		{
			tr = troncons[k-1] ;
			tr.rect[rectangle_dimension(dilatation_direction, tr.sens)] = value(tr.rect, dilatation_direction, tr.sens) ;
			console.log(k);
			console.log("tr.rect=" + JSON.stringify(tr.rect));
		}

		if (k+1 < troncons.length)
		{
			tr = troncons[k+1] ;
			var sens = reverse(tr.sens) ;
			tr.rect[rectangle_dimension(dilatation_direction, sens)] = value(tr.rect, dilatation_direction, sens) ;
			console.log(k);
			console.log("tr.rect=" + JSON.stringify(tr.rect));
		}
	}
}


function value(r, direction, sens)
{
	switch (direction)
	{
	case 0://Troncon::EAST_WEST:
		switch (sens)
		{
			case +1:
				return r.right;
			case -1:
				return r.left;
		}
	case 1://Troncon::NORTH_SOUTH:
		switch (sens)
		{
			case +1:
				return r.bottom;
			case -1:
				return r.top;
		}
	}
}

function reverse(sens)
{
	switch (sens)
	{
	case 0:
		return 1;
	case 1:
		return 0;
	}
}


//_FwdIt lower_bound(_FwdIt _First, _FwdIt _Last, const _Ty& _Val)
// find first element not before _Val, using operator<
function lower_bound(partition, val)
{
	for (var i=0; i<partition.length; i++)
	{
		if (partition[i] >= val)
			return i;
	}
	return -1;
}
function upper_bound(partition, val)
{
	for (var i=partition.length-1; i>=0; i--)
	{
		if (partition[i] <= val)
			return i;
	}
	return -1;
}

function compute_nr_troncons(polylines_data,
							 polylines_partition,
							 partition,
							 excluded_partition,
							 nr_troncons)
{
	var N = partition[1/*Troncon::NORTH_SOUTH*/].GetPartitionCount(); 
	var M = partition[0/*Troncon::EAST_WEST*/].GetPartitionCount();

	for (let direction in [0,1]){
		nr_troncons[direction]=new Array(N*M);
		for (var i=0; i < N*M; i++){
			nr_troncons[direction][i]= 0 ;
		}
	}
	
	for (var p=0; p < polylines_partition.GetPartitionCount(); p++)
	{
		if (p == excluded_partition)
			continue ;
		var start_pos = polylines_partition[p].startPosFromPartition(p) ;
		var end_pos = polylines_partition[p+1].endPosFromPartition(p) ;
		for (var pos=start_pos; pos<end_pos; pos++)
		{
			var tr = polylines_data[pos] ;
			var r = tr.rect ;
			var index ={left:-1, right:-1, top:-1, bottom:-1};
/*
utilisation de std::upper_bound() (resp. std::lower_bound()):
partition est construite avec les 'expanded_rectangles', tandis que polylines_data contient des
Troncon qui sont passés par 'troncons_dilatation()' donc plutot par rapport a 'rectangles'.
Du coup les valeurs ne vont pas matcher.
*/
//_FwdIt lower_bound(_FwdIt _First, _FwdIt _Last, const _Ty& _Val)
// find first element not before _Val, using operator<

			for (let direction in [0,1])
			{
				var low = lower_bound(partition[direction].positions, r[direction==0?"left":"top"]) ;
				var up = upper_bound(partition[direction].positions, r[direction==0?"right":"bottom"]) ;
				if (low != -1 && up != -1)
				{
					index[direction==0?"left":"top"] = low ;
					index[direction==0?"right":"bottom"] = up ;
					if (low>0 && partition[direction].positions[low] > r.m_left)
						index[direction==0?"left":"top"]-- ;
					if (up+1<partition[direction].positions.length && partition[direction].positions[up] < r.m_right)
						index[direction==0?"right":"bottom"]++ ;
				}
			}

			var x_min = index.left ;
			var x_max = index.right ;
			var y_min = index.top ;
			var y_max = index.bottom ;

			if(x_min==-1 || x_max==-1 || y_min==-1 || y_max==-1)
				continue ;

			for (var x=x_min; x < x_max; x++)
			{
				for (var y=y_min; y < y_max; y++)
				{
					nr_troncons[tr.direction][y*M+x]++ ;
				}
			}
		}
	}
}


function center(rect)
{
	return {x: Math.floor((rect.left+rect.right)/2), y: Math.floor((rect.top+rect.bottom)/2)};
}


function compute_arc_troncons(rectangle_names,
							  rectangles,
							  expanded_rectangles,
							  expanded_frame,
							  polylines_data,
							  polylines_partition,
							  excluded_partition,
							  i,
							  j,
							  penalty_on_turn,
							  penalty_on_crossing,
							  troncons)
{
//graphe de calcul des connections
	var partition = [new MyPartitioning(),new MyPartitioning()] ;
	load_graph_data(rectangles,
					expanded_rectangles,
					expanded_frame,
					i,
					j,
					partition) ;

	var M = partition[0/*Troncon::EAST_WEST*/].GetPartitionCount() ;
	var N = partition[1/*Troncon::NORTH_SOUTH*/].GetPartitionCount();

	var sources = new Set();
	var targets = new Set();
	var forbidden_nodes = new Set();
	
	console.log("partition=" + JSON.stringify(partition));
	
	console.log("rectangle[" + i + "]=" + JSON.stringify(rectangles[i]));
	var rrs = compute_rectangle_nodes(partition, rectangles[i], sources) ;
	console.log("sourceNodes:" + sources.size + "/" + N*M);
	console.log("sourceNodes=");
	console.log("covered by r[" + rectangle_names[i] + "]=" + JSON.stringify(rrs) );
	for (let u of sources) console.log(u + "=" + JSON.stringify({x: u%M, y: Math.floor(u / M)}));
	
	console.log("rectangle[" + j + "]=" + JSON.stringify(rectangles[j]));
	var rrt = compute_rectangle_nodes(partition, rectangles[j], targets) ;
	console.log("targetNodes:" + targets.size + "/" + N*M);	
	console.log("targetNodes=");
	console.log("covered by r[" + rectangle_names[j] + "]=" + JSON.stringify(rrt) );
	for (let u of targets) console.log(u + "=" + JSON.stringify({x: u%M, y: Math.floor(u / M)}));
	
	console.log("forbiddenNodes=");
	for (var k=0; k < expanded_rectangles.length; k++)
	{
		if (k==i || k==j)
			continue ;
		var r = expanded_rectangles[k];
		my_forbidden_nodes = new Set();
		var rr = compute_rectangle_nodes(partition, r, my_forbidden_nodes) ;
		for (let u of my_forbidden_nodes)
			forbidden_nodes.add(u);
		console.log("covered by r[" + rectangle_names[k] + "]=" + JSON.stringify(rr));
		for (let u of my_forbidden_nodes){
			console.log(u + "=" + JSON.stringify({x: u%M, y: Math.floor(u / M)}));
		}
	}
	console.log("forbiddenNodes:" + forbidden_nodes.size + "/" + N*M);

	var nr_troncons = [[],[]] ;
	compute_nr_troncons(polylines_data,
						polylines_partition,
						partition,
						excluded_partition,
						nr_troncons) ;
	console.log("nr_troncons=" + JSON.stringify(nr_troncons));

	var optimal_path=[] ;
	var selected_source=-1;
	var selected_target=-1 ;
	let [selected_source_, selected_target_] = dijkstra(sources,
														targets,
														forbidden_nodes,
														partition,
														nr_troncons,
														penalty_on_turn,
														penalty_on_crossing,
														optimal_path,
														selected_source,
														selected_target) ;
	selected_source = selected_source_;
	selected_target = selected_target_;
	
	console.log("selected_source=" + selected_source + " " + JSON.stringify({x:selected_source%M, y:Math.floor(selected_source/M)}));
	console.log("selected_target=" + selected_target + " " + JSON.stringify({x:selected_target%M, y:Math.floor(selected_target/M)}));
	console.log("optimal_path=" + JSON.stringify(optimal_path));
	
	if (selected_source==-1 || selected_target==-1)
	{
		var pi = center(rectangles[i]);
		var pj = center(rectangles[j]) ;
		troncons.push({
			direction: 0,/*Troncon::EAST_WEST*/
			rect: {left: Math.min(pi.x,pj.x), right: Math.max(pi.x,pj.x), top: pi.y, bottom: pi.y},
			sens: pj.x > pi.x ? +1/*Troncon::INCREASE*/ : -1/*Troncon::DECREASE*/
		}) ;
		tr = {
				direction : 1,/*Troncon::NORTH_SOUTH*/
				rect : {left: pj.x, right: pj.x, top: Math.min(pi.y,pj.y), bottom: Math.max(pi.y,pj.y)},
				sens : pj.y > pi.y ? +1/*Troncon::INCREASE*/ : -1/*Troncon::DECREASE*/
		}
		return ;
	}

	compute_troncons(selected_source,
					 selected_target,
					 optimal_path, 
					 partition, 
					 troncons) ;
	console.log("compute_troncons->" + JSON.stringify(troncons));
	console.log("rectangle[i]=" + JSON.stringify(rectangles[i]));
	console.log("rectangle[j]=" + JSON.stringify(rectangles[j]));
	console.log("rectangles=" + JSON.stringify(rectangles));
	console.log("expanded_frame=" + JSON.stringify(expanded_frame));
	
	troncons_dilatation(troncons,
						rectangles[i],
						rectangles[j],
						rectangles, 
						expanded_frame) ;
						
	console.log("troncons_dilatation->" + JSON.stringify(troncons))
}


function replace_polyline(polylines_data,
						  polylines_partition,
						  p,
						  troncons)
{
	var my_polylines_data = [];
	var start_pos = polylines_partition[p].startPosFromPartition(p) ;
	var end_pos = polylines_partition[p+1].endPosFromPartition(p) ;
	for (var i=0; i < start_pos; i++)
		my_polylines_data.push(polylines_data[i]);
	for (var i=0; i < troncons.length; i++)
		my_polylines_data.push(troncons[i]);
	for (var i=end_pos; i < polylines_data.length; i++)
		my_polylines_data.push(polylines_data[i]);

	polylines_partition.ResizePartition(p, troncons.size()) ;
	polylines_data = my_polylines_data ;
	for (var pos=polylines_partition.startPosFromPartition(p); pos < polylines_partition.endPosFromPartition(p); pos++)
		polylines_data[pos].p = p ;

	set_up_polyline_links(polylines_data, polylines_partition) ;
}

function test_compute_polyline_1()
{
	var rectangles = [
		{"left":254,"right":387,"top":376,"bottom":448},
		{"left":651,"right":841,"top":280,"bottom":336},
		{"left":581,"right":736,"top":376,"bottom":416},
		{"left":881,"right":1057,"top":280,"bottom":336},
		{"left":832,"right":1057,"top":376,"bottom":432},
		{"left":776,"right":931,"top":184,"bottom":240},
		{"left":254,"right":416,"top":184,"bottom":240},
		{"left":481,"right":706,"top":143,"bottom":199},
		{"left":427,"right":540,"top":376,"bottom":432},
		{"left":580,"right":700,"top":456,"bottom":512},
		{"left":137,"right":214,"top":200,"bottom":272},
		{"left":831,"right":979,"top":472,"bottom":528},
		{"left":65,"right":212,"top":104,"bottom":160},
		{"left":254,"right":374,"top":280,"bottom":336},
		{"left":414,"right":541,"top":280,"bottom":336},
		{"left":253,"right":359,"top":488,"bottom":560},
		{"left":18,"right":165,"top":488,"bottom":576},
		{"left":18,"right":214,"top":312,"bottom":448}
	];

	compute_polyline(rectangles, rectangleFromIndex=1, rectangleToIndex=7);
}

function test_compute_polyline_3()
{
	var rectangle_names=[
		"AlignedBuffers",					//0
		"MDInboundConnectionHandler",		//1
		"MultiForwardChainLink",			//2
		"MulticastChannelListener",			//3
		"MulticastChannelListenerForward",	//4
		"MulticastLineListener",			//5
		"PacketizedSocketReader",			//6
		"PacketizedSocketReadingStrategy",	//7
		"ReadBufferChain",					//8
		"ReadBufferHeader",					//9
		"Reader",							//10
		"ReaderReceptionActor",				//11
		"SocketsUtilities",					//12
		"WriteBufferChain",					//13
		"WriteBufferHeader",				//14
		"ZeroCopyHolder",					//15
		"ZeroCopyRecvPacket",				//16
		"read_write_list"					//17
	];
	
	var rectangles=[
		{"left":246,"right":379,"top":282,"bottom":354},	//0
		{"left":643,"right":833,"top":186,"bottom":242},	//1
		{"left":619,"right":774,"top":281,"bottom":321},	//2
		{"left":873,"right":1049,"top":186,"bottom":242},	//3
		{"left":824,"right":1049,"top":282,"bottom":338},	//4
		{"left":768,"right":923,"top":90,"bottom":146},		//5
		{"left":246,"right":408,"top":90,"bottom":146},		//6
		{"left":460,"right":685,"top":90,"bottom":146},		//7
		{"left":419,"right":532,"top":282,"bottom":338},	//8
		{"left":572,"right":692,"top":362,"bottom":418},	//9
		{"left":129,"right":206,"top":106,"bottom":178},	//10
		{"left":823,"right":971,"top":378,"bottom":434},	//11
		{"left":57,"right":204,"top":10,"bottom":66},		//12
		{"left":246,"right":366,"top":186,"bottom":242},	//13
		{"left":406,"right":533,"top":186,"bottom":242},	//14
		{"left":245,"right":351,"top":394,"bottom":466},	//15
		{"left":10,"right":157,"top":394,"bottom":482},		//16
		{"left":10,"right":206,"top":218,"bottom":354}		//17
	];
	
/*
	rectangleFromIndex=14
	rectangleFromName=WriteBufferHeader
	rectangleToIndex=2
	RectangleToName=MultiForwardChainLink
*/
	compute_polyline(rectangle_names, rectangles, rectangleFromIndex=14, rectangleToIndex=2);
}

function test_troncons_dilatation_3()
{
	var troncons = [
		{
			"rect":{"left":533,"right":623,"top":198,"bottom":242},
			"direction":0,
			"sens":1
		 },
		 {
			"rect":{"left":619,"right":623,"top":198,"bottom":281},
			"direction":1,
			"sens":1
		}
	];
	
	var rectangles=[
		{"left":246,"right":379,"top":282,"bottom":354},
		{"left":643,"right":833,"top":186,"bottom":242},
		{"left":619,"right":774,"top":281,"bottom":321},
		{"left":873,"right":1049,"top":186,"bottom":242},
		{"left":824,"right":1049,"top":282,"bottom":338},
		{"left":768,"right":923,"top":90,"bottom":146},
		{"left":246,"right":408,"top":90,"bottom":146},
		{"left":460,"right":685,"top":90,"bottom":146},
		{"left":419,"right":532,"top":282,"bottom":338},
		{"left":572,"right":692,"top":362,"bottom":418},
		{"left":129,"right":206,"top":106,"bottom":178},
		{"left":823,"right":971,"top":378,"bottom":434},
		{"left":57,"right":204,"top":10,"bottom":66},
		{"left":246,"right":366,"top":186,"bottom":242},
		{"left":406,"right":533,"top":186,"bottom":242},
		{"left":245,"right":351,"top":394,"bottom":466},
		{"left":10,"right":157,"top":394,"bottom":482},
		{"left":10,"right":206,"top":218,"bottom":354}
	];

	var expanded_frame={"left":-40,"right":1099,"top":-40,"bottom":532};
	
	console.log("troncons=" + JSON.stringify(troncons));

	troncons_dilatation(troncons,
						rectangles[i=14],
						rectangles[j=2],
						rectangles, 
						expanded_frame) ;
						
	console.log("troncons=" + JSON.stringify(troncons));
}

function test_compute_polyline_2()
{
	var rectangle_names=[
		"AlignedBuffers",
		"MDInboundConnectionHandler",
		"MultiForwardChainLink",
		"MulticastChannelListener",
		"MulticastChannelListenerForward",
		"MulticastLineListener",
		"PacketizedSocketReader",
		"PacketizedSocketReadingStrategy",
		"ReadBufferChain",
		"ReadBufferHeader",
		"Reader",
		"ReaderReceptionActor",
		"SocketsUtilities",
		"WriteBufferChain",
		"WriteBufferHeader",
		"ZeroCopyHolder",
		"ZeroCopyRecvPacket",
		"read_write_list"];

	var rectangles=[
		{"left":246,"right":379,"top":282,"bottom":354},
		{"left":643,"right":833,"top":186,"bottom":242},
		{"left":573,"right":728,"top":282,"bottom":322},
		{"left":873,"right":1049,"top":186,"bottom":242},
		{"left":824,"right":1049,"top":282,"bottom":338},
		{"left":768,"right":923,"top":90,"bottom":146},
		{"left":246,"right":408,"top":90,"bottom":146},
		{"left":460,"right":685,"top":90,"bottom":146},
		{"left":419,"right":532,"top":282,"bottom":338},
		{"left":572,"right":692,"top":395,"bottom":451},
		{"left":129,"right":206,"top":106,"bottom":178},
		{"left":823,"right":971,"top":378,"bottom":434},
		{"left":57,"right":204,"top":10,"bottom":66},
		{"left":246,"right":366,"top":186,"bottom":242},
		{"left":406,"right":533,"top":186,"bottom":242},
		{"left":245,"right":351,"top":394,"bottom":466},
		{"left":10,"right":157,"top":394,"bottom":482},
		{"left":10,"right":206,"top":218,"bottom":354}
	]

	compute_polyline(rectangle_names, rectangles, rectangleFromIndex=8, rectangleToIndex=9);
}

function test() {
	
	var rectangle_names = ["bibi", "kiki"];
	
	var rectangles = [
		{left:30, right:110, top:30, bottom:110}, 
		{left:160, right:210, top:50, bottom:100}
	];
	var expanded_rectangles = JSON.parse(JSON.stringify(rectangles));
	for (i=0; i < expanded_rectangles.length; i++) {
		expandBy(expanded_rectangles[i], RECT_BORDER);
	}
	console.log("rectangles=" + JSON.stringify(rectangles));
	console.log("expanded_rectangles=" + JSON.stringify(expanded_rectangles));

	var frame = {left:0, right:400, top:0, bottom:200};
	var expanded_frame = JSON.parse(JSON.stringify(frame));
	expandBy(expanded_frame, FRAME_BORDER);
	console.log("expanded_frame=" + JSON.stringify(expanded_frame));
	var partition = [new MyPartitioning(), new MyPartitioning()];
	var i = 0;
	var j = 1;
	
	load_graph_data(rectangles,
					expanded_rectangles,
					frame,
					i,
					j,
					partition);
					
	console.log("partition=" + JSON.stringify(partition));
	
	var sourceNodes = new Set();
	compute_rectangle_nodes(partition, 
							rectangles[0],
							sourceNodes);
	console.log("sourceNodes=");
	for (let item of sourceNodes) console.log(item);
	
	var targetNodes = new Set();
	compute_rectangle_nodes(partition, 
							rectangles[1],
							targetNodes);
	console.log("targetNodes=");
	for (let item of targetNodes) console.log(item);
	
	var forbiddenNodes = new Set();
	
	var edge = {u:0,v:1,distance_v:1,direction:0/*Troncon::EAST_WEST*/,sens:1/*Troncon::INCREASE*/};
	var M = partition[0/*Troncon::EAST_WEST*/].GetPartitionCount();
	var N = partition[1/*Troncon::NORTH_SOUTH*/].GetPartitionCount();
	var nr_troncons=[new Array(N*M), new Array(N*M)];
	for (var i=0; i < 2; i++)
	{
		for (var j=0; j < N*M; j++)
		{
			nr_troncons[i][j]=0;
		}
	}
	console.log("nr_troncons=" + JSON.stringify(nr_troncons));
	var direction_from_predecessor = new Array(N*M);
	for (i=0; i < N*M; i++)
	{
		direction_from_predecessor[i]=0;
	}
	console.log("direction_from_predecessor=" + JSON.stringify(direction_from_predecessor));
	
	cost = edge_cost(edge,
					 partition,
					 nr_troncons,
					 direction_from_predecessor,
					 penalty_on_turn,
					 penalty_on_crossing);
	console.log("edge cost=" + cost);
	
	var optimal_path = [];
	var selected_source=-1;
	var selected_target=-1;
	
	var adj = adjacency_list(3, N, M);
	var b = can_shift(3, M, N, 0, +1);
	console.log("can_shift(3, M, N, 0, +1)->" + b);
	var b = can_shift(3, M, N, 0, -1);
	console.log("can_shift(3, M, N, 0, -1)->" + b);
	console.log("adjacency_list(3,N,M)=" + JSON.stringify(adj));

	var edges = [];
	edges.push({u:1,v:1,distance_v:-1,direction:0,sens:+1}) ;
	console.log("edges=" + JSON.stringify(edges));
	
	var v = shift(u=1, M, N, direction=0, sens=+1)
	console.log("shift(u=1, M, N, direction=0, sens=+1)->" + v);
	
	console.log("sourceNodes:");
	for (let item of sourceNodes) console.log(item);
	console.log("targetNodes:");
	for (let item of targetNodes) console.log(item);
	console.log("forbiddenNodes:");
	for (let item of forbiddenNodes) console.log(item);	
	
	let [selected_source_, selected_target_] = dijkstra(sourceNodes,
														 targetNodes,
														 forbiddenNodes,
														 partition,
														 nr_troncons,
														 penalty_on_turn,
														 penalty_on_crossing,
														 optimal_path,
														 selected_source,
														 selected_target);
			 
	console.log("optimal_path=" + JSON.stringify(optimal_path));
	console.log("selected_source=" + selected_source_);
	console.log("selected_target=" + selected_target_);
	selected_source = selected_source_;
	selected_target = selected_target_;
	
	var troncons = [];
	compute_troncons(selected_source,
					 selected_target,
					 optimal_path, 
					 partition, 
					 troncons);
					 
	console.log("troncons=" + JSON.stringify(troncons));
	
	troncons_dilatation(troncons,
						rectangles[0],
						rectangles[1],
						rectangles,
						frame);
						
	var excluded_partition = 0;
	var polylines_data=[];
	var polylines_partition = new MyPartitioning();
	compute_nr_troncons(polylines_data,
						polylines_partition,
						partition,
						excluded_partition,
						nr_troncons);
					
	var i=0;
	var j=1;
	console.log("rectangles=" + JSON.stringify(rectangles));
	console.log("expanded_rectangles" + JSON.stringify(expanded_rectangles));
	console.log("expanded_frame=" + JSON.stringify(expanded_frame));
	var troncons=[];
	var polylines_data=[];
	var polylines_partition = new MyPartitioning();
	compute_arc_troncons(rectangle_names,
						 rectangles,
						  expanded_rectangles,
						  expanded_frame,
						  polylines_data,
						  polylines_partition,
						  excluded_partition,
						  i,
						  j,
						  penalty_on_turn,
						  penalty_on_crossing,
						  troncons);
	console.log("polylines_data=" + JSON.stringify(polylines_data));
	console.log("polylines_partition=" + JSON.stringify(polylines_partition));
	console.log("troncons=" + JSON.stringify(troncons));
}