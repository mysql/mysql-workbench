DROP TABLE IF EXISTS Target;
CREATE TABLE Target (
    tpk1 INTEGER,
    tpk2 INTEGER,
    extra TEXT,
    other INTEGER UNIQUE NOT NULL,
    PRIMARY KEY (tpk1, tpk2)
);

DROP TABLE IF EXISTS Source;
CREATE TABLE Source (
    sid INTEGER PRIMARY KEY,    
    sfk1 INTEGER,
    sfk2 INTEGER,
    sother INTEGER REFERENCES Target (other) ON DELETE SET NULL,
    sextra TEXT,
    FOREIGN KEY (sfk1, sfk2) REFERENCES Target (pk1, pk2) ON DELETE CASCADE
);

