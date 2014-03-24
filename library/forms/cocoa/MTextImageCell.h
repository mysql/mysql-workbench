//
//  MTextImageCell.h
//  MySQLGUICommon
//
//  Created by Alfredo Kojima on Wed Jul 07 2004.
//  Copyright (c) 2004 MySQL AB. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface MTextImageCell : NSTextFieldCell {
  @private
  NSImage *_image;
}

- (void)setImage:(NSImage*)image;
- (NSImage*)image;

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)view;
- (NSSize)cellSize;

@end
