CREATE OR REPLACE ALGORITHM = TEMPTABLE DEFINER = CURRENT_USER SQL SECURITY DEFINER VIEW test.view1 (f01, f02)
AS
select f01, f01 from table1;
