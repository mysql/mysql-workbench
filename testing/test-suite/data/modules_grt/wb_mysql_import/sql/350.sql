/*
CREATE SERVER server_name
    FOREIGN DATA WRAPPER wrapper_name
    OPTIONS (option ...)

option:
  { HOST character-literal
  | DATABASE character-literal
  | USER character-literal
  | PASSWORD character-literal
  | SOCKET character-literal
  | OWNER character-literal
  | PORT numeric-literal }
*/

CREATE SERVER server_name1
FOREIGN DATA WRAPPER wrapper_name1
OPTIONS(
HOST 'host-literal',
DATABASE 'database-literal',
USER 'user-literal',
PASSWORD 'password-literal',
SOCKET 'socket-literal',
OWNER 'owner-literal',
PORT 3306);
