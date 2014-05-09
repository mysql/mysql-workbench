

-- test table with all MySQL 5.5 datatypes
create table alltypes (
    -- numeric types
    num_tinyint TINYINT,
    num_smallint SMALLINT,
    num_mediumint MEDIUMINT,
    num_int INT,
    num_bigint BIGINT,
    --
    num_decimal DECIMAL(65, 5),
    num_numeric NUMERIC(65, 5),
    -- 
    num_float FLOAT,
    num_double DOUBLE,
    -- bit
    bit_1 BIT(1),
    bit_64 BIT(64),
    -- date and time
    dt_date DATE,
    dt_datetime DATETIME,
    dt_timestamp TIMESTAMP,
    dt_time TIME,
    dt_year YEAR(4),
    -- string
    str_char CHAR(255),
    str_varchar VARCHAR(10000),
    str_binary BINARY(255),
    str_varbinary VARBINARY(10000),
    -- blob
    bl_tinyblob TINYBLOB,
    bl_blob BLOB,
    bl_mediumblob MEDIUMBLOB,
    bl_longblob LONGBLOB,
    bl_tinytext TINYTEXT,
    bl_text TEXT,
    bl_mediumtext MEDIUMTEXT,
    bl_longtext LONGTEXT,
    -- enum
    en_enum ENUM('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'),
    -- set
    se_set SET('x', 'y', 'z', 'w')
);
