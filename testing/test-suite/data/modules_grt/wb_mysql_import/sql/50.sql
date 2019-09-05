CREATE TABLE test.table1 (
f01 INTEGER,
f02 VARCHAR(100)
);

CREATE unique INDEX index1
using btree
ON
test.table1(f01 (10) asc)
KEY_BLOCK_SIZE 100;

CREATE fulltext INDEX index2
ON
test.table1(f02)
KEY_BLOCK_SIZE 200
WITH PARSER parser_name;
