WITH cte_fk AS (
        SELECT fks.constraint_name as "FK_NAME",
        fks.constraint_schema as "schema_name",
     fks.table_name as "table",
       fks.referenced_table_name as "referenced_table",
       kcu.column_name as "column",
       kcu.referenced_column_name as "referenced_column"
from information_schema.referential_constraints fks
join information_schema.key_column_usage kcu
     on fks.constraint_schema = kcu.table_schema
     and fks.table_name = kcu.table_name
     and fks.constraint_name = kcu.constraint_name
where kcu.table_schema = "DilabProject"
)
, cte_pk AS (
select tco.table_schema as "schema_name",
       tco.constraint_name as "pk_name",
       kcu.column_name as "columns",
       tco.table_name as "table_name"
from information_schema.table_constraints tco
join information_schema.key_column_usage kcu
     on tco.constraint_schema = kcu.constraint_schema
     and tco.constraint_name = kcu.constraint_name
     and tco.table_name = kcu.table_name
where tco.constraint_type = 'PRIMARY KEY'
      and tco.table_schema = "DilabProject"
) , cte_table_list AS (
	SELECT table_name, ROW_NUMBER() OVER (ORDER BY table_name) AS rn
	FROM INFORMATION_SCHEMA.TABLES
	WHERE EXISTS (SELECT * FROM cte_fk WHERE TABLE_NAME IN (cte_fk.table, cte_fk.referenced_table))
) , cte_table_column_list AS (
	SELECT c.table_name, c.column_name, tl.rn AS rn_table,
		ROW_NUMBER() OVER (PARTITION BY c.table_name ORDER BY pk.pk_name DESC, fk.FK_NAME DESC, c.column_name) AS rn_column
	FROM INFORMATION_SCHEMA.COLUMNS c
	JOIN cte_table_list tl ON c.TABLE_NAME=tl.TABLE_NAME
	LEFT JOIN cte_pk pk ON c.TABLE_NAME=pk.table_name AND c.COLUMN_NAME = pk.columns
	LEFT JOIN cte_fk fk ON c.TABLE_NAME=fk.table AND c.COLUMN_NAME = fk.column
), cte_fields AS (
	SELECT tl.table_name AS title, tl.rn-1 AS id, (
		SELECT CONCAT('[',GROUP_CONCAT(JSON_OBJECT( 'name',tc.COLUMN_NAME, 
			'isPrimaryKey',CASE WHEN pk.pk_name IS NULL THEN false ELSE true END,
			"isForeignKey",CASE WHEN fk.FK_NAME IS NULL THEN false ELSE true END)),']') AS "fields"
		FROM cte_table_column_list tc
		LEFT JOIN cte_pk pk ON tc.TABLE_NAME=pk.table_name AND tc.COLUMN_NAME = pk.columns
		LEFT JOIN cte_fk fk ON tc.TABLE_NAME=fk.table AND tc.COLUMN_NAME = fk.column
		WHERE tc.TABLE_NAME = tl.table_name
		ORDER BY tc.rn_column
	) AS fields 
	FROM cte_table_list tl
),  cte_rectangle_constants AS (
	SELECT 7 AS MONOSPACE_FONT_PIXEL_WIDTH,
		16 AS CHAR_RECT_HEIGHT,	-- in reality 14,8 + 1 + 1 (top and bottom padding) = 16,8
		200 AS RECTANGLE_BOTTOM_CAP
), cte_rectangles_staging AS (
	SELECT tc.TABLE_NAME,
		CASE 
			WHEN pk.pk_name IS NOT NULL AND fk.FK_NAME IS NOT NULL
				THEN (LENGTH('PK FK ') + LENGTH(column_name)) * MONOSPACE_FONT_PIXEL_WIDTH
			WHEN pk.pk_name IS NOT NULL AND fk.FK_NAME IS NULL
				THEN (LENGTH('PK ') + LENGTH(column_name)) * MONOSPACE_FONT_PIXEL_WIDTH
			WHEN pk.pk_name IS NULL AND fk.FK_NAME IS NOT NULL
				THEN (LENGTH('FK ') + LENGTH(column_name)) * MONOSPACE_FONT_PIXEL_WIDTH
			WHEN pk.pk_name IS NULL AND fk.FK_NAME IS NULL
				THEN LENGTH(column_name) * MONOSPACE_FONT_PIXEL_WIDTH
		END AS column_width
	FROM cte_table_column_list tc
	LEFT JOIN cte_pk pk ON tc.TABLE_NAME=pk.table_name AND tc.COLUMN_NAME = pk.columns
	LEFT JOIN cte_fk fk ON tc.TABLE_NAME=fk.table AND tc.COLUMN_NAME = fk.column
	CROSS JOIN cte_rectangle_constants

	UNION ALL

	SELECT table_name, 2*4 + LENGTH(table_name) * MONOSPACE_FONT_PIXEL_WIDTH
	FROM cte_table_list 
	CROSS JOIN cte_rectangle_constants
), cte_rectangles AS (
	SELECT tl.TABLE_NAME,
		0 AS "left", 
		(
			SELECT MAX(column_width)
			FROM cte_rectangles_staging rs 
			WHERE rs.table_name = tl.table_name
		) AS "right",
		0 AS "top",
		(
			SELECT 8 + CHAR_RECT_HEIGHT * (COUNT(*) + 1)
			FROM cte_table_column_list tcl 
			WHERE tcl.table_name = tl.table_name
		) AS "bottom"
	FROM cte_table_list tl
	CROSS JOIN cte_rectangle_constants rc
), cte_diagdata AS (
	SELECT 'DilabProject' AS documentTitle,
	(
		SELECT CONCAT('[',GROUP_CONCAT(JSON_OBJECT("title",cte_fields.title,
        "id",id,
        "fields",cte_fields.fields)),
        ']')
		FROM cte_fields
		ORDER BY title
	) AS boxes,
	(
		SELECT CONCAT('[',GROUP_CONCAT(JSON_OBJECT("from", tcl_from.rn_table - 1,	-- zero based
			"fromField",tcl_from.rn_column - 1, -- zero based
			"fromCardinality", NULL, 
			"to", tcl_to.rn_table - 1,	-- zero based
			"toField", tcl_to.rn_column - 1,-- zero based
			"toCardinality", NULL)),']')
		FROM cte_fk fk
		JOIN cte_table_column_list tcl_from ON fk.table=tcl_from.TABLE_NAME AND fk.column=tcl_from.COLUMN_NAME
		JOIN cte_table_column_list tcl_to ON fk.referenced_table=tcl_to.TABLE_NAME AND fk.referenced_column=tcl_to.COLUMN_NAME
	) AS links,
	(
		SELECT CONCAT('[',GROUP_CONCAT(JSON_OBJECT(
        "left", cte_rectangles.left,
        "right", cte_rectangles.right,
        "top", cte_rectangles.top,
        "bottom",
			CASE 
				WHEN cte_rectangles.bottom > RECTANGLE_BOTTOM_CAP 
					THEN RECTANGLE_BOTTOM_CAP 
				ELSE cte_rectangles.bottom 
			END)),']')
		FROM cte_rectangles 
		CROSS JOIN cte_rectangle_constants
		ORDER BY TABLE_NAME
	) AS rectangles,
	json_query('{ "key1" : [] }','$.key1') AS "values",
	json_query('{ "key1" : [] }','$.key1') AS "boxComments",
	json_query('{ "key1" : [] }','$.key1') AS "fieldComments",
	json_query('{ "key1" : [] }','$.key1') AS "fieldColors"
)
SELECT 
GROUP_CONCAT(JSON_OBJECT(
'documentTitle', documentTitle,
"boxes",boxes,
"links",links,
"rectangles",rectangles,
"values", cte_diagdata.values,
"boxComments",boxComments,
"fieldComments",fieldComments,
"fieldColors",fieldColors))
FROM cte_diagdata;
