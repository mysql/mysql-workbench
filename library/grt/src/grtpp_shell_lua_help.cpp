/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "grtpp.h"
#include "base/string_utilities.h"
#include "base/util_functions.h"

#define NL "\n"
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

using namespace base;

typedef struct
{
  const char *cmd;
  const char *desc;
  const char *syntax;
  const char *params;
  const char *examples;
  const char *see_also;
} MYX_GRT_SHELL_COMMAND_HELP_TEXT;

typedef struct
{
  const char *group_name;
  const char *group_caption;
  const char *group_desc;
  MYX_GRT_SHELL_COMMAND_HELP_TEXT *commands;
} MYX_GRT_SHELL_COMMAND_HELP_GROUP;

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_shell[] =
{
  {
    "quit",
    "Quits the application. Instead of quit the synonym 'exit' can be used.",
    "quit"NL
      "exit",
    NULL,
    NULL,
    NULL
  },
  {
    "exit",
    "Quits the application. 'exit' is a synonym for 'quit'.",
    "quit"NL
      "exit",
    NULL,
    NULL,
    NULL
  },
  {
    "ls",
    "When called without options all sub-objects of the current global object are "
      "printed. ls can also be used to list the items stored in a Lua table (-t) or "
      "to print information about the available GRT modules.",
    "ls [-t luaTable | -m [moduleName] | -s [structName]]",
    "-t             Prints a list of all items of the given Lua table."NL
      "luaTable       The Lua table to be printed."NL
      "-m             Prints a list of all available GRT modules."NL
      "moduleName     If a module name is specified, a list of functions the module"NL
      "-s             Prints a list of all available GRT structs."NL
      "structName     If a struct name is specfiied, a list of member variables"NL
      "               contains is printed.",
    "  ls -t grtV"NL
      "               Lists all entries of te grtV Lua table."NL
      "  ls -m"NL
      "               Lists all available GRT modules."NL
      "  ls -m Base"NL
      "               Lists the functions of the GRT module 'Base'",
    "  help cd, help show, help table, help grtM"
  },
  {
    "cd",
    "Changes the current global object. The current global object is displayed in "
      "the GRT shell prompt and is used for the ls and show shell commands. If cd "
      "is called without parameters the current global object path is printed.",
    "cd [objectName | .. | path]",
    "objectName     The name of the object to change into relative from the"NL
      "               current path."NL
      "..             Goes up one object relative to the current path."NL
      "path           Absolute (starting with /) or relative path (starting with ./)"NL
      "               path of the object to make the new current global object.",
    "  cd Migration"NL
      "               Changes the current global object to ./Migration. If the current"NL
      "               global object has been e.g. '/app' it is changed to "NL
      "               '/app/Migration'."NL
      "  cd .."NL
      "               If the current global object has been e.g. '/app/Migration' it is"NL
      "               changed to '/app'.",
    "  help ls, help show"
  },
  {
    "show",
    "Prints the current global object or the object with the given path.",
    "show [path]",
    "path           The absolute (starting with /) or relative (starting with ./) path"NL
      "               of the object to print.",
    "  cd Migration"NL
      "               Changes the current global object to ./Migration. If the current"NL
      "               global object has been e.g. '/app' it is changed to "NL
      "               '/app/Migration'."NL
      "  cd .."NL
      "               If the current global object has been e.g. '/app/Migration' it is"NL
      "               changed to '/app'.",
    "  help ls, help show"
  },
  {
    "run",
    "Load and execute a lua script file.",
    "run filename",
    "filename       File that should be loaded and executed.",
    "  run scripts/test.lua"NL
      "               Runs the script scripts/test.lua.",
    NULL
  },
  {
    "/path",
    "Used for fast access to global objects. /path is converted to grtV.getGlobal() "
      "and grtV.setGlobal() by the shell's preprocessor. Please note that you cannot "
      "use /path in scripts.",
    "/path = var                 Assigns the Lua variable var to the global object "NL
      "                            defined by /path."NL
      "//path with spaces/ = var   Assigns the Lua variable var to the global object "NL
      "                            defined by /path if the path contains spaces."NL
      "var = /path                 Assigns the global object defined by /path to then "NL
      "                            Lua variable var."NL
      "var = //path with space/    Assigns the global object defined by /path to then "NL
      "                            Lua variable var whifen the path contains spaces.",
    NULL,
    "  /test= 27"NL
      "               Assigns the number 27 to the global object with the path '/test'."NL
      "  //test object/= \"testing\""NL
      "               Assigns the string \"testing\" to the global object with the path"NL
      "               '/test object'."NL
      "  catalog= /migration/srcCatalog"NL
      "               Assigns the global object with the path '/migration/srcCatalog'"NL
      "               to the Lua variable catalog",
    "  help ls, help cd"
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help__G[] = 
{ 
  {
    "assert",
    "Issues an error when the value of its argument v is nil or false; otherwise, returns this value. message is an error message; when absent, it defaults to \"assertion failed!\"",
    "assert (v [, message])",
    NULL,
    NULL,
    NULL
  },
  {
    "collectgarbage",
    "Sets the garbage-collection threshold to the given limit (in Kbytes) and checks it against the byte counter. If the new threshold is smaller than the byte counter, then Lua immediately runs the garbage collector. If limit is absent, it defaults to zero (thus forcing a garbage-collection cycle).",
    "collectgarbage ([limit])",
    NULL,
    NULL,
    NULL
  },
  {
    "dofile",
    "Opens the named file and executes its contents as a Lua chunk. When called without arguments, dofile executes the contents of the standard input (stdin). Returns any value returned by the chunk. In case of errors, dofile propagates the error to its caller (that is, it does not run in protected mode).",
    "dofile (filename)",
    NULL,
    NULL,
    NULL
  },
  {
    "error",
    "Terminates the last protected function called and returns message as the error message. Function error never returns. The level argument specifies where the error message points the error. With level 1 (the default), the error position is where the error function was called. Level 2 points the error to where the function that called error was called; and so on.",
    "error (message [, level])",
    NULL,
    NULL,
    NULL
  },
  {
    "_G",
    "A global variable (not a function) that holds the global environment (that is, _G._G = _G). Lua itself does not use this variable; changing its value does not affect any environment. (Use setfenv to change environments.)",
    "_G",
    NULL,
    NULL,
    NULL
  },
  {
    "getfenv",
    "Returns the current environment in use by the function. f can be a Lua function or a number, which specifies the function at that stack level: Level 1 is the function calling getfenv. If the given function is not a Lua function, or if f is 0, getfenv returns the global environment. The default for f is 1. If the environment has a \"__fenv\" field, returns the associated value, instead of the environment.",
    "getfenv (f)",
    NULL,
    NULL,
    NULL
  },
  {
    "getmetatable",
    "If the object does not have a metatable, returns nil. Otherwise, if the object's metatable has a \"__metatable\" field, returns the associated value. Otherwise, returns the metatable of the given object.",
    "getmetatable (object)",
    NULL,
    NULL,
    NULL
  },
  {
    "gcinfo",
    "Returns two results: the number of Kbytes of dynamic memory that Lua is using and the current garbage collector threshold (also in Kbytes).",
    "gcinfo ()",
    NULL,
    NULL,
    NULL
  },
  {
    "input",
    "Promnts for a keyboard input.",
    "input (str)",
    "str            String that will be printed before the input prompt"NL,
    "  res= input(\"Please enter a number form the list:\")"NL
    "               Asks the user to enter a number and stores it in the variable res",
    NULL
  },
  {
    "ipairs",
    "Returns an iterator function, the table t, and 0, so that the construction"NL"  for i,v in ipairs(t) do ... end"NL"will iterate over the pairs (1,t[1]), (2,t[2]), ..., up to the first integer key with a nil value in the table.",
    "ipairs (t)",
    NULL,
    NULL,
    NULL
  },
  {
    "loadfile",
    "Loads a file as a Lua chunk (without running it). If there are no errors, returns the compiled chunk as a function; otherwise, returns nil plus the error message. The environment of the returned function is the global environment.",
    "loadfile (filename)",
    NULL,
    NULL,
    NULL
  },
  {
    "loadlib",
    "Links the program with the dynamic C library libname. Inside this library, looks for a function funcname and returns this function as a C function. libname must be the complete file name of the C library, including any eventual path and extension.",
    "loadlib (libname, funcname)",
    NULL,
    NULL,
    NULL
  },
  {
    "loadstring",
    "Loads a string as a Lua chunk (without running it). If there are no errors, returns the compiled chunk as a function; otherwise, returns nil plus the error message. The environment of the returned function is the global environment. The optional parameter chunkname is the name to be used in error messages and debug information. To load and run a given string, use the idiom"NL"  assert(loadstring(s))()",
    "loadlib (libname, funcname)",
    NULL,
    NULL,
    NULL
  },
  {
    "next",
    "Allows a program to traverse all fields of a table. Its first argument is a table and its second argument is an index in this table. next returns the next index of the table and the value associated with the index. When called with nil as its second argument, next returns the first index of the table and its associated value. When called with the last index, or with nil in an empty table, next returns nil. If the second argument is absent, then it is interpreted as nil. Lua has no declaration of fields; There is no difference between a field not present in a table or a field with value nil. Therefore, next only considers fields with non-nil values. The order in which the indices are enumerated is not specified, even for numeric indices. (To traverse a table in numeric order, use a numerical for or the ipairs function.). The behavior of next is undefined if, during the traversal, you assign any value to a non-existent field in the table.",
    "next (table [, index])",
    NULL,
    NULL,
    NULL
  },
  {
    "pairs",
    "Returns the next function and the table t (plus a nil), so that the construction"NL"  for k,v in pairs(t) do ... end"NL"will iterate over all key-value pairs of table t.",
    "pairs (t)",
    NULL,
    NULL,
    NULL
  },
  {
    "pcall",
    "Calls function f with the given arguments in protected mode. That means that any error inside f is not propagated; instead, pcall catches the error and returns a status code. Its first result is the status code (a boolean), which is true if the call succeeds without errors. In such case, pcall also returns all results from the call, after this first result. In case of any error, pcall returns false plus the error message.",
    "pcall (f, arg1, arg2, ...)",
    NULL,
    NULL,
    NULL
  },
  {
    "print",
    "Receives any number of arguments, and prints their values in stdout, using the tostring function to convert them to strings. This function is not intended for formatted output, but only as a quick way to show a value, typically for debugging. For formatted output, use format.",
    "print (e1, e2, ...)",
    NULL,
    NULL,
    NULL
  },
  {
    "rawequal",
    "Checks whether v1 is equal to v2, without invoking any metamethod. Returns a boolean.",
    "rawequal (v1, v2)",
    NULL,
    NULL,
    NULL
  },
  {
    "rawget",
    "Gets the real value of table[index], without invoking any metamethod. table must be a table; index is any value different from nil.",
    "rawget (table, index)",
    NULL,
    NULL,
    NULL
  },
  {
    "rawset",
    "Sets the real value of table[index] to value, without invoking any metamethod. table must be a table, index is any value different from nil, and value is any Lua value.",
    "rawset (table, index, value)",
    NULL,
    NULL,
    NULL
  },
  {
    "require",
    "Loads the given package. The function starts by looking into the table _LOADED to determine whether packagename is already loaded. If it is, then require returns the value that the package returned when it was first loaded. Otherwise, it searches a path looking for a file to load. If the global variable LUA_PATH is a string, this string is the path. Otherwise, require tries the environment variable LUA_PATH. As a last resort, it uses the predefined path \"?;?.lua\".",
    "require (packagename)",
    NULL,
    NULL,
    NULL
  },
  {
    "setfenv",
    "Sets the current environment to be used by the given function. f can be a Lua function or a number, which specifies the function at that stack level: Level 1 is the function calling setfenv. As a special case, when f is 0 setfenv changes the global environment of the running thread. If the original environment has a \"__fenv\" field, setfenv raises an error.",
    "setfenv (f, table)",
    NULL,
    NULL,
    NULL
  },
  {
    "setmetatable",
    "Sets the metatable for the given table. (You cannot change the metatable of a userdata from Lua.) If metatable is nil, removes the metatable of the given table. If the original metatable has a \"__metatable\" field, raises an error.",
    "setmetatable (table, metatable)",
    NULL,
    NULL,
    NULL
  },
  {
    "sleep",
    "Pauses the execution of the script for ms milliseconds.",
    "sleep (ms)",
    "ms             Time to sleep in ms."NL,
    "  sleep(1000)"NL
    "               Sleeps for one second.",
    NULL
  },
  {
    "tonumber",
    "Tries to convert its argument to a number. If the argument is already a number or a string convertible to a number, then tonumber returns that number; otherwise, it returns nil. An optional argument specifies the base to interpret the numeral. The base may be any integer between 2 and 36, inclusive. In bases above 10, the letter `A¥ (in either upper or lower case) represents 10, `B¥ represents 11, and so forth, with `Z¥ representing 35. In base 10 (the default), the number may have a decimal part, as well as an optional exponent part (see 2.2.1). In other bases, only unsigned integers are accepted.",
    "tonumber (e [, base])",
    NULL,
    NULL,
    NULL
  },
  {
    "tostring",
    "Receives an argument of any type and converts it to a string in a reasonable format. For complete control of how numbers are converted, use format. If the metatable of e has a \"__tostring\" field, tostring calls the corresponding value with e as argument, and uses the result of the call as its result.",
    "tostring (e)",
    NULL,
    NULL,
    NULL
  },
  {
    "type",
    "Returns the type of its only argument, coded as a string. The possible results of this function are \"nil\" (a string, not the value nil), \"number\", \"string\", \"boolean\", \"table\", \"function\", \"thread\", and \"userdata\".",
    "type (v)",
    NULL,
    NULL,
    NULL
  },
  {
    "unpack",
    "Returns all elements from the given list. This function is equivalent to \"NL\"  return list[1], list[2], ..., list[n]"NL"except that the above code can be written only for a fixed n. The number n is the size of the list, as defined for the table.getn function.",
    "unpack (list)",
    NULL,
    NULL,
    NULL
  },
  {
    "_VERSION",
    "A global variable (not a function) that holds a string containing the current interpreter version.",
    "_VERSION)",
    NULL,
    NULL,
    NULL
  },
  {
    "xpcall",
    "This function is similar to pcall, except that you can set a new error handler. xpcall calls function f in protected mode, using err as the error handler. Any error inside f is not propagated; instead, xpcall catches the error, calls the err function with the original error object, and returns a status code. Its first result is the status code (a boolean), which is true if the call succeeds without errors. In such case, xpcall also returns all results from the call, after this first result. In case of any error, xpcall returns false plus the result from err.",
    "xpcall (f, err))",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_grt[] =
{
  /* obsolete
  {
    "success",
    "Used to return a value from a GRT module function written in Lua. The result value will be encapsulate into a new dict.",
    "grt.success (result)",
    "result         The value which should be returned by the GRT module function.",
    "  function calcDouble(args)"NL
      "    return _success(args[1] * 2)"NL
      "  end",
    "  help grt.error"
  },
  {
    "error",
    "Used to return an error from a GRT module function written in Lua.",
    "grt.error (errorText, errorDetails)",
    "errorText      the error message"NL
    "errorDetails   detailed information about the error",
    "  function calcDouble(args)"NL
      "    return args[1] and "NL
      "      grt.success(args[1] * 2) or "NL
      "      grt.error(\"You need to pass a number as argument\")"NL
      "  end",
    "  help grt.success"
  },
  {
    "getRes",
    "Prints the error message if a GRT module function call failed and passes the result through. Checks if the grtError and prints the error if it is set.",
    "grt.getRes(result)",
    "result         the result from a GRT module function call.",
    "  res= grt.getRes(Base:getAppDataDir())"NL
    "               Calls the Base:getAppDataDir() module function and prints the "NL
    "               error if any."NL,
    "  help grt.success, help grt.error"
  },
  {
    "getResLua",
    "Prints the error message if a GRT module function call failed and passes the result converted to Lua objects. Checks if the grtError and prints the error if it is set.",
    "grt.getResLua(result)",
    "result         the result from a GRT module function call.",
    "  res= grt.getResLua(Base:getAppDataDir())"NL
      "               Calls the Base:getAppDataDir() module function and prints the "NL
      "               error if any."NL,
    "  help grt.success, help grt.error"
  },*/
  {
    "newGuid",
    "Returns a new generated GUID as lua string.",
    "grt.newGuid()",
    NULL,
    "  print(grt.newGuid())"NL
      "               Prints a new generated GUID.",
    NULL
  },
  {
    "fileExists",
    "Checks if a files exists. Tries to open the file. If the file exists it will close the file again and return true. If it does not exists it will return false (as Lua will return false when nothing is returned explicitly",
    "grt.fileExists(filename)",
    "filename       the file to check",
    NULL,
    NULL
  },
  {
    "split",
    "Splits the given Lua string based on the separator and puts the tokens into a Lua list.",
    "function grt.split(str, sep)",
    "str            the string to split"NL
      "sep            the separator character",
    NULL,
    NULL
  },
  /* obsolete
  {
    "callRemoteFunction",
    "Invokes a remote function, polling the agent until it is finished. Once finished, it will return the exit status and result value.",
    "grt.callRemoteFunction(agent, module, funcname, argument, syncGlobals)",
    "agent          a table with connection information, e.g. {hostname= \"192.168.1.100\","NL
    "               port= 12345}"NL
      "module         name of the module to call"NL
      "funcname       name of the module function to call"NL
      "argument       the arguments to pass to the module function (have to be in a list)"NL
      "syncGlobals    if true the global object trees are synced on both sides",
    "  agent= {hostname= \"192.168.1.100\", port= 12345}"NL
      "  res= grt.callRemoteFunction(agent, \"BaseJava\", \"engineVersion\", nil, false)",
    NULL
  },*/
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_coroutine[] = 
{ 
  {
    "create",
    "Creates a new coroutine, with body f. f must be a Lua function. Returns this new coroutine, an object with type \"thread\".",
    "coroutine.create (f)",
    NULL,
    NULL,
    NULL
  },
  {
    "resume",
    "Starts or continues the execution of coroutine co. The first time you resume a coroutine, it starts running its body. The arguments val1, ... go as the arguments to the body function. If the coroutine has yielded, resume restarts it; the arguments val1, ... go as the results from the yield. If the coroutine runs without any errors, resume returns true plus any values passed to yield (if the coroutine yields) or any values returned by the body function (if the coroutine terminates). If there is any error, resume returns false plus the error message.",
    "coroutine.resume (co, val1, ...)",
    NULL,
    NULL,
    NULL
  },
  {
    "status",
    "Returns the status of coroutine co, as a string: \"running\", if the coroutine is running (that is, it called status); \"suspended\", if the coroutine is suspended in a call to yield, or if it has not started running yet; and \"dead\" if the coroutine has finished its body function, or if it has stopped with an error.",
    "coroutine.status (co)",
    NULL,
    NULL,
    NULL
  },
  {
    "wrap",
    "Creates a new coroutine, with body f. f must be a Lua function. Returns a function that resumes the coroutine each time it is called. Any arguments passed to the function behave as the extra arguments to resume. Returns the same values returned by resume, except the first boolean. In case of error, propagates the error.",
    "coroutine.wrap (f)",
    NULL,
    NULL,
    NULL
  },
  {
    "yield",
    "Suspends the execution of the calling coroutine. The coroutine cannot be running neither a C function, nor a metamethod, nor an iterator. Any arguments to yield go as extra results to resume.",
    "coroutine.yield (val1, ...)",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_string[] = 
{ 
  {
    "byte",
    "Returns the internal numerical code of the i-th character of s, or nil if the index is out of range. If i is absent, then it is assumed to be 1. i may be negative.",
    "string.byte (s [, i])",
    NULL,
    NULL,
    NULL
  },
  {
    "char",
    "Receives 0 or more integers. Returns a string with length equal to the number of arguments, in which each character has the internal numerical code equal to its correspondent argument.",
    "string.char (i1, i2, ...)",
    NULL,
    NULL,
    NULL
  },
  {
    "dump",
    "Returns a binary representation of the given function, so that a later loadstring on that string returns a copy of the function. function must be a Lua function without upvalues.",
    "string.dump (function)",
    NULL,
    NULL,
    NULL
  },
  {
    "find",
    "Looks for the first match of pattern in the string s. If it finds one, then find returns the indices of s where this occurrence starts and ends; otherwise, it returns nil. If the pattern specifies captures (see string.gsub below), the captured strings are returned as extra results. A third, optional numerical argument init specifies where to start the search; it may be negative and its default value is 1. A value of true as a fourth, optional argument plain turns off the pattern matching facilities, so the function does a plain \"find substring\" operation, with no characters in pattern being considered \"magic\". Note that if plain is given, then init must be given too.",
    "string.find (s, pattern [, init [, plain]])",
    NULL,
    NULL,
    NULL
  },
  {
    "len",
    "Receives a string and returns its length. The empty string \"\" has length 0. Embedded zeros are counted, so \"a\\000b\\000c\" has length 5.",
    "string.len (s)",
    NULL,
    NULL,
    NULL
  },
  {
    "lower",
    "Receives a string and returns a copy of that string with all uppercase letters changed to lowercase. All other characters are left unchanged. The definition of what is an uppercase letter depends on the current locale.",
    "string.lower (s)",
    NULL,
    NULL,
    NULL
  },
  {
    "rep",
    "Returns a string that is the concatenation of n copies of the string s.",
    "string.rep (s, n)",
    NULL,
    NULL,
    NULL
  },
  {
    "sub",
    "Returns the substring of s that starts at i and continues until j; i and j may be negative. If j is absent, then it is assumed to be equal to -1 (which is the same as the string length). In particular, the call string.sub(s,1,j) returns a prefix of s with length j, and string.sub(s, -i) returns a suffix of s with length i.",
    "string.sub (s, i [, j])",
    NULL,
    NULL,
    NULL
  },
  {
    "upper",
    "Receives a string and returns a copy of that string with all lowercase letters changed to uppercase. All other characters are left unchanged. The definition of what is a lowercase letter depends on the current locale.",
    "string.upper (s)",
    NULL,
    NULL,
    NULL
  },
  {
    "format",
    "Returns a formatted version of its variable number of arguments following the description given in its first argument (which must be a string). The format string follows the same rules as the printf family of standard C functions. The only differences are that the options/modifiers *, l, L, n, p, and h are not supported, and there is an extra option, q. The q option formats a string in a form suitable to be safely read back by the Lua interpreter: The string is written between double quotes, and all double quotes, newlines, and backslashes in the string are correctly escaped when written. The options c, d, E, e, f, g, G, i, o, u, X, and x all expect a number as argument, whereas q and s expect a string. The * modifier can be simulated by building the appropriate format string. For example, \"%*g\" can be simulated with \"%\"..width..\"g\". String values to be formatted with %s cannot contain embedded zeros.",
    "string.format (formatstring, e1, e2, ...)",
    NULL,
    "  string.format('%q', 'a string with \"quotes\" and \n new line')"NL
      "             will produce the string:"NL
      "\"a string with \\\"quotes\\\" and \\"NL
      " new line\"",
    NULL
  },
  {
    "gfind",
    "Returns an iterator function that, each time it is called, returns the next captures from pattern pat over string s. If pat specifies no captures, then the whole match is produced in each call.",
    "string.gfind (s, pat)",
    NULL,
    "  s = \"hello world from Lua\""NL
      "  for w in string.gfind(s, \"%a+\") do"NL
      "    print(w)"NL
      "  end"NL
      "               will iterate over all the words from string s, printing"NL
      "               one per line. The next example collects all pairs key=value"NL
      "               from the given string into a table:"NL
      "  t = {}"NL
      "  s = \"from=world, to=Lua\""NL
      "  for k, v in string.gfind(s, \"(%w+)=(%w+)\") do"NL
      "    t[k] = v"NL
      "  end",
    NULL
  },
  {
    "gsub",
    "Returns a copy of s in which all occurrences of the pattern pat have been replaced by a replacement string specified by repl. gsub also returns, as a second value, the total number of substitutions made. If repl is a string, then its value is used for replacement. Any sequence in repl of the form %n, with n between 1 and 9, stands for the value of the n-th captured substring (see below). If repl is a function, then this function is called every time a match occurs, with all captured substrings passed as arguments, in order; if the pattern specifies no captures, then the whole match is passed as a sole argument. If the value returned by this function is a string, then it is used as the replacement string; otherwise, the replacement string is the empty string. The optional last parameter n limits the maximum number of substitutions to occur. For instance, when n is 1 only the first occurrence of pat is replaced.",
    "string.gsub (s, pat, repl [, n])",
    NULL,
    "  x = string.gsub(\"hello world\", \"(%w+)\", \"%1 %1\")"NL
      "  --> x=\"hello hello world world\""NL
      NL
      "  x = string.gsub(\"hello world\", \"(%w+)\", \"%1 %1\", 1)"NL
      "  --> x=\"hello hello world\""NL
      NL
      "  x = string.gsub(\"hello world from Lua\", \"(%w+)%s*(%w+)\", \"%2 %1\")"NL
      "  --> x=\"world hello Lua from\""NL
      NL
      "  x = string.gsub(\"home = $HOME, user = $USER\", \"%$(%w+)\", os.getenv)"NL
      "  --> x=\"home = /home/roberto, user = roberto\""NL
      NL
      "  x = string.gsub(\"4+5 = $return 4+5$\", \"%$(.-)%$\", function (s)"NL
      "        return loadstring(s)()"NL
      "      end)"NL
      "  --> x=\"4+5 = 9\""NL
      NL
      "  local t = {name=\"lua\", version=\"5.0\"}"NL
      "  x = string.gsub(\"$name_$version.tar.gz\", \"%$(%w+)\", function (v)"NL
      "        return t[v]"NL
      "      end)"NL
      "  --> x=\"lua_5.0.tar.gz\""NL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_table[] = 
{ 
  {
    "concat",
    "Returns table[i]..sep..table[i+1] ... sep..table[j]. The default value for sep is the empty string, the default for i is 1, and the default for j is the size of the table. If i is greater than j, returns the empty string.",
    "table.concat (table [, sep [, i [, j]]])",
    NULL,
    NULL,
    NULL
  },
  {
    "foreach",
    "Executes the given f over all elements of table. For each element, f is called with the index and respective value as arguments. If f returns a non-nil value, then the loop is broken, and this value is returned as the final value of foreach.",
    "table.foreach (table, f)",
    NULL,
    NULL,
    "help _G.next"
  },
  {
    "foreachi",
    "Executes the given f over the numerical indices of table. For each index, f is called with the index and respective value as arguments. Indices are visited in sequential order, from 1 to n, where n is the size of the table. If f returns a non-nil value, then the loop is broken and this value is returned as the result of foreachi.",
    "table.foreachi (table, f)",
    NULL,
    NULL,
    NULL
  },
  {
    "getn",
    "Returns the size of a table, when seen as a list. If the table has an n field with a numeric value, this value is the size of the table. Otherwise, if there was a previous call to table.setn over this table, the respective value is returned. Otherwise, the size is one less the first integer index with a nil value.",
    "table.getn (table)",
    NULL,
    NULL,
    NULL
  },
  {
    "sort",
    "Sorts table elements in a given order, in-place, from table[1] to table[n], where n is the size of the table. If comp is given, then it must be a function that receives two table elements, and returns true when the first is less than the second (so that not comp(a[i+1],a[i]) will be true after the sort). If comp is not given, then the standard Lua operator < is used instead.",
    "table.sort (table [, comp])",
    NULL,
    NULL,
    NULL
  },
  {
    "insert",
    "Inserts element value at position pos in table, shifting up other elements to open space, if necessary. The default value for pos is n+1, where n is the size of the table, so that a call table.insert(t,x) inserts x at the end of table t. This function also updates the size of the table by calling table.setn(table, n+1).",
    "table.insert (table, [pos,] value)",
    NULL,
    NULL,
    NULL
  },
  {
    "remove",
    "Removes from table the element at position pos, shifting down other elements to close the space, if necessary. Returns the value of the removed element. The default value for pos is n, where n is the size of the table, so that a call table.remove(t) removes the last element of table t. This function also updates the size of the table by calling table.setn(table, n-1).",
    "table.remove (table [, pos])",
    NULL,
    NULL,
    NULL
  },
  {
    "setn",
    "Updates the size of a table. If the table has a field \"n\" with a numerical value, that value is changed to the given n. Otherwise, it updates an internal state so that subsequent calls to table.getn(table) return n.",
    "table.setn (table, n)",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_math[] = 
{ 
  {
    "abs",
    NULL,
    "math.abs (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "acos",
    "Returns the arc cosine of x in radians.",
    "math.acos (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "asin",
    "Returns the arc sine of x in radians.",
    "math.asin (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "atan",
    "Returns the arc tangent of x in radians.",
    "math.atan (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "atan2",
    "Returns the arc tangent in radians of y/x based on the signs of both values to determine the correct quadrant.",
    "math.atan2 (y, x)",
    NULL,
    NULL,
    NULL
  },
  {
    "ceil",
    "Returns the absolute value of x (a negative value becomes positive, positive value is unchanged).",
    "math.ceil (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "cos",
    "Returns the cosine of a radian angle x.",
    "math.cos (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "deg",
    NULL,
    "math.deg (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "exp",
    "Returns the value of e raised to the xth power.",
    "math.exp (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "floor",
    "Returns the largest integer value less than or equal to x.",
    "math.floor (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "log",
    "Returns the natural logarithm (base-e logarithm) of x.",
    "math.log (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "log10",
    "Returns the common logarithm (base-10 logarithm) of x.",
    "math.log10 (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "max",
    NULL,
    "math.max (x, y)",
    NULL,
    NULL,
    NULL
  },
  {
    "min",
    NULL,
    "math.min (x, y)",
    NULL,
    NULL,
    NULL
  },
  {
    "mod",
    NULL,
    "math.mod (x, y)",
    NULL,
    NULL,
    NULL
  },
  {
    "pow",
    "Returns x raised to the power of y.",
    "math.pow (x, y)",
    NULL,
    NULL,
    NULL
  },
  {
    "rad",
    NULL,
    "math.rad (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "sin",
    "Returns the sine of a radian angle x.",
    "math.sin (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "sqrt",
    "Returns the square root of x.",
    "math.sqrt (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "tan",
    "Returns the tangent of a radian angle x.",
    "math.tan (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "frexp",
    "The floating-point number x is broken up into a mantissa and exponent.  The returned value is the mantissa and the integer pointed to by exponent is the exponent. The resultant value is x=mantissa * 2^exponent.",
    "math.frexp (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "ldexp",
    "Returns x multiplied by 2 raised to the power of exponent. x*2^exponent",
    "math.ldexp (x, y)",
    NULL,
    NULL,
    NULL
  },
  {
    "random",
    NULL,
    "math.random (x)",
    NULL,
    NULL,
    NULL
  },
  {
    "randomseed",
    NULL,
    "math.randomseed (x)",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_io[] = 
{ 
  {
    "close",
    "Equivalent to file:close. Without a file, closes the default output file.",
    "io.close ([file])",
    NULL,
    NULL,
    NULL
  },
  {
    "flush",
    "Equivalent to file:flush over the default output file.",
    "io.flush ()",
    NULL,
    NULL,
    NULL
  },
  {
    "input",
    "When called with a file name, it opens the named file (in text mode), and sets its handle as the default input file. When called with a file handle, it simply sets that file handle as the default input file. When called without parameters, it returns the current default input file. In case of errors this function raises the error, instead of returning an error code.",
    "io.input ([file])",
    NULL,
    NULL,
    NULL
  },
  {
    "lines",
    "Opens the given file name in read mode and returns an iterator function that, each time it is called, returns a new line from the file.",
    "io.lines ([filename])",
    NULL,
    "  for line in io.lines(filename) do ... end"NL
      "               This construction will iterate over all lines of the file."NL
      "               When the iterator function detects the end of file, it returns"NL
      "               nil (to finish the loop) and automatically closes the file.",
    NULL
  },
  {
    "open",
    "This function opens a file, in the mode specified in the string mode. It returns a new file handle, or, in case of errors, nil plus an error message. The mode string may also have a b at the end, which is needed in some systems to open the file in binary mode. This string is exactly what is used in the standard C function fopen.",
    "io.open (filename [, mode])",
    "filename         Name of the file to open."NL
      "mode             The mode string can be any of the following:"NL
      "                 \"r\" read mode (the default);"NL
      "                 \"w\" write mode;"NL
      "                 \"a\" append mode;"NL
      "                 \"r+\" update mode, all previous data is preserved;"NL
      "                 \"w+\" update mode, all previous data is erased;"NL
      "                 \"a+\" append update mode, previous data is preserved, writing is"NL
      "                 only allowed at the end of file.",
    NULL,
    NULL
  },
  {
    "output",
    "Similar to io.input, but operates over the default output file.",
    "io.output ([file])",
    NULL,
    NULL,
    NULL
  },
  {
    "read",
    "Equivalent to io.input():read.",
    "io.read (format1, ...)",
    NULL,
    NULL,
    NULL
  },
  {
    "tmpfile",
    "Returns a handle for a temporary file. This file is open in update mode and it is automatically removed when the program ends.",
    "io.tmpfile ()",
    NULL,
    NULL,
    NULL
  },
  {
    "type",
    "Checks whether obj is a valid file handle. Returns the string \"file\" if obj is an open file handle, \"closed file\" if obj is a closed file handle, and nil if obj is not a file handle.",
    "io.type (obj)",
    NULL,
    NULL,
    NULL
  },
  {
    "write",
    "Equivalent to io.output():write.",
    "io.write (value1, ...)",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_file[] = 
{ 
  {
    "close",
    "Closes file.",
    "file:close ()",
    NULL,
    NULL,
    NULL
  },
  {
    "flush",
    "Saves any written data to file.",
    "file:flush ()",
    NULL,
    NULL,
    NULL
  },
  {
    "lines",
    "Returns an iterator function that, each time it is called, returns a new line from the file.",
    "file:lines ()",
    NULL,
    "  for line in file:lines() do ... end"NL
      "               will iterate over all lines of the file. (Unlike io.lines,"NL
      "               this function does not close the file when the loop ends.)",
    NULL
  },
  {
    "read",
    "Reads the file file, according to the given formats, which specify what to read. For each format, the function returns a string (or a number) with the characters read, or nil if it cannot read data with the specified format. When called without formats, it uses a default format that reads the entire next line.",
    "file:read (format1, ...)",
    "format1            The available formats are:"NL
      "                   \"*n\" reads a number; this is the only format that returns a"NL
      "                   number instead of a string."NL
      "                   \"*a\" reads the whole file, starting at the current position."NL
      "                   On end of file, it returns the empty string."NL
      "                   \"*l\" reads the next line (skipping the end of line), "NL
      "                   returning nil on end of file. This is the default format."NL
      "                   number reads a string with up to that number of characters,"NL
      "                   returning nil on end of file. If number is zero, it reads"NL
      "                   nothing and returns an empty string, or nil on end of file.",
    NULL,
    NULL
  },
  {
    "seek",
    "Sets and gets the file position, measured from the beginning of the file, to the position given by offset plus a base specified by the string whence. In case of success, function seek returns the final file position, measured in bytes from the beginning of the file. If this function fails, it returns nil, plus a string describing the error. The default value for whence is \"cur\", and for offset is 0. Therefore, the call file:seek() returns the current file position, without changing it; the call file:seek(\"set\") sets the position to the beginning of the file (and returns 0); and the call file:seek(\"end\") sets the position to the end of the file, and returns its size.",
    "file:seek ([whence] [, offset])",
    "whence             \"set\" base is position 0 (beginning of the file);"NL
      "                   \"cur\" base is current position;"NL
      "                   \"end\" base is end of file;",
    NULL,
    NULL
  },
  {
    "write",
    "Writes the value of each of its arguments to the filehandle file. The arguments must be strings or numbers. To write other values, use tostring or string.format before write.",
    "file:write (value1, ...)",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_os[] = 
{ 
  {
    "clock",
    "Returns an approximation of the amount of CPU time used by the program, in seconds.",
    "os.clock ()",
    NULL,
    NULL,
    NULL
  },
  {
    "date",
    "Returns a string or a table containing date and time, formatted according to the given string format. If the time argument is present, this is the time to be formatted (see the os.time function for a description of this value). Otherwise, date formats the current time. If format starts with `!¥, then the date is formatted in Coordinated Universal Time. After that optional character, if format is *t, then date returns a table with the following fields: year (four digits), month (1--12), day (1--31), hour (0--23), min (0--59), sec (0--61), wday (weekday, Sunday is 1), yday (day of the year), and isdst (daylight saving flag, a boolean). If format is not *t, then date returns the date as a string, formatted according to the same rules as the C function strftime. When called without arguments, date returns a reasonable date and time representation that depends on the host system and on the current locale (that is, os.date() is equivalent to os.date(\"%c\")).",
    "os.date ([format [, time]])",
    NULL,
    NULL,
    NULL
  },
  {
    "difftime",
    "Returns the number of seconds from time t1 to time t2. In Posix, Windows, and some other systems, this value is exactly t2-t1.",
    "os.difftime (t2, t1)",
    NULL,
    "  for line in file:lines() do ... end"NL
      "               will iterate over all lines of the file. (Unlike io.lines,"NL
      "               this function does not close the file when the loop ends.)",
    NULL
  },
  {
    "read",
    "This function is equivalent to the C function system. It passes command to be executed by an operating system shell. It returns a status code, which is system-dependent.",
    "os.execute (command)",
    NULL,
    NULL,
    NULL
  },
  {
    "exit",
    "Calls the C function exit, with an optional code, to terminate the host program. The default value for code is the success code.",
    "os.exit ([code])",
    "whence             \"set\" base is position 0 (beginning of the file);"NL
      "                   \"cur\" base is current position;"NL
      "                   \"end\" base is end of file;",
    NULL,
    NULL
  },
  {
    "getenv",
    "Returns the value of the process environment variable varname, or nil if the variable is not defined.",
    "os.getenv (varname)",
    NULL,
    NULL,
    NULL
  },
  {
    "remove",
    "Deletes the file with the given name. If this function fails, it returns nil, plus a string describing the error.",
    "os.remove (filename)",
    NULL,
    NULL,
    NULL
  },
  {
    "rename",
    "Renames file named oldname to newname. If this function fails, it returns nil, plus a string describing the error.",
    "os.rename (oldname, newname)",
    NULL,
    NULL,
    NULL
  },
  {
    "setlocale",
    "Sets the current locale of the program. locale is a string specifying a locale; category is an optional string describing which category to change: \"all\", \"collate\", \"ctype\", \"monetary\", \"numeric\", or \"time\"; the default category is \"all\". The function returns the name of the new locale, or nil if the request cannot be honored.",
    "os.setlocale (locale [, category])",
    NULL,
    NULL,
    NULL
  },
  {
    "time",
    "Returns the current time when called without arguments, or a time representing the date and time specified by the given table. This table must have fields year, month, and day, and may have fields hour, min, sec, and isdst (for a description of these fields, see the os.date function). The returned value is a number, whose meaning depends on your system. In Posix, Windows, and some other systems, this number counts the number of seconds since some given start time (the \"epoch\"). In other systems, the meaning is not specified, and the number returned by time can be used only as an argument to date and difftime.",
    "os.time ([table])",
    NULL,
    NULL,
    NULL
  },
  {
    "tmpname",
    "Returns a string with a file name that can be used for a temporary file. The file must be explicitly opened before its use and removed when no longer needed. This function is equivalent to the tmpnam C function, and many people (and even some compilers!) advise against its use, because between the time you call this function and the time you open the file, it is possible for another process to create a file with the same name.",
    "os.tmpname ()",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_grtV[] = 
{ 
  {
    "clearList",
    "Clears the given list.",
    "grtV.clearList (list)",
    "list         the GRT list to clear.",
    "  grtV.clearList(pluginList)"NL
      "               will clear the list pluginList",
    NULL
  },
  {
    "child",
    "Returns a child object defined by a subpath.",
    "grtV.child (grtObj, path)",
    "grtObj         the GRT object that contains the child object."NL
      "path           the relative path to the child object.",
    "  res= grtV.child(grtV.getGlobal(\"/rdbmsMgmt\"), \"/storedConns/0\")"NL
      "               will return the first stored connection from the "NL
      "               global \"/rdbmsMgmt\" object.",
    NULL
  },
  /* obsolete
  {
    "diffMake",
    "Makes a diff between two GRT values.",
    "grtV.diffMake (sourceGrtObj, targetGrtObj)",
    "sourceGrtObj         the source GRT value."NL
      "targetGrtObj         the target GRT value",
    "  local diff= grtV.diffMake(src, tar)"NL
      "               creates a diff between the two values and returns it.",
    NULL
  },
  {
    "diffApply",
    "Applys a diff to a GRT value.",
    "grtV.diffApply (grtObj, diff)",
    "grtObj               the GRT value to apply the diff to."NL
      "targetGrtObj         the diff created with grtV.diffMake",
    "  grtV.diffApply(table, diff)"NL
      "               applys the diff to the GRT table value.",
    NULL
  },*/
  //{
  //  "changeTreeMake",
  //  "Makes a change tree for two GRT values.",
  //  "grtV.changeTreeMake (originalGrtObj, modifiedGrtObj)",
  //  "originalGrtObj         the original GRT value."NL
  //    "modifiedGrtObj         a modified GRT value",
  //  "  local tree= grtV.changeTreeMake(org, mod)"NL
  //    "               builds a change tree for the two values and returns it.",
  //  NULL
  //},
  /*
  {
    "duplicate",
    "Returns a duplicate of the original GRT value.",
    "grtV.duplicate (grtObj)",
    "grtObj         the GRT object to be duplicated.",
    "  local schemaList= grtV.duplicate(self.sourceSchemataNames)"NL
      "               creates a duplicate of the original self.sourceSchemataNames"NL
      "               object that can be modified without changing the original.",
    NULL
  },*/
  {
    "fromXml",
    "Generates a GRT value from the given XML string.",
    "value= grtV.fromXml (xmlString)",
    "xmlString      the XML string that represents a GRT value.",
    NULL,
    NULL
  },
  {
    "getContentType",
    "Returns the content type of the given list or dict.",
    "grtV.getContentType (grtObj)",
    "grtObj         The GRT dict or list to check.",
    NULL,
    NULL
  },
  {
    "getKey",
    "Returns the dict's key with the given index.",
    "grtV.getKey (grtDict, index)",
    "grtDict        The GRT dict to check.",
    "  rdbmsMgmt= RdbmsManagement:getManagementInfo()"NL
    "  print(grtV.getKey(rdbmsMgmt.rdbms[1].drivers[1].defaultModules, 1))"NL
      "               Prints the first key of the defaultModules dict.",
    NULL
  },
  {
    "getListItemByObjName",
    "Returns a list item object based on its name.",
    "grtV.getListItemByObjName (grtList, \"name\")",
    "grtList        The GRT list to check."NL
      "name           The object name to search",
    NULL,
    NULL
  },
  {
    "getn",
    "Returns the number of entries in the given list or dict.",
    "grtV.getn (grtValue)",
    "grtList        The GRT list or dict to check.",
    "  print(grtV.getn(grtV.getGlobal(\"/rdbmsMgmt/storedConns\")))"NL
      "               Returns the number of items in the global"NL
      "               \"/rdbmsMgmt/storedConns\" list",
    NULL
  },
  {
    "getGlobal",
    "Returns the global GRT object at the given path. Global objects are referred by a tree that can be accessed from all languages bound to the GRT.",
    "grtV.getGlobal (path)",
    "path           The path to the global object"NL,
    "  grtV.getGlobal(\"/rdbmsMgmt/storedConns/0\")"NL
    "               Returns the GRT object at the global path "NL
    "               \"/rdbmsMgmt/storedConns/0\"",
    NULL
  },
  {
    "insert",
    "Inserts the given obj into the grtList. If index is ommited, the obj will be inserted at the very end of the list.",
    "grtV.insert (grtList, obj [, index])",
    "grtList        The GRT list that will hold the obj."NL
      "obj            the object to insert."NL
      "index          optional index of where to insert the obj in the list",
    "  grtV.insert(rdbmsMgmt.rdbms, rdbms)"NL
      "               appends the rdbms object to the rdbmsMgmt.rdbms list.",
    NULL
  },
  {
    "load",
    "Loads and returns a GRT object from a file.",
    "grtV.load (filename)",
    "filename       Filename of the file to load",
      "  if grt.fileExists(storedConnsFilename) then"NL
      "    rdbmsMgmt.storedConns= grtV.load(storedConnsFilename)"NL
      "  end"NL
      "               Loads the file with the filename stored in"NL
      "               storedConnsFilename and assigns it to rdbmsMgmt.storedConns",
    NULL
  },
  {
    "newDict",
    "Creates a new GRT dict with the optional contentType and contentStructName.",
    "grtV.newDict (contentType, contentStructName)",
    "contentType        If submitted a typed dict is created and the dict can only"NL
      "                   hold members of the given type. Possible values are \"int\","NL
      "                   \"string\", \"real\", \"list\", \"dict\""NL
      "contentStructName  If the contentType is \"dict\" a struct name can be defined."NL
      "                   Only dicts with this struct name can be used as dict members.",
    "  obj= grtV.newDict(\"dict\", \"db.Table\")"NL
      "                   Creates a new dict that can only have dict members of the"NL
      "                   struct db.Table",
    NULL
  },
  {
    "newList",
    "Creates a new GRT list with the optional contentType and contentStructName.",
    "grtV.newList (contentType, contentStructName)",
    "contentType        If submitted a typed list is created and the list can only"NL
      "                   hold values of the given type. Possible values are \"int\","NL
      "                   \"string\", \"real\", \"list\", \"dict\""NL
      "contentStructName  If the contentType is \"dict\" a struct name can be defined."NL
      "                   Only dicts with this struct name can be added to the list.",
    "  list= grtV.newList(\"string\")"NL
      "                   Creates a new list that can only hold string values",
    NULL
  },
  {
    "newObj",
    "Creates a new GRT object initialized with the optional given values. All simple values"NL
    "(\"int\", \"string\", \"real\") are initialized and all lists and dicts are created."NL
    "Object references are left null.",
    "grtV.newObj (structName[, initValuesDict])",
    "structName         Struct name of the object to create"NL
    "initValuesDict     A dictionary containing initial values for object fields."NL,
    "  rdbmsMgmt= grtV.newObj(\"db.mgmt.Management\", "NL
    "    {name=\"rdbmsManagement\", owner=app})"NL
    "                   Create a new object from the struct \"db.mgmt.Management\"",
    NULL
  },
  {
    "remove",
    "Removes the list item with the given index.",
    "grtV.remove (list, index)",
    "list               GRT list to manipulate"NL
    "index              Index of the item to remove",
    NULL,
    NULL
  },
  {
    "save",
    "Saves the given object to a file.",
    "grtV.save (obj, filename)",
    "obj                The object to save."NL
      "filename           The name of the file to create",
    NULL,
    NULL
  },
  {
    "setGlobal",
    "Assigns the object to the given global path.",
    "grtV.setGlobal (path, obj)",
    "path               The global path that the object will be assigned to"NL
      "obj                Object that will be assigned to the global path",
    "  grtV.setGlobal(\"/rdbmsMgmt\", RdbmsManagement:getManagementInfo())"NL
      "                   Calls the module function getManagementInfo() and"NL
      "                   assignes the returned GRT value to the global path"NL
      "                   \"/rdbmsMgmt\"",
    NULL
  },
  {
    "toLua",
    "Converts the given GRT value to a Lua value.",
    "grtV.toLua (value)",
    "value              The GRT value to convert",
    "  print(grtV.toLua(Base:getOsTypeName()))"NL
      "                   Calls the module function getOsTypeName() and"NL
      "                   converts the returned GRT string to a Lua string.",
    NULL
  },
  {
    "toXml",
    "Converts a GRT value to and XML string.",
    "xmlString= grtV.fromXml (value)",
    "value          a GRT value.",
    NULL,
    NULL
  },
  {
    "typeOf",
    "Returns the type of a GRT value.",
    "typeName= grtV.typeOf (value)",
    "value          a GRT value."NL
    "typeName       the name of the type: int, real, string, list, dict, object",
    NULL,
    NULL
  },  
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_grtS[] = 
{ 
  {
    "exists",
    "Returns true if the given struct name is defined.",
    "grtS.exists (structname)",
    NULL,
    NULL,
    NULL
  },
  {
    "get",
    "Returns the struct assigned to the object as a Lua string.",
    "grtS.get (obj)",
    NULL,
    NULL,
    NULL
  },
  {
    "getAttrib",
    "Returns the named attribute from the struct (eg. caption, description etc).",
    "grtS.getAttrib (structName, attribName)",
    NULL,
    NULL,
    NULL
  },
  {
    "getMembers",
    "Returns a Lua table containing all member names of the given struct name.",
    "grtS.getMembers (structName)",
    "structName         The name of the struct",
    "  m= grtS.getMembers(\"db.Schema\")"NL
      "  ls -t m"NL
      "                   Lists all members of the struct \"db.Schema\"",
    NULL
  },
  {
    "getMemberType",
    "Returns the type of a member as Lua string.",
    "grtS.getMemberType (structName, memberName)",
    "structName         The name of the struct"NL
      "memberName         The name of the member",
    "  print(grtS.getMemberType(\"db.Schema\", \"tables\"))"NL
      "                   Prints the type of the member tables.",
    NULL
  },
  {
    "getMemberContentType",
    "Returns the content type of a member as Lua string.",
    "grtS.getMemberContentType (structName, memberName)",
    "structName         The name of the struct"NL
      "memberName         The name of the member",
    "  print(grtS.getMemberContentType(\"db.Schema\", \"tables\"))"NL
      "                   Prints the content type of the member tables.",
    NULL
  },
  {
    "getMemberContentStruct",
    "Returns the content struct name of a member as Lua string.",
    "grtS.getMemberContentStruct (structName, memberName)",
    "structName         The name of the struct"NL
      "memberName         The name of the member",
    "  print(grtS.getMemberContentStruct(\"db.Schema\", \"tables\"))"NL
      "                   Prints the content struct name of the member tables.",
    NULL
  },
  {
    "isOrInheritsFrom",
    "Returns true if the given struct is or inherits from the given struct.",
    "grtS.inheritsFrom (structName, givenStructName)",
    NULL,
    NULL,
    NULL
  },
  {
    "load",
    "Loads struct definitions from a file.",
    "grtS.load (filename)",
    NULL,
    NULL,
    NULL
  },
  {
    "list",
    "Lists all available struct names.",
    "grtS.list ()",
    NULL,
    NULL,
    NULL
  },
  {
    "show",
    "Lists all members of the struct with the given name.",
    "grtS.show (structName)",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_grtM[] = 
{ 
  {
    "callFunction",
    "Calls the module function with the given name and arguments.",
    "grtM.callFunction (moduleName, functionName, argumentlist)",
    NULL,
    NULL,
    NULL
  },
  {
    "get",
    "Returns all module names in a Lua table. If a extend is passed only modules that extend this module are returned.",
    "grtM.get ([extend])",
    NULL,
    "  modules= grtM.get(\"RdbmsInfo\")"NL
      "                   Returns all modules that extend \"RdbmsInfo\"",
    NULL
  },
  {
    "getFunctions",
    "Returns all function names of the given module in a Lua table.",
    "grtM.getFunctions (modulename)",
    NULL,
    NULL,
    NULL
  },
  {
    "list",
    "Lists all available module names.",
    "grtM.list ()",
    NULL,
    NULL,
    NULL
  },
  {
    "show",
    "Lists all functions of the module with the name moduleName.",
    "grtM.show (moduleName)",
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

/* not functional
static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_grtA[] = 
{
  {
    "connect",
    "Creates a new session to a remote GRT agent, in the specified host and port number. Returns the session object.",
    "grtA.connect (host, port)",
    NULL,
    NULL,
    NULL
  },
 {
    "close",
    "Closes a session to a GRT agent, freeing any resources allocated by it.",
    "grtA.close (session)",
    NULL,
    NULL,
    NULL
  },
 {
    "invoke",
    "Invokes a remote function in a GRT agent session.",
    "grtA.invoke (session, module_name, function_name, argument_table)",
    NULL,
    NULL,
    NULL
  },
 {
    "check",
    "Checks the status of the GRT agent."NL
    "Returns 0 if it's idle, 1 if there was an error, 2 if it's executing a call and 3 if it has finished executing a call.",
    "grtA.check (session)",
    NULL,
    NULL,
    NULL
  },
  {
    "finish",
    "Finishes a GRT agent call, returning the value returned by the function called remotely.",
    "grtA.finish (session)",
    NULL,
    NULL,
    NULL
  },
  {
    "messages",
    "Retrieves any messages that are pending in the agent.",
    "grtA.messages (session)",
    NULL,
    NULL,
    NULL
  },
  {
    "getGlobal",
    "Fetches the global GRT value tree from the agent and updates the local value tree from it.",
    "grtA.getGlobal (session)",
    NULL,
    NULL,
    NULL
  },
  {
    "setGlobal",
    "Sends the local global GRT value tree to the agent and make it update it's tree from it.",
    "grtA.setGlobal (session)",
    NULL,
    NULL,
    NULL
  },
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};
*/

static MYX_GRT_SHELL_COMMAND_HELP_TEXT help_grtU[] = 
{
  {
    "regExVal",
    "Evaluates the given regex against the text and returns the substring with the given subStringIndex",
    "grtU.regExVal (text, regex [, subStringIndex])",
    "text               The text to search"NL
      "regex              The regular expression to use"NL
      "subStringIndex     The Substring index to use, 1 if ommited",
    "  print(regexVal(\"Int(10,5)\", "NL
      "    \"(\\\\w*)\\\\s*(\\\\((\\\\d+)\\\\s*(\\\\,\\\\s*(\\\\d+))?\\\\))?\", 1))"NL
      "                   Prints \"Int\"",
    NULL
  },
  {
    "replace",
    "Replaces occurrences of the 'from' string to 'to' in the give text.",
    "grtU.replace (text, from , to)",
    "text             The text to work on"NL
    "from             The string to search"NL
    "to               The string to replace with",
    "  print(replace(\"hello world\", \"hello\", \"bye\"))",
    NULL
  },  
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};

static MYX_GRT_SHELL_COMMAND_HELP_GROUP help_groups[] =
{
  {
    "shell",
    "Shell Command",
    "Commands that can be used in a GRT shell.",
    help_shell
  },
  {
    "_G",
    "Global Functions and Objects",
    "Basic functions and object with a global scope.",
    help__G
  },
  {
    "grt",
    "grt Library",
    "Library with common used functions for GRT scripting",
    help_grt
  },
  {
    "coroutine",
    "Coroutine Manipulation Library",
    "Library to manage collaborative multithreading",
    help_coroutine
  },
  {
    "string",
    "String Manipulation",
    "This library provides generic functions for string manipulation, such as finding"NL
    "and extracting substrings, and pattern matching.",
    help_string
  },
  {
    "file",
    "File Manipulation",
    "This library provides generic functions for file manipulation.",
    help_file
  },
  {
    "table",
    "Table Manipulation",
    "This library provides generic functions for table manipulation.",
    help_table
  },
  {
    "math",
    "Mathematical functions",
    "This library provides mathematical functions.",
    help_math
  },
  {
    "io",
    "Input and Output Facilities",
    "A library for file manipulation.",
    help_io
  },
  {
    "file",
    "The file class",
    "A class for file manipulation.",
    help_io
  },
  {
    "os",
    "Operating System Facilities",
    "A library to work with the operating system.",
    help_os
  },
  {
    "grtV",
    "GRT Value Management Library",
    "A library that contains functions to work with GRT values.",
    help_grtV
  },
  {
    "grtS",
    "GRT Struct Management Library",
    "A library that contains functions to work with GRT structs.",
    help_grtS
  },
  {
    "grtM",
    "GRT Module Management Library",
    "A library that contains functions to work with GRT modules.",
    help_grtM
  },/*
  {
    "grtA",
    "GRT Agent Management Library",
    "A library that contains functions to work with GRT agents.",
    help_grtA
  },*/
  {
    "grtU",
    "GRT Utility Function Library",
    "A library that contains utility functions.",
    help_grtU
  },
  {
    NULL,
    NULL,
    NULL,
    NULL
  }
};

//============================================================================

static void myx_grt_shell_show_command_help_print(grt::GRT *grt, const char *group_name, const char *cmd)
{
  MYX_GRT_SHELL_COMMAND_HELP_GROUP help_group;
  int j, found= 0;

  for (j= 0; (help_group= help_groups[j]).group_name; j++)
  {
    if (strcmp(group_name, help_group.group_name) == 0)
    {
      MYX_GRT_SHELL_COMMAND_HELP_TEXT help_text;
      unsigned int i;

      // check if the group was specified
      if (!cmd)
      {
        char ul[80];
        int k;

        const int max_len = (int)min(strlen(help_group.group_caption) + strlen(help_group.group_name) + 3, 79U);
        for (k= 0; k < max_len; k++)
          ul[k]= '-';
        ul[k]= 0;

        grt->send_output(strfmt(NL
          "%s - %s"NL
          "%s"NL
          "%s"NL
          NL,
          help_group.group_caption,
          help_group.group_name,
          ul,
          help_group.group_desc));
          //help_group.group_name));

        std::string dir;
        std::string spaces= "                        ";

        for(i= 0; (help_text= help_group.commands[i]).cmd; i++)
        {          
          dir.append(help_text.cmd);
          dir.append(spaces.substr(0, spaces.length()-strlen(help_text.cmd)));

          if ((i + 1) % 3 == 0)
            dir.append(NL);
        }

        grt->send_output(dir);

        grt->send_output(strfmt(NL NL
          "Type 'help %s.<command>' to get help on a specific command."NL,
          help_group.group_name));

        found= 1;

        break;
      }
      else
      {
        for(i= 0; (help_text= help_group.commands[i]).cmd; i++)
        {
          if (strcmp(help_text.cmd, cmd) == 0)
          {
            char ul[80];
            unsigned int k, c;

            c= (int)min(strlen(help_group.group_caption) + strlen(help_group.group_name) + strlen(help_text.cmd) + 4, 79U);
            for (k= 0; k < c; k++)
              ul[k]= '-';
            ul[k]= 0;

            grt->send_output(strfmt(NL"%s - %s.%s"NL, help_group.group_caption, help_group.group_name, help_text.cmd));
            grt->send_output(ul);
            grt->send_output(NL);

            if (help_text.desc)
            {
              char *desc= auto_line_break(help_text.desc, 80, ' ');

              grt->send_output(strfmt("%s" NL NL, desc));

              g_free(desc);
            }

            grt->send_output(strfmt("%s" NL, help_text.syntax));

            if (help_text.params)
              grt->send_output(strfmt(NL "Parameters:" NL "%s" NL, help_text.params));

            if (help_text.examples)
              grt->send_output(strfmt(NL "Examples:" NL "%s" NL, help_text.examples));

            if (help_text.see_also)
              grt->send_output(strfmt(NL "See also:" NL "%s" NL, help_text.see_also));

            found= 1;
            break;
          }
        }

        if (found)
          break;
      }
    }
  }

  if (!found)
    grt->send_output("Unknown command or function." NL);
}

void myx_grt_shell_show_command_help(grt::GRT *grt, const char *command)
{
  if ((strcmp(command, "quit") == 0) || (strcmp(command, "exit") == 0) ||
    (strcmp(command, "ls") == 0) || (strcmp(command, "cd") == 0) ||
    (strcmp(command, "show") == 0) || (strcmp(command, "run") == 0) ||
    (strcmp(command, "/path") == 0))
  {
    myx_grt_shell_show_command_help_print(grt, "shell", command);
  }
  else
  {
    char **cmd_split= g_strsplit(command, ".", 0);  

    myx_grt_shell_show_command_help_print(grt, cmd_split[0], cmd_split[1]);

    g_strfreev(cmd_split);
  }
}

/** 
 ****************************************************************************
 * @brief Shows help text pertaining to the Lua Shell.
 *
 * Displays help text for the given command or a general help text, if
 * NULL is passed.
 * 
 * @param grt  The GRT environment the shell belongs to.
 * @param command  The command to give help about or NULL, for general help.
 ****************************************************************************
 */
void myx_grt_shell_show_help(grt::GRT *grt, const char *command)
{
  if(!command || !*command)
  {
    grt->send_output(
      "Shell Commands (only available in the GRT Shell)"NL
      "--------------"NL
      "help    (\\h)     Display this help."NL
      "?       (\\?)     Synonym for 'help'."NL
      "quit    (\\q)     Exit the shell."NL
      "exit    (\\e)     Synonym for 'quit'."NL
      "ls               List all objects in the current path, modules or tables."NL
      "cd               Changes the current globals path"NL
      "show             Prints an object"NL
      "run     (\\r)     Load and execute a lua script file."NL
      //"/path            Returns the global object at the given path"NL
      NL
      "Global Functions and Objects"NL
      "----------------------------"NL
      "_G               Basic functions with a global scope"NL
      //"grt              Library with common used functions for GRT scripting"NL
      NL
      "Lua Standard Libraries"NL
      "----------------------"NL
      "coroutine        Functions for collaborative multithreading"NL
      "string           String manipulation functions"NL
      "table            Generic functions for table manipulation"NL
      "math             Mathematical functions"NL
      "io               Input and Output Facilities"NL
      "file             File class"NL
      "os               Operating System Facilities"NL
      NL
      "GRT Scripting Libraries"NL
      "-----------------------"NL
      "grtV             Library to work with GRT values"NL
      "grtS             GRT struct management library"NL
      "grtM             Library to return information about GRT modules"NL
//      "grtA             GRT agent management library"NL
      "grtU             GRT utility function library"NL
      NL
      "Type 'help <command/lib>' to get information about the command or library."NL
      );
  }
  else
  {
    myx_grt_shell_show_command_help(grt, command);
  }
}

