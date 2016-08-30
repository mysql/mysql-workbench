//
//  mysql_parserAppDelegate.m
//  mysql.parser
//
//  Created by Mike on 03.04.12.
//  Copyright 2012, 2016, Oracle Corporation. All rights reserved.
//

#import "mysql_parserAppDelegate.h"

#include <sstream>
#include <vector>

#include <stdlib.h>
#include <string.h>

static std::string last_error;

extern "C" {
  
  void onMySQLParseError(struct ANTLR3_BASE_RECOGNIZER_struct * recognizer, pANTLR3_UINT8 * tokenNames)
  {
    std::ostringstream error;
    
    pANTLR3_PARSER parser;
    pANTLR3_TREE_PARSER tparser;
    pANTLR3_LEXER lexer;

    pANTLR3_INT_STREAM is;
    pANTLR3_STRING ttext;
    pANTLR3_STRING ftext;
    pANTLR3_EXCEPTION ex;
    pANTLR3_COMMON_TOKEN theToken;
    pANTLR3_BASE_TREE theBaseTree;
    pANTLR3_COMMON_TREE theCommonTree;

    // Retrieve some info for easy reading.
    //
    ex = recognizer->state->exception;
    ttext = NULL;

    // See if there is a 'filename' we can use
    //
    if	(ex->streamName == NULL)
    {
      if	(((pANTLR3_COMMON_TOKEN)(ex->token))->type == ANTLR3_TOKEN_EOF)
        error << "-end of input-(";
      else
        error << "-unknown source-(";
    }
    else
    {
      ftext = ex->streamName->to8(ex->streamName);
      error << ftext->chars << "(";
    }

    // Next comes the line number.
    error << recognizer->state->exception->line << ") ";

    // How we determine the next piece is dependent on which thing raised the error.
    switch (recognizer->type)
    {
      case ANTLR3_TYPE_LEXER:
      {
        lexer = (pANTLR3_LEXER)(recognizer->super);
        parser = NULL;
        tparser = NULL;

        error << " : lexer error " << recognizer->state->exception->type << " : " <<	(pANTLR3_UINT8)(ex->message);
        error << ", at offset " << recognizer->state->exception->charPositionInLine + 1;

        {
          ANTLR3_INT32	width;

          width	= ANTLR3_UINT32_CAST(( (pANTLR3_UINT8)(lexer->input->data) + (lexer->input->size(lexer->input) )) - (pANTLR3_UINT8)(ex->index));

          if	(width >= 1)
          {
            if	(isprint(ex->c))
              error << " near '" << (char)ex->c << "' :\n";
            else
              error << " near char(" << (ANTLR3_UINT8)(ex->c) << ") :\n";
          }
          else
          {
            error << "(end of input).\n";
            width = ANTLR3_UINT32_CAST(((pANTLR3_UINT8)(lexer->input->data)+(lexer->input->size(lexer->input))) - (pANTLR3_UINT8)(lexer->rec->state->tokenStartCharIndex));
          }
        }
        
        break;
      }

      case	ANTLR3_TYPE_PARSER:
        lexer = NULL;
        parser = (pANTLR3_PARSER) (recognizer->super);
        tparser = NULL;

        is = parser->tstream->istream;
        theToken = (pANTLR3_COMMON_TOKEN)(recognizer->state->exception->token);
        ttext = theToken->toString(theToken);

        error << " : parser error " << recognizer->state->exception->type << " : " <<	(pANTLR3_UINT8)(ex->message);
        error << ", at offset " << recognizer->state->exception->charPositionInLine;
        if  (theToken != NULL)
        {
          if (theToken->type == ANTLR3_TOKEN_EOF)
            error << ", at <EOF>";
          else
            error << "\n    near " << (ttext == NULL ? (pANTLR3_UINT8)"<no text for the token>" : ttext->chars) << "\n    ";
        }
        break;
        
    case	ANTLR3_TYPE_TREE_PARSER:
        lexer = NULL;
        parser = NULL;
      tparser		= (pANTLR3_TREE_PARSER) (recognizer->super);
      is			= tparser->ctnstream->tnstream->istream;
      theBaseTree	= (pANTLR3_BASE_TREE)(recognizer->state->exception->token);
      ttext		= theBaseTree->toStringTree(theBaseTree);

      if  (theBaseTree != NULL)
      {
        theCommonTree	= (pANTLR3_COMMON_TREE)	    theBaseTree->super;

        if	(theCommonTree != NULL)
          theToken	= (pANTLR3_COMMON_TOKEN)theBaseTree->getToken(theBaseTree);

        error << " : tree parser error " << recognizer->state->exception->type << " : " <<	(pANTLR3_UINT8)(recognizer->state->exception->message);
        error << ", at offset " << theBaseTree->getCharPositionInLine(theBaseTree) << ", near " << ttext->chars;
      }
      break;

    default:
      error << "Internal error: unknown recognizer type appeared in error reporting.\n";
      return;
      break;
    }

    // Although this function should generally be provided by the implementation, this one
    // should be as helpful as possible for grammar developers and serve as an example
    // of what you can do with each exception type. In general, when you make up your
    // 'real' handler, you should debug the routine with all possible errors you expect
    // which will then let you be as specific as possible about all circumstances.
    //
    // Note that in the general case, errors thrown by tree parsers indicate a problem
    // with the output of the parser or with the tree grammar itself. The job of the parser
    // is to produce a perfect (in traversal terms) syntactically correct tree, so errors
    // at that stage should really be semantic errors that your own code determines and handles
    // in whatever way is appropriate.
    switch  (ex->type)
    {
    case	ANTLR3_UNWANTED_TOKEN_EXCEPTION:

      // Indicates that the recognizer was fed a token which seesm to be
      // spurious input. We can detect this when the token that follows
      // this unwanted token would normally be part of the syntactically
      // correct stream. Then we can see that the token we are looking at
      // is just something that should not be there and throw this exception.
      //
      if	(tokenNames == NULL)
        error << " : Extraneous input...";
      else
      {
        if	(ex->expecting == ANTLR3_TOKEN_EOF)
          error << " : Extraneous input - expected <EOF>\n";
        else
          error << " : Extraneous input - expected " << tokenNames[ex->expecting] <<  "...\n";
      }
      break;

    case	ANTLR3_MISSING_TOKEN_EXCEPTION:

      // Indicates that the recognizer detected that the token we just
      // hit would be valid syntactically if preceeded by a particular 
      // token. Perhaps a missing ';' at line end or a missing ',' in an
      // expression list, and such like.
      //
      if	(tokenNames == NULL)
      {
        error << " : Missing token (" << ex->expecting << ")...\n";
      }
      else
      {
        if	(ex->expecting == ANTLR3_TOKEN_EOF)
          error << " : Missing <EOF>\n";
        else
          error << " : Missing " << tokenNames[ex->expecting] << " \n";
      }
      break;

    case	ANTLR3_RECOGNITION_EXCEPTION:

      // Indicates that the recognizer received a token
      // in the input that was not predicted. This is the basic exception type 
      // from which all others are derived. So we assume it was a syntax error.
      // You may get this if there are not more tokens and more are needed
      // to complete a parse for instance.
      error << " : syntax error...\n";    
      break;

    case    ANTLR3_MISMATCHED_TOKEN_EXCEPTION:

      // We were expecting to see one thing and got another. This is the
      // most common error if we coudl not detect a missing or unwanted token.
      // Here you can spend your efforts to
      // derive more useful error messages based on the expected
      // token set and the last token and so on. The error following
      // bitmaps do a good job of reducing the set that we were looking
      // for down to something small. Knowing what you are parsing may be
      // able to allow you to be even more specific about an error.
      if	(tokenNames == NULL)
        error << " : syntax error...\n";
      else
      {
        if	(ex->expecting == ANTLR3_TOKEN_EOF)
          error << " : expected <EOF>\n";
        else
          error << " : expected " << tokenNames[ex->expecting] << " ...\n";
      }
      break;

    case	ANTLR3_NO_VIABLE_ALT_EXCEPTION:
      error << " unexpected input\n";

      break;

    case	ANTLR3_MISMATCHED_SET_EXCEPTION:

      {
        ANTLR3_UINT32	  count;
        ANTLR3_UINT32	  bit;
        ANTLR3_UINT32	  size;
        ANTLR3_UINT32	  numbits;
        pANTLR3_BITSET	errBits;

        // This means we were able to deal with one of a set of
        // possible tokens at this point, but we did not see any
        // member of that set.
        error << " : unexpected input...\n  expected one of : ";

        // What tokens could we have accepted at this point in the parse?
        count   = 0;
        errBits = antlr3BitsetLoad(ex->expectingSet);
        numbits = errBits->numBits(errBits);
        size    = errBits->size(errBits);

        if  (size > 0)
        {
          // However many tokens we could have dealt with here, it is usually
          // not useful to print ALL of the set here. I arbitrarily chose 8
          // here, but you should do whatever makes sense for you of course.
          // No token number 0, so look for bit 1 and on.
          for	(bit = 1; bit < numbits && count < 8 && count < size; bit++)
          {
            // TODO: This doesn;t look right - should be asking if the bit is set!!
            if  (tokenNames[bit])
            {
              error << (count > 0 ? ", " : "") << tokenNames[bit]; 
              count++;
            }
          }
          error << "\n";
        }
        else
        {
          error << "Actually dude, we didn't seem to be expecting anything here, or at least\n";
          error << "I could not work out what I was expecting, like so many of us these days!\n";
        }
      }
      break;

    case	ANTLR3_EARLY_EXIT_EXCEPTION:

      // We entered a loop requiring a number of token sequences
      // but found a token that ended that sequence earlier than
      // we should have done.
      error << " : missing elements...\n";
      break;

    default:

      // We don't handle any other exceptions here, but you can
      // if you wish. If we get an exception that hits this point
      // then we are just going to report what we know about the
      // token.
      error << " : syntax not recognized...\n";
      break;
    }

    last_error += error.str();
  }
  
  // Here we just fake some charsets, as we don't have the entire parser context.
  std::string charsets[] = {"utf8", "ucs2", "big5", "latin2", "ujis", "binary", "cp1250", "latin1"};
  
  /**
   * Checks the given identifier whether it is an actually defined charset, which directs
   * the lexer to either classify it apropriately.
   */
  ANTLR3_UINT32 check_charset(void *payload, pANTLR3_STRING text)
  {
    // Get the actual token text and skip the initial underscore char.
    // There's an additional char at the end of the input for some reason (maybe a lexer bug),
    // so we also ignore the last char.
    std::string token_text((const char*)text->chars + 1, text->len - 2);
    std::transform(token_text.begin(), token_text.end(), token_text.begin(), ::tolower);

    for (int i = 0; i < sizeof(charsets) / sizeof(charsets[0]); i++)
      if (charsets[i] == token_text)
        return UNDERSCORE_CHARSET;
    return IDENTIFIER;
  }

}

NSString *sql1 = @"select 2 as expected, /*!01000/**/*/ 2 as result";
NSString *sql2 = @"select (select\n t1.id as a, sakila.actor.actor_id b, t2.id c, "
  "(select  1 * 0.123, a from t3) from  `ÄÖÜ丈` t1, sakila.actor as t2\n"
  "where ((t1.id = t2.id)) and (t1.id = sakila.actor.actor_id)) as r1, 2";
NSString *sql3 = @"select 1 from\n\t{OJ d left outer join e on a = b} left outer join ee on aa = bb,\n\ta t1 force index for "
  "order by (a, b),\n\t(select 2 from zz) yy straight_join (select 3) as xx on ww,\n\t(b, c),\n\t(f, g) inner join h using (aa, bb),\n"
  "\t(h) straight_join `schema`.`table` on yy < zz natural right join ({OJ i left outer join j on ii = jj})";
NSString *sql4 = @"CREATE PROCEDURE do_insert(value INT)\nBEGIN\n  -- declare variables to hold diagnostics area information\n"
  "  DECLARE code CHAR(5) DEFAULT '00000';\n  DECLARE msg TEXT;\n  DECLARE rows INT;\n  DECLARE result TEXT;\n"
  "  -- declare exception handler for failed insert\n  DECLARE CONTINUE HANDLER FOR SQLEXCEPTION\n    BEGIN\n"
  "      GET DIAGNOSTICS CONDITION 1\n        code = RETURNED_SQLSTATE, msg = MESSAGE_TEXT;\n    END;\n\n"
  "  -- perform the insert\n  INSERT INTO t1 (int_col) VALUES(value);\n  -- check whether the insert was successful\n"
  "  IF code = '00000' THEN\n    GET DIAGNOSTICS rows = ROW_COUNT;\n   -- SET result = CONCAT('insert succeeded, row count = ',rows);\n"
  "  ELSE\n   -- SET result = CONCAT('insert failed, error = ',code,', message = ',msg);\n  END IF;\n"
  "  -- say what happened\n  SELECT result;\nEND";
NSString *sql5 = @"grant alter (a, b) on table * to 'mike'@'%'";
NSString *sql6 = @"select A, B, A OR B, A XOR B, A AND B from t1_30237_bool where C is null order by A, B";
NSString *sql7 = @"select count(distinct a.actor_id), phone, first_name, a.last_name, country.country \n"
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

@implementation mysql_parserAppDelegate

@synthesize window;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {

  // Make the SQL edit control scroll horizontally too.
  [singleQueryText.textContainer setContainerSize: NSMakeSize(FLT_MAX, FLT_MAX)];
  [singleQueryText.textContainer setWidthTracksTextView: NO];
  singleQueryText.string = sql13;

  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  NSString *text = [defaults objectForKey: @"statements-file"];
  if (text != nil)
    pathEdit.stringValue = text;
  text = [defaults objectForKey: @"single-query"];
  if (text != nil)
    singleQueryText.string = text;
}

- (NSString*)dumpTree: (pANTLR3_BASE_TREE)tree state: (pANTLR3_RECOGNIZER_SHARED_STATE)state indentation: (NSString*)indentation
{
  NSString *result;
  
  ANTLR3_UINT32 char_pos = tree->getCharPositionInLine(tree);
  ANTLR3_UINT32 line = tree->getLine(tree);
  pANTLR3_STRING token_text = tree->getText(tree);

  pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
  NSString *utf8 = [NSString stringWithUTF8String: (const char*)token_text->chars];
  if (token != NULL) {
    ANTLR3_UINT32 token_type = token->getType(token);

    pANTLR3_UINT8 token_name;
    if (token_type != ANTLR3_TOKEN_EOF)
      token_name = state->tokenNames[token_type];
    if (token_type == ANTLR3_TOKEN_EOF || token_name == (pANTLR3_UINT8)ANTLR3_TOKEN_EOF)
      token_name = (pANTLR3_UINT8)"ANTLR3_TOKEN_EOF";

    result = [NSString stringWithFormat: @"%@(line: %i, offset: %i, length: %lli, index: %lli, %s [%i])    %@\n",
              indentation, line, char_pos, token->stop - token->start + 1,  token->index, token_name, token_type, utf8];
  } else {
    result = [NSString stringWithFormat: @"%@(line: %i, offset: %i, nil)    %@\n", indentation, line, char_pos, utf8];
  }
  
  for (ANTLR3_UINT32 index = 0; index < tree->getChildCount(tree); index++)
  {
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)tree->getChild(tree, index);
    NSString *child_text = [self dumpTree: child state: state indentation: [indentation stringByAppendingString: @"\t"]];
    result = [result stringByAppendingString: child_text];
  }
  return result;
}

- (NSString*)dumpTokens: (pANTLR3_BASE_TREE)tree state: (pANTLR3_RECOGNIZER_SHARED_STATE)state
{
  NSString *result;

  ANTLR3_UINT32 char_pos = tree->getCharPositionInLine(tree);
  ANTLR3_UINT32 line = tree->getLine(tree);
  pANTLR3_STRING token_text = tree->getText(tree);

  pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
  if (token != NULL) {
    ANTLR3_UINT32 token_type = token->getType(token);
    pANTLR3_UINT8 token_name;
    if (token_type != ANTLR3_TOKEN_EOF)
      token_name = state->tokenNames[token_type];
    if (token_type == ANTLR3_TOKEN_EOF || token_name == (pANTLR3_UINT8)ANTLR3_TOKEN_EOF)
      token_name = (pANTLR3_UINT8)"ANTLR3_TOKEN_EOF";

    result = [NSString stringWithFormat: @"(%s) ", token_name];
  } else {
    result = @"";
  }

  for (ANTLR3_UINT32 index = 0; index < tree->getChildCount(tree); index++)
  {
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)tree->getChild(tree, index);
    NSString *child_text = [self dumpTokens: child state: state];
    result = [result stringByAppendingString: child_text];
  }
  return result;
}

- (unsigned)getSqlModes
{
  unsigned result = 0;

  if (modeIgnoreSpaceButton.state == NSOnState)
    result |= SQL_MODE_IGNORE_SPACE;
  if (modeAnsiQuotesButton.state == NSOnState)
    result |= SQL_MODE_ANSI_QUOTES;
  if (modePipesAsConcatButton.state == NSOnState)
    result |= SQL_MODE_PIPES_AS_CONCAT;
  if (modeHighNotPrecedenceButton.state == NSOnState)
    result |= SQL_MODE_HIGH_NOT_PRECEDENCE;
  if (modeNoBackslashEscapeButton.state == NSOnState)
    result |= SQL_MODE_NO_BACKSLASH_ESCAPES;

  return result;
}

- (unsigned)getServerVersion
{
  NSString *t = versionText.stringValue;
  if (t.length == 0)
    return 50630;
  return t.intValue;
}

- (IBAction)parseFromClipboard: (id)sender
{
  [singleQueryText selectAll: sender];
  [singleQueryText pasteAsPlainText: sender];
  [self parse: sender];
}

- (IBAction)parse: (id)sender
{
  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  [defaults setObject: singleQueryText.string forKey: @"single-query"];

  last_error = "";
  [errorText setString: @""];

  NSDate *start = [NSDate new];

  pANTLR3_INPUT_STREAM input;
  pMySQLLexer lexer;
  pANTLR3_COMMON_TOKEN_STREAM tokens;
  pMySQLParser parser;

  RecognitionContext context = { 0, 0, [self getServerVersion], [self getSqlModes], nil };

  NSString *sql = singleQueryText.string;
  std::string utf8 = [sql UTF8String];
  input = antlr3StringStreamNew((pANTLR3_UINT8)utf8.c_str(), ANTLR3_ENC_UTF8, utf8.size(), (pANTLR3_UINT8)"sql-script");
  input->setUcaseLA(input, ANTLR3_TRUE); // Make input case-insensitive. String literals must all be upper case in the grammar!
  
  lexer = MySQLLexerNew(input);
  lexer->pLexer->rec->state->userp = &context;
  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
  parser = MySQLParserNew(tokens);
  parser->pParser->rec->state->userp = &context;

  pANTLR3_BASE_TREE tree = parser->query(parser).tree;

  NSTimeInterval lastDuration = -[start timeIntervalSinceNow];

  NSString *combinedErrorText = [NSString stringWithFormat: @"Parse time: %.6fs\n\n", lastDuration];
  ANTLR3_UINT32 error_count = parser->pParser->rec->state->errorCount;
  error_count += lexer->pLexer->rec->state->errorCount;
  if (error_count > 0) {
    combinedErrorText = [combinedErrorText stringByAppendingFormat: @"%i errors found\n%s", error_count, last_error.c_str()];
  } else {
    combinedErrorText = [combinedErrorText stringByAppendingString: @"No errors found"];
  }
  [errorText setString: combinedErrorText];

  if (tree != NULL) {
    NSString *token_text = [self dumpTree: tree state: parser->pParser->rec->state indentation: @""];
    [output setString: token_text];
  } else {
    [output setString: @"no tree"];
  }

  // Must manually clean up.
  parser->free(parser);
  tokens ->free(tokens);
  lexer->free(lexer);
  input->close(input); 
}

- (BOOL)parseQuery: (NSString *)query version: (unsigned)serverVersion modes: (unsigned)sqlModes
{
  pANTLR3_INPUT_STREAM input;
  pMySQLLexer lexer;
  pANTLR3_COMMON_TOKEN_STREAM tokens;
  pMySQLParser parser;
  
  RecognitionContext context = { 0, 0, serverVersion, sqlModes, nil };
  
  std::string utf8 = [query UTF8String];
  input = antlr3StringStreamNew((pANTLR3_UINT8)utf8.c_str(), ANTLR3_ENC_UTF8, utf8.size(), (pANTLR3_UINT8)"sql-script");
  input->setUcaseLA(input, ANTLR3_TRUE);
  
  lexer = MySQLLexerNew(input);
  lexer->pLexer->rec->state->userp = &context;
  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
  parser = MySQLParserNew(tokens);
  parser->pParser->rec->state->userp = &context;
  
  MySQLParser_query_return ast = parser->query(parser);
  
  BOOL result = last_error.size() == 0;
  
  // Must manually clean up.
  parser->free(parser);
  tokens ->free(tokens);
  lexer->free(lexer);
  input->close(input);

  return result;
}

bool parse_and_compare(const std::string query, pANTLR3_BASE_TREE tree, unsigned sql_modes, unsigned server_version)
{
  pANTLR3_INPUT_STREAM input;
  pMySQLLexer lexer;
  pANTLR3_COMMON_TOKEN_STREAM tokens;
  pMySQLParser parser;
  
  RecognitionContext context = { 0, 0, server_version, sql_modes, nil };
  
  input = antlr3StringStreamNew((pANTLR3_UINT8)query.c_str(), ANTLR3_ENC_UTF8, query.size(), (pANTLR3_UINT8)"sql-script");
  input->setUcaseLA(input, ANTLR3_TRUE);
  
  lexer = MySQLLexerNew(input);
  lexer->pLexer->rec->state->userp = &context;
  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
  parser = MySQLParserNew(tokens);
  parser->pParser->rec->state->userp = &context;
  
  MySQLParser_query_return ast = parser->query(parser);
  
  BOOL result = parser->pParser->rec->state->errorCount == 0;
  if (result)
  {
    // If there was no error then check the order of tokens.
  }
  
  parser->free(parser);
  tokens ->free(tokens);
  lexer->free(lexer);
  input->close(input);
  
  return result;
}

- (IBAction)selectFile: (id)sender
{
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

- (void)appendStepText: (NSString *)newText
{
  stepsList.string = [stepsList.string stringByAppendingString: newText];
}

- (IBAction)startTests: (id)sender
{
  if (running) {
    stopTests = YES;
    return;
  }
  
  NSString *fileName = pathEdit.stringValue;
  NSError *error = nil;
  NSString *contents = [NSString stringWithContentsOfFile: fileName encoding: NSUTF8StringEncoding error: &error];
  if (error != nil) {
    NSAlert *alert = [NSAlert alertWithError: error];
    [alert runModal];
    return;
  }

  NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
  [defaults setObject: fileName forKey: @"statements-file"];

  stopTests = NO;
  errorQueryText.string = @"";
  last_error = "";

  statusText.stringValue = @"Counting queries...";
  stepsList.string = @"Counting queries\n";
  progressLabel.stringValue = @"0 of 0";
  
  [self countQueriesInFile: fileName];
  [self appendStepText: [NSString stringWithFormat: @"Found %lu queries in file\n", (unsigned long)queryCount]];
  
  [topView setNeedsDisplay: YES];

  NSThread *thread = [[[NSThread alloc] initWithTarget: self
                                              selector: @selector(runTestsWithData:)
                                                object: contents] autorelease];
  [thread setStackSize: 8 * 1024 * 1024];
  [thread start];
}

- (void)countQueriesInFile: (NSString*)fileName
{

  NSFileHandle * file = [NSFileHandle fileHandleForReadingAtPath: fileName];
  NSUInteger chunkSize = 1024 * 1024;
  NSData * chunk = [file readDataOfLength: chunkSize];
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
    chunk = [file readDataOfLength: chunkSize];
  }
  
}

- (void)runTestsWithData: (NSString*)data
{
  NSDate *start = [NSDate new];

  try {
    // 1. Part: parse example queries from file.
    dispatch_async(dispatch_get_main_queue(), ^{
      statusText.stringValue = @"Running tests from file...";
      [self appendStepText: @"Running tests from file\n"];
    });

    NSUInteger count = 0;
    BOOL gotError = NO;
    NSArray *queries = [data componentsSeparatedByString: @"$$\n"];
    unsigned serverVersion = [self getServerVersion];
    unsigned sqlModes = [self getSqlModes];
    for (NSString* query in queries) {
      if (stopTests)
        break;
      
      if (query.length > 0) {
        // The progress label is updated in the worker thread, otherwise it would not show changes in realtime.
        progressLabel.stringValue = [NSString stringWithFormat: @"%lu of %lu", ++count, queryCount];
        [progressLabel setNeedsDisplay];
        if (![self parseQuery: query version: serverVersion modes: sqlModes])
        {
          dispatch_async(dispatch_get_main_queue(), ^{
            statusText.stringValue = @"Error encountered, offending query below:";
            errorQueryText.string = query;
          });
          gotError = YES;
          break;
        }
      }
    }
    
    // 2. Part: parse queries with function names for foreign keys.
    if (!gotError && !stopTests)
    {
      dispatch_async(dispatch_get_main_queue(), ^{
        [self appendStepText: [NSString stringWithFormat: @"Parsed %lu queries in file successfully.\n", (unsigned long)queryCount]];

        [self appendStepText: @"Running function name tests\n"];
        statusText.stringValue = @"Running function name tests...";
      });
      gotError = ![self runFunctionNamesTest];
    }
    
    NSTimeInterval duration = [start timeIntervalSinceNow];
    NSString *durationText = [NSString stringWithFormat: @"Parse time: %.3fs\n\n", -duration];
    dispatch_async(dispatch_get_main_queue(), ^{
      if (stopTests)
        statusText.stringValue = @"Execution stopped by user.";
      else
        if (!gotError)
          statusText.stringValue = @"All queries parsed fine. Great!";
      [self appendStepText: durationText];
    });
    running = NO;
  } catch (std::exception& e) {
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
- (BOOL)runFunctionNamesTest
{
  static const char* functions[] = {
    "acos",
    "adddate",
    "addtime"
    "aes_decrypt",
    "aes_encrypt",
    "area",
    "asbinary",
    "asin",
    "astext",
    "aswkb",
    "aswkt",
    "atan",
    "atan2",
    "benchmark",
    "bin",
    "bit_count",
    "bit_length",
    "ceil",
    "ceiling",
    "centroid",
    "character_length",
    "char_length",
    "coercibility",
    "compress",
    "concat",
    "concat_ws",
    "connection_id",
    "conv",
    "convert_tz",
    "cos",
    "cot",
    "crc32",
    "crosses",
    "datediff",
    "date_format",
    "dayname",
    "dayofmonth",
    "dayofweek",
    "dayofyear",
    "decode",
    "degrees",
    "des_decrypt",
    "des_encrypt",
    "dimension",
    "disjoint",
    "elt",
    "encode",
    "encrypt",
    "endpoint",
    "envelope",
    "equals",
    "exp",
    "export_set",
    "exteriorring",
    "extractvalue",
    "find_in_set",
    "floor",
    "found_rows",
    "from_days",
    "from_unixtime",
    "geomcollfromtext",
    "geomcollfromwkb",
    "geometrycollectionfromtext",
    "geometrycollectionfromwkb",
    "geometryfromtext",
    "geometryfromwkb",
    "geometryn",
    "geometrytype",
    "geomfromtext",
    "geomfromwkb",
    "get_lock",
    "glength",
    "greatest",
    "hex",
    "ifnull",
    "inet_aton",
    "inet_ntoa",
    "instr",
    "interiorringn",
    "intersects",
    "isclosed",
    "isempty",
    "isnull",
    "issimple",
    "is_free_lock",
    "is_used_lock",
    "last_day",
    "last_insert_id",
    "lcase",
    "least",
    "length",
    "linefromtext",
    "linefromwkb",
    "linestringfromtext",
    "linestringfromwkb",
    "ln",
    "load_file",
    "locate",
    "log",
    "log10",
    "log2",
    "lower",
    "lpad",
    "ltrim",
    "makedate",
    "maketime",
    "make_set",
    "master_pos_wait",
    "mbrcontains",
    "mbrdisjoint",
    "mbrequal",
    "mbrintersects",
    "mbroverlaps",
    "mbrtouches",
    "mbrwithin",
    "md5",
    "mlinefromtext",
    "mlinefromwkb",
    "monthname",
    "mpointfromtext",
    "mpointfromwkb",
    "mpolyfromtext",
    "mpolyfromwkb",
    "multilinestringfromtext",
    "multilinestringfromwkb",
    "multipointfromtext",
    "multipointfromwkb",
    "multipolygonfromtext",
    "multipolygonfromwkb",
    "name_const",
    "nullif",
    "numgeometries",
    "numinteriorrings",
    "numpoints",
    "oct",
    "octet_length",
    "ord",
    "overlaps",
    "period_add",
    "period_diff",
    "pi",
    "pointfromtext",
    "pointfromwkb",
    "pointn",
    "polyfromtext",
    "polyfromwkb",
    "polygonfromtext",
    "polygonfromwkb",
    "pow",
    "power",
    "quote",
    "radians",
    "rand",
    "release_lock",
    "reverse",
    "round",
    "row_count",
    "rpad",
    "rtrim",
    "sec_to_time",
    "session_user",
    "sha",
    "sha1",
    "sign",
    "sin",
    "sleep",
    "soundex",
    "space",
    "sqrt",
    "srid",
    "startpoint",
    "strcmp",
    "str_to_date",
    "subdate",
    "substring_index",
    "subtime",
    "system_user",
    "tan",
    "timediff",
    "time_format",
    "time_to_sec",
    "touches",
    "to_days",
    "ucase",
    "uncompress",
    "uncompressed_length",
    "unhex",
    "unix_timestamp",
    "updatexml",
    "upper",
    "uuid",
    "version",
    "weekday",
    "weekofyear",
    "within",
    "x",
    "y",
    "yearweek"};
  
  const char *query1 = "CREATE TABLE %s(\n"
    "col1 int not null,\n"
    "col2 int not null,\n"
    "col3 varchar(10),\n"
    "CONSTRAINT pk PRIMARY KEY (col1, col2)\n"
    ") ENGINE InnoDB";
  
  const char *query2 = "CREATE TABLE bug21114_child(\n"
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
  unsigned serverVersion = [self getServerVersion];
  unsigned sqlModes = [self getSqlModes];
  for (size_t i = 0; i < count; i++)
  {
    if (stopTests)
      return NO;
    
    NSString *query = [NSString stringWithFormat: [NSString stringWithUTF8String: query1], functions[i]];
    if (query.length > 0) {
      progressLabel.stringValue = [NSString stringWithFormat: @"%lu of %li", i + 1, count];
      [progressLabel setNeedsDisplay];
      if (![self parseQuery: query version: serverVersion modes: sqlModes])
      {
        statusText.stringValue = @"Error encountered, offending query below:";
        errorQueryText.string = query;
        return NO;
      }
    }
    
    query = [NSString stringWithFormat: [NSString stringWithUTF8String: query2], functions[i], functions[i]];
    if (query.length > 0) {
      [progressLabel setNeedsDisplay];
      if (![self parseQuery: query version: serverVersion modes: sqlModes])
      {
        dispatch_async(dispatch_get_main_queue(), ^{
          statusText.stringValue = @"Error encountered, offending query below:";
          errorQueryText.string = query;
        });
        return NO;
      }
    }
  }
  return YES;
}

#pragma mark - Action handling

- (IBAction)startConversion: (id)sender
{
  NSString *tokenListString = @"";
  
  NSArray *queries = [queryList.string componentsSeparatedByCharactersInSet: [NSCharacterSet characterSetWithCharactersInString: @"$$"]];
  for (NSString *entry in queries) {
    NSString *query = [entry stringByTrimmingCharactersInSet: [NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if (query.length == 0)
      continue;
    
    pANTLR3_INPUT_STREAM input;
    pMySQLLexer lexer;
    pANTLR3_COMMON_TOKEN_STREAM tokenStream;
    pMySQLParser parser;

    RecognitionContext context = { 0, 0, [self getServerVersion], [self getSqlModes], nil};

    std::string utf8 = [query UTF8String];
    input = antlr3StringStreamNew((pANTLR3_UINT8)utf8.c_str(), ANTLR3_ENC_UTF8, utf8.size(), (pANTLR3_UINT8)"sql-script");
    input->setUcaseLA(input, ANTLR3_TRUE);

    lexer = MySQLLexerNew(input);
    lexer->pLexer->rec->state->userp = &context;
    tokenStream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
    parser = MySQLParserNew(tokenStream);
    parser->pParser->rec->state->userp = &context;

    MySQLParser_query_return ast = parser->query(parser);

    if (parser->pParser->rec->state->errorCount > 0) {
      conversionErrorText.stringValue = @"Errors occurred.";
    }

    NSString *tokens;
    if (ast.tree != NULL) {
      tokens = [self dumpTokens: ast.tree state: parser->pParser->rec->state];
    } else {
      tokens = @"(no tree)";
    }
    tokenListString = [tokenListString stringByAppendingString:
                       [NSString stringWithFormat: @"list_of %@,\n", tokens]
                       ];

    // Must manually clean up.
    parser->free(parser);
    tokenStream ->free(tokenStream);
    lexer->free(lexer);
    input->close(input);
  }

  tokenList.string = tokenListString;
  conversionErrorText.stringValue = @"Everything went fine.";
}

@end
