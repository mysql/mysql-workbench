/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#import "WBMiniToolbar.h"
#import "NSColor_extras.h"

static const float DEFAULT_HEIGHT = 24;

@implementation WBMiniToolbar

- (id)initWithFrame:(NSRect)frame 
{
  frame.size.height = DEFAULT_HEIGHT;
  self = [super initWithFrame: frame];
  if (self)
  {
    mOptionInfoList= [[NSMutableArray array] retain];
  }
  return self;
}


- (void) dealloc
{
  [mGradient release];
  [mOptionInfoList release];
  [super dealloc];
}

- (void)setGradient: (NSGradient*)gradient
{
  [mGradient autorelease];
  mGradient = [gradient retain];
}

- (NSSize)minimumSize
{
  return NSMakeSize(10, DEFAULT_HEIGHT);
}

- (BOOL)expandsOnLayoutVertically:(BOOL)vertically
{
  if (vertically)
    return NO;
  return YES;
}

- (void)resizeSubviewsWithOldSize:(NSSize)osize
{
  [self tile];
  [super resizeSubviewsWithOldSize: osize];
}


- (void)drawRect:(NSRect)rect 
{
  if (mGradient)
    [mGradient drawInRect: [self bounds] angle: 90];
  else
  {
    [[NSColor colorWithDeviceWhite: 0xe4/256.0 alpha: 1.0] set];
    NSRectFill(rect);
  }
}


- (void)removeAllItems
{
  for (id view in [[self subviews] reverseObjectEnumerator])
    [view removeFromSuperview];
}


- (void)setDelegate:(id)delegate
{
  mDelegate= delegate;
}


- (void)colorPopupChanged:(id)sender
{
  id info= [mOptionInfoList objectAtIndex: [sender tag]];
  
  [mDelegate miniToolbar: self
            popupChanged: [info objectForKey: @"name"]
                  option: [info objectForKey: @"option"]
                   value: [[sender selectedItem] representedObject]];
}


- (void)popupChanged:(id)sender
{
  id info= [mOptionInfoList objectAtIndex: [sender tag]];
  
  [mDelegate miniToolbar: self
            popupChanged: [info objectForKey: @"name"]
                  option: [info objectForKey: @"option"]
                   value: [sender titleOfSelectedItem]];
}


- (void)tile
{
  float height= NSHeight([self frame]);
  float x= 5;
  float fixedWidth = 0;
  int expanderCount = 0;

  for (id item in [self subviews])
  {
    if ([item class] == [NSView class] && NSHeight([item frame]) == 0)
      expanderCount++;
    else
      fixedWidth += NSWidth([item frame]);
  }
  fixedWidth += 6 * ([[self subviews] count]-1);

  for (id item in [self subviews])
  {
    NSRect frame= [item frame];
    frame.origin.x= x;
    frame.origin.y= (height - NSHeight(frame)) / 2;
    if ([item class] == [NSView class] && NSHeight([item frame]) == 0)
    {
      frame.size.width = (NSWidth([self frame]) - 10 - fixedWidth) / expanderCount;
    }   
    [item setFrame: frame];
    x+= NSWidth(frame);
    x+= 6;
  }
}


- (NSButton*)addButtonWithTitle:(NSString*)title
                         target:(id)target
                         action:(SEL)action
                            tag:(int)tag
{
  NSButton *button= [[[NSButton alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)] autorelease];
  
  [button setButtonType: NSMomentaryLightButton];
  [button setTitle: title];
  [button setTag: tag];
  [button setTarget: target];
  [button setAction: action];
  [button setBordered: YES];

  [button setBezelStyle: NSRoundedBezelStyle];
  
  [self addSubview: button];
  
  [button sizeToFit];
  [self tile];
  
  return button;
}

- (NSButton*)addButtonWithIcon:(NSImage*)icon
                        target:(id)target
                        action:(SEL)action
                           tag:(int)tag
{
  NSButton *button= [[[NSButton alloc] initWithFrame: NSMakeRect(0, 0, 18, 18)] autorelease];
  
  [button setButtonType: NSMomentaryLightButton];
  [button setBordered: NO];
  [button setImagePosition: NSImageOnly];
  [button setImage: icon];
  [button setTag: tag];
  [button setTarget: target];
  [button setAction: action];
  [button setBordered: NO];
  
  //[button setBezelStyle: NSRoundedBezelStyle];
  
  [self addSubview: button];
  
//  [button sizeToFit];
  [self tile];
  
  return button;
}


- (NSSegmentedControl*)addSegmentedButtonsWithIconsAndTags:(NSArray*)iconsAndTags
                                                    target:(id)target
                                                    action:(SEL)action
{
  NSSegmentedControl *seg = [[[NSSegmentedControl alloc] initWithFrame: NSMakeRect(0, 0, 30 * ([iconsAndTags count]/2), 24)] autorelease];
  [seg setSegmentCount: [iconsAndTags count] / 2];
  [seg setSegmentStyle: NSSegmentStyleTexturedSquare];
  [[seg cell] setTrackingMode: NSSegmentSwitchTrackingSelectAny];
  for (NSUInteger i = 0; i < [iconsAndTags count]/2; i++)
  {
    [seg setImage: [iconsAndTags objectAtIndex: i*2] forSegment: i];
    [[seg cell] setTag: [[iconsAndTags objectAtIndex: i*2+1] intValue] forSegment: i];
  }

  [self addSubview: seg];
  [self tile];
  
  return seg;
}

- (NSTextField*)addLabelWithTitle:(NSString*)title
{
  NSTextField *label= [[[NSTextField alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)] autorelease];
  
  [label setStringValue: title];
  [label setEditable: NO];
  [label setDrawsBackground: NO];
  [label setBordered: NO];
  [label setFont: [NSFont systemFontOfSize: [NSFont labelFontSize]]];
  
  [self addSubview: label];
  [label sizeToFit];
  [self tile];
  
  return label;
}

- (void)addExpandingSpace
{
  NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
  [self addSubview: view];
  [view release];
  [view setHidden: YES];
}

- (void)addSeparator
{
  NSBox *box= [[[NSBox alloc] initWithFrame: NSMakeRect(0, 2, 1, 18)] autorelease];
  [box setTitlePosition: NSNoTitle];
  [box setBoxType: NSBoxSeparator];
  [self addSubview: box];

  [self tile];
}

- (NSPopUpButton*)addSelectionPopUpWithItems:(NSArray*)items
                            target:(id)target
                            action:(SEL)action
                      defaultValue: (NSString*)defaultValue
{
  NSPopUpButton *popup= [[[NSPopUpButton alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)] autorelease];
  for (NSString *title in items)
  {
    if ([title length] == 0)
      [[popup menu] addItem: [NSMenuItem separatorItem]];
    else
      [popup addItemWithTitle: title];
  }
  [popup setTarget: target];
  [popup setAction: action];
  [[popup cell] setControlSize: NSSmallControlSize];
  [popup setFont: [NSFont systemFontOfSize: [NSFont smallSystemFontSize]]];
  [popup sizeToFit];
  [popup setBordered: NO];
  [self addSubview: popup];
  
  if (!defaultValue)
    [popup selectItemAtIndex: 0];
  else
    [popup selectItemWithTitle: defaultValue];
  
  [self tile];
  
  return popup;
}


- (NSPopUpButton*)addSelectionPopUpWithItems:(NSArray*)items
                              name: (NSString*)name
                            option: (NSString*)option
                      defaultValue: (NSString*)defaultValue
{
  NSPopUpButton *popup = [self addSelectionPopUpWithItems: items
                            target: self
                            action: @selector(popupChanged:) 
                      defaultValue: defaultValue];
  
  [popup setTag: [mOptionInfoList count]];  
    
  [mOptionInfoList addObject: [NSDictionary dictionaryWithObjectsAndKeys: 
                               name, @"name", 
                               option, @"option", nil]];
  
  return popup;
}

- (void)addSelectionPopUpWithColors:(NSArray*)colors
                               name: (NSString*)name
                             option: (NSString*)option
                       defaultValue: (NSString*)defaultValue
{
  NSPopUpButton *popup= [[[NSPopUpButton alloc] initWithFrame: NSMakeRect(0, 0, 10, 10)] autorelease];
  NSMenu *menu= [[[NSMenu alloc] initWithTitle: @""] autorelease];
  NSMenuItem *selected= nil;
  [[popup cell] setControlSize: NSSmallControlSize];
  for (NSColor *color in colors)
  {
    NSMenuItem *item= [[NSMenuItem alloc] init];
    NSImage *image= [[NSImage alloc] initWithSize: NSMakeSize(24, 16)];
    [image lockFocus];
    [[NSColor lightGrayColor] set];
    NSFrameRect(NSMakeRect(1, 1, 22, 14));
    [color set];
    NSRectFill(NSMakeRect(2, 2, 20, 12));
    [image unlockFocus];
    [item setImage: image];
    [item setTitle: @""];
    [item setRepresentedObject: [color hexString]];
    [image release];
    [menu addItem: item];
    [item release];
    
    if ([defaultValue isEqual: [item representedObject]])
      selected= item;
  }
  [popup setMenu: menu];
  
  if (selected)
    [popup selectItem: selected];
  
  [popup setBordered: NO];
  [popup setTarget: self];
  [popup setAction: @selector(colorPopupChanged:)];
  [popup sizeToFit];
  { // fix the extra unneeded padding we get for some reason
    NSRect frame= [popup frame];
    frame.size.width-= 24;
    [popup setFrame: frame];
  }
  [self addSubview: popup];
  
  [popup setTag: [mOptionInfoList count]];
  [mOptionInfoList addObject: [NSDictionary dictionaryWithObjectsAndKeys: name, @"name", option, @"option", nil]];

  [self tile];
}

@end
