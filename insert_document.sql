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
), cte_boxes AS (
    SELECT key AS boxPosition, value->>'$.title' AS title
    FROM cte_tree box
    WHERE box.path = '$.boxes' 
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO box(diagramId, position, title)
SELECT cte_diagram.id, boxPosition, title 
FROM cte_boxes
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'))
), cte_fields AS (
    SELECT box.key AS boxPosition, field.key AS fieldPosition, field.value->>'$.name' AS name, field.value->>'$.isPrimaryKey' AS isPrimaryKey, field.value->>'$.isForeignKey' AS isForeignKey, field.value->>'$.type' AS type
    FROM cte_tree box 
    JOIN cte_tree fields ON fields.parent = box.id
    JOIN cte_tree field ON field.parent = fields.id
    WHERE fields.key = 'fields' AND box.path = '$.boxes'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO field(position, boxPosition, diagramId, name, isPrimaryKey, isForeignKey, fieldType)
SELECT fieldPosition, boxPosition, cte_diagram.id, name, isPrimaryKey, isForeignKey, type
FROM cte_fields
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.values')
), cte_values AS (
    SELECT value->>'$.box' AS boxName, value->>'$.field' AS fieldName, value->>'$.value' AS fieldValue 
    FROM cte_tree 
    WHERE path='$.values'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO fieldValue(diagramId, boxPosition, fieldPosition, fieldValue)
SELECT cte_diagram.id AS diagramId, box.Position AS boxPosition, field.position AS fieldPosition, cte_values.fieldValue AS fieldValue
FROM cte_values
CROSS JOIN cte_diagram
JOIN box ON box.diagramId = cte_diagram.id AND box.title=cte_values.boxName
JOIN field ON field.diagramId = cte_diagram.id AND field.boxPosition=box.position AND field.name=cte_values.fieldName;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.fieldColors')
), cte_field_colors AS (
    SELECT value->>'$.index' AS indexValue, value->>'$.box' AS boxName, value->>'$.field' AS fieldName, value->>'$.color' AS color 
    FROM cte_tree
    WHERE path='$.fieldColors'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO fieldColor(diagramId, index_, boxPosition, fieldPosition, color)
SELECT cte_diagram.id AS diagramId, indexValue AS index_, box.Position AS boxPosition, field.position AS fieldPosition, cte_field_colors.color AS color
FROM cte_field_colors
CROSS JOIN cte_diagram
JOIN box ON box.diagramId = cte_diagram.id AND box.title=cte_field_colors.boxName
JOIN field ON field.diagramId = cte_diagram.id AND field.boxPosition=box.position AND field.name=cte_field_colors.fieldName;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.boxComments')
), cte_box_comments AS (
    SELECT value->>'$.box' AS boxName, value->>'$.comment' AS comment 
    FROM cte_tree
    WHERE path='$.boxComments'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO boxComment(diagramId, boxPosition, bComment)
SELECT cte_diagram.id AS diagramId, box.Position AS boxPosition, cte_box_comments.comment AS bComment
FROM cte_box_comments
CROSS JOIN cte_diagram
JOIN box ON box.diagramId = cte_diagram.id AND box.title=cte_box_comments.boxName;



WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.links')
), cte_links AS (
    SELECT key AS linkPosition, value->>'$.from' AS from_, value->>'$.fromField' AS fromField, value->>'$.fromCardinality' AS fromCardinality, value->>'$.to' AS to_, value->>'$.toField' AS toField, value->>'$.toCardinality' AS toCardinality, value->>'$.category' AS category
    FROM cte_tree link
    WHERE link.path='$.links'
) , cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO link(diagramId, fromBoxPosition, fromFieldPosition, fromCardinality, toBoxPosition, toFieldPosition, toCardinality, category)
SELECT cte_diagram.id, from_ AS fromBoxPosition, fromField AS fromFieldPosition, fromCardinality, to_ AS toBoxPosition, toField AS toFieldPosition, toCardinality, category
FROM cte_links
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT diagData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.pictures')
), cte_pictures AS (
    SELECT key AS picturePosition, value->>'$.height' AS height,  value->>'$.width' AS width, value->>'$.name' AS name, value->>'$.zoomPercentage' AS zoomPercentage, value->>'$.hash' AS hash
    FROM cte_tree picture
    WHERE path='$.pictures'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO picture(diagramId, height, width, name, zoomPercentage, hash)
SELECT cte_diagram.id, height, width, name, zoomPercentage, hash
FROM cte_pictures
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.rectangles')
), cte_rectangles AS (
    SELECT key AS rectanglePosition, value->>'$.left' AS left, value->>'$.right' AS right, value->>'$.top' AS top, value->>'$.bottom' AS bottom 
    FROM cte_tree rectangle
    WHERE path='$.rectangles'
), cte_diagram AS (
	SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO rectangle(diagramId, boxPosition, [left], [right], top, bottom)
SELECT cte_diagram.id, rectanglePosition, [left], [right], top, bottom
FROM cte_rectangles
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.contexts')
), cte_contexts AS (
    SELECT key AS contextPosition
    FROM cte_tree context
    WHERE path='$.contexts'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO context(diagramId, contextPosition)
SELECT cte_diagram.id, contextPosition
FROM cte_contexts
CROSS JOIN cte_diagram;

WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.contexts')
), cte_frames AS (
    SELECT context.key AS contextPosition, frame.value->>'$.left' AS left, frame.value->>'$.right' AS right, frame.value->>'$.top' AS top, frame.value->>'$.bottom' AS bottom
    FROM cte_tree context
    JOIN cte_tree frame ON frame.parent=context.id
    WHERE context.path='$.contexts' AND frame.key='frame'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO frame(diagramId, contextPosition, [left], [right], top, bottom)
SELECT cte_diagram.id, contextPosition, [left], [right], top, bottom
FROM cte_frames
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.contexts')
), cte_tbs AS (
    SELECT context.key AS contextPosition, tB.key AS tbPosition, tB.value->>'$.id' AS boxPosition, tB.value->>'$.translation.x' AS translationX, tB.value->>'$.translation.y' AS translationY
    FROM cte_tree context
    JOIN cte_tree translatedBoxes ON translatedBoxes.parent=context.id
    JOIN cte_tree tB ON tB.parent=translatedBoxes.id
    WHERE context.path='$.contexts' AND translatedBoxes.key='translatedBoxes'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO translatedBoxes(diagramId, contextPosition, boxPosition, translationX, translationY)
SELECT cte_diagram.id, contextPosition, boxPosition, translationX, translationY
FROM cte_tbs
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.contexts')
), cte_polylines AS (
    SELECT context.key AS contextPosition, link.key AS polylinePosition, link.value->>'$.from' AS [from], link.value->>'$.to' AS [to]
    FROM cte_tree context
    JOIN cte_tree links ON links.parent=context.id
    JOIN cte_tree link ON link.parent=links.id
    WHERE context.path='$.contexts' AND links.key='links'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO polyline(diagramId, contextPosition, polylinePosition, [from], [to])
SELECT cte_diagram.id, contextPosition, polylinePosition, [from], [to]
FROM cte_polylines
CROSS JOIN cte_diagram;


WITH cte_tree AS (
    SELECT * FROM json_tree((SELECT geoData FROM document WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'), '$.contexts')
), cte_points AS (
    SELECT context.key AS contextPosition, link.key AS polylinePosition, point.key AS pointPosition, point.value->>'$.x' AS x, point.value->>'$.y' AS y
    FROM cte_tree context
    JOIN cte_tree links ON links.parent=context.id
    JOIN cte_tree link ON link.parent=links.id
    JOIN cte_tree polyline ON polyline.parent = link.id
    JOIN cte_tree point ON point.parent = polyline.id
    WHERE context.path='$.contexts' AND links.key='links' AND polyline.key='polyline'
), cte_diagram AS (
    SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
)
INSERT INTO point(diagramId, contextPosition, polylinePosition, pointPosition, x, y)
SELECT cte_diagram.id, contextPosition, polylinePosition, pointPosition, x, y
FROM cte_points
CROSS JOIN cte_diagram;
