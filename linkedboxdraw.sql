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
    deleted INTEGER,
    UNIQUE (diagramId, boxPosition, position),
    FOREIGN KEY (diagramId, boxPosition) REFERENCES box(diagramId, position)
);

CREATE TABLE links(
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
