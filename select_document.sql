WITH cte_bool (bit, boolValue) AS (
    SELECT 0, 'false' UNION ALL
    SELECT 1, 'true'
) ,cte_fields AS (
    SELECT boxPosition, 
                        json_group_array(
                            CASE
                                WHEN fieldType IS NULL THEN
                                    json_object('name',name, 'isPrimaryKey', pk.boolValue, 'isForeignKey', fk.boolValue)
                                ELSE
                                    json_object('name',name, 'isPrimaryKey', pk.boolValue, 'isForeignKey', fk.boolValue, 'type', fieldType)
                            END
                        ) AS fields
    FROM field
    JOIN cte_bool pk ON field.isPrimaryKey = pk.bit
    JOIN cte_bool fk ON field.isForeignKey = fk.bit
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
    SELECT json_object('documentTitle', diagram.title, 
		       'boxes', json(cte_boxes.boxes), 
		       'values', json_array(),
		       'boxComments', json_array(),
		       'fieldComments', json_array(),
		       'links', json(cte_links.links), 
		       'pictures', json(cte_pictures.pictures)) AS document
    FROM diagram 
    CROSS JOIN cte_boxes
    CROSS JOIN cte_links
    CROSS JOIN cte_pictures
    WHERE diagram.id=1
)
/*diag-contexts*/
, cte_rectangles AS (
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
