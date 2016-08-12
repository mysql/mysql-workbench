Generating parser files with ANTLR
=====================================

Building the parser is simple. You need Java installed on your system and need the associated res folder for each platform present (for the antlr-4.5.4-complete.jar). Run "build-parser" (Linux), "build-parser-mac" (OS X) or "build-parser.cmd" (Windows) to do the actual generation. There should be no warning or error or any other message if everything goes fine.

The generated files are placed in the associated folder one level up and are immediately ready for compilation. The script accepts a parameter specifying which parser to generate (mysql, js or python).