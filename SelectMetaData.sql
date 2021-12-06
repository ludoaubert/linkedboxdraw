
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
	SELECT table_name
	FROM INFORMATION_SCHEMA.TABLES
	WHERE EXISTS (SELECT * FROM cte_fk WHERE TABLE_NAME IN ([table], [referenced_table]))
) , cte_table_column_list AS (
	SELECT c.table_name, c.column_name 
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
)
SELECT tc.TABLE_NAME, tc.COLUMN_NAME, pk.pk_name, fk.FK_NAME
FROM cte_table_column_list tc
LEFT JOIN cte_pk pk ON tc.TABLE_NAME=pk.table_name AND tc.COLUMN_NAME = pk.[columns]
LEFT JOIN cte_fk fk ON tc.TABLE_NAME=fk.[table] AND tc.COLUMN_NAME = fk.[column]
GROUP BY tc.TABLE_NAME