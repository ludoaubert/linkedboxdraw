WITH cte_fields AS (
    SELECT boxPosition, 
                        '[' || group_concat(json_object('name',name,
                                                'isPrimaryKey',json(CASE isPrimaryKey WHEN 0 THEN 'false' WHEN 1 THEN 'true' END),
                                                'isForeignKey',json(CASE isForeignKey WHEN 0 THEN 'false' WHEN 1 THEN 'true' END))
                                    ) || ']' AS fields
    FROM field
    WHERE diagramId=1
    GROUP BY boxPosition
) , cte_boxes AS (
    SELECT '[' || group_concat( json_object('title', title, 'id', position, 'fields', fields), ',') || ']' AS boxes  
    FROM box
    JOIN cte_fields ON box.position=cte_fields.boxPosition
    WHERE box.diagramId=1
) , cte_links AS (
    SELECT '[' || group_concat( json_object('from',fromBoxPosition,'fromField',fromFieldPosition,'fromCardinality',fromCardinality,'to',toBoxPosition,'toField',toFieldPosition,'toCardinality',toCardinality,'category',category), ',') || ']' AS links
    FROM link
    WHERE diagramId=1
) ,cte_pictures AS (
    SELECT '[' || group_concat( json_object('height', height, 'width', width, 'name', name, 'base64', base64)) || ']' AS pictures
    FROM picture
    WHERE diagramId=1
), cte_doc AS (
    SELECT json_object('documentTitle', diagram.title, 'boxes', cte_boxes.boxes, 'links', cte_links.links, 'pictures', cte_pictures.pictures) AS document
    FROM diagram 
    CROSS JOIN cte_boxes
    CROSS JOIN cte_links
    CROSS JOIN cte_pictures
    WHERE diagram.id=1
)
SELECT REPLACE(REPLACE(REPLACE(REPLACE(document,'\\\',''), '\"', '"'),'"[','['),']"',']') AS document FROM cte_doc;