BEGIN TRANSACTION;

DROP TABLE IF EXISTS TimeContainer;

CREATE TABLE TimeContainer (
  id INTEGER PRIMARY KEY,
  date_data DATE,
  time_data TIME,
  timestamp_data TIMESTAMP
);

INSERT INTO TimeContainer (id, date_data, time_data, timestamp_data) VALUES (1, NULL, NULL, NULL);
INSERT INTO TimeContainer (id, date_data, time_data, timestamp_data) VALUES (2, '2013-12-30', '23:59:59', '2013-12-30 23:59:59');
INSERT INTO TimeContainer (id, date_data, time_data, timestamp_data) VALUES (3, '2012-01-15', '00:00:01', '2012-01-15 00:00:01');

COMMIT TRANSACTION;
