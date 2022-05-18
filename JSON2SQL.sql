CREATE DATABASE crec;
USE crec;


DECLARE @JSON varchar(max);

SELECT @JSON = BulkColumn 
FROM OPENROWSET (BULK 'C:\ludo\diagrams\test-lola-diagdata.json', SINGLE_CLOB) import;

SELECT box, boxComment
FROM OPENJSON(@JSON, '$.boxComments') WITH (
	box varchar(60) '$.box',
	boxComment varchar(500) '$.comment'
)

SELECT box, field, fieldComment
FROM OPENJSON(@JSON, '$.fieldComments') WITH (
	box varchar(60) '$.box',
	field varchar(60) '$.field',
	fieldComment varchar(500) '$.comment'
)

SELECT box, field, color
FROM OPENJSON(@JSON, '$.fieldColors') WITH (
	box varchar(60) '$.box',
	field varchar(60) '$.field',
	color varchar(20) '$.color'
)

DROP TABLE IF EXISTS #box_staging;

SELECT boxtitle, boxid, fieldname, fieldIsPrimaryKey, fieldIsForeignKey, ROW_NUMBER() OVER(PARTITION BY boxid ORDER BY (SELECT NULL)) AS field_position
INTO #box_staging
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

SELECT * FROM #box_staging;

DROP TABLE IF EXISTS boxes;

CREATE TABLE boxes(
	id INTEGER PRIMARY KEY CLUSTERED,
	title VARCHAR(60) UNIQUE,
	soft_deleled BIT
);

DROP TABLE IF EXISTS #boxes;

CREATE TABLE #boxes(
	id INTEGER PRIMARY KEY CLUSTERED,
	title VARCHAR(60) UNIQUE,
);

DROP TABLE IF EXISTS fields; 

CREATE TABLE fields(
	id INTEGER PRIMARY KEY CLUSTERED,
	boxid INTEGER,
	field_position INTEGER,-- position of field in box.fields array
	[name] VARCHAR(60),
	isPrimaryKey BIT,
	isForeignKey BIT,
	soft_deleted BIT,
	UNIQUE(boxid, [name]),
	UNIQUE(boxid, field_position),
	FOREIGN KEY (boxid) REFERENCES boxes(id)
);

DROP TABLE IF EXISTS #fields; 

CREATE TABLE #fields(
	id INTEGER PRIMARY KEY CLUSTERED,
	boxid INTEGER,
	field_position INTEGER,-- position of field in box.fields array
	[name] VARCHAR(60),
	isPrimaryKey BIT,
	isForeignKey BIT,
	UNIQUE(boxid, [name]),
	UNIQUE(boxid, field_position),
	FOREIGN KEY (boxid) REFERENCES #boxes(id)
);

WITH cte AS (
	SELECT DISTINCT boxid, boxtitle
	FROM #box_staging
)
INSERT INTO #boxes(id, title)
SELECT boxid, boxtitle
FROM cte
ORDER BY boxid;

INSERT INTO #fields(id, boxid, field_position, [name], isPrimaryKey, isForeignKey)
SELECT ROW_NUMBER() OVER(ORDER BY (SELECT NULL)) AS id, 
	boxid, 
	field_position,
	fieldname, 
	fieldIsPrimaryKey, 
	fieldIsForeignKey
FROM #box_staging
ORDER BY boxid, field_position;

SELECT * FROM boxes ORDER BY id;

