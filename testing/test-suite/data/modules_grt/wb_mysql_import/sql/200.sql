/*
CREATE
    [DEFINER = { user | CURRENT_USER }]
    TRIGGER trigger_name trigger_time trigger_event
    ON tbl_name FOR EACH ROW trigger_stmt
*/

DELIMITER //

CREATE
DEFINER = user1@'localhost'
TRIGGER trigger1 BEFORE INSERT
ON test.table1 FOR EACH ROW
BEGIN
INSERT INTO table2(id) values (1);
END //

CREATE
DEFINER = 'user1'@'localhost'
TRIGGER trigger2 BEFORE UPDATE
ON table1 FOR EACH ROW
INSERT INTO table2(id) values (1) //

CREATE
DEFINER = user1
TRIGGER trigger3 BEFORE DELETE
ON table1 FOR EACH ROW
BEGIN
INSERT INTO test.table2(id) values (1);
END //

CREATE
DEFINER = 'user1'
TRIGGER trigger4 AFTER INSERT
ON table1 FOR EACH ROW
BEGIN
INSERT INTO table2(id) values (1);
END //

CREATE
DEFINER = CURRENT_USER
TRIGGER trigger5 AFTER UPDATE
ON table1 FOR EACH ROW
BEGIN
INSERT INTO table2(id) values (1);
END //

CREATE
DEFINER = CURRENT_USER
TRIGGER trigger6 AFTER DELETE
ON table1 FOR EACH ROW
BEGIN
INSERT INTO table2(id) values (1);
END //

DELIMITER ;
;
