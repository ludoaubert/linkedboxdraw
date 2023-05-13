import {mydata} from "./table_edit.js";
export {compute_box_rectangle};

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

function compute_column_width(field, key_distrib)
{
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
	
	return column_width;
}

function compute_box_rectangle(box)
{
	const {title,id,fields} = box;

	const key_distrib = compute_key_distrib(fields) ;

	const widths = [
		2*4 + title.length * MONOSPACE_FONT_PIXEL_WIDTH,
		fields.filter(field => field?.type == undefined)
				.map(field => compute_column_width(field, key_distrib) * MONOSPACE_FONT_PIXEL_WIDTH),
		fields.filter(field => field?.type == "image")
				.map(field => field.name)
				.map(name => mydata.pictures.find(pic => pic.name==name))
				.map(pic => pic.width)
	].flat();
	
	const max_width = Math.max(...widths);
	
	const heights = [
		8 + CHAR_RECT_HEIGHT,
		fields.filter(field => field?.type == undefined)
				.map(field => CHAR_RECT_HEIGHT),
		fields.filter(field => field?.type == "image")
				.map(field => field.name)
				.map(name => mydata.pictures.find(pic => pic.name==name))
				.map(pic => pic.height)
	].flat()

	const height = heights.reduce((a,b) => a + b, 0);

	const rec = {
		left:0,
		right:max_width,
		top:0,
		bottom:Math.min(height, RECTANGLE_BOTTOM_CAP)
	} ;

	return rec;
}

