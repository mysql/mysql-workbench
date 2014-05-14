 //
//  mysql_parserAppDelegate.h
//  mysql.parser
//
//  Created by Mike on 03.04.12.
//  Copyright 2012 Oracle Corporation. All rights reserved.
//

#include "MySQLLexer.h"
#include "MySQLParser.h"

extern "C" void on_parse_error(struct ANTLR3_BASE_RECOGNIZER_struct * recognizer, pANTLR3_UINT8 * tokenNames);

#import <Cocoa/Cocoa.h>

@interface mysql_parserAppDelegate : NSObject <NSApplicationDelegate, NSTextViewDelegate> {
  IBOutlet NSTextView *text;
  IBOutlet NSTextView *errorText;
  IBOutlet NSTextView *output;
  IBOutlet NSTextField *pathEdit;
  
  IBOutlet NSTextField *statusText;
  IBOutlet NSTextView *queryText;
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

@private
  NSWindow *window;
  
  NSUInteger queryCount;
  BOOL stopTests;
  BOOL running;
}

@property (assign) IBOutlet NSWindow *window;

- (IBAction)parse: (id)sender;
- (IBAction)selectFile: (id)sender;
- (IBAction)startTests: (id)sender;

- (IBAction)startConversion: (id)sender;

@end
