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
    FOREIGN KEY (diagramId, toBoxPosition, toBoxPosition) REFERENCES field(diagramId, boxPosition, position)
);

CREATE TABLE diagData(
    id INTEGER PRIMARY KEY,
    diagramId INTEGER,
    document text
);


SELECT id, diagramId, document->'$.documentTitle', document FROM diagData;
SELECT id, diagramId, document->'$.boxes', document FROM diagData;

WITH cte_box AS (
    SELECT key AS boxPosition, value AS boxValue 
    FROM json_each((SELECT document FROM diagData WHERE id=1), '$.boxes')
)
SELECT * FROM cte_box;

SELECT * FROM json_tree((SELECT document FROM diagData WHERE id=1));

SELECT json_array_length((SELECT document FROM diagData WHERE id=1), '$.boxes');

SELECT value FROM generate_series(5,100,5); --error