echo off

if [%1] == [] goto Usage

IF %1 == antlr (
  echo Selected ANLTR parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-4.6-complete.jar -Dlanguage=Cpp -listener -visitor -o ../antlr -package parsers ANTLRv4Parser.g4 ANTLRv4Parser.g4 
) ELSE IF %1 == mysql (
  echo Selected MySQL parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-4.6-complete.jar -Dlanguage=Cpp -listener -visitor -o ../mysql -package parsers MySQLLexer.g4 MySQLParser.g4
) ELSE IF %1 == js (
  echo Selected JS parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-4.6-complete.jar -Dlanguage=Cpp -listener -visitor -o ../ecma -package parsers ECMA.g4
) ELSE IF %1 == python (
  echo Selected Python parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-4.6-complete.jar -Dlanguage=Cpp -listener -visitor -o ../python -package parsers Python2.5.g4
) ELSE (
  echo Unknown parser type %1
)

goto EndOfScript

:Usage
echo "Usage: $0 [antlr|mysql|python|js]"

:EndOfScript