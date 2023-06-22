WITH cte_diagram AS (
	SELECT id FROM diagram WHERE guid='a8828ddfef224d36935a1c66ae86ebb3'
) ,cte_bool (bit, boolValue) AS (
    SELECT 0, 'false' UNION ALL
    SELECT 1, 'true'
) ,cte_fields AS (
    SELECT boxPosition, 
                        json_group_array(
                            CASE
                                WHEN fieldType IS NULL THEN
                                    json_object('name',name, 'isPrimaryKey', json(pk.boolValue), 'isForeignKey', json(fk.boolValue))
                                ELSE
                                    json_object('name',name, 'isPrimaryKey', json(pk.boolValue), 'isForeignKey', json(fk.boolValue), 'type', fieldType)
                            END
                        ) AS fields
    FROM field
    LEFT JOIN cte_bool pk ON field.isPrimaryKey = pk.bit --Should be JOIN instead of LEFT JOIN, but LEFT JOIN preserves the ordering from field, unlike JOIN.
    LEFT JOIN cte_bool fk ON field.isForeignKey = fk.bit --If we used JOIN instead of LEFT JOIN, the order in json_group_array()'s output would be compromised.
	CROSS JOIN cte_diagram
    WHERE diagramId=cte_diagram.id
    GROUP BY boxPosition
) , cte_boxes AS (
    SELECT json_group_array( json_object('title', title, 'id', position, 'fields', COALESCE(json(fields), json_array()))) AS boxes  
    FROM box
    LEFT JOIN cte_fields ON box.position=cte_fields.boxPosition
	CROSS JOIN cte_diagram
    WHERE box.diagramId=cte_diagram.id
) , cte_links AS (
    SELECT json_group_array( json_object('from',fromBoxPosition,'fromField',fromFieldPosition,'fromCardinality',fromCardinality,'to',toBoxPosition,'toField',toFieldPosition,'toCardinality',toCardinality,'category',category)) AS links
    FROM link
	CROSS JOIN cte_diagram
    WHERE diagramId=cte_diagram.id
), cte_field_values AS (
    SELECT json_group_array( json_object('box', box.title, 'field', field.name, 'value', fieldValue.fieldValue)) AS field_values
    FROM fieldValue
    JOIN box ON box.diagramId = fieldValue.diagramId AND box.position=fieldValue.boxPosition
    JOIN field ON field.diagramId = fieldValue.diagramId AND field.position=fieldValue.fieldPosition AND field.boxPosition = fieldValue.boxPosition
    CROSS JOIN cte_diagram
    WHERE fieldValue.diagramId=cte_diagram.id
), cte_field_colors AS (
    SELECT json_group_array( json_object('index', fieldColor.index_, 'box', box.title, 'field', field.name, 'color', fieldColor.color)) AS field_colors
    FROM fieldColor
    JOIN box ON box.diagramId = fieldColor.diagramId AND box.position=fieldColor.boxPosition
    JOIN field ON field.diagramId = fieldColor.diagramId AND field.position=fieldColor.fieldPosition AND field.boxPosition = fieldColor.boxPosition
    CROSS JOIN cte_diagram
    WHERE fieldColor.diagramId=cte_diagram.id
), cte_box_comments AS (
    SELECT json_group_array( json_object('box', box.title, 'comment', boxComment.bComment)) AS box_comments
    FROM boxComment
    JOIN box ON box.diagramId=boxComment.diagramId AND box.position=boxComment.boxPosition
	CROSS JOIN cte_diagram
    WHERE boxComment.diagramId = cte_diagram.id
) ,cte_pictures AS (
    SELECT json_group_array( json_object('height', height, 'width', width, 'name', name, 'zoomPercentage', zoomPercentage, 'hash', hash)) AS pictures
    FROM picture
	CROSS JOIN cte_diagram
    WHERE diagramId=cte_diagram.id
), cte_doc AS (
    SELECT json_object('documentTitle', diagram.title, 
		       'boxes', json(cte_boxes.boxes), 
		       'values', COALESCE(json(cte_field_values.field_values), json_array()),
		       'boxComments', COALESCE(json(cte_box_comments.box_comments), json_array()),
		       'fieldComments', json_array(),
		       'links', json(cte_links.links),
		       'fieldColors', COALESCE(json(cte_field_colors.field_colors), json_array()),
		       'pictures', json(cte_pictures.pictures)) AS document
    FROM diagram 
    CROSS JOIN cte_boxes
    CROSS JOIN cte_links
    LEFT JOIN cte_field_values
	LEFT JOIN cte_field_colors
    LEFT JOIN cte_box_comments
    CROSS JOIN cte_pictures
	CROSS JOIN cte_diagram
    WHERE diagram.id=cte_diagram.id
)
/*diag-contexts*/
, cte_rectangles AS (
    SELECT json_group_array( json_object('left', [LEFT], 'right', [RIGHT], 'top', TOP, 'bottom', BOTTOM)) AS rectangles
    FROM rectangle
	CROSS JOIN cte_diagram
    WHERE diagramId=cte_diagram.id
    GROUP BY diagramId
), cte_polylines AS (
    SELECT contextPosition, polylinePosition, json_group_array( json_object('x',x,'y',y)) AS polyline
    FROM point
	CROSS JOIN cte_diagram
    WHERE diagramId=cte_diagram.id
    GROUP BY contextPosition, polylinePosition
), cte_geo_links AS (
    SELECT cte_polylines.contextPosition, json_group_array( json_object('polyline', json(cte_polylines.polyline), 'from', polyline.[from], 'to', polyline.[to])) AS links
    FROM cte_polylines
    JOIN polyline ON cte_polylines.contextPosition=polyline.contextPosition AND cte_polylines.polylinePosition=polyline.polylinePosition
	CROSS JOIN cte_diagram
    WHERE polyline.diagramId=cte_diagram.id
    GROUP BY cte_polylines.contextPosition
), cte_tb AS (
    SELECT contextPosition, json_group_array(json_object('id', boxPosition, 'translation', json_object('x', translationX, 'y', translationY))) AS tb
    FROM translatedBoxes
	CROSS JOIN cte_diagram
    WHERE diagramId=cte_diagram.id
    GROUP BY contextPosition
), cte_frame AS (
    SELECT contextPosition, json_object('left', [LEFT], 'right', [RIGHT], 'top', TOP, 'bottom', BOTTOM) AS frame
    FROM frame
	CROSS JOIN cte_diagram
    WHERE diagramId=cte_diagram.id
), cte_contexts AS (
    SELECT json_group_array(json_object('frame', json(cte_frame.frame), 'translatedBoxes', json(cte_tb.tb), 'links', COALESCE(json(cte_geo_links.links), json_array()))) AS contexts
    FROM cte_frame
    JOIN cte_tb ON cte_tb.contextPosition = cte_frame.contextPosition
    LEFT JOIN cte_geo_links ON cte_geo_links.contextPosition = cte_frame.contextPosition
), cte_diag_contexts AS (
	SELECT json_object('contexts', json(cte_contexts.contexts), 'rectangles', json(cte_rectangles.rectangles)) AS document
	FROM cte_contexts
	CROSS JOIN cte_rectangles
), cte_diag_data AS (
	SELECT document FROM cte_doc
)
SELECT json_object('data', json(cte_diag_data.document), 'contexts', json(cte_diag_contexts.document)) AS document
FROM cte_diag_data
CROSS JOIN cte_diag_contexts;
