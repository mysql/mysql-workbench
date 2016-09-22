echo off

if [%1] == [] goto Usage

IF %1 == mysql (
  echo Selected MySQL parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-3.4-complete.jar  -make MySQL.g -o .. -Xmaxswitchcaselabels 30
) ELSE IF %1 == mysqls (
  echo Selected simple MySQL parser
  java -Xmx1024m -jar ..\..\..\..\mysql-win-res\bin\antlr-3.4-complete.jar  -make MySQLSimpleParser.g -o .. -Xmaxswitchcaselabels 30
) ELSE (
  echo Unknown parser type %1
)

goto EndOfScript

:Usage
echo "Usage: $0 [mysql|mysqls|python|js]"

:EndOfScript