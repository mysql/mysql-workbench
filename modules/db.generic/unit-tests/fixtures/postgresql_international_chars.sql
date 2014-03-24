-- Use this from the command line to create the DB:
-- createdb Iñŧęřñⱥtìönal --encoding utf-8
-- psql -f postgresql_international_chars.sql Iñŧęřñⱥtìönal

BEGIN;

SET NAMES 'utf8';

CREATE TABLE México ( 
estado VARCHAR(50),
delegación VARCHAR(50)
);

CREATE TABLE Україна (
область VARCHAR(50),
район VARCHAR(50)
);

COMMIT;
