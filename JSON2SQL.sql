CREATE DATABASE crec;
USE crec;


DECLARE @JSON varchar(max);

SELECT @JSON = BulkColumn 
FROM OPENROWSET (BULK 'C:\ludo\diagrams\test-lola-diagdata.json', SINGLE_CLOB) import;


WITH cte_fields AS (
	SELECT boxtitle, boxid, fields, fieldname, fieldIsPrimaryKey, fieldIsForeignKey
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
FROM cte_fields f
LEFT JOIN cte_box_comments bc ON f.boxtitle = bc.box
LEFT JOIN cte_field_comments fc ON f.boxtitle = fc.box AND f.fieldname = fc.field
LEFT JOIN cte_field_colors fcl ON f.boxtitle = fcl.box AND f.fieldname = fcl.field;

