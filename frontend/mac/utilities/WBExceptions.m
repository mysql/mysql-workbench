/*
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "WBExceptions.h"

@implementation NSException (WBExceptionExtensions)

/*!
 Print the symbolic stack trace to console.
 */
- (void) logStackTrace;
{
  NSMutableArray* stack = nil;
  
  // Look for stack trace info in the userInfo dictionary.
  NSString* stackString = [self userInfo][NSStackTraceKey];
  if (stackString != nil) {
    NSArray* stackStrings = [stackString componentsSeparatedByString: @"  "]; // Function addresses are separated by double spaces, not single.
    stack = [NSMutableArray array];
    for (NSString* s in stackStrings) {
      [stack addObject: s];
    }
  }
  
  if (stack == nil) {
    // There was no stack trace info in the userInfo dictionary, so we try to get it from -callStackReturnAddresses.
    NSArray* stackNumbers = [self callStackReturnAddresses];
    if ([stackNumbers count] > 0) {
      stack = [NSMutableArray array];
      for (NSNumber* n in stackNumbers) {
        [stack addObject: [NSString stringWithFormat: @"0x%x", [n intValue]]];
      }
    }
  }
  
  if (stack != nil) {
    // Set up the arguments for the atos tool.
    NSMutableArray* args = [NSMutableArray arrayWithCapacity: 20];
    NSString* pid = [@([[NSProcessInfo processInfo] processIdentifier]) stringValue];
    [args addObject: @"-p"];
    [args addObject: pid];
    [args addObjectsFromArray: stack];
    
    // Set up an NSTask to run the atos tool.
    NSTask* ls = [[NSTask alloc] init];
    [ls setLaunchPath: @"/usr/bin/atos"];
    [ls setArguments: args];
    
    NSLog(@"===================================");
    NSLog(@"=== Exception stack trace begin ===");
    [ls launch];
//    [ls waitUntilExit];

    [ls release];
  }
  else {
    NSLog(@"No stack trace available.");
  }
}



@end



@implementation WBExceptionHandlerDelegate

- (BOOL) exceptionHandler: (NSExceptionHandler*) sender
       shouldLogException: (NSException*) exception
                     mask: (NSUInteger) aMask;
{
  NSUserDefaults* sud = [NSUserDefaults standardUserDefaults];
  BOOL reportException = [sud boolForKey: @"ReportException"];
  
  // To enable exception reporting, write the following in terminal:
  // defaults write com.sun.MySQLWorkbench ReportException YES
  // To disable exception reporting, write the following in terminal:
  // defaults write com.sun.MySQLWorkbench ReportException NO
  
  if (reportException) {
    [exception logStackTrace];
    
    int reply = NSRunCriticalAlertPanel(@"An exception has occurred. The stack trace will be written to the log.",
                                        @"%@", nil, @"Terminate", nil, exception.reason);
    
    if (reply == NSAlertAlternateReturn) {
      exit(-1);
    }
  }
  
  return YES;
}



- (BOOL) exceptionHandler: (NSExceptionHandler*) sender
    shouldHandleException: (NSException*) exception
                     mask: (NSUInteger) aMask;
{
  return YES;
}



@end


