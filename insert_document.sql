/*
SELECT id, diagramId, diagData->'$.documentTitle', diagData FROM document;
SELECT id, diagramId, diagData->'$.boxes', diagData FROM document;
*/

INSERT INTO document(/*id,*/ diagData, geoData, guid)
VALUES(/*1,*/ '${diagData}', '${geoData}', 'a8828ddfef224d36935a1c66ae86ebb3');


INSERT INTO diagram(/*id,*/ title, deleted, guid)
VALUES(/*1,*/ '${title}', 0, 'a8828ddfef224d36935a1c66ae86ebb3');


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
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
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO box(diagramId, position, title, deleted)
SELECT diagramId, boxPosition, title, 0 AS deleted 
FROM cte_boxes_pivot
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
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
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO field(position, boxPosition, diagramId, name, isPrimaryKey, isForeignKey, fieldType, deleted)
SELECT fieldPosition, boxPosition, diagramId, name, isPrimaryKey, isForeignKey, type, 0 AS deleted
FROM cte_fields_pivot
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
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
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO link(diagramId, fromBoxPosition, fromFieldPosition, fromCardinality, toBoxPosition, toFieldPosition, toCardinality, category, deleted)
SELECT diagramId, from_ AS fromBoxPosition, fromField AS fromFieldPosition, fromCardinality, to_ AS toBoxPosition, toField AS toFieldPosition, toCardinality, category, 0 AS deleted 
FROM cte_links_pivot
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
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
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO picture(diagramId, height, width, name, base64)
SELECT diagramId, height, width, name, base64
FROM cte_pictures_pivot
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
), cte_rectangles AS (
    SELECT array_index.key AS rectanglePosition, attr.key, attr.value
    FROM cte_tree array_index
    JOIN cte_tree attr ON attr.parent=array_index.id
    WHERE array_index.path='$.rectangles'
), cte_rectangles_pivot AS (
    SELECT rectanglePosition,
            MAX(case when key = 'left' then value end) as [left],
            MAX(case when key = 'right' then value end) as [right],
            MAX(case when key = 'top' then value end) as top,
            MAX(case when key = 'bottom' then value end) as bottom        
    FROM cte_rectangles
    GROUP BY rectanglePosition
    ORDER BY rectanglePosition
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO rectangle(diagramId, boxPosition, [left], [right], top, bottom)
SELECT diagramId, rectanglePosition, [left], [right], top, bottom
FROM cte_rectangles_pivot
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
), cte_contexts AS (
    SELECT key AS contextPosition
    FROM cte_tree array_index
    WHERE path='$.contexts'
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO context(diagramId, contextPosition)
SELECT diagramId, contextPosition
FROM cte_contexts
CROSS JOIN cte_diagram;

WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
), cte_frames AS (
    SELECT context_array_index.key AS contextPosition, attr.key, attr.value
    FROM cte_tree attr
    JOIN cte_tree frame ON attr.parent=frame.id
    JOIN cte_tree context_array_index ON frame.parent=context_array_index.id
    WHERE context_array_index.path='$.contexts'
), cte_frames_pivot AS (
    SELECT contextPosition,
            MAX(case when key = 'left' then value end) as [left],
            MAX(case when key = 'right' then value end) as [right],
            MAX(case when key = 'top' then value end) as top,
            MAX(case when key = 'bottom' then value end) as bottom        
    FROM cte_frames
    GROUP BY contextPosition
    ORDER BY contextPosition
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO frame(diagramId, contextPosition, [left], [right], top, bottom)
SELECT diagramId, contextPosition, [left], [right], top, bottom
FROM cte_frames_pivot
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
), cte_tbs AS (
    SELECT context_array_index.key AS contextPosition, tB_array_index.key AS tbPosition, attr.key, attr.value
    FROM cte_tree context_array_index
    JOIN cte_tree tB ON tB.parent=context_array_index.id
    JOIN cte_tree tB_array_index ON tB_array_index.parent=tB.id
    JOIN cte_tree attr ON attr.parent=tB_array_index.id
    WHERE context_array_index.path='$.contexts' AND tB.key='translatedBoxes'
), cte_tbs_pivot AS (
    SELECT contextPosition, tbPosition,
            MAX(case when key = 'id' then value end) as [boxPosition],
            MAX(case when key = 'translation' then value end) as [translation]
    FROM cte_tbs
    GROUP BY contextPosition, tbPosition
    ORDER BY contextPosition, tbPosition
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO translatedBoxes(diagramId, contextPosition, boxPosition, translationX, translationY)
SELECT diagramId, contextPosition, boxPosition, translation->'$.x' AS translationX, translation->'$.y' AS translationY
FROM cte_tbs_pivot
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
), cte_polylines AS (
    SELECT context_array_index.key AS contextPosition, lk_array_index.key AS polylinePosition, attr.key, attr.value
    FROM cte_tree context_array_index
    JOIN cte_tree lk ON lk.parent=context_array_index.id
    JOIN cte_tree lk_array_index ON lk_array_index.parent=lk.id
    JOIN cte_tree attr ON attr.parent=lk_array_index.id
    WHERE context_array_index.path='$.contexts' AND lk.key='links'
), cte_polylines_pivot AS (
    SELECT contextPosition, polylinePosition,
            MAX(case when key = 'from' then value end) as [from],
            MAX(case when key = 'to' then value end) as [to]
    FROM cte_polylines
    GROUP BY contextPosition, polylinePosition
    ORDER BY contextPosition, polylinePosition
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO polyline(diagramId, contextPosition, polylinePosition, [from], [to])
SELECT diagramId, contextPosition, polylinePosition, [from], [to]
FROM cte_polylines_pivot
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
), cte_points AS (
    SELECT context_array_index.key AS contextPosition, lk_array_index.key AS polylinePosition, point_array_index.key AS pointPosition, attr.key, attr.value
    FROM cte_tree context_array_index
    JOIN cte_tree lk ON lk.parent=context_array_index.id
    JOIN cte_tree lk_array_index ON lk_array_index.parent=lk.id
    JOIN cte_tree polyline ON polyline.parent = lk_array_index.id
    JOIN cte_tree point_array_index ON point_array_index.parent = polyline.id
    JOIN cte_tree attr ON attr.parent=point_array_index.id
    WHERE context_array_index.path='$.contexts' AND lk.key='links' AND polyline.key='polyline'
), cte_points_pivot AS (
    SELECT contextPosition, polylinePosition, pointPosition,
            MAX(case when key = 'x' then value end) as x,
            MAX(case when key = 'y' then value end) as y
    FROM cte_points
    GROUP BY contextPosition, polylinePosition, pointPosition
    ORDER BY contextPosition, polylinePosition, pointPosition
), cte_diagram AS (
	SELECT diagramId FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO point(diagramId, contextPosition, polylinePosition, pointPosition, x, y)
SELECT diagramId, contextPosition, polylinePosition, pointPosition, x, y
FROM cte_points_pivot
CROSS JOIN cte_diagram;
