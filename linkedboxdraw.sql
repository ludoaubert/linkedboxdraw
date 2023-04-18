PRAGMA foreign_keys = ON;


CREATE TABLE diagram(
    id INTEGER PRIMARY KEY,
    title VARCHAR(100),
    deleted INTEGER,
    guid CHAR(16)
);

CREATE TABLE user(
    id INTEGER PRIMARY KEY,
    firstName VARCHAR(60),
    lastName VARCHAR(60),
    guid CHAR(16)
);

CREATE TABLE diagram_user(
    id INTEGER PRIMARY KEY,
    userId INTEGER,
    diagramId INTEGER,
    FOREIGN KEY (userId) REFERENCES user(id),
    FOREIGN KEY (diagramId) REFERENCES diagram(id)
);

CREATE TABLE box(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    position INTEGER,
    title VARCHAR(100),
    deleted INTEGER,
    UNIQUE(diagramId, position),
    FOREIGN KEY (diagramId) REFERENCES diagram(id)
);

CREATE TABLE field(
    id INTEGER PRIMARY KEY,
    position INTEGER,
    boxPosition INTEGER NOT NULL,
    diagramId INTEGER NOT NULL,
    name varchar(100),
    isPrimaryKey INTEGER,
    isForeignKey INTEGER,
    fieldType varchar(10),
    deleted INTEGER,
    UNIQUE (diagramId, boxPosition, position),
    FOREIGN KEY (diagramId, boxPosition) REFERENCES box(diagramId, position)
);

CREATE TABLE picture(
	id INTEGER PRIMARY KEY,
	diagramId INTEGER,
	height INTEGER,
	width INTEGER,
	name varchar(100),
	base64 text NOT NULL
);


CREATE TABLE link(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    fromBoxPosition INTEGER,
    fromFieldPosition INTEGER,
    fromCardinality char,
    toBoxPosition INTEGER,
    toFieldPosition INTEGER,
    toCardinality char,
    category varchar(3),
    deleted INTEGER,
    FOREIGN KEY (diagramId, fromBoxPosition) REFERENCES box(diagramId, position),
    FOREIGN KEY (diagramId, toBoxPosition) REFERENCES box(diagramId, position),
    FOREIGN KEY (diagramId, fromBoxPosition, fromFieldPosition) REFERENCES field(diagramId, boxPosition, position),
    FOREIGN KEY (diagramId, toBoxPosition, toFieldPosition) REFERENCES field(diagramId, boxPosition, position)
);

CREATE TABLE document(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    diagData text,
    geoData text
);

SELECT * FROM document;


SELECT id, diagramId, diagData->'$.documentTitle', diagData FROM document;
SELECT id, diagramId, diagData->'$.boxes', diagData FROM document;

WITH cte_box AS (
    SELECT key AS boxPosition, value AS boxValue 
    FROM json_each((SELECT diagData FROM document WHERE id=1), '$.boxes')
)
SELECT * FROM cte_box;

SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1));

SELECT json_array_length((SELECT diagData FROM document WHERE id=1), '$.boxes');

SELECT value FROM generate_series(5,100,5); --error


INSERT INTO diagram(id, title, deleted)
VALUES(1, 'CV Ludovic Aubert', 0);

UPDATE diagram SET guid='a8828ddfef224d36935a1c66ae86ebb3' WHERE id=1;

SELECT * FROM diagram;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_box AS (
    SELECT cte_boxPosition.value AS boxPosition, '$.boxes[' || cte_boxPosition.value || ']' AS path
    FROM cte_series AS cte_boxPosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1))
), cte_boxes AS (
    SELECT cte_tree.*, cte_box.boxPosition 
    FROM cte_tree
    JOIN cte_box ON cte_tree.path = cte_box.path
), cte_boxes_pivot AS (
    SELECT boxPosition,
            MAX(case when key = 'title' then value end) as title        
    FROM cte_boxes
    GROUP BY boxPosition
    ORDER BY boxPosition
)
INSERT INTO box(diagramId, position, title, deleted)
SELECT 1 AS diagramId, boxPosition, title, 0 AS deleted 
FROM cte_boxes_pivot;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_bf AS (
    SELECT cte_boxPosition.value AS boxPosition, cte_fieldPosition.value AS fieldPosition, '$.boxes[' || cte_boxPosition.value || '].fields[' || cte_fieldPosition.value || ']' AS path
    FROM cte_series AS cte_boxPosition
    CROSS JOIN cte_series AS cte_fieldPosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1))
), cte_fields AS (
    SELECT cte_tree.*, cte_bf.boxPosition, cte_bf.fieldPosition 
    FROM cte_tree
    JOIN cte_bf ON cte_tree.path = cte_bf.path
), cte_fields_pivot AS (
    SELECT boxPosition, fieldPosition,
            MAX(case when key = 'name' then value end) as name,
            MAX(case when key = 'isPrimaryKey' then value end) as isPrimaryKey,
            MAX(case when key = 'isForeignKey' then value end) as isForeignKey,
            MAX(case when key = 'type' then value end) as type            
    FROM cte_fields
    GROUP BY boxPosition, fieldPosition
    ORDER BY boxPosition, fieldPosition
)
INSERT INTO field(position, boxPosition, diagramId, name, isPrimaryKey, isForeignKey, fieldType, deleted)
SELECT fieldPosition, boxPosition, 1 AS diagramId, name, isPrimaryKey, isForeignKey, type, 0 AS deleted
FROM cte_fields_pivot;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_link AS (
    SELECT cte_linkPosition.value AS linkPosition, '$.links[' || cte_linkPosition.value || ']' AS path
    FROM cte_series AS cte_linkPosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1))
), cte_links AS (
    SELECT cte_tree.*, cte_link.linkPosition 
    FROM cte_tree
    JOIN cte_link ON cte_tree.path = cte_link.path
), cte_links_pivot AS (
    SELECT linkPosition,
            MAX(case when key = 'from' then value end) as from_,
            MAX(case when key = 'fromField' then value end) as fromField,
            MAX(case when key = 'fromCardinality' then value end) as fromCardinality,
            MAX(case when key = 'to' then value end) as to_,
            MAX(case when key = 'toField' then value end) as toField,
            MAX(case when key = 'toCardinality' then value end) as toCardinality,
            MAX(case when key = 'category' then value end) as category                    
    FROM cte_links
    GROUP BY linkPosition
    ORDER BY linkPosition
)
INSERT INTO link(diagramId, fromBoxPosition, fromFieldPosition, fromCardinality, toBoxPosition, toFieldPosition, toCardinality, category, deleted)
SELECT 1 AS diagramId, from_ AS fromBoxPosition, fromField AS fromFieldPosition, fromCardinality, to_ AS toBoxPosition, toField AS toFieldPosition, toCardinality, category, 0 AS deleted 
FROM cte_links_pivot;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_picture AS (
    SELECT cte_picturePosition.value AS picturePosition, '$.pictures[' || cte_picturePosition.value || ']' AS path
    FROM cte_series AS cte_picturePosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1))
), cte_pictures AS (
    SELECT cte_tree.*, cte_picture.picturePosition 
    FROM cte_tree
    JOIN cte_picture ON cte_tree.path = cte_picture.path
) , cte_pictures_pivot AS (
    SELECT picturePosition,
            MAX(case when key = 'height' then value end) as height,
            MAX(case when key = 'width' then value end) as width,
            MAX(case when key = 'name' then value end) as name,
            MAX(case when key = 'base64' then value end) as base64        
    FROM cte_pictures
    GROUP BY picturePosition
    ORDER BY picturePosition
)
INSERT INTO picture(diagramId, height, width, name, base64)
SELECT 1 AS diagramId, height, width, name, base64
FROM cte_pictures_pivot;

--reproducing JSON from tables: