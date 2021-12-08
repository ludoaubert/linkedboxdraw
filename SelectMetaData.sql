
WITH cte_fk AS (
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
) , cte_table_list AS (
	SELECT table_name, ROW_NUMBER() OVER (ORDER BY table_name) AS rn
	FROM INFORMATION_SCHEMA.TABLES
	WHERE EXISTS (SELECT * FROM cte_fk WHERE TABLE_NAME IN ([table], [referenced_table]))
) , cte_table_column_list AS (
	SELECT c.table_name, c.column_name, tl.rn AS rn_table,
		ROW_NUMBER() OVER (PARTITION BY c.table_name ORDER BY c.column_name) AS rn_column
	FROM INFORMATION_SCHEMA.COLUMNS c
	JOIN cte_table_list tl ON c.TABLE_NAME=tl.TABLE_NAME
) , cte_pk AS (
	select schema_name(tab.schema_id) as [schema_name], 
		pk.[name] as pk_name,
		substring(column_names, 1, len(column_names)-1) as [columns],
		tab.[name] as table_name
	from sys.tables tab
		inner join sys.indexes pk
			on tab.object_id = pk.object_id 
			and pk.is_primary_key = 1
	   cross apply (select col.[name] + ', '
						from sys.index_columns ic
							inner join sys.columns col
								on ic.object_id = col.object_id
								and ic.column_id = col.column_id
						where ic.object_id = tab.object_id
							and ic.index_id = pk.index_id
								order by col.column_id
								for xml path ('') ) D (column_names)
) , cte_fields AS (
	SELECT tl.table_name AS title, (
		SELECT tc.COLUMN_NAME AS [name], 
			CAST(CASE WHEN pk.pk_name IS NULL THEN 0 ELSE 1 END AS BIT) AS isPrimaryKey,
			CAST(CASE WHEN fk.FK_NAME IS NULL THEN 0 ELSE 1 END AS BIT) AS isForeignKey
		FROM cte_table_column_list tc
		LEFT JOIN cte_pk pk ON tc.TABLE_NAME=pk.table_name AND tc.COLUMN_NAME = pk.[columns]
		LEFT JOIN cte_fk fk ON tc.TABLE_NAME=fk.[table] AND tc.COLUMN_NAME = fk.[column]
		WHERE tc.TABLE_NAME = tl.table_name
		ORDER BY tc.COLUMN_NAME
		FOR JSON PATH
	) AS fields 
	FROM cte_table_list tl
),  cte_rectangle_constants AS (
	SELECT 7 AS MONOSPACE_FONT_PIXEL_WIDTH,
		16 AS CHAR_RECT_HEIGHT	-- in reality 14,8 + 1 + 1 (top and bottom padding) = 16,8
),  cte_rectangles_staging AS (
	SELECT tc.TABLE_NAME,
		CASE 
			WHEN pk.pk_name IS NOT NULL AND fk.FK_NAME IS NOT NULL
				THEN (LEN('PK FK ') + LEN(column_name)) * MONOSPACE_FONT_PIXEL_WIDTH
			WHEN pk.pk_name IS NOT NULL AND fk.FK_NAME IS NULL
				THEN (LEN('PK ') + LEN(column_name)) * MONOSPACE_FONT_PIXEL_WIDTH
			WHEN pk.pk_name IS NULL AND fk.FK_NAME IS NOT NULL
				THEN (LEN('FK ') + LEN(column_name)) * MONOSPACE_FONT_PIXEL_WIDTH
		END AS column_width
	FROM cte_table_column_list tc
	LEFT JOIN cte_pk pk ON tc.TABLE_NAME=pk.table_name AND tc.COLUMN_NAME = pk.[columns]
	LEFT JOIN cte_fk fk ON tc.TABLE_NAME=fk.[table] AND tc.COLUMN_NAME = fk.[column]
	CROSS JOIN cte_rectangle_constants

	UNION ALL

	SELECT table_name, 2*4 + LEN(table_name) * MONOSPACE_FONT_PIXEL_WIDTH
	FROM cte_table_list 
	CROSS JOIN cte_rectangle_constants
), cte_rectangles AS (
	SELECT tl.TABLE_NAME,
		0 AS [left], 
		(
			SELECT MAX(column_width)
			FROM cte_rectangles_staging rs 
			WHERE rs.table_name = tl.table_name
		) AS [right],
		0 AS [top],
		(
			SELECT 8 + CHAR_RECT_HEIGHT * (COUNT(*) + 1)
			FROM cte_table_column_list tcl 
			WHERE tcl.table_name = tl.table_name
		) AS [bottom]
	FROM cte_table_list tl
	CROSS JOIN cte_rectangle_constants rc
) , cte_diagdata AS (
	SELECT (
		SELECT *
		FROM cte_fields
		ORDER BY title
		FOR JSON PATH
	) AS boxes,
	(
		SELECT tcl_from.rn_table AS [from],
			tcl_from.rn_column AS fromField,
			NULL AS fromCardinality,
			tcl_to.rn_table AS [to],
			tcl_to.rn_column AS toField,
			NULL AS toCardinality
		FROM cte_fk fk
		JOIN cte_table_column_list tcl_from ON fk.[table]=tcl_from.TABLE_NAME AND fk.[column]=tcl_from.COLUMN_NAME
		JOIN cte_table_column_list tcl_to ON fk.referenced_table=tcl_to.TABLE_NAME AND fk.referenced_column=tcl_to.COLUMN_NAME
		FOR JSON PATH
	) AS links,
	(
		SELECT [left], [right], [top], [bottom] 
		FROM cte_rectangles 
		ORDER BY TABLE_NAME
		FOR JSON PATH
	) AS rectangles
)
SELECT *
FROM cte_diagdata
FOR JSON PATH;