Building the MySQL parser with ANTLR
=====================================

Building the parser is simple. You need Java installed on your system and need the associated res folder for each platform present (for the antlr-3.4-complete.jar). Run either "build-parser" (Linux, OS X) or "build-parser.cmd" (Windows) to do the actual generation. There should be no warning or error or any other message if everything goes fine.

The generated parser is placed in the parent folder (along with the token file) and is immediately ready for compilation.