CREATE TABLE test.table1 (
f11 INTEGER key,
f12 INTEGER unique
);

CREATE TABLE test.table2 (
f21 INTEGER,
f22 integer references test.table1(f11),
PRIMARY KEY (f21)
);
