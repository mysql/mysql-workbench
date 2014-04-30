//
//  MTogglePane.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 6/Oct/08.
//  Copyright 2008 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface MTogglePane : NSView 
{
  NSImageView *_header;
  NSTextField *_label;
  NSButton *_toggleButton;
  
  NSView *_content;
  
  NSMutableArray *_buttons;
  
  BOOL _initializing;
  BOOL _relayouting;
}

- (id)initWithFrame:(NSRect)frame includeHeader:(BOOL)flag;

- (IBAction)toggle:(id)sender;

- (void)setExpanded:(BOOL)flag;

- (void)setLabel:(NSString*)label;
- (void)setContentView:(NSView*)view;
- (NSView*)contentView;

- (void)relayout;

- (NSButton*)addButton:(NSImage*)icon
            withAction:(SEL)selector
                target:(id)target;

@end
