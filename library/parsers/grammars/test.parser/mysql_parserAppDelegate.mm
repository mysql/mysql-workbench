/*
 * Copyright (c) 2012, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#import "mysql_parserAppDelegate.h"

#include <sstream>
#include <vector>

#include <stdlib.h>
#include <string.h>

#include "antlr4-runtime.h"

using namespace parsers;
using namespace antlr4;
using namespace antlr4::atn;

static std::string lastErrors;

NSString *sql1 = @"select 2 as expected, /*!01000/**/*/ 2 as result";
NSString *sql2 = @"select (select\n t1.id as a, sakila.actor.actor_id b, t2.id c, "
                  "(select  1 * 0.123, a from t3) from  `ÄÖÜ丈` t1, sakila.actor as t2\n"
                  "where ((t1.id = t2.id)) and (t1.id = sakila.actor.actor_id)) as r1, 2";
NSString *sql3 =
  @"select 1 from\n\t{OJ d left outer join e on a = b} left outer join ee on aa = bb,\n\ta t1 force index for "
   "order by (a, b),\n\t(select 2 from zz) yy straight_join (select 3) as xx on ww,\n\t(b, c),\n\t(f, g) inner join h "
   "using (aa, bb),\n"
   "\t(h) straight_join `schema`.`table` on yy < zz natural right join ({OJ i left outer join j on ii = jj})";
NSString *sql4 =
  @"CREATE PROCEDURE do_insert(value INT)\nBEGIN\n  -- declare variables to hold diagnostics area information\n"
   "  DECLARE code CHAR(5) DEFAULT '00000';\n  DECLARE msg TEXT;\n  DECLARE rows INT;\n  DECLARE result TEXT;\n"
   "  -- declare exception handler for failed insert\n  DECLARE CONTINUE HANDLER FOR SQLEXCEPTION\n    BEGIN\n"
   "      GET DIAGNOSTICS CONDITION 1\n        code = RETURNED_SQLSTATE, msg = MESSAGE_TEXT;\n    END;\n\n"
   "  -- perform the insert\n  INSERT INTO t1 (int_col) VALUES(value);\n  -- check whether the insert was successful\n"
   "  IF code = '00000' THEN\n    GET DIAGNOSTICS rows = ROW_COUNT;\n   -- SET result = CONCAT('insert succeeded, row "
   "count = ',rows);\n"
   "  ELSE\n   -- SET result = CONCAT('insert failed, error = ',code,', message = ',msg);\n  END IF;\n"
   "  -- say what happened\n  SELECT result;\nEND";
NSString *sql5 = @"grant alter (a, b) on table * to 'mike'@'%'";
NSString *sql6 = @"select A, B, A OR B, A XOR B, A AND B from t1_30237_bool where C is null order by A, B";
NSString *sql7 =
  @"select count(distinct a.actor_id), phone, first_name, a.last_name, country.country \n"
   "from sakila.actor a, address aa, country\nwhere (a.actor_id = 0 and country_id > 0) \ngroup by actor_id";
NSString *sql8 = @"drop user current_user(), 'mike'@localhost";
NSString *sql9 = @"CREATE definer = `root`@`localhost` trigger `upd_film` AFTER UPDATE ON `film` FOR EACH ROW BEGIN\n"
                  "  IF (old.title != new.title) or (old.description != new.description)\n"
                  "  THEN\n"
                  "    UPDATE film_text\n"
                  "      SET title=new.title,\n"
                  "          description=new.description,\n"
                  "          film_id=new.film_id\n"
                  "    WHERE film_id=old.film_id;\n"
                  "  END IF;\n"
                  "END";
NSString *sql10 = @"CREATE TABLE total (\n"
                   "  a INT NOT NULL AUTO_INCREMENT,\n"
                   "  message CHAR(20), INDEX(a))\n"
                   "  ENGINE=MERGE UNION=(t1,t2) INSERT_METHOD=LAST;\n";
NSString *sql11 = @"SELECT a FROM tick t WHERE timestamp > (((((((SELECT 1)  + 1))))))";
NSString *sql12 = @"select * from (select 1 from dual)";
NSString *sql13 = @"ALTER USER u@localhost IDENTIFIED WITH sha256_password BY 'test';";

static std::set<std::string> charsets = { "_utf8", "_utf8mb3", "_utf8mb4", "_ucs2",   "_big5",   "_latin2",
                                          "_ujis", "_binary", "_cp1250", "_latin1" };

@interface mysql_parserAppDelegate () {
  IBOutlet NSTextView *singleQueryText;
  IBOutlet NSScrollView *singleQueryTextScrollView;
  IBOutlet NSTextView *errorText;
  IBOutlet NSTextView *output;
  IBOutlet NSScrollView *outputScrollView;
  IBOutlet NSTextField *pathEdit;
  IBOutlet NSTextView *parseTreeView;

  IBOutlet NSTextField *statusText;
  IBOutlet NSTextView *errorQueryText;
  IBOutlet NSButton *startStopButton;
  IBOutlet NSTextField *progressLabel;
  IBOutlet NSTextView *stepsList;

  IBOutlet NSView *topView;

  // For query to token conversion.
  IBOutlet NSTextView *queryList;
  IBOutlet NSTextView *tokenList;
  IBOutlet NSTextField *conversionErrorText;
  IBOutlet NSTextField *versionText;

  IBOutlet NSButton *modeIgnoreSpaceButton;
  IBOutlet NSButton *modeAnsiQuotesButton;
  IBOutlet NSButton *modePipesAsConcatButton;
  IBOutlet NSButton *modeHighNotPrecedenceButton;
  IBOutlet NSButton *modeNoBackslashEscapeButton;

  IBOutlet NSSplitView *singleQuerySplitView;

  NSUInteger queryCount;
  BOOL stopTests;
  BOOL running;

  NSTimeInterval lastDuration;
}
@end

@implementation mysql_parserAppDelegate

@synthesize window;

- (void)applicationDidFinishLaunching: (NSNotification *)aNotification {
  // Make the SQL edit control + output scroll horizontally too.
  singleQueryTextScrollView.hasHorizontalScroller = YES;
  singleQueryText.maxSize = NSMakeSize(FLT_MAX, FLT_MAX);
  singleQueryText.horizontallyResizable = YES;
  singleQueryText.textContainer.size = NSMakeSize(FLT_MAX, FLT_MAX);
  singleQueryText.textContainer.widthTracksTextView = NO;
  singleQueryText.string = sql13;

  outputScrollView.hasHorizontalScroller = YES;
  output.maxSize = NSMakeSize(FLT_MAX, FLT_MAX);
  output.horizontallyResizable = YES;
  output.textContainer.size = NSMakeSize(FLT_MAX, FLT_MAX);
  output.textContainer.widthTracksTextView = NO;

  singleQuerySplitView.autosaveName = @"parser.test.single.split";

  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  NSString *text = [defaults objectForKey: @"statements-file"];
  if (text != nil)
    pathEdit.stringValue = text;
  text = [defaults objectForKey: @"single-query"];
  if (text != nil)
    singleQueryText.string = text;
  text = [defaults objectForKey: @"version"];
  if (text != nil)
    versionText.stringValue = text;
}

class TestErrorListener : public BaseErrorListener {
public:
  virtual void syntaxError(Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
                           size_t charPositionInLine, const std::string &msg, std::exception_ptr e) override {
    // Here we use the message provided by the DefaultErrorStrategy class.
    if (!lastErrors.empty())
      lastErrors += "\n";
    lastErrors += "line " + std::to_string(line) + ":" + std::to_string(charPositionInLine) + " " + msg;
  }
};

using SqlMode = MySQLRecognizerCommon::SqlMode;

- (SqlMode)getSqlModes {
  size_t result = 0;

  if (modeIgnoreSpaceButton.state == NSOnState)
    result |= SqlMode::IgnoreSpace;
  if (modeAnsiQuotesButton.state == NSOnState)
    result |= MySQLRecognizerCommon::AnsiQuotes;
  if (modePipesAsConcatButton.state == NSOnState)
    result |= MySQLRecognizerCommon::PipesAsConcat;
  if (modeHighNotPrecedenceButton.state == NSOnState)
    result |= MySQLRecognizerCommon::HighNotPrecedence;
  if (modeNoBackslashEscapeButton.state == NSOnState)
    result |= MySQLRecognizerCommon::NoBackslashEscapes;

  return MySQLRecognizerCommon::SqlMode(result);
}

- (unsigned)getServerVersion {
  NSString *t = versionText.stringValue;
  if (t.length == 0)
    return 50630;
  return t.intValue;
}

- (IBAction)parseFromClipboard: (id)sender {
  [singleQueryText selectAll:sender];
  [singleQueryText pasteAsPlainText:sender];
  [self parse:sender];
}

template <typename E>
void rethrow_unwrapped(const E &e) {
  try {
    std::rethrow_if_nested(e);
  } catch (const std::nested_exception &e) {
    rethrow_unwrapped(e);
  } catch (...) {
    throw;
  }
}

static Ref<BailErrorStrategy> errorStrategy = std::make_shared<BailErrorStrategy>();

- (unsigned long)parseQuery: (NSString *)query
                    version: (long)serverVersion
                      modes: (MySQLRecognizerCommon::SqlMode)sqlModes
               dumpToOutput: (BOOL)dump
                   needTree: (BOOL)buildParseTree {
  NSDate *start = [NSDate new];

  TestErrorListener errorListener;

  ANTLRInputStream input(query.UTF8String);
  MySQLLexer lexer(&input);
  lexer.removeErrorListeners();
  lexer.addErrorListener(&errorListener);

  lexer.serverVersion = serverVersion;
  lexer.sqlMode = sqlModes;
  lexer.charsets = charsets;
  CommonTokenStream tokens(&lexer);

  MySQLParser parser(&tokens);
  parser.serverVersion = serverVersion;
  parser.sqlMode = sqlModes;
  parser.setBuildParseTree(buildParseTree);

  parser.setErrorHandler(errorStrategy); // Bail out at the first found error.
  parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::SLL);
  parser.removeErrorListeners();

  try {
    tokens.fill();
  } catch (IllegalStateException &) {
    return 1;
  }

  ParserRuleContext *tree;
  try {
    tree = parser.query();
  } catch (ParseCancellationException &) {
    // If parsing was cancelled we either really have a syntax error or we need to do a second step,
    // now with the default strategy and LL parsing.
    tokens.reset();
    parser.reset();
    parser.setErrorHandler(std::make_shared<DefaultErrorStrategy>());
    parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::LL);
    parser.addErrorListener(&errorListener);

    tree = parser.query();
  }

  auto toks = tokens.getTokens();
  std::string t = input.getText(misc::Interval((ssize_t)toks[0]->getStartIndex(), std::numeric_limits<ssize_t>::max()));

  lastDuration = -[start timeIntervalSinceNow];

  if (dump && buildParseTree) {
    if (tree == nullptr)
      output.string = @"No parse tree available";
    else {
      std::string t = tree->getText();
      parseTreeView.string = [NSString stringWithUTF8String: tree->toStringTree(&parser).c_str()];

      std::string text = MySQLRecognizerCommon::dumpTree(tree, parser.getVocabulary());
      text = "Token count: " + std::to_string(tokens.size()) + "\n" + text;
      NSString *utf8 = [NSString stringWithUTF8String: text.c_str()];
      if (utf8 == nil) {
        output.string = @"dump contains invalid utf8 chars";
        printf("%s\n", text.c_str());
      } else
        [output setString:utf8];
    }
  }

  return parser.getNumberOfSyntaxErrors() + lexer.getNumberOfSyntaxErrors();
}

- (IBAction)parse: (id)sender {
  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  [defaults setObject:singleQueryText.string forKey: @"single-query"];
  [defaults setObject:versionText.stringValue forKey: @"version"];

  lastErrors = "";
  errorText.string = @"";
  [errorText setNeedsDisplay: YES];
  output.string = @"";
  [output setNeedsDisplay: YES];
  parseTreeView.string = @"";
  [parseTreeView setNeedsDisplay: YES];

  size_t errorCount = [self parseQuery: singleQueryText.string
                               version: [self getServerVersion]
                                 modes: [self getSqlModes]
                          dumpToOutput: YES
                              needTree: YES];

  NSString *combinedErrorText = [NSString stringWithFormat: @"Parse time: %.6fs\n\n", lastDuration];

  if (errorCount > 0) {
    combinedErrorText =
      [combinedErrorText stringByAppendingFormat: @"%zu errors found\n%s", errorCount, lastErrors.c_str()];
  } else {
    combinedErrorText = [combinedErrorText stringByAppendingString: @"No errors found"];
  }
  [errorText setString: combinedErrorText];
}

- (IBAction)selectFile: (id)sender {
  NSOpenPanel *panel = [NSOpenPanel openPanel];
  panel.canChooseDirectories = NO;
  panel.allowsMultipleSelection = NO;
  panel.canChooseFiles = YES;
  panel.allowedFileTypes = [NSArray arrayWithObjects: @"txt", @"sql", nil];
  panel.directoryURL = [NSURL fileURLWithPath: [pathEdit stringValue]];
  if ([panel runModal] == NSFileHandlingPanelOKButton) {
    pathEdit.stringValue = panel.URL.path;
  }
}

- (void)appendStepText: (NSString *)newText {
  stepsList.string = [stepsList.string stringByAppendingString: newText];
}

- (IBAction)startTests: (id)sender {
  if (running) {
    stopTests = YES;
    return;
  }

  NSString *fileName = pathEdit.stringValue;
  NSError *error = nil;
  NSString *contents = [NSString stringWithContentsOfFile: fileName encoding: NSUTF8StringEncoding error: &error];
  if (error != nil) {
    NSAlert *alert = [NSAlert alertWithError:error];
    [alert runModal];
    return;
  }

  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  [defaults setObject: fileName forKey: @"statements-file"];
  [defaults setObject: singleQueryText.string forKey: @"single-query"];
  [defaults setObject: versionText.stringValue forKey: @"version"];


  stopTests = NO;
  errorQueryText.string = @"";
  lastErrors = "";

  statusText.stringValue = @"Counting queries...";
  stepsList.string = @"Counting queries\n";
  progressLabel.stringValue = @"0 of 0";

  [self countQueriesInFile: fileName];
  [self appendStepText: [NSString stringWithFormat: @"Found %lu queries in file\n", (unsigned long)queryCount]];

  [topView setNeedsDisplay: YES];

  NSThread *thread = [[NSThread alloc] initWithTarget:self selector: @selector(runTestsWithData:) object: contents];
  [thread setStackSize: 16 * 1024 * 1024];
  [thread start];
}

- (void)countQueriesInFile: (NSString *)fileName {
  NSFileHandle *file = [NSFileHandle fileHandleForReadingAtPath:fileName];
  NSUInteger chunkSize = 1024 * 1024;
  NSData *chunk = [file readDataOfLength:chunkSize];
  queryCount = 0;
  NSUInteger seenDollars = 0;
  while ([chunk length] > 0) {
    const char *chars = (const char *)[chunk bytes];
    for (int index = 0; index < [chunk length]; ++index) {
      switch (chars[index]) {
        case '$':
          seenDollars++;
          break;
        case '\n':
          if (seenDollars == 2) {
            seenDollars = 0;
            queryCount++;
          }
          break;
        default:
          seenDollars = 0;
          break;
      }
    }
    chunk = [file readDataOfLength:chunkSize];
  }
}

- (void)runTestsWithData: (NSString *)data {
  NSDate *start = [NSDate new];

  try {
    // 1. Part: parse example queries from file.
    dispatch_async(dispatch_get_main_queue(), ^{
      self->statusText.stringValue = @"Running tests from file...";
      [self appendStepText: @"Running tests from file\n"];
    });

    NSUInteger count = 0;
    BOOL gotError = NO;
    NSArray *queries = [data componentsSeparatedByString: @"$$\n"];
    __block unsigned serverVersion;
    dispatch_sync(dispatch_get_main_queue(), ^{
      serverVersion = [self getServerVersion];
    });

    __block MySQLRecognizerCommon::SqlMode sqlModes;
    dispatch_sync(dispatch_get_main_queue(), ^{
      sqlModes = [self getSqlModes];
    });

    static NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern: @"\\[(<|<=|>|>=|=)(\\d{5})\\]"
                                                                                  options: NSRegularExpressionCaseInsensitive
                                                                                    error: nil];
    static std::map<std::string, int> relationMap = {
      { "<", 0 }, { "<=", 1 }, { "=", 2 }, { ">=", 3 }, { ">", 4 }
    };

    for (NSString *entry in queries) {
      if (stopTests)
        break;

      NSString *query = entry;
      if ([query hasPrefix: @"["]) {
        // There's a version tag.
        auto match = [regex firstMatchInString: query options: NSMatchingAnchored range: NSMakeRange(0, query.length)];
        if (match != nil) {
          auto relation = [entry substringWithRange: [match rangeAtIndex: 1]];
          auto targetVersion = static_cast<unsigned>([entry substringWithRange: [match rangeAtIndex: 2]].intValue);
          switch (relationMap[relation.UTF8String]) {
            case 0:
              if (serverVersion >= targetVersion)
                continue;
              break;
            case 1:
              if (serverVersion > targetVersion)
                continue;
              break;
            case 2:
              if (serverVersion != targetVersion)
                continue;
              break;
            case 3:
              if (serverVersion < targetVersion)
                continue;
              break;
            case 4:
              if (serverVersion <= targetVersion)
                continue;
              break;
          }

          // Remove the tag.
          query = [regex stringByReplacingMatchesInString: query
                                                  options: 0
                                                    range: NSMakeRange(0, query.length)
                                             withTemplate: @""];
        }
      }

      if (query.length > 0) {
        // The progress label is updated in the worker thread, otherwise it would not show changes in realtime.
        ++count;

        dispatch_async(dispatch_get_main_queue(), ^{
          self->progressLabel.stringValue = [NSString stringWithFormat: @"%lu of %lu", count, self->queryCount];
          [self->progressLabel setNeedsDisplay];
        });

        if ([self parseQuery: query version: serverVersion modes: sqlModes dumpToOutput: NO needTree: NO] > 0) {
          dispatch_async(dispatch_get_main_queue(), ^{
            self->statusText.stringValue = @"Error encountered, offending query below:";
            self->errorQueryText.string = query;
          });
          gotError = YES;
          break;
        }
      }
    }

    // 2. Part: parse queries with function names for foreign keys.
    if (!gotError && !stopTests) {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self appendStepText: [NSString stringWithFormat: @"Parsed %lu queries in file successfully.\n",
                                                        (unsigned long)self->queryCount]];

        [self appendStepText: @"Running function name tests\n"];
        self->statusText.stringValue = @"Running function name tests...";
      });
      gotError = ![self runFunctionNamesTest];
    }

    NSTimeInterval duration = [start timeIntervalSinceNow];
    NSString *durationText = [NSString stringWithFormat: @"Parse time: %.3fs\n\n", -duration];
    dispatch_async(dispatch_get_main_queue(), ^{
      if (self->stopTests)
        self->statusText.stringValue = @"Execution stopped by user.";
      else if (!gotError)
        self->statusText.stringValue = @"All queries parsed fine. Great!";
      [self appendStepText:durationText];
    });
    running = NO;
  } catch (std::exception &e) {
    running = NO;
    statusText.stringValue = [NSString stringWithFormat: @"Parser raised exception: %s", e.what()];
  }
}

/**
 * Generates queries with many MySQL functions (all?) used in foreign key creation (parser bug #21114).
 * Note: this test is currently only partially useful as most of the used functions are not reserved words
 *       (and hence not contained in the grammar). This might change if the parsing process does an
 *       explicit differentiation between general identifiers and function names.
 * Returns YES if all went ok.
 */
- (BOOL)runFunctionNamesTest {
  static const char *functions[] = {
    "acos", "adddate",
    "addtime"
    "aes_decrypt",
    "aes_encrypt", "area", "asbinary", "asin", "astext", "aswkb", "aswkt", "atan", "atan2", "benchmark", "bin",
    "bit_count", "bit_length", "ceil", "ceiling", "centroid", "character_length", "char_length", "coercibility",
    "compress", "concat", "concat_ws", "connection_id", "conv", "convert_tz", "cos", "cot", "crc32", "crosses",
    "datediff", "date_format", "dayname", "dayofmonth", "dayofweek", "dayofyear", "decode", "degrees", "des_decrypt",
    "des_encrypt", "dimension", "disjoint", "elt", "encode", "encrypt", "endpoint", "envelope", "equals", "exp",
    "export_set", "exteriorring", "extractvalue", "find_in_set", "floor", "found_rows", "from_days", "from_unixtime",
    "geomcollfromtext", "geomcollfromwkb", "geometrycollectionfromtext", "geometrycollectionfromwkb",
    "geometryfromtext", "geometryfromwkb", "geometryn", "geometrytype", "geomfromtext", "geomfromwkb", "get_lock",
    "glength", "greatest", "hex", "ifnull", "inet_aton", "inet_ntoa", "instr", "interiorringn", "intersects",
    "isclosed", "isempty", "isnull", "issimple", "is_free_lock", "is_used_lock", "last_day", "last_insert_id", "lcase",
    "least", "length", "linefromtext", "linefromwkb", "linestringfromtext", "linestringfromwkb", "ln", "load_file",
    "locate", "log", "log10", "log2", "lower", "lpad", "ltrim", "makedate", "maketime", "make_set", "master_pos_wait",
    "mbrcontains", "mbrdisjoint", "mbrequal", "mbrintersects", "mbroverlaps", "mbrtouches", "mbrwithin", "md5",
    "mlinefromtext", "mlinefromwkb", "monthname", "mpointfromtext", "mpointfromwkb", "mpolyfromtext", "mpolyfromwkb",
    "multilinestringfromtext", "multilinestringfromwkb", "multipointfromtext", "multipointfromwkb",
    "multipolygonfromtext", "multipolygonfromwkb", "name_const", "nullif", "numgeometries", "numinteriorrings",
    "numpoints", "oct", "octet_length", "ord", "overlaps", "period_add", "period_diff", "pi", "pointfromtext",
    "pointfromwkb", "pointn", "polyfromtext", "polyfromwkb", "polygonfromtext", "polygonfromwkb", "pow", "power",
    "quote", "radians", "rand", "release_lock", "reverse", "round", "row_count", "rpad", "rtrim", "sec_to_time",
    "session_user", "sha", "sha1", "sign", "sin", "sleep", "soundex", "space", "sqrt", "srid", "startpoint", "strcmp",
    "str_to_date", "subdate", "substring_index", "subtime", "system_user", "tan", "timediff", "time_format",
    "time_to_sec", "touches", "to_days", "ucase", "uncompress", "uncompressed_length", "unhex", "unix_timestamp",
    "updatexml", "upper", "uuid", "version", "weekday", "weekofyear", "within", "x", "y", "yearweek"};

  const char *query1 =
    "CREATE TABLE %s(\n"
    "col1 int not null,\n"
    "col2 int not null,\n"
    "col3 varchar(10),\n"
    "CONSTRAINT pk PRIMARY KEY (col1, col2)\n"
    ") ENGINE InnoDB";

  const char *query2 =
    "CREATE TABLE bug21114_child(\n"
    "pk int not null,\n"
    "fk_col1 int not null,\n"
    "fk_col2 int not null,\n"
    "fk_col3 int not null,\n"
    "fk_col4 int not null,\n"
    "CONSTRAINT fk_fct FOREIGN KEY (fk_col1, fk_col2)\n"
    "REFERENCES %s(col1, col2),\n"
    "CONSTRAINT fk_fct_space FOREIGN KEY (fk_col3, fk_col4)\n"
    "REFERENCES %s (col1, col2)\n"
    ") ENGINE InnoDB";

  NSInteger count = sizeof(functions) / sizeof(functions[0]);
  __block unsigned serverVersion;
  dispatch_sync(dispatch_get_main_queue(), ^{
    serverVersion = [self getServerVersion];
  });

  __block MySQLRecognizerCommon::SqlMode sqlModes;
  dispatch_sync(dispatch_get_main_queue(), ^{
    sqlModes = [self getSqlModes];
  });

  for (size_t i = 0; i < count; i++) {
    if (stopTests)
      return NO;

    NSString *query = [NSString stringWithFormat: [NSString stringWithUTF8String:query1], functions[i]];
    if (query.length > 0) {
      dispatch_async(dispatch_get_main_queue(), ^{
        self->progressLabel.stringValue = [NSString stringWithFormat: @"%lu of %li", i + 1, count];
        [self->progressLabel setNeedsDisplay];
      });
      if ([self parseQuery:query version:serverVersion modes:sqlModes dumpToOutput: NO needTree: NO] > 0) {
        statusText.stringValue = @"Error encountered, offending query below:";
        errorQueryText.string = query;
        return NO;
      }
    }

    query = [NSString stringWithFormat: [NSString stringWithUTF8String:query2], functions[i], functions[i]];
    if (query.length > 0) {
      if ([self parseQuery:query version:serverVersion modes:sqlModes dumpToOutput: NO needTree: NO] > 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
          self->statusText.stringValue = @"Error encountered, offending query below:";
          self->errorQueryText.string = query;
        });
        return NO;
      }
    }
  }
  return YES;
}

#pragma mark - Action handling

#include "MySQLParserBaseListener.h"

NSString *dumpTokens(ParserRuleContext *context, MySQLParser &parser) {
  NSString *result = @"";

  const dfa::Vocabulary &vocabulary = parser.getVocabulary();
  for (size_t index = 0; index < context->children.size(); ++index) {
    tree::ParseTree *child = context->children[index];
    if (antlrcpp::is<RuleContext *>(child)) {
      NSString *childText = dumpTokens(dynamic_cast<ParserRuleContext *>(child), parser);
      result = [result stringByAppendingString:childText];
    } else {
      // A terminal node.
      tree::TerminalNode *node = dynamic_cast<tree::TerminalNode *>(child);
      Token *token = node->getSymbol();

      NSString *childText;
      if (token->getType() == Token::EOF)
        childText = @"Token::EOF";
      else
        childText = [NSString stringWithFormat: @"P::%s, ", vocabulary.getSymbolicName(token->getType()).c_str()];
      result = [result stringByAppendingString:childText];
    }
  }

  return [result stringByAppendingString: @""];
}

- (IBAction)startConversion: (id)sender {
  NSString *tokenListString = @"";
  size_t errorCount = 0;

  TestErrorListener errorListener;

  NSArray *queries =
    [queryList.string componentsSeparatedByCharactersInSet: [NSCharacterSet characterSetWithCharactersInString: @"$$"]];
  for (NSString *entry in queries) {
    NSString *query = [entry stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (query.length == 0)
      continue;

    ANTLRInputStream input(query.UTF8String);
    MySQLLexer lexer(&input);
    lexer.removeErrorListeners();

    lexer.serverVersion = [self getServerVersion];
    lexer.sqlMode = [self getSqlModes];
    lexer.charsets = charsets;
    CommonTokenStream tokens(&lexer);

    MySQLParser parser(&tokens);
    parser.serverVersion = [self getServerVersion];
    parser.sqlMode = [self getSqlModes];
    parser.setBuildParseTree(true);

    parser.getInterpreter<ParserATNSimulator>()->setPredictionMode(PredictionMode::LL);
    parser.removeErrorListeners();
    parser.addErrorListener(&errorListener);

    ParserRuleContext *tree = parser.query();

    errorCount += parser.getNumberOfSyntaxErrors();

    NSString *dump = dumpTokens(tree, parser);
    tokenListString = [tokenListString stringByAppendingString: [NSString stringWithFormat: @"{ %@ },\n", dump]];
  }

  tokenList.string = tokenListString;
  if (errorCount > 0) {
    conversionErrorText.stringValue = @"Errors occurred.";
  } else {
    conversionErrorText.stringValue = @"Everything went fine.";
  };
}

@end
