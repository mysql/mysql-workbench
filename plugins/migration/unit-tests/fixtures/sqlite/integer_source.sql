BEGIN TRANSACTION;

DROP TABLE IF EXISTS IntegerContainer;

CREATE TABLE IntegerContainer (
  id INTEGER PRIMARY KEY,
  tint TINYINT,
  sint SMALLINT,
  mint MEDIUMINT,
  iint INT,
  bint BIGINT
);

INSERT INTO IntegerContainer (id, tint, sint, mint, iint, bint) VALUES (1, NULL, NULL, NULL, NULL, NULL);
INSERT INTO IntegerContainer (id, tint, sint, mint, iint, bint) VALUES (2, 0, 0, 0, 0, 0);
INSERT INTO IntegerContainer (id, tint, sint, mint, iint, bint) VALUES (3, -128, -32768, -8388608, -2147483648, -9223372036854775808);
INSERT INTO IntegerContainer (id, tint, sint, mint, iint, bint) VALUES (4, 127, 32767, 8388607, 2147483647, 9223372036854775807);
 
COMMIT TRANSACTION;
