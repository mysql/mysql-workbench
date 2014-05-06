//
//  MTogglePane.m
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 6/Oct/08.
//  Copyright 2008 Sun Microsystems Inc. All rights reserved.
//

#import "MTogglePane.h"


@implementation MTogglePane

- (id)initWithFrame:(NSRect)frame includeHeader:(BOOL)hasHeader
{
  if ((self= [super initWithFrame:frame]) != nil)
  {
    _initializing= YES;
    if (hasHeader)
    {
      _header= [[[NSImageView alloc] initWithFrame:NSMakeRect(0, 0, NSWidth(frame), 23)] autorelease];
      [self addSubview:_header];
      [_header setAutoresizingMask:NSViewWidthSizable];
      [_header setImageScaling:NSImageScaleAxesIndependently];
      [_header setImage:[NSImage imageNamed:@"collapsing_panel_header_bg_flat.png"]];
  
      _toggleButton= [[[NSButton alloc] initWithFrame:NSMakeRect(5, 5, 13, 13)] autorelease];
      [_toggleButton setBezelStyle:NSDisclosureBezelStyle];
      [_toggleButton setButtonType:NSOnOffButton];
      [_toggleButton setTitle:@""];
      [_toggleButton setAction:@selector(toggle:)];
      [_toggleButton setTarget:self];
      [_toggleButton setState: NSOnState]; // expanded by default
      [self addSubview:_toggleButton];
    
      _label= [[[NSTextField alloc] initWithFrame:NSMakeRect(20, 3, 20, 20)] autorelease];
      [_label setBordered:NO];
      [_label setEditable:NO];
      [_label setFont:[NSFont boldSystemFontOfSize:12]];
      [_label setDrawsBackground:NO];
      [self addSubview:_label];
    }
    _buttons= [[NSMutableArray array] retain];
    
    _initializing= NO;
    _relayouting= NO;
  }
  return self;
}

- (id)initWithFrame:(NSRect)frame
{
  return [self initWithFrame:frame includeHeader:YES];
}


- (void) dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver: self];
  
  [_buttons release];
  [super dealloc];
}



- (BOOL)isFlipped
{
  return YES;
}


- (IBAction)toggle:(id)sender
{
  [self relayout];
}


- (void)setExpanded:(BOOL)flag
{
  [_toggleButton setState:flag ? NSOnState : NSOffState];
  [self relayout];
}


- (void)setLabel:(NSString*)label
{
  [_label setStringValue:label];
  [_label sizeToFit];
}

- (void)contentFrameDidChange:(NSNotification*)notif
{
  [self relayout];
}


- (void)didAddSubview:(NSView *)subview
{
  // for working with IB
  if (!_initializing)
  {
    _content= subview;
    [_content setPostsFrameChangedNotifications:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(contentFrameDidChange:)
                                                 name:NSViewFrameDidChangeNotification
                                               object:_content];
    [subview setFrameOrigin:NSMakePoint(0, _header ? NSHeight([_header frame]) : 0)];
    [subview setAutoresizingMask:NSViewWidthSizable|NSViewMaxYMargin];
    [self relayout];
  }
}


- (void)setContentView:(NSView*)view
{
  if (_content)
  {
    [_content removeFromSuperview];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:nil object:_content];
  }

  [self addSubview:view];
}


- (NSView*)contentView
{
  return _content;
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  [self relayout];
  [super resizeSubviewsWithOldSize:oldBoundsSize];
}


- (void)relayout
{
  NSRect contentRect= [_content frame];
  NSRect newContentRect;
  NSRect buttonRect;
  NSRect rect= [self frame];
  CGFloat headerHeight= _header ? NSHeight([_header frame]) : 0;
  
  if (_relayouting)
    return;
  _relayouting= YES;
  
  if (!_toggleButton || [_toggleButton state] == NSOnState)
  {
    rect.size.height= headerHeight + NSHeight(contentRect);

    [_content setHidden:NO];
  }
  else
  {
    rect.size.height= headerHeight - 1;
    
    [_content setHidden:YES];
  }
  
  buttonRect.origin.x= rect.size.width - [_buttons count] * headerHeight;
  buttonRect.origin.y= 0;
  buttonRect.size.width= headerHeight;
  buttonRect.size.height= headerHeight;
  for (NSButton *btn in _buttons)
  {
    [btn setFrame:buttonRect];
    buttonRect.origin.x+= headerHeight;
  }
  
  newContentRect= NSMakeRect(0, headerHeight, NSWidth(rect), NSHeight(contentRect));
  
  if (!NSEqualRects(newContentRect, contentRect))
    [_content setFrame:newContentRect];
  
  if (!NSEqualRects([self frame], rect))
    [self setFrame:rect];
  
  _relayouting= NO;
}


- (NSButton*)addButton:(NSImage*)icon
            withAction:(SEL)selector
                target:(id)target
{
  CGFloat headerHeight= NSHeight([_header frame]);
  NSButton *button= [[[NSButton alloc] initWithFrame:NSMakeRect(0, 0, headerHeight, headerHeight)] autorelease];

  _initializing= YES;
  
  [_buttons addObject:button];
  
  [button setBordered:NO];
  [button setImage:icon];
  [button setImagePosition:NSImageOnly];
  [button setAction:selector];
  [button setTarget:target];
  [button setEnabled: (selector != nil) && (target != nil)];
  
  [self addSubview:button];
  
  [self relayout];
  
  _initializing= NO;
  
  return button;
}


@end
