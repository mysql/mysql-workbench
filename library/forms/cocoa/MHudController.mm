/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

#import "MHudController.h"

//----------------------------------------------------------------------------------------------------------------------

@interface MHudController() {
  IBOutlet NSPanel* hudPanel;
  IBOutlet NSTextField* shortHudDescription;
  IBOutlet NSTextField* longHudDescription;
  IBOutlet NSButton* cancelButton;

  NSModalSession modalSession;
  BOOL stopped;

  std::function<bool ()> cancelAction;

  NSMutableArray *nibObjects;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MHudController

static MHudController* instance = nil;

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    NSMutableArray *temp;
    if ([NSBundle.mainBundle loadNibNamed: @"HUDPanel" owner: self topLevelObjects: &temp]) {
      nibObjects = temp;
      [hudPanel setBecomesKeyOnlyIfNeeded: YES];
    }
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
}

//----------------------------------------------------------------------------------------------------------------------

+ (void)showHudWithTitle: (NSString*) title andDescription: (NSString*) description {
  // TODO: perhaps this should be made thread safe (even tho it must never be called outside the main thread)?
  if (instance == nil)
    instance = [[MHudController alloc] init];
  
  NSWindow* mainWindow = NSApplication.sharedApplication.mainWindow;
  if (mainWindow != nil)
  {
    // The applications main window can be nil if the app has not finished loading or is hidden.
    // In those cases we don't need to show the hud either.
    NSRect parentFrame = mainWindow.frame;
    NSSize popupSize = instance.hud.frame.size;
    NSRect newFrame = NSMakeRect(parentFrame.origin.x + (parentFrame.size.width - popupSize.width) / 2,
                                 parentFrame.origin.y + (parentFrame.size.height - popupSize.height) / 2,
                                 popupSize.width, popupSize.height);

    [instance->cancelButton setHidden: YES];
    
    [instance showAnimatedWithFrame: newFrame title: title andDescription: description];
  }  
}

//----------------------------------------------------------------------------------------------------------------------

static NSCondition *modalLoopRunningCond = nil;
static BOOL modalHUDRunning = NO;

//----------------------------------------------------------------------------------------------------------------------

+ (void)initialize {
  modalLoopRunningCond = [[NSCondition alloc] init];
}

//----------------------------------------------------------------------------------------------------------------------

+ (BOOL)runModalHudWithTitle: (NSString*) title andDescription: (NSString*) description
                 notifyReady: (std::function<void ()>)signalReady
                cancelAction: (std::function<bool ()>)cancelAction {
  if (instance == nil)
    instance = [[MHudController alloc] init];
  
  NSWindow* mainWindow = [NSApplication sharedApplication].mainWindow;
  // The applications main window can be nil if the app has not finished loading or is hidden.
  NSRect parentFrame = mainWindow ? mainWindow.frame : [NSScreen mainScreen].frame;
  NSSize popupSize = instance.hud.frame.size;
  NSRect newFrame = NSMakeRect(parentFrame.origin.x + (parentFrame.size.width - popupSize.width) / 2,
                               parentFrame.origin.y + (parentFrame.size.height - popupSize.height) / 2,
                               popupSize.width, popupSize.height);
  instance->cancelAction = cancelAction;
  [instance->cancelButton setHidden: NO];
  [instance->cancelButton setNeedsDisplay: YES];
  
  instance->stopped = NO;
  
  [instance showAnimatedWithFrame: newFrame title: title andDescription: description];
  
  [modalLoopRunningCond lock];

  instance->modalSession = [NSApp beginModalSessionForWindow: instance.hud];
  
  modalHUDRunning = YES;
  [modalLoopRunningCond signal];
  [modalLoopRunningCond unlock];

  if (signalReady)
    signalReady();
    
  NSInteger ret = -1;
  // Can't use runModalForWindow because it will just block until some event happens
  // (like mouse move), even after stopModal is called.
  for (;;) {
    if (instance->stopped ||
        (ret = [NSApp runModalSession: instance->modalSession]) != NSModalResponseContinue)
      break;
    usleep(1000);
  }
  modalHUDRunning = NO;
  
  [NSApp endModalSession: instance->modalSession];
  instance->modalSession = nil;

  // Make sure shared_refs bound to it are not kept dangling.
  instance->cancelAction = std::function<bool()>();
  
  if (ret == NSModalResponseAbort) {
    [instance hideAnimated];
    return NO; // cancelled
  }
  [instance hideAnimated];
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

+ (void)stopModalHud {
  if (instance) {
    modalHUDRunning = NO;
    if (!instance->stopped) {
      dispatch_sync(dispatch_get_main_queue(), ^{
        [NSApp stopModal];
      });
    }
    instance->stopped = YES;
  }
}

//----------------------------------------------------------------------------------------------------------------------

+ (BOOL)hideHud {
  if (instance != nil) {
    BOOL result = instance.hud.visible;
    if (result)
      [instance hideAnimated];
    return result;
  }
  return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSPanel*)hud {
  return hudPanel;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)showAnimatedWithFrame: (NSRect) frame title: (NSString*) title andDescription: (NSString*) description {
  shortHudDescription.stringValue = title;
  longHudDescription.stringValue = description;

  [hudPanel setFrame: frame display: NO];
  [hudPanel makeKeyAndOrderFront: nil];
  
  [NSAnimationContext currentContext].duration = 0.5;
  [hudPanel animator].alphaValue = 1;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)orderOutPanel {
  [hudPanel orderOut: nil];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)hideAnimated {
  [NSAnimationContext currentContext].duration = 0.5;
  [hudPanel animator].alphaValue = 0;
  [self performSelector: @selector(orderOutPanel) withObject: nil afterDelay: 0.5];
}

//----------------------------------------------------------------------------------------------------------------------

- (IBAction)cancelClicked:(id)sender {
  if (cancelAction()) {
    [modalLoopRunningCond lock];
    if (modalHUDRunning) {
      modalHUDRunning = NO;
      if (!stopped)
        [NSApp abortModal];
      stopped = YES;
    }
    [modalLoopRunningCond unlock];
  }
}

@end

//----------------------------------------------------------------------------------------------------------------------
