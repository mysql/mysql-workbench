/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "MySQLLexer.h"
#include "MySQLParser.h"

#import <Cocoa/Cocoa.h>

@interface mysql_parserAppDelegate : NSObject<NSApplicationDelegate, NSTextViewDelegate> {
  IBOutlet NSTextView *singleQueryText;
  IBOutlet NSTextView *errorText;
  IBOutlet NSTextView *output;
  IBOutlet NSTextField *pathEdit;

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
  NSWindow *window;

  NSUInteger queryCount;
  BOOL stopTests;
  BOOL running;
}

@property(assign) IBOutlet NSWindow *window;

- (IBAction)parse:(id)sender;
- (IBAction)selectFile:(id)sender;
- (IBAction)startTests:(id)sender;

- (IBAction)startConversion:(id)sender;

@end
