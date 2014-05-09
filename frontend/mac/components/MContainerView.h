//
//  MContainerView.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 12/May/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface MContainerView : NSView 
{
  NSSize mMinSize;
  NSSize mPadding;
}

- (void)setMinContentSize:(NSSize)size;
- (void)setPadding:(NSSize)padding;

@end
