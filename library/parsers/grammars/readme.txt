Generating parser files with ANTLR
=====================================

Building the parser is simple. You need Java installed on your system and need the associated res folder for each platform present (for the antlr-x.x-complete.jar and the runtime). Run "build-parser-mac" (OS X) or "build-parser.cmd" (Windows) to do the actual generation. On Linux it is part of the cmake setup. There should be no warning or error or any other message if everything goes fine.

The generated files are placed in the associated folder one level up and are immediately ready for compilation.

Usually however you don't need to run this script manually, because it is automatically executed in Visual Studio, in XCode and by Cmake, if necessary.
