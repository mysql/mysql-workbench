BEGIN TRANSACTION;

DROP TABLE IF EXISTS UnsignedIntegerContainer;

CREATE TABLE UnsignedIntegerContainer (
  id INTEGER PRIMARY KEY,
  tint TINYINT UNSIGNED,
  sint SMALLINT UNSIGNED,
  mint MEDIUMINT UNSIGNED,
  iint INT UNSIGNED,
  bint BIGINT UNSIGNED
);

INSERT INTO UnsignedIntegerContainer (id, tint, sint, mint, iint, bint) VALUES (1, NULL, NULL, NULL, NULL, NULL);
INSERT INTO UnsignedIntegerContainer (id, tint, sint, mint, iint, bint) VALUES (2, 0, 0, 0, 0, 0);
INSERT INTO UnsignedIntegerContainer (id, tint, sint, mint, iint, bint) VALUES (3, 127, 32767, 8388607, 2147483647, 9223372036854775807);
INSERT INTO UnsignedIntegerContainer (id, tint, sint, mint, iint, bint) VALUES (4, 255, 65535, 16777215, 4294967295, 18446744073709551615);
 
COMMIT TRANSACTION;
