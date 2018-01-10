/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "GreenhouseWindowController.h"
#import "WBPaletteContainer.h"



@interface NSObject (Toolbar_Hack_Warning_Preventor)
// Declare a non-public method in Cocoa.
- (void) _setDrawsBaseline: (BOOL) flag;
@end



@implementation GreenhouseWindowController



- (void) tabViewItemDidReceiveDoubleClick: (id) itemIdentifier;
{
//  NSLog(@"Double click on item: %@", itemIdentifier);
}



- (void) addTabs;
{
	NSTabViewItem* tabViewItem = [[NSTabViewItem alloc] initWithIdentifier: @"new"];
	[tabViewItem setLabel: @"new"];
	[tabViewItem setView: mNewView];
	
	[mTabView addTabViewItem: tabViewItem];
	
	[mTabView selectLastTabViewItem: self];
	[mTabView setDelegate: self];
}

- (void) addTabs2;
{
	NSTabViewItem* tabViewItem = [[NSTabViewItem alloc] initWithIdentifier: @"new2"];
	[tabViewItem setLabel: @"new 2"];
	[tabViewItem setView: mNewView];
	
	[mTabView addTabViewItem: tabViewItem];
	
	[mTabView selectLastTabViewItem: self];
	[mTabView setDelegate: self];
}


- (void) renameTabs;
{
  [[mTabView tabViewItemAtIndex: 2] setLabel: @"Yoda"];
}



- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
//	NSLog(@"Did select");
}

- (BOOL)tabView:(NSTabView *)tabView shouldSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
//	NSLog(@"Should select");
	return YES;
}

- (void)tabView:(NSTabView *)tabView willSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
//	NSLog(@"Will select");
}

- (void)tabViewDidChangeNumberOfTabViewItems:(NSTabView *)tabView
{
//	NSLog(@"Did change number of items");
}



- (NSMenu*) tabView: (NSTabView*) tabView
  menuForIdentifier: (id) identifier;
{
  NSMenu* menu = nil;
  
  if (! [identifier isEqual: @"2"]) {
    menu = [[[NSMenu alloc] initWithTitle: @"yoda"] autorelease];
    [menu addItemWithTitle: [NSString stringWithFormat: @"This is not the '2' but the '%@' tab", identifier]
                    action: @selector(yoda)
             keyEquivalent: @""];
  }
  
  return menu;
}



#pragma mark -



- (void) applicationWillFinishLaunching: (NSNotification*) notification;
{
	[[self window] setToolbar: mToolbar];
	if ([mToolbar respondsToSelector: @selector(_toolbarView)]) {
		id toolbarview = [mToolbar performSelector: @selector(_toolbarView)];
		if ([toolbarview respondsToSelector: @selector(_setDrawsBaseline:)]) {
			[toolbarview _setDrawsBaseline: NO];
		}
	}

//  [mEditorTabView setTabViewType: NSNoTabsNoBorder];
}



- (void) applicationDidFinishLaunching: (NSNotification*) notification;
{
	[self performSelector: @selector(addTabs)
			   withObject: nil
			   afterDelay: 0.5];
	[self performSelector: @selector(addTabs2)
             withObject: nil
             afterDelay: 1.0];
//	[self performSelector: @selector(renameTabs)
//             withObject: nil
//             afterDelay: 1.0];
}



- (BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication*) theApplication;
{
	return YES;
}



@end


