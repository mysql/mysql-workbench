BEGIN TRANSACTION;

DROP TABLE IF EXISTS NumericContainer;

CREATE TABLE NumericContainer (
  id INTEGER PRIMARY KEY,
  fdata FLOAT,
  ddata DOUBLE,
  ndata NUMERIC,
  decdata DECIMAL(10,5)
);

INSERT INTO NumericContainer (id, fdata, ddata, ndata, decdata) VALUES (1, NULL, NULL, NULL, NULL);
INSERT INTO NumericContainer (id, fdata, ddata, ndata, decdata) VALUES (2, 0, 0, 0, 0);
INSERT INTO NumericContainer (id, fdata, ddata, ndata, decdata) VALUES (3, 0.00001, 0.00001, 0.00001, 0.00001);
INSERT INTO NumericContainer (id, fdata, ddata, ndata, decdata) VALUES (4, -0.00001, -0.00001, -0.00001, -0.00001);
INSERT INTO NumericContainer (id, fdata, ddata, ndata, decdata) VALUES (5, 99.9999, 99.9999, 99.9999, 99999.99999);
 
COMMIT TRANSACTION;
