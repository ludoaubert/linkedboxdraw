CREATE TABLE box(
    id INTEGER PRIMARY KEY,
    title VARCHAR(100),
    deleted INTEGER
);

CREATE TABLE field(
    id INTEGER PRIMARY KEY,
    position INTEGER,
    boxId INTEGER NOT NULL,
    name varchar(100),
    isPrimaryKey INTEGER,
    isForeignKey INTEGER,
    deleted INTEGER,
    UNIQUE (boxId, position),
    FOREIGN KEY (boxId) REFERENCES box(id)
);

CREATE TABLE links(
    fromBoxId INTEGER,
    fromFieldPosition INTEGER,
    fromCardinality char,
    toBoxId INTEGER,
    toFieldPosition INTEGER,
    toCardinality char,
    category varchar(3),
    deleted INTEGER,
    FOREIGN KEY (fromBox) REFERENCES box(id),
    FOREIGN KEY (toBox) REFERENCES box(id),
    FOREIGN KEY (fromBoxId, fromFieldPosition) REFERENCES field(boxId, position),
    FOREIGN KEY (toBoxId, toBoxPosition) REFERENCES field(boxId, position)
);