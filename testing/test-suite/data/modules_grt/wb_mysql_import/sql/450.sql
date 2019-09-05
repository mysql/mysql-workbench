create database db0;
create database db1;
create database db2;

use db0;

create table t0 (f0 integer);
create table t1 (f0 integer);
create table t2 (f0 integer);

create view v0 as select * from t0;
create view v1 as select * from t1;
create view v2 as select * from t2;

drop view v0, v2;

drop table t0, t2;

drop database db1;
drop database db2;
