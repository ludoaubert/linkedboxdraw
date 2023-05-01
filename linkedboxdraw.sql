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

WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1))
) , cte_boxes AS (
    SELECT box.key AS boxPosition, box_attr.key, box_attr.value
    FROM cte_tree box
    JOIN cte_tree box_attr ON box_attr.parent=box.id
    WHERE box.path = '$.boxes' AND box_attr.key='title' 
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


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1))
), cte_fields AS (
    SELECT box_array_index.key AS boxPosition, field_array_index.key AS fieldPosition, field.key, field.value
    FROM cte_tree field
    JOIN cte_tree field_array_index ON field.parent = field_array_index.id
    JOIN cte_tree field_array ON field_array_index.parent = field_array.id
    JOIN cte_tree box_array_index ON field_array.parent = box_array_index.id
    WHERE field.key IN ('name', 'isPrimaryKey', 'isForeignKey', 'type')
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


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1))
), cte_links AS (
    SELECT link_array_index.key AS linkPosition, link_attr.key, link_attr.value
    FROM cte_tree link_attr
    JOIN cte_tree link_array_index ON link_attr.parent=link_array_index.id
    WHERE link_array_index.path='$.links'
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


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE id=1))
), cte_pictures AS (
    SELECT picture_array_index.key AS picturePosition, picture_attr.key, picture_attr.value
    FROM cte_tree picture_array_index
    JOIN cte_tree picture_attr ON picture_attr.parent=picture_array_index.id
    WHERE picture_array_index.path='$.pictures'
), cte_pictures_pivot AS (
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
WITH cte_fields AS (
    SELECT boxPosition, 
                        json_group_array(json_object('name',name,
                                                'isPrimaryKey',json(CASE isPrimaryKey WHEN 0 THEN 'false' WHEN 1 THEN 'true' END),
                                                'isForeignKey',json(CASE isForeignKey WHEN 0 THEN 'false' WHEN 1 THEN 'true' END))
                                    ) AS fields
    FROM field
    WHERE diagramId=1
    GROUP BY boxPosition
) , cte_boxes AS (
    SELECT json_group_array( json_object('title', title, 'id', position, 'fields', json(fields))) AS boxes  
    FROM box
    JOIN cte_fields ON box.position=cte_fields.boxPosition
    WHERE box.diagramId=1
) , cte_links AS (
    SELECT json_group_array( json_object('from',fromBoxPosition,'fromField',fromFieldPosition,'fromCardinality',fromCardinality,'to',toBoxPosition,'toField',toFieldPosition,'toCardinality',toCardinality,'category',category)) AS links
    FROM link
    WHERE diagramId=1
) ,cte_pictures AS (
    SELECT json_group_array( json_object('height', height, 'width', width, 'name', name, 'base64', base64)) AS pictures
    FROM picture
    WHERE diagramId=1
), cte_doc AS (
    SELECT json_object('documentTitle', diagram.title, 'boxes', json(cte_boxes.boxes), 'links', json(cte_links.links), 'pictures', json(cte_pictures.pictures)) AS document
    FROM diagram 
    CROSS JOIN cte_boxes
    CROSS JOIN cte_links
    CROSS JOIN cte_pictures
    WHERE diagram.id=1
)
SELECT document FROM cte_doc;

CREATE TABLE context(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    contextPosition INTEGER,
    UNIQUE(diagramId, contextPosition),
    FOREIGN KEY (diagramId) REFERENCES diagram(id)
);


CREATE TABLE translatedBoxes(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    contextPosition INTEGER,
    boxPosition INTEGER,
    translationX INTEGER,
    translationY INTEGER,
    UNIQUE(diagramId, boxPosition),
    FOREIGN KEY (diagramId, boxPosition) REFERENCES box(diagramId, position),
    FOREIGN KEY (diagramId, contextPosition) REFERENCES context(diagramId, contextPosition)
);

CREATE TABLE polyline(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    contextPosition INTEGER,
    polylinePosition INTEGER,
    [from] INTEGER,
    [to] INTEGER,
    UNIQUE(diagramId, contextPosition, polylinePosition),
    FOREIGN KEY (diagramId, [from]) REFERENCES box(diagramId, position),
    FOREIGN KEY (diagramId, [to]) REFERENCES box(diagramId, position),
    FOREIGN KEY (diagramId, contextPosition) REFERENCES context(diagramId, contextPosition)
);


CREATE TABLE point(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    contextPosition INTEGER,
    polylinePosition INTEGER,
    pointPosition INTEGER,
    x INTEGER,
    y INTEGER,
    FOREIGN KEY (diagramId, contextPosition, polylinePosition) REFERENCES polyline(diagramId, contextPosition, polylinePosition)
);

CREATE TABLE frame(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    contextPosition INTEGER,
    [LEFT] INTEGER,
    [RIGHT] INTEGER,
    TOP INTEGER,
    BOTTOM INTEGER,
    UNIQUE(diagramId, contextPosition),
    FOREIGN KEY (diagramId, contextPosition) REFERENCES context(diagramId, contextPosition)	
);


CREATE TABLE rectangle(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    boxPosition INTEGER,
    [LEFT] INTEGER,
    [RIGHT] INTEGER,
    TOP INTEGER,
    BOTTOM INTEGER,
    UNIQUE(diagramId, boxPosition),
    FOREIGN KEY (diagramId, boxPosition) REFERENCES box(diagramId, position)
);


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_rectangle AS (
    SELECT cte_rectanglePosition.value AS rectanglePosition, '$.rectangles[' || cte_rectanglePosition.value || ']' AS path
    FROM cte_series AS cte_rectanglePosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE id=1))
), cte_rectangles AS (
    SELECT cte_tree.*, cte_rectangle.rectanglePosition 
    FROM cte_tree
    JOIN cte_rectangle ON cte_tree.path = cte_rectangle.path
), cte_rectangles_pivot AS (
    SELECT rectanglePosition,
            MAX(case when key = 'left' then value end) as [left],
            MAX(case when key = 'right' then value end) as [right],
            MAX(case when key = 'top' then value end) as top,
            MAX(case when key = 'bottom' then value end) as bottom        
    FROM cte_rectangles
    GROUP BY rectanglePosition
    ORDER BY rectanglePosition
)
INSERT INTO rectangle(diagramId, boxPosition, [left], [right], top, bottom)
SELECT 1 AS diagramId, rectanglePosition, [left], [right], top, bottom
FROM cte_rectangles_pivot;

SELECT * FROM rectangle;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_context AS (
    SELECT cte_contextPosition.value AS contextPosition, '$.contexts[' || cte_contextPosition.value || ']' AS path
    FROM cte_series AS cte_contextPosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE id=1))
), cte_contexts AS (
    SELECT *
    FROM cte_context
    WHERE EXISTS (SELECT * FROM cte_tree WHERE cte_tree.path = cte_context.path)
) 
INSERT INTO context(diagramId, contextPosition)
SELECT 1 AS diagramId, contextPosition
FROM cte_contexts;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_frame AS (
    SELECT cte_contextPosition.value AS contextPosition, '$.contexts[' || cte_contextPosition.value || '].frame' AS path
    FROM cte_series AS cte_contextPosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE id=1))
), cte_frames AS (
    SELECT cte_tree.*, cte_frame.contextPosition 
    FROM cte_tree
    JOIN cte_frame ON cte_tree.path = cte_frame.path
), cte_frames_pivot AS (
    SELECT contextPosition,
            MAX(case when key = 'left' then value end) as [left],
            MAX(case when key = 'right' then value end) as [right],
            MAX(case when key = 'top' then value end) as top,
            MAX(case when key = 'bottom' then value end) as bottom        
    FROM cte_frames
    GROUP BY contextPosition
    ORDER BY contextPosition
)
INSERT INTO frame(diagramId, contextPosition, [left], [right], top, bottom)
SELECT 1 AS diagramId, contextPosition, [left], [right], top, bottom
FROM cte_frames_pivot;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_tb AS (
    SELECT cte_contextPosition.value AS contextPosition, cte_tbPosition.value AS tbPosition, '$.contexts[' || cte_contextPosition.value || '].translatedBoxes[' ||cte_tbPosition.value || ']' AS path
    FROM cte_series AS cte_contextPosition
    CROSS JOIN cte_series AS cte_tbPosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE id=1))
), cte_tbs AS (
    SELECT cte_tree.*, cte_tb.contextPosition, cte_tb.tbPosition
    FROM cte_tree
    JOIN cte_tb ON cte_tree.path = cte_tb.path
), cte_tbs_pivot AS (
    SELECT contextPosition, tbPosition,
            MAX(case when key = 'id' then value end) as [boxPosition],
            MAX(case when key = 'translation' then value end) as [translation]
    FROM cte_tbs
    GROUP BY contextPosition, tbPosition
    ORDER BY contextPosition, tbPosition
)
INSERT INTO translatedBoxes(diagramId, contextPosition, boxPosition, translationX, translationY)
SELECT 1 AS diagramId, contextPosition, boxPosition, translation->'$.x' AS translationX, translation->'$.y' AS translationY
FROM cte_tbs_pivot;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_polyline AS (
    SELECT cte_contextPosition.value AS contextPosition, cte_polylinePosition.value AS polylinePosition, '$.contexts[' || cte_contextPosition.value || '].links[' ||cte_polylinePosition.value || ']' AS path
    FROM cte_series AS cte_contextPosition
    CROSS JOIN cte_series AS cte_polylinePosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE id=1))
), cte_polylines AS (
    SELECT cte_tree.*, cte_polyline.contextPosition, cte_polyline.polylinePosition
    FROM cte_tree
    JOIN cte_polyline ON cte_tree.path = cte_polyline.path
), cte_polylines_pivot AS (
    SELECT contextPosition, polylinePosition,
            MAX(case when key = 'from' then value end) as [from],
            MAX(case when key = 'to' then value end) as [to]
    FROM cte_polylines
    GROUP BY contextPosition, polylinePosition
    ORDER BY contextPosition, polylinePosition
)
INSERT INTO polyline(diagramId, contextPosition, polylinePosition, [from], [to])
SELECT 1 AS diagramId, contextPosition, polylinePosition, [from], [to]
FROM cte_polylines_pivot;


WITH cte_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 100
), cte_short_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 10
), cte_very_short_series(value) AS (
    SELECT 0 
    UNION ALL
    SELECT value + 1
    FROM cte_series
    WHERE value + 1 <= 4
), cte_point AS (
    SELECT cte_contextPosition.value AS contextPosition, cte_polylinePosition.value AS polylinePosition, cte_pointPosition.value AS pointPosition, '$.contexts[' || cte_contextPosition.value || '].links[' ||cte_polylinePosition.value || '].polyline[' || cte_pointPosition.value ||']' AS path
    FROM cte_very_short_series AS cte_contextPosition
    CROSS JOIN cte_series AS cte_polylinePosition
    CROSS JOIN cte_short_series AS cte_pointPosition
), cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE id=1))
), cte_points AS (
    SELECT cte_tree.*, cte_point.contextPosition, cte_point.polylinePosition, cte_point.pointPosition
    FROM cte_tree
    JOIN cte_point ON cte_tree.path = cte_point.path
), cte_points_pivot AS (
    SELECT contextPosition, polylinePosition, pointPosition,
            MAX(case when key = 'x' then value end) as x,
            MAX(case when key = 'y' then value end) as y
    FROM cte_points
    GROUP BY contextPosition, polylinePosition, pointPosition
    ORDER BY contextPosition, polylinePosition, pointPosition
)
INSERT INTO point(diagramId, contextPosition, polylinePosition, pointPosition, x, y)
SELECT 1 AS diagramId, contextPosition, polylinePosition, pointPosition, x, y
FROM cte_points_pivot;

--reproducing JSON from tables:
--use json(cte_polylines.polyline) to avoid 
WITH cte_rectangles AS (
    SELECT json_group_array( json_object('left', [LEFT], 'right', [RIGHT], 'top', TOP, 'bottom', BOTTOM)) AS rectangles
    FROM rectangle
    WHERE diagramId=1
    GROUP BY diagramId
), cte_polylines AS (
    SELECT contextPosition, polylinePosition, json_group_array( json_object('x',x,'y',y)) AS polyline
    FROM point
    WHERE diagramId=1
    GROUP BY contextPosition, polylinePosition
), cte_geo_links AS (
    SELECT cte_polylines.contextPosition, json_group_array( json_object('polyline', json(cte_polylines.polyline), 'from', polyline.[from], 'to', polyline.[to])) AS links
    FROM cte_polylines
    JOIN polyline ON cte_polylines.contextPosition=polyline.contextPosition AND cte_polylines.polylinePosition=polyline.polylinePosition
    WHERE polyline.diagramId=1
    GROUP BY cte_polylines.contextPosition
), cte_tb AS (
    SELECT contextPosition, json_group_array(json_object('id', boxPosition, 'translation', json_object('x', translationX, 'y', translationY))) AS tb
    FROM translatedBoxes
    WHERE diagramId=1
    GROUP BY contextPosition
), cte_frame AS (
    SELECT contextPosition, json_object('left', [LEFT], 'right', [RIGHT], 'top', TOP, 'bottom', BOTTOM) AS frame
    FROM frame
    WHERE diagramId=1
), cte_contexts AS (
    SELECT json_group_array(json_object('frame', json(cte_frame.frame), 'translatedBoxes', json(cte_tb.tb), 'links', json(cte_geo_links.links))) AS contexts
    FROM cte_frame
    JOIN cte_tb ON cte_tb.contextPosition = cte_frame.contextPosition
    JOIN cte_geo_links ON cte_geo_links.contextPosition = cte_frame.contextPosition
)
SELECT json_object('contexts', json(cte_contexts.contexts), 'rectangles', json(cte_rectangles.rectangles)) AS document
FROM cte_contexts
CROSS JOIN cte_rectangles