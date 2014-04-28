BEGIN TRANSACTION;

DROP TABLE IF EXISTS StringContainer;

CREATE TABLE StringContainer (
  id INTEGER PRIMARY KEY,
  str_data VARCHAR(100)
);

INSERT INTO StringContainer (id, str_data) VALUES (1, NULL);
INSERT INTO StringContainer (id, str_data) VALUES (2, '');
INSERT INTO StringContainer (id, str_data) VALUES (3, 'A normal string with spaces and numb3r5');
INSERT INTO StringContainer (id, str_data) VALUES (4, '"`'';');
INSERT INTO StringContainer (id, str_data) VALUES (5, '!$%&/()=?¡');
INSERT INTO StringContainer (id, str_data) VALUES (6, 'áéíóú àèìòù âêîôû äëïöü €¬');
 
COMMIT TRANSACTION;
