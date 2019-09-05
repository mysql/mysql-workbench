CREATE TABLE test.table1 (
f02 INTEGER,
f03 INTEGER,
FOREIGN KEY (f02) REFERENCES test.table2(id),
FOREIGN KEY (f03) REFERENCES test.table3(id)
);

CREATE TABLE test.table2 (
id INTEGER
);

CREATE TABLE test.table3 (
id INTEGER
);
