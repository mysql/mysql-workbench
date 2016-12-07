echo off

if [%1] == [] goto Usage

IF %1 == antlr (
  echo Selected ANLTR parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-4.6-complete.jar  ANTLRv4Parser.g4 ANTLRv4Parser.g4 -o ../antlr
) ELSE IF %1 == mysql (
  echo Selected MySQL parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-4.6-complete.jar  MySQLLexer.g4 MySQLParser.g4 -o ../mysql
) ELSE IF %1 == js (
  echo Selected JS parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-4.6-complete.jar  ECMA.g4 -o ../ecma
) ELSE IF %1 == python (
  echo Selected Python parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-4.6-complete.jar  Python2.5.g -o ../python
) ELSE (
  echo Unknown parser type %1
)

goto EndOfScript

:Usage
echo "Usage: $0 [antlr|mysql|python|js]"

:EndOfScript