//
//  WBPluginWindowController.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 11/Nov/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "WBPluginEditorBase.h"


@interface WBPluginWindowController : NSObject {
  WBPluginEditorBase *mPluginEditor;
  
  IBOutlet NSWindow *window;
  IBOutlet NSView *contentView;
}

- (id)initWithPlugin:(WBPluginEditorBase*)plugin;

- (IBAction)buttonClicked:(id)sender;

@end
