-- drop the event table if it exists, then recreate it

DROP TABLE IF EXISTS event;

CREATE TABLE event
(
  name   VARCHAR(20),
  date   DATE,
  type   VARCHAR(15),
  remark VARCHAR(255)
);
