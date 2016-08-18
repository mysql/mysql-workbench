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
}

@property (weak) IBOutlet NSWindow *window;

- (IBAction)parse: (id)sender;
- (IBAction)selectFile: (id)sender;
- (IBAction)startTests: (id)sender;

- (IBAction)startConversion: (id)sender;

@end
