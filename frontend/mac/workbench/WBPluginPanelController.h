//
//  WBPluginPanelController.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 22/Oct/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "WBPluginPanel.h"


@interface WBPluginPanelController : NSObject 
{
  NSPanel *_panel;
  WBPluginPanel *_editor;
}

- (id)initWithEditor:(WBPluginPanel*)editor;
- (void)show:(id)sender;
- (void)hide:(id)sender;
@end
