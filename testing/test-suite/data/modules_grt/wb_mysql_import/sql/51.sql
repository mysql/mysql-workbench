CREATE TABLE test.table1 (
f01 INTEGER
);

CREATE unique INDEX index1
ON
test.table1(f01 (10) desc)
KEY_BLOCK_SIZE 100
using hash;
