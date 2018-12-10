/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#import "WBSplitView.h"

//----------------------------------------------------------------------------------------------------------------------

@interface WBSplitView () {
  CGFloat dividerWidth;
  BOOL mEnabled;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBSplitView

@synthesize backgroundColor;

//----------------------------------------------------------------------------------------------------------------------

- (void) handleDidBecomeMain: (id) aNotification {
  mEnabled = YES;
  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)handleDidResignMain: (id) aNotification {
  mEnabled = NO;
  [self setNeedsDisplay: YES];
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Create and Destroy

- (void)awakeFromNib {
  mEnabled = YES;

  // Set up notifications.
  NSNotificationCenter* dc = [NSNotificationCenter defaultCenter];
  [dc addObserver: self
         selector: @selector(handleDidBecomeMain:)
             name: NSWindowDidBecomeMainNotification
           object: self.window];
  [dc addObserver: self
         selector: @selector(handleDidResignMain:)
             name: NSWindowDidResignMainNotification
           object: self.window];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  NSNotificationCenter* dc = [NSNotificationCenter defaultCenter];
  [dc removeObserver: self];
}

@end

//----------------------------------------------------------------------------------------------------------------------
