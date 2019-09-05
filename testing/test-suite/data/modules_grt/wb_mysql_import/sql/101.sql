CREATE OR REPLACE ALGORITHM = TEMPTABLE DEFINER = usr1@localhost SQL SECURITY DEFINER VIEW `test`.`view1`
AS
select f01, f01 as f02 from table1
union all
select f01, f01 from table1;
