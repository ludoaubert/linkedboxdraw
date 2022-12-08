
WITH cte_fk AS (
-- stay away from INFORMATION_SCHEMA on SQL Server (Aaron Bertrand)
	SELECT  obj.name AS FK_NAME,
		sch.name AS [schema_name],
		tab1.name AS [table],
		col1.name AS [column],
		tab2.name AS [referenced_table],
		col2.name AS [referenced_column]
	FROM sys.foreign_key_columns fkc
	INNER JOIN sys.objects obj
		ON obj.object_id = fkc.constraint_object_id
	INNER JOIN sys.tables tab1
		ON tab1.object_id = fkc.parent_object_id
	INNER JOIN sys.schemas sch
		ON tab1.schema_id = sch.schema_id
	INNER JOIN sys.columns col1
		ON col1.column_id = parent_column_id AND col1.object_id = tab1.object_id
	INNER JOIN sys.tables tab2
		ON tab2.object_id = fkc.referenced_object_id
	INNER JOIN sys.columns col2
		ON col2.column_id = referenced_column_id AND col2.object_id = tab2.object_id
), cte_pk AS (
	SELECT Tab.CONSTRAINT_SCHEMA AS [schema_name],
		Tab.CONSTRAINT_NAME AS [pk_name],
		Col.Column_Name AS [columns],
		Col.Table_Name AS [table_name]
	FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS Tab 
	JOIN INFORMATION_SCHEMA.CONSTRAINT_COLUMN_USAGE Col ON Col.Constraint_Name = Tab.Constraint_Name
		AND Col.Table_Name = Tab.Table_Name 
	WHERE Tab.Constraint_Type = 'PRIMARY KEY'
) , cte_table_list AS (
	SELECT table_name, ROW_NUMBER() OVER (ORDER BY table_name) AS rn
	FROM INFORMATION_SCHEMA.TABLES
	--WHERE EXISTS (SELECT * FROM cte_fk WHERE TABLE_NAME IN ([table], [referenced_table]))
) , cte_table_column_list AS (
	SELECT c.table_name, c.column_name, tl.rn AS rn_table,
		ROW_NUMBER() OVER (PARTITION BY c.table_name ORDER BY MAX(pk.pk_name) DESC, MAX(fk_dest.FK_NAME) DESC, MAX(fk_orig.FK_NAME) DESC, c.column_name) AS rn_column
	FROM INFORMATION_SCHEMA.COLUMNS c
	JOIN cte_table_list tl ON c.TABLE_NAME=tl.TABLE_NAME
	LEFT JOIN cte_pk pk ON c.TABLE_NAME=pk.table_name AND c.COLUMN_NAME = pk.[columns]
	LEFT JOIN cte_fk fk_orig ON c.TABLE_NAME=fk_orig.[table] AND c.COLUMN_NAME = fk_orig.[column]
	LEFT JOIN cte_fk fk_dest ON c.TABLE_NAME=fk_dest.[referenced_table] AND c.COLUMN_NAME = fk_dest.[referenced_column]
	GROUP BY c.table_name, c.column_name, tl.rn
) , cte_fields AS (
	SELECT tl.table_name AS title, tl.rn-1 AS id, (
		SELECT tc.COLUMN_NAME AS [name], 
			CAST(CASE WHEN pk.pk_name IS NULL THEN 0 ELSE 1 END AS BIT) AS isPrimaryKey,
			CAST(CASE WHEN fk.FK_NAME IS NULL THEN 0 ELSE 1 END AS BIT) AS isForeignKey
		FROM cte_table_column_list tc
		LEFT JOIN cte_pk pk ON tc.TABLE_NAME=pk.table_name AND tc.COLUMN_NAME = pk.[columns]
		LEFT JOIN cte_fk fk ON tc.TABLE_NAME=fk.[table] AND tc.COLUMN_NAME = fk.[column]
		WHERE tc.TABLE_NAME = tl.table_name
		ORDER BY tc.rn_column
		FOR JSON PATH
	) AS fields 
	FROM cte_table_list tl
),  cte_links AS (
		SELECT tcl_from.rn_table - 1 AS [from],	-- zero based
			tcl_from.rn_column - 1 AS fromField, -- zero based
			'' AS fromCardinality,
			tcl_to.rn_table - 1 AS [to],	-- zero based
			tcl_to.rn_column - 1 AS toField,-- zero based
			'' AS toCardinality
		FROM cte_fk fk
		JOIN cte_table_column_list tcl_from ON fk.[table]=tcl_from.TABLE_NAME AND fk.[column]=tcl_from.COLUMN_NAME
		JOIN cte_table_column_list tcl_to ON fk.referenced_table=tcl_to.TABLE_NAME AND fk.referenced_column=tcl_to.COLUMN_NAME
),	cte_tr2 AS (
		SELECT l1.[from], l2.[to]
		FROM cte_links l1
		JOIN cte_links l2 ON l1.[to] = l2.[from]
),  cte_diagdata AS (
	SELECT DB_NAME() AS documentTitle,
	(
		SELECT *
		FROM cte_fields
		ORDER BY title
		FOR JSON PATH
	) AS boxes,
	(
		SELECT l.*,
			CASE
				WHEN EXISTS(SELECT * FROM cte_tr2 WHERE cte_tr2.[from]=l.[from] AND cte_tr2.[to]=l.[to])
					THEN 'TR2'
				ELSE ''
			END AS category
		FROM cte_links l
		FOR JSON PATH
	) AS links,
	json_query('[]') AS [values],
	json_query('[]') AS boxComments,
	json_query('[]') AS fieldComments,
	json_query('[]') AS fieldColors
)
SELECT *
FROM cte_diagdata
FOR JSON PATH, WITHOUT_ARRAY_WRAPPER;
