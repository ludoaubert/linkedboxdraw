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
		ROW_NUMBER() OVER (PARTITION BY c.table_name ORDER BY MAX(pk.pk_name) DESC, MAX(fk_dest.FK_NAME) DESC, MAX(fk_orig.FK_NAME) DESC, c.column_name) AS rn_column
	FROM INFORMATION_SCHEMA.COLUMNS c
	JOIN cte_table_list tl ON c.TABLE_NAME=tl.TABLE_NAME
	LEFT JOIN cte_pk pk ON c.TABLE_NAME=pk.table_name AND c.COLUMN_NAME = pk.columns
	LEFT JOIN cte_fk fk_orig ON c.TABLE_NAME=fk_orig.table AND c.COLUMN_NAME = fk_orig.column
	LEFT JOIN cte_fk fk_dest ON c.TABLE_NAME=fk_dest.referenced_table AND c.COLUMN_NAME = fk_dest.referenced_column
	GROUP BY c.table_name, c.column_name, tl.rn
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
),  cte_links AS (
		SELECT tcl_from.rn_table - 1 AS [from],	-- zero based
			tcl_from.rn_column - 1 AS fromField, -- zero based
			NULL AS fromCardinality,
			tcl_to.rn_table - 1 AS [to],	-- zero based
			tcl_to.rn_column - 1 AS toField,-- zero based
			NULL AS toCardinality
		FROM cte_fk fk
		JOIN cte_table_column_list tcl_from ON fk.[table]=tcl_from.TABLE_NAME AND fk.[column]=tcl_from.COLUMN_NAME
		JOIN cte_table_column_list tcl_to ON fk.referenced_table=tcl_to.TABLE_NAME AND fk.referenced_column=tcl_to.COLUMN_NAME
),	cte_tr2 AS (
		SELECT l1.[from], l2.[to]
		FROM cte_links l1
		JOIN cte_links l2 ON l1.[to] = l2.[from]
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
		SELECT CONCAT('[',GROUP_CONCAT(JSON_OBJECT("from", l.[from],
			"fromField",l.[fromField],
			"fromCardinality", NULL, 
			"to", l.[to],
			"toField", l.[toField],
			"toCardinality", NULL,
			"Category",CASE
						WHEN EXISTS(SELECT * FROM cte_tr2 WHERE cte_tr2.[from]=l.[from] AND cte_tr2.[to]=l.[to])
							THEN 'TR2'
						ELSE ''
						END
			)),']')
		FROM cte_links l
	) AS links,
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
"values", cte_diagdata.values,
"boxComments",boxComments,
"fieldComments",fieldComments,
"fieldColors",fieldColors))
FROM cte_diagdata;
