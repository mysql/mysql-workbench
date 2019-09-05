CREATE SCHEMA `test`;
USE `test`;

create view view_table1 as select * from table1;
create view view_table1 as select * from table1;
create table view_table1 (f01 integer);
create table view_table2 (f01 integer);
create table view_table2 (f01 integer);
create view view_table2 as select * from table1;
