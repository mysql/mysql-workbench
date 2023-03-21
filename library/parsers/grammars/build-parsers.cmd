echo off

if [%1] == [] goto Usage

IF %1 == mysql (
  echo Selected MySQL parser
  java -Xmx1024m -jar %WB_3DPARTY_PATH%\bin\antlr-4.11.1-complete.jar -Dlanguage=Cpp -listener -visitor -o ../mysql -package parsers MySQLLexer.g4 MySQLParser.g4
) ELSE (
  echo Unknown parser type %1
)

goto EndOfScript

:Usage
echo "Usage: $0 [mysql]"

:EndOfScript
