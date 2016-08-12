 //
//  mysql_parserAppDelegate.h
//  mysql.parser
//
//  Created by Mike on 03.04.12.
//  Copyright 2012, 2016, Oracle Corporation. All rights reserved.
//

#include "MySQLLexer.h"
#include "MySQLParser.h"

#import <Cocoa/Cocoa.h>

@interface mysql_parserAppDelegate : NSObject <NSApplicationDelegate, NSTextViewDelegate> {
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

@private
  NSUInteger queryCount;
  BOOL stopTests;
  BOOL running;
}

@property (weak) IBOutlet NSWindow *window;

- (IBAction)parse: (id)sender;
- (IBAction)selectFile: (id)sender;
- (IBAction)startTests: (id)sender;

- (IBAction)startConversion: (id)sender;

@end
