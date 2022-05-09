

function compute_key_distrib(fields)
{
	var key_distrib = {"PK":0,"FK":0,"PKFK":0};
	
	for (let {name,isPrimaryKey,isForeignKey} of fields)
	{
		if (isPrimaryKey && isForeignKey)
			key_distrib["PKFK"]++;
		else if (isPrimaryKey)
			key_distrib["PK"]++;
		else if (isForeignKey)
			key_distrib["FK"]++;
	}

	return key_distrib;
}

const MONOSPACE_FONT_PIXEL_WIDTH=7;
const CHAR_RECT_HEIGHT=16;	// in reality 14,8 + 1 + 1 (top and bottom padding) = 16,8
const RECTANGLE_BOTTOM_CAP=200;

function compute_box_rectangles(boxes)
{
	var rectangles = []
	for (const {title,id,fields} of boxes)
	{
		var key_distrib = compute_key_distrib(fields) ;

		var nr_col = 0 ;
		var width = 2*4 + title.length * MONOSPACE_FONT_PIXEL_WIDTH ;
		var max_width = width;
		
		for (const field of fields)
		{
			nr_col++ ;
			const column_name = field.name;

			var column_width=0;

			if (key_distrib["PKFK"])
			{
		//at least one 'PK FK' is present
				column_width = ("PK FK " + column_name).length ;
			}
			else if (key_distrib["PK"] || key_distrib["FK"])
			{
		//no 'PK FK' is present, but at least one PK|FK is present.
				column_width = ("PK " + column_name).length ;
			}
			else
			{
		//no 'PK FK' is present. no PK|FK either.
				column_width = column_name.length ;
			}

			max_width = Math.max(column_width * MONOSPACE_FONT_PIXEL_WIDTH, max_width);
		}
		
		const bottom = 8 + CHAR_RECT_HEIGHT * (nr_col+1) ;

		rectangles.push({"left":0, "right":max_width, "top":0, "bottom":Math.min(bottom, RECTANGLE_BOTTOM_CAP)}) ;
	}
	return rectangles;
}
