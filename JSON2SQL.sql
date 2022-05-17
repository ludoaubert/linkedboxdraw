CREATE DATABASE crec;
USE crec;


DECLARE @JSON varchar(max);

SELECT @JSON = BulkColumn 
FROM OPENROWSET (BULK 'C:\ludo\diagrams\test-lola-diagdata.json', SINGLE_CLOB) import;


DROP TABLE IF EXISTS #box_staging;

WITH cte_fields AS (
	SELECT boxtitle, boxid, fieldname, fieldIsPrimaryKey, fieldIsForeignKey, ROW_NUMBER() OVER(PARTITION BY boxid ORDER BY (SELECT NULL)) AS field_position
	FROM OPENJSON(@JSON, '$.boxes') WITH (
		[boxtitle] varchar(60) '$.title',
		[boxid] int '$.id',
		[fields] nvarchar(max) '$.fields' AS JSON
	) AS a
	CROSS APPLY OPENJSON(a.fields) WITH (
		[fieldname] varchar(60) '$.name',
		[fieldIsPrimaryKey] BIT '$.isPrimaryKey',
		[fieldIsForeignKey] BIT '$.isForeignKey'
	)
) , cte_box_comments AS (
	SELECT box, boxComment
	FROM OPENJSON(@JSON, '$.boxComments') WITH (
		box varchar(60) '$.box',
		boxComment varchar(500) '$.comment'
	)
) , cte_field_comments AS (
	SELECT box, field, fieldComment
	FROM OPENJSON(@JSON, '$.fieldComments') WITH (
		box varchar(60) '$.box',
		field varchar(60) '$.field',
		fieldComment varchar(500) '$.comment'
	)
) , cte_field_colors AS (
	SELECT box, field, color
	FROM OPENJSON(@JSON, '$.fieldColors') WITH (
		box varchar(60) '$.box',
		field varchar(60) '$.field',
		color varchar(20) '$.color'
	)
)
SELECT f.* , bc.boxComment, fc.fieldComment, fcl.color
INTO #box_staging
FROM cte_fields f
LEFT JOIN cte_box_comments bc ON f.boxtitle = bc.box
LEFT JOIN cte_field_comments fc ON f.boxtitle = fc.box AND f.fieldname = fc.field
LEFT JOIN cte_field_colors fcl ON f.boxtitle = fcl.box AND f.fieldname = fcl.field;

SELECT * FROM #box_staging;

DROP TABLE IF EXISTS boxes;

CREATE TABLE boxes(
	id INTEGER PRIMARY KEY CLUSTERED,
	title VARCHAR(60) UNIQUE
);

DROP TABLE IF EXISTS fields; 

CREATE TABLE fields(
	id INTEGER PRIMARY KEY CLUSTERED,
	boxid INTEGER,
	field_position INTEGER,-- position of field in box.fields array
	[name] VARCHAR(60),
	isPrimaryKey BIT,
	isForeignKey BIT,
	UNIQUE(boxid, [name]),
	UNIQUE(boxid, field_position),
	FOREIGN KEY (boxid) REFERENCES boxes(id)
);

WITH cte AS (
	SELECT DISTINCT boxid, boxtitle
	FROM #box_staging
)
INSERT INTO boxes(id, title)
SELECT boxid, boxtitle
FROM cte
ORDER BY boxid;

INSERT INTO fields(id, boxid, field_position, [name], isPrimaryKey, isForeignKey)
SELECT ROW_NUMBER() OVER(ORDER BY (SELECT NULL)) AS id, 
	boxid, 
	field_position,
	fieldname, 
	fieldIsPrimaryKey, 
	fieldIsForeignKey
FROM #box_staging
ORDER BY boxid, field_position;

SELECT * FROM boxes ORDER BY id;

