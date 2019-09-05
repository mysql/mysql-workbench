/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "AppDelegate.h"
#import "dialog.h"

#include <iostream>

@interface TreeItem: NSObject
@property long level;
@property long index;

@end;

@implementation TreeItem
@end

static NSMutableArray<NSDictionary *> *iconViewContent;

@interface AppDelegate () {
  NSString *pidFileName;
  DialogController *dialogController;
}

@property (weak) IBOutlet NSTextField *sliderLabel3;
@property (weak) IBOutlet NSTextField *sliderLabel1;
@property (weak) IBOutlet NSTextField *edit2;
@property IBOutlet NSTextView *edit4;
@property (weak) IBOutlet NSProgressIndicator *progress1;
@property (weak) IBOutlet NSProgressIndicator *progress2;
@property (weak) IBOutlet NSArrayController *iconViewController;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching: (NSNotification *)aNotification {
  NSProcessInfo *processInfo = [NSProcessInfo processInfo];

  for (NSString *argument in processInfo.arguments) {
    if ([argument hasPrefix: @"--pidfile="])
      pidFileName = [argument substringFromIndex: 10];
  }

  if (pidFileName.length == 0) {
    // We use the (Node compatible) way of getting the temp dir.
    const char *temp = getenv("TMPDIR");
    NSString *tempDir = (temp == NULL) ? NSTemporaryDirectory() : [NSString stringWithUTF8String: temp];
    pidFileName = [NSString stringWithFormat: @"%@/%@.app.pid", tempDir, processInfo.processName];
    std::cout << "Used pid file: " << pidFileName.UTF8String << std::endl;
  }

  NSString *pid = [NSString stringWithFormat: @"%i", processInfo.processIdentifier];
  NSError *error;
  [pid writeToFile: pidFileName atomically: YES encoding: NSUTF8StringEncoding error: &error];
  if (error != nil)
    NSLog(@"Couldn't write pid file: %@", error);

  self.progress1.doubleValue = 44.44;
  [self.progress1 startAnimation: nil];
  [self.progress2 startAnimation: nil];

  iconViewContent = [NSMutableArray new];
  NSUInteger i = 0;
  for (NSString *icon in @[@"NSCMYKButton",
                           @"NSColorPickerCrayon",
                           @"NSColorPickerList",
                           @"NSColorPickerSliders",
                           @"NSColorPickerUser",
                           @"NSColorPickerWheel",
                           @"NSColorSwatchResizeDimple",
                           @"NSGreyButton",
                           @"NSHSBButton",
                           @"NSMagnifyingGlass",
                           @"NSRGBButton",
                           @"NSSmallMagnifyingGlass",
                           @"NSStatusAvailableFlat",
                           @"NSStatusAway",
                           @"NSStatusIdle",
                           @"NSStatusNoneFlat",
                           @"NSStatusOffline",
                           @"NSStatusPartiallyAvailableFlat",
                           @"NSStatusUnavailableFlat",
                           @"NSStatusUnknown",
                           @"NSAddBookmarkTemplate",
                           @"NSAudioOutputMuteTemplate",
                           @"NSAudioOutputVolumeHighTemplate",
                           @"NSAudioOutputVolumeLowTemplate",
                           @"NSAudioOutputVolumeMedTemplate",
                           @"NSAudioOutputVolumeOffTemplate",
                           @"NSChildContainerEmptyTemplate",
                           @"NSChildContainerTemplate",
                           @"NSDropDownIndicatorTemplate",
                           @"NSGoLeftSmall",
                           @"NSGoRightSmall",
                           @"NSMenuMixedStateTemplate",
                           @"NSMenuOnStateTemplate",
                           @"NSNavEjectButton.normal",
                           @"NSNavEjectButton.normalSelected",
                           @"NSNavEjectButton.pressed",
                           @"NSNavEjectButton.rollover",
                           @"NSNavEjectButton.small.normal",
                           @"NSNavEjectButton.small.normalSelected",
                           @"NSNavEjectButton.small.pressed",
                           @"NSNavEjectButton.small.rollover",
                           @"NSPathLocationArrow",
                           @"NSPrivateArrowNextTemplate",
                           @"NSPrivateArrowPreviousTemplate",
                           @"NSPrivateChaptersTemplate",
                           @"NSScriptTemplate"]) {

    [iconViewContent addObject: [NSMutableDictionary
                                 dictionaryWithObjectsAndKeys: [NSString stringWithFormat: @"Title %lu", ++i], @"caption",
                                 [NSImage imageNamed: icon], @"image", nil]];
  }
  self.iconViewController.content = iconViewContent;
}

- (void)applicationWillTerminate: (NSNotification *)aNotification {
  [NSFileManager.defaultManager removeItemAtPath: pidFileName error: nil];
}

- (NSInteger)outlineView: (NSOutlineView *)outlineView numberOfChildrenOfItem: (id)item {
  if (item == nil)
    return 5;
  else {
    TreeItem *treeItem = (TreeItem *)item;
    if (treeItem.level < 5)
      return 5;

    return 0;
  }
}

- (void)outlineView: (NSOutlineView *)outlineView
    willDisplayCell: (id)cell
     forTableColumn: (NSTableColumn *)tableColumn
               item: (id)item {

  TreeItem *treeItem = (TreeItem *)item;
  if ([tableColumn.identifier isEqualToString: @"0"])
    [cell setStringValue: [NSString stringWithFormat: @"Entry %li.%li", treeItem.level, treeItem.index]];
  else
    [cell setStringValue: [NSString stringWithFormat: @"Data %li.%li", treeItem.level, treeItem.index]];
}

- (id)outlineView: (NSOutlineView *)outlineView child: (NSInteger)index ofItem: (id)item {
  if (item == nil) {
    TreeItem *treeItem = [TreeItem new];
    treeItem.level = 0;
    treeItem.index = index;
    return treeItem;
  }

  TreeItem *treeItem = item;
  if (treeItem.level < 5) {
    TreeItem *child = [TreeItem new];
    child.level = treeItem.level + 1;
    child.index = index;
    return child;
  }
  return nil;
}

- (BOOL)outlineView: (NSOutlineView *)outlineView isItemExpandable: (id)item {
  TreeItem *treeItem = item;
  return treeItem.level < 5;
}

- (NSInteger)numberOfRowsInTableView: (NSTableView *)tableView {
  return 20;
}

 - (nullable id)tableView: (NSTableView *)tableView
objectValueForTableColumn: (nullable NSTableColumn *)tableColumn
                      row: (NSInteger)row {
   int column = tableColumn.identifier.intValue;
   return @(row + column);
 }

#pragma mark - Actions

- (IBAction)menuClick:(NSMenuItem *)sender {
}

- (IBAction)onButtonClick: (NSButton *)sender {
  switch (sender.tag) {
    case 1: {
      NSOpenPanel *op = [NSOpenPanel openPanel];
      op.canChooseFiles = YES;
      op.canChooseDirectories = YES;
      [op runModal];
      self.edit2.stringValue = op.URLs.firstObject.absoluteString;
      break;
    }

    case 3: {
      if (dialogController == nil) {
        dialogController = [[DialogController alloc] init];
      }
      [dialogController.window makeKeyAndOrderFront: nil];
      [NSApp runModalForWindow: dialogController.window];

      break;
    }
  }
}

- (IBAction)slider3Changed:(id)sender {
  self.sliderLabel3.stringValue = [NSString stringWithFormat: @"%.4g%%", [sender floatValue]];
}

- (IBAction)slider1Changed:(id)sender {
  self.sliderLabel1.stringValue = [NSString stringWithFormat: @"%.4g%%", [sender floatValue]];
}

- (IBAction)radioButtonChanged:(id)sender {
  // Dummy body to enable radio button group behavior.
}

@end

@interface CollectionEntry : NSBox {
  BOOL selected;
}
@end

@implementation CollectionEntry

- (void)drawRect: (NSRect)dirtyRect {
  if (selected) {
    [[NSColor selectedControlColor] set];
    NSRectFill(dirtyRect);
  }
}

- (void)setSelected: (BOOL)value {
  if (value == selected) {
    return;
  }

  selected = value;
  [self setNeedsDisplay: YES];
}

@end

@interface CollectionView : NSCollectionView
@end

@implementation CollectionView

- (void)setSelectionIndexes: (NSIndexSet *)indexes {
  [super setSelectionIndexes: indexes];

  for (NSUInteger i = 0; i < iconViewContent.count; ++i) {
    NSCollectionViewItem *item = [self itemAtIndex: i];
    CollectionEntry *view = (CollectionEntry *)[item view];
    [view setSelected: [indexes containsIndex: i]];
  }
}

@end

