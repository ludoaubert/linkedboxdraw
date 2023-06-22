PRAGMA foreign_keys = ON;


CREATE TABLE diagram(
    id INTEGER PRIMARY KEY,
    title VARCHAR(100),
    deleted INTEGER,
    guid CHAR(16) UNIQUE
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
    UNIQUE(diagramId, position),
	UNIQUE(diagramId, title),
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
    UNIQUE (diagramId, boxPosition, position),
	UNIQUE (diagramId, boxPosition, name),
    FOREIGN KEY (diagramId, boxPosition) REFERENCES box(diagramId, position)
);

CREATE TABLE fieldValue(
	id INTEGER PRIMARY KEY,
	diagramId INTEGER NOT NULL,
	boxPosition INTEGER NOT NULL,
	fieldPosition INTEGER NOT NULL,
	fieldValue varchar(30),
	FOREIGN KEY(diagramId, boxPosition, fieldPosition) REFERENCES field(diagramId, boxPosition, position)	
);

CREATE TABLE boxComment(
	id INTEGER PRIMARY KEY,
	diagramId INTEGER NOT NULL,
	boxPosition INTEGER NOT NULL,
	bComment varchar(500),
	UNIQUE (diagramId, boxPosition),
	FOREIGN KEY(diagramId, boxPosition) REFERENCES box(diagramId, position)
);

CREATE TABLE fieldComment(
	id INTEGER PRIMARY KEY,
	diagramId INTEGER NOT NULL,
	boxPosition INTEGER NOT NULL,
	fieldPosition INTEGER NOT NULL,
	fComment varchar(20),
	UNIQUE (diagramId, boxPosition, fieldPosition),
	FOREIGN KEY(diagramId, boxPosition, fieldPosition) REFERENCES field(diagramId, boxPosition, position)	
);

CREATE TABLE fieldColor(
	id INTEGER PRIMARY KEY,
	index_ INTEGER,
	diagramId INTEGER NOT NULL,
	boxPosition INTEGER NOT NULL,
	fieldPosition INTEGER NOT NULL,
	color varchar(20),
	UNIQUE (diagramId, boxPosition, fieldPosition),
	FOREIGN KEY(diagramId, boxPosition, fieldPosition) REFERENCES field(diagramId, boxPosition, position)
);

CREATE TABLE picture(
	id INTEGER PRIMARY KEY,
	diagramId INTEGER,
	height INTEGER,
	width INTEGER,
	name varchar(100),
	zoomPercentage INTEGER,
	hash CHAR(512) NOT NULL
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
    FOREIGN KEY (diagramId, fromBoxPosition) REFERENCES box(diagramId, position),
    FOREIGN KEY (diagramId, toBoxPosition) REFERENCES box(diagramId, position),
    FOREIGN KEY (diagramId, fromBoxPosition, fromFieldPosition) REFERENCES field(diagramId, boxPosition, position),
    FOREIGN KEY (diagramId, toBoxPosition, toFieldPosition) REFERENCES field(diagramId, boxPosition, position)
);

CREATE TABLE document(
    id INTEGER PRIMARY KEY,
    diagData text,
    geoData text,
	guid char(16) UNIQUE
);


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
    SELECT json_group_array( json_object('height', height, 'width', width, 'name', name, 'hash', hash)) AS pictures
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