//
//  WBMFormsPluginPanel.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 1/Sep/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "WBBasePanel.h"

namespace mforms {
  class AppView;
  class MenuBar;
};

@interface WBMFormsPluginPanel : WBBasePanel
{
  mforms::AppView *_owner;
  mforms::MenuBar *_defaultMenuBar;
  NSString *_title;
}

+ (WBMFormsPluginPanel*)panelOfAppView:(mforms::AppView*)view;

- (id)initWithAppView:(mforms::AppView*)view;

- (void)setDefaultMenuBar:(mforms::MenuBar*)menu;

- (NSView*)topView;
- (NSString*)title;
- (NSString*)identifier;
- (NSImage*)tabIcon;
- (bec::UIForm*)formBE;
- (NSSize)minimumSize;

- (void)setTitle:(NSString*)title;

- (BOOL)willClose;
- (void)didOpen;

@end
