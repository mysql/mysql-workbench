-- table
CREATE TABLE `test`.`test` (
id INTEGER,
f01 INTEGER,
f02 INTEGER,
f03 INTEGER,
f04 VARCHAR(10),
f05 INTEGER,
f06 INTEGER,
CONSTRAINT c01 PRIMARY KEY (f01),
INDEX (f03),
CONSTRAINT c02 UNIQUE INDEX (f02),
FULLTEXT(f04),
FOREIGN KEY (f05) REFERENCES test.test(id) ON DELETE SET NULL ON UPDATE no action,
FOREIGN KEY (f06) REFERENCES test(id) ON DELETE SET NULL ON UPDATE no action,
CHECK (f01<>f02)
);

-- view
CREATE OR REPLACE ALGORITHM = TEMPTABLE DEFINER = usr1@localhost SQL SECURITY DEFINER VIEW `test`.`test`
AS
select test.test.test, test.test from test.test
union all
select test.test, test from test;

-- trigger
DELIMITER //
CREATE
DEFINER = user1@'localhost'
TRIGGER test BEFORE INSERT
ON test.test FOR EACH ROW
BEGIN
INSERT INTO test.test(id) values (1);
INSERT INTO test(id) values (1);
END //
DELIMITER ;

-- routine
DELIMITER //
CREATE
DEFINER=`serg`@`%`
PROCEDURE test.test (IN p1 INTEGER)
LANGUAGE SQL
NOT DETERMINISTIC
MODIFIES SQL DATA
SQL SECURITY INVOKER
COMMENT 'procedure desc'
BEGIN
INSERT INTO test.test(id) values (p1);
INSERT INTO test(id) values (p1);
END //
DELIMITER ;
