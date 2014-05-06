//
//  PrintDialog.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 3/Jun/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "WBPluginWindowBase.h"

@class CairoPrintView;

@interface PrintDialog : WBPluginWindowBase 
{
  CairoPrintView *printView;
  NSPrintInfo *printInfo;
}

@end
