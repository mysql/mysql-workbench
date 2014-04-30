/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "MTabSwitcher.h"

@implementation MTabSwitcher

@synthesize minTabWidth = mMinTabWidth;

static NSImage *TabExtender = nil;
static NSImage *CloseButtonImagePressed = nil;
static NSImage *CloseButtonImageUnpressed = nil;
static NSDictionary *MainTabImages = nil;
static NSDictionary *MainTabInactiveImages = nil;
static NSDictionary *EditorTabImages = nil;

#define MAIN_TAB_PADDING 34

+ (void)initialize
{
  [self exposeBinding: @"tabStyle"];
  
  NSBundle *bundle = [NSBundle bundleForClass: [MTabSwitcher class]];

  TabExtender = [[NSImage alloc] initWithContentsOfFile:
                             [bundle pathForImageResource: @"tab_extender"]];
  CloseButtonImagePressed = [[NSImage alloc] initWithContentsOfFile:
                             [bundle pathForImageResource: @"wb_tab-close_down"]];
  CloseButtonImageUnpressed = [[NSImage alloc] initWithContentsOfFile:
                               [bundle pathForImageResource: @"wb_tab-close"]];

  MainTabImages = [[NSDictionary dictionaryWithObjectsAndKeys:
                    [[[NSImage alloc] initWithContentsOfFile:
                      [bundle pathForImageResource: @"maintab_background"]] autorelease], @"bg",
                    [[[NSImage alloc] initWithContentsOfFile:
                      [bundle pathForImageResource: @"maintab_left"]] autorelease], @"left",
                    [[[NSImage alloc] initWithContentsOfFile:
                      [bundle pathForImageResource: @"maintab_center"]] autorelease], @"middle",
                    [[[NSImage alloc] initWithContentsOfFile:
                      [bundle pathForImageResource: @"maintab_right"]] autorelease], @"right",
                    [[[NSImage alloc] initWithContentsOfFile:
                      [bundle pathForImageResource: @"maintab_home"]] autorelease], @"home",
                    [[[NSImage alloc] initWithContentsOfFile:
                      [bundle pathForImageResource: @"maintab_home_home"]] autorelease], @"home_home",
                    [[[NSImage alloc] initWithContentsOfFile:
                      [bundle pathForImageResource: @"maintab_home_background"]] autorelease], @"home_bg",

                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"wb_tab-close_high"]] autorelease], @"close_high",
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"wb_tab-close_down"]] autorelease], @"close_down",
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"wb_tab-close"]] autorelease], @"close",
                      nil] retain];
  MainTabInactiveImages = [[NSDictionary dictionaryWithObjectsAndKeys:
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"maintab_background_inactive"]] autorelease], @"bg",
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"maintab_left_inactive"]] autorelease], @"left",
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"maintab_center_inactive"]] autorelease], @"middle",
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"maintab_right_inactive"]] autorelease], @"right",
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"maintab_home_inactive"]] autorelease], @"home",
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"maintab_home_home"]] autorelease], @"home_home",
                    [[[NSImage alloc] initWithContentsOfFile:
                     [bundle pathForImageResource: @"maintab_home_background"]] autorelease], @"home_bg",
                    
                    [MainTabImages objectForKey: @"close_high"], @"close_high",
                    [MainTabImages objectForKey: @"close_high"], @"close_high",
                    [MainTabImages objectForKey: @"close"], @"close",
                    nil] retain];

  EditorTabImages = [[NSDictionary dictionaryWithObjectsAndKeys:
                      [[[NSImage alloc] initWithContentsOfFile:
                        [bundle pathForImageResource: @"tab_background"]] autorelease], @"bg",
                      [[[NSImage alloc] initWithContentsOfFile:
                        [bundle pathForImageResource: @"tab_left"]] autorelease], @"left",
                      [[[NSImage alloc] initWithContentsOfFile:
                        [bundle pathForImageResource: @"tab_middle"]] autorelease], @"middle",
                      [[[NSImage alloc] initWithContentsOfFile:
                        [bundle pathForImageResource: @"tab_right"]] autorelease], @"right",
                      [[[NSImage alloc] initWithContentsOfFile:
                       [bundle pathForImageResource: @"tab_left_white"]] autorelease], @"left_white",
                      [[[NSImage alloc] initWithContentsOfFile:
                       [bundle pathForImageResource: @"tab_middle_white"]] autorelease], @"middle_white",
                      [[[NSImage alloc] initWithContentsOfFile:
                       [bundle pathForImageResource: @"tab_right_white"]] autorelease], @"right_white",
                      [[[NSImage alloc] initWithContentsOfFile:
                        [bundle pathForImageResource: @"wb_tab-close_high"]] autorelease], @"close_high",
                      [[[NSImage alloc] initWithContentsOfFile:
                       [bundle pathForImageResource: @"wb_tab-close_down"]] autorelease], @"close_down",
                      [[[NSImage alloc] initWithContentsOfFile:
                        [bundle pathForImageResource: @"wb_tab-close"]] autorelease], @"close",
                      nil] retain];
}


- (id)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame: frameRect];
  if (self)
  {
    // make editor the default. object editors rely on this default
    [self setTabStyle: MEditorBottomTabSwitcher];
    
    mToolTipTags = [[NSMutableArray alloc] init];
    
    mTrack = [[NSTrackingArea alloc] initWithRect: [self bounds]
                                          options: NSTrackingMouseEnteredAndExited|NSTrackingMouseMoved|NSTrackingActiveAlways
                                            owner: self
                                         userInfo: nil];
    [self addTrackingArea: mTrack];

    mCloseButtonRects = [[NSMutableDictionary alloc] init];

    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(frameDidChange:)
                                                 name: NSViewFrameDidChangeNotification
                                               object: self];
  }
  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver: self];

  [mCloseButtonRects release];

  for (id tag in mToolTipTags)
    [self removeToolTip: [tag intValue]];
  [mToolTipTags release];
  [mTrack release];
  [mLabelAttributes release];
  [mLabelDisabledAttributes release];
  [mLabelShadowAttributes release];

  [super dealloc];
}

- (void)setTabView:(NSTabView*)tabView
{
  if (mTabView != tabView)
  {
    if (mTabView)
      [mTabView setDelegate: mDelegate];
    
    mTabView = tabView;
    mDelegate = [mTabView delegate];
    [mTabView setDelegate: self];
  }
}

- (void)setDelegate:(id)delegate
{
  mDelegate = delegate;
}

- (void)setAllowTabReordering:(BOOL)flag
{
  mAllowTabReordering = flag;
}

- (id)delegate
{
  return mDelegate;
}


- (NSSize)_sizeOfTabViewItem:(NSTabViewItem*)item 
{
  NSSize size = NSZeroSize;
  if (item)
  {
    NSImage *icon = nil;
    switch (mStyle)
    {
      case MPaletteTabSwitcher:
      case MPaletteTabSwitcherSmallText:
      case MSectionTabSwitcher:
        size.height = 24;
        size.width = MAX(ceil([[item label] sizeWithAttributes: mLabelAttributes].width) + 20, mMinTabWidth);
        if (mMinTabWidth < mDefaultMinTabWidth)
          size.width = mMinTabWidth;
        if ([mDelegate respondsToSelector: @selector(tabView:iconForItem:)] &&
            (icon = [mDelegate tabView: mTabView iconForItem: item]))
          size.width += [icon size].width + 5;
        break;

      case MEditorTabSwitcher:
        size.height = 24;
        size.width = MAX(ceil([[item label] sizeWithAttributes: mLabelAttributes].width) + 23*2, mMinTabWidth);
        break;

      case MEditorBottomTabSwitcher:
        size.height = 30;
        size.width = MAX(ceil([[item label] sizeWithAttributes: mLabelAttributes].width) + 20, mMinTabWidth);
        if ([mDelegate respondsToSelector: @selector(tabView:iconForItem:)] && 
            (icon = [mDelegate tabView: mTabView iconForItem: item]))
          size.width += [icon size].width + 5;
        break;

      case MMainTabSwitcher:
        size.height = 23;
        if (![item label])
          size.width = [[MainTabImages objectForKey: @"home"] size].width;
        else
          size.width = MAX(ceil([[item label] sizeWithAttributes: mLabelAttributes].width) + MAIN_TAB_PADDING, mMinTabWidth);
        break;
    }
  }
  else
  {
    // return padding here
    switch (mStyle)
    {
      case MPaletteTabSwitcher:
      case MPaletteTabSwitcherSmallText:
        size.height = 24;
        size.width = 0;
        for (NSTabViewItem *item in [mTabView tabViewItems])
        {
          if (![[item view] isHidden])
            size.width += [self _sizeOfTabViewItem: item].width;
        }
        size.width = floor((NSWidth([self frame]) - size.width) / 2);
        if (size.width < 0)
          size.width = 1;
        break;
      case MSectionTabSwitcher:
        size.height = 24;
        size.width = 16;
        break;
      case MEditorTabSwitcher:
        size.height = 24;
        size.width = 10;
        break;
      case MEditorBottomTabSwitcher:
        size.height = 30;
        size.width = 10;
        break;
      case MMainTabSwitcher:
        size.height = 23;
        size.width = 8;
        break;
    }
  }
  return size;
}


- (void)setTabStyle:(MTabSwitcherStyle)style
{
  mStyle = style;
  [self setNeedsDisplay: YES];
  
  [mLabelDisabledAttributes release];
  mLabelDisabledAttributes = nil;
  [mLabelShadowAttributes release];
  mLabelShadowAttributes = nil;
  [mLabelAttributes release];
  switch (style)
  {
    case MSectionTabSwitcher:
    case MPaletteTabSwitcher:
    {
      NSMutableParagraphStyle *style = [[[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
      [style setAlignment: NSCenterTextAlignment];
      [style setLineBreakMode: NSLineBreakByTruncatingTail];
      
      mLabelAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                          [NSFont boldSystemFontOfSize: [NSFont systemFontSizeForControlSize: NSSmallControlSize]], NSFontAttributeName,
                          [NSColor blackColor], NSForegroundColorAttributeName,
                          style, NSParagraphStyleAttributeName,
                          nil];
      mMinTabWidth = 92;
      mDefaultMinTabWidth = mMinTabWidth;
      break;
    }
    case MPaletteTabSwitcherSmallText:
    {
      NSMutableParagraphStyle *style = [[[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
      [style setAlignment: NSCenterTextAlignment];
      [style setLineBreakMode: NSLineBreakByTruncatingTail];

      mLabelAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                          [NSFont boldSystemFontOfSize: [NSFont systemFontSizeForControlSize: NSMiniControlSize]], NSFontAttributeName,
                          [NSColor blackColor], NSForegroundColorAttributeName,
                          style, NSParagraphStyleAttributeName,
                          nil];
      mMinTabWidth = 70;
      mDefaultMinTabWidth = mMinTabWidth;
      break;
    }
    case MEditorTabSwitcher:
    {
      NSMutableParagraphStyle *style = [[[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
      [style setAlignment: NSCenterTextAlignment];
      [style setLineBreakMode: NSLineBreakByTruncatingTail];
      
      mLabelAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                          [NSFont boldSystemFontOfSize: [NSFont systemFontSizeForControlSize: NSSmallControlSize]], NSFontAttributeName,
                          [NSColor colorWithDeviceWhite: 53/255.0 alpha: 1.0], NSForegroundColorAttributeName,
                          style, NSParagraphStyleAttributeName,
                          nil];
      mLabelShadowAttributes = [mLabelAttributes mutableCopy];
      [mLabelShadowAttributes setObject: [NSColor colorWithDeviceWhite: 245/255.0 alpha: 1.0] forKey: NSForegroundColorAttributeName];
      mLabelDisabledAttributes = [mLabelAttributes mutableCopy];
      [mLabelDisabledAttributes setObject: [NSColor colorWithDeviceWhite: 108/255.0 alpha: 1.0] forKey: NSForegroundColorAttributeName];
      mMinTabWidth = 105;
      mDefaultMinTabWidth = mMinTabWidth;
      break;
    }
    case MEditorBottomTabSwitcher:
      mLabelAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                          [NSFont systemFontOfSize: [NSFont systemFontSizeForControlSize: NSSmallControlSize]], NSFontAttributeName,
                          [NSColor blackColor], NSForegroundColorAttributeName,
                          nil];
      mMinTabWidth = 80;
      mDefaultMinTabWidth = mMinTabWidth;
      break;
    case MMainTabSwitcher:
      mLabelAttributes = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
                          [NSFont systemFontOfSize: 11.5], NSFontAttributeName,
                          [NSColor blackColor], NSForegroundColorAttributeName,
                          nil];
      mMinTabWidth = 20;
      mDefaultMinTabWidth = mMinTabWidth;
      break;
  }
  if (!mLabelShadowAttributes)
    mLabelShadowAttributes = [mLabelAttributes mutableCopy];
  
  [self setFrameSize: NSMakeSize(NSWidth([self frame]), [self _sizeOfTabViewItem: nil].height)];
}


- (NSRect)_tabItemRect: (NSTabViewItem*)aItem
{
  int skip = mFirstVisibleTabIndex;
  float x = [self _sizeOfTabViewItem: nil].width;
  for (NSTabViewItem *item in [mTabView tabViewItems])
  {
    if ([[item view] isHidden]) continue;
    if (skip > 0)
    {
      skip--;
      continue;
    }
    NSSize size = [self _sizeOfTabViewItem: item];      
    if (item == aItem)
      return NSMakeRect(x, 0, size.width, size.height);
    x += size.width;
  }
  return NSZeroRect;
}


- (NSTabViewItem*)tabViewItemAtPoint:(NSPoint)pos
{
  // If extender button is visible then all x positions right to it (minus the spacing) don't hit a tab.
  if (NSWidth(mExternderButtonRect) > 0 && pos.x > NSMinX(mExternderButtonRect) - 4)
    return nil;
  
  switch (mStyle)
  {
    case MEditorBottomTabSwitcher:
      if (pos.y < 8)
        return nil;
      break;
    case MEditorTabSwitcher:
      if (pos.y > NSHeight([self frame]) - 8)
        return nil;
    default:
      break;
  }
  if (pos.y < 0 || pos.y > NSHeight([self frame]))
    return nil;
  
  int skip = mFirstVisibleTabIndex;
  float x = [self _sizeOfTabViewItem: nil].width;
  for (NSTabViewItem *item in [mTabView tabViewItems])
  {
    if ([[item view] isHidden]) continue;
    if (skip > 0)
    {
      skip--;
      continue;
    }
    float w = [self _sizeOfTabViewItem: item].width;
    if (pos.x >= x && pos.x < x + w)
    {
      return item;
    }
    x += w;
  }
  return nil;
}


- (NSSize)minimumSize
{
  return NSMakeSize(0, [self _sizeOfTabViewItem: nil].height);
}

- (BOOL)expandsOnLayoutVertically:(BOOL)vertically
{
  if (vertically)
    return NO;
  return YES;
}


static void tile_image(NSImage *tile, float y, float minX, float maxX)
{
  if (tile == nil)
    [NSException raise: @"Unexpected NULL value" format: @"tile_image() called with nil image"];

  NSSize isize = [tile size];

  float x = minX;
  for (; x + isize.width < maxX; x += isize.width)
    [tile drawInRect: NSMakeRect(x, y, isize.width, isize.height)
            fromRect: NSMakeRect(0, 0, isize.width, isize.height)
           operation: NSCompositeSourceOver fraction: 1.0];
  if (x < maxX)
    [tile drawInRect: NSMakeRect(x, y, (maxX - x), isize.height)
            fromRect: NSMakeRect(0, 0, (maxX - x), isize.height)
           operation: NSCompositeSourceOver fraction: 1.0];
}

static void draw_tab_images(NSImage *left, NSImage *middle, NSImage *right,
                            int minX, int maxX)
{
  NSSize lsize, rsize;

  lsize = [left size];
  rsize = [right size];
  [left drawInRect: NSMakeRect(minX - lsize.width, 0, lsize.width, lsize.height)
         fromRect: NSMakeRect(0, 0, lsize.width, lsize.height)
        operation: NSCompositeSourceOver fraction: 1.0];

  tile_image(middle, 0, minX, maxX - rsize.width);

  [right drawInRect: NSMakeRect(maxX - rsize.width, 0, rsize.width, rsize.height)
          fromRect: NSMakeRect(0, 0, rsize.width, rsize.height)
         operation: NSCompositeSourceOver fraction: 1.0];
}


- (float)drawMainTabStyleInFrame: (NSRect)rect forItem: (NSTabViewItem*)item
{
  float x = NSMinX(rect);
  float width = [self _sizeOfTabViewItem: item].width;
  if (width > NSWidth(rect))
    return 0;

  float height = NSHeight(rect);
  NSDictionary *images = [[self window] isMainWindow] ? MainTabImages : MainTabInactiveImages;

  if (![item label])
  {
    NSImage *icon = [images objectForKey: [item tabState] == NSBackgroundTab ? @"home" : @"home_home"];
    [icon drawInRect: NSMakeRect(x, 0, [icon size].width, [icon size].height)
            fromRect: NSMakeRect(0, 0, [icon size].width, [icon size].height)
           operation: NSCompositeSourceOver fraction: 1.0];
  }
  else
  {
    if ([item tabState] != NSBackgroundTab)
    {
      float selectedItemMinX = x + 0.5;
      float selectedItemMaxX = x + width + 0.5;

      draw_tab_images([images objectForKey: @"left"],
                      [images objectForKey: @"middle"],
                      [images objectForKey: @"right"],
                      selectedItemMinX, selectedItemMaxX);
    }

    float textoffs = [[images objectForKey: @"left"] size].width + 2;

    if ([item tabState] == NSSelectedTab)
      [mLabelShadowAttributes setObject: [NSColor colorWithDeviceWhite: 245/255.0 alpha: 1.0] forKey: NSForegroundColorAttributeName];
    else
      [mLabelShadowAttributes setObject: [NSColor colorWithDeviceWhite: 200/255.0 alpha: 1.0] forKey: NSForegroundColorAttributeName];
    
    if (![[self window] isMainWindow])
      [[item label] drawInRect: NSMakeRect(x + textoffs + 0.5, 0.5, width, height-6) withAttributes: mLabelShadowAttributes];
    [[item label] drawInRect: NSMakeRect(x + textoffs + 0.5, 1.5, width, height-6) withAttributes: mLabelAttributes];
    
    {
      NSImage *image;
      NSRect closeRect;
      if (mCloseHighlighted && mHoverItem == item)
      {
        if (mClosePressed)
          image = [images objectForKey: @"close_down"];
        else
          image = [images objectForKey: @"close_high"];
      }
      else
        image = [images objectForKey: @"close"];
      closeRect = NSMakeRect(x + width - [image size].width - [[images objectForKey: @"right"] size].width - 4,
                             floor((height - [image size].height) / 2),
                             [image size].width, [image size].height);
      [image drawInRect: closeRect fromRect: NSMakeRect(0, 0, [image size].width, [image size].height)
              operation: NSCompositeSourceOver fraction: 1.0];
      [mCloseButtonRects setObject: [NSValue valueWithRect: NSInsetRect(closeRect, -4, -4)]
                            forKey: [item identifier]];
    }
  }
  return width;
}


- (float)drawSectionTabStyleInFrame:(NSRect)rect forItem:(NSTabViewItem*)item
{
  NSString *label = [item label];
  NSSize labelSize = [label sizeWithAttributes: mLabelAttributes];
  NSRect tabRect;
  tabRect.origin = rect.origin;
  tabRect.size = [self _sizeOfTabViewItem: item];

  if ([item tabState] != NSBackgroundTab && [mTabView numberOfTabViewItems] > 1)
  {
    [[[[NSGradient alloc] initWithColors: [NSArray arrayWithObjects: 
                                           [NSColor colorWithDeviceWhite: 225/255.0 alpha:1.0],
                                           [NSColor colorWithDeviceWhite: 221/255.0 alpha:1.0],
                                           [NSColor colorWithDeviceWhite: 226/255.0 alpha:1.0],
                                           [NSColor colorWithDeviceWhite: 247/255.0 alpha:1.0],
                                           nil]] autorelease]
     drawInBezierPath: [NSBezierPath bezierPathWithRect: tabRect]
     angle: 270];
    
    // side bounds
    NSGradient *grad = [[NSGradient alloc] initWithColors: [NSArray arrayWithObjects:
                                                             [NSColor colorWithDeviceWhite: 203/255.0 alpha: 1.0],
                                                             [NSColor colorWithDeviceWhite: 138/255.0 alpha: 1.0],
                                                             [NSColor colorWithDeviceWhite: 226/255.0 alpha: 1.0],
                                                             nil]];
    [grad drawInRect: NSMakeRect(NSMaxX(tabRect), NSMinY(tabRect)+1, 1, NSHeight(tabRect)-2) angle: 270];
    [grad drawInRect: NSMakeRect(NSMinX(tabRect), NSMinY(tabRect)+1, 1, NSHeight(tabRect)-2) angle: 270];
    [grad release];
    
    grad = [[NSGradient alloc] initWithColors: [NSArray arrayWithObjects:
                                                [NSColor colorWithDeviceWhite: 224/255.0 alpha: 1.0],
                                                [NSColor colorWithDeviceWhite: 200/255.0 alpha: 1.0],
                                                [NSColor colorWithDeviceWhite: 246/255.0 alpha: 1.0],
                                                nil]];
    [grad drawInRect: NSMakeRect(NSMaxX(tabRect)-1, NSMinY(tabRect)+1, 1, NSHeight(tabRect)-2) angle: 270];
    [grad drawInRect: NSMakeRect(NSMinX(tabRect)+1, NSMinY(tabRect)+1, 1, NSHeight(tabRect)-2) angle: 270];
    [grad release];    
  }
  tabRect.origin.y = floor(NSMinY(tabRect) + 1 + (NSHeight(tabRect)-labelSize.height)/2);
  [label drawInRect: tabRect//NSMakePoint(NSMinX(tabRect) + (NSWidth(tabRect) - labelSize.width) / 2, NSMinY(tabRect) + 1 + (NSHeight(tabRect)-labelSize.height)/2)
     withAttributes: mLabelAttributes];

  return NSWidth(tabRect);
}


//
// Palette selector
//
- (float)drawPaletteTabStyleInFrame:(NSRect)rect forItem:(NSTabViewItem*)item
{
  NSString *label = [item label];
  NSSize labelSize = [label sizeWithAttributes: mLabelAttributes];
  NSRect tabRect;
  tabRect.origin = NSMakePoint(floor(NSMinX(rect)), floor(NSMinY(rect)));
  tabRect.size = [self _sizeOfTabViewItem: item];
    
  if ([item tabState] != NSBackgroundTab/* && [mTabView numberOfTabViewItems] > 1*/)
  {
    [[[[NSGradient alloc] initWithColorsAndLocations:
       [NSColor colorWithDeviceWhite: 0xba/255.0 alpha:1.0], (CGFloat)0.0,
       [NSColor colorWithDeviceWhite: 0xb8/255.0 alpha:1.0], (CGFloat)0.53,
       [NSColor colorWithDeviceWhite: 0xd2/255.0 alpha:1.0], (CGFloat)1.0,
       nil] autorelease]
     drawInBezierPath: [NSBezierPath bezierPathWithRect: tabRect]
     angle: 90];
    
    // side bounds
    NSGradient *grad = [[NSGradient alloc] initWithColorsAndLocations: 
                        [NSColor colorWithDeviceWhite: 0xaf/255.0 alpha: 1.0], (CGFloat)0.0,
                        [NSColor colorWithDeviceWhite: 0x71/255.0 alpha: 1.0], (CGFloat)0.5,
                        [NSColor colorWithDeviceWhite: 0xc7/255.0 alpha: 1.0], (CGFloat)1.0,
                        nil];
    [grad drawInRect: NSMakeRect(NSMaxX(tabRect), NSMinY(tabRect)+1, 1, NSHeight(tabRect)-2) angle: 90];
    [grad drawInRect: NSMakeRect(NSMinX(tabRect), NSMinY(tabRect)+1, 1, NSHeight(tabRect)-2) angle: 90];
    [grad release];
    
    grad = [[NSGradient alloc] initWithColorsAndLocations:
            [NSColor colorWithDeviceWhite: 0xb9/255.0 alpha: 1.0], (CGFloat)0.0,
            [NSColor colorWithDeviceWhite: 0xa6/255.0 alpha: 1.0], (CGFloat)0.5,
            [NSColor colorWithDeviceWhite: 0xcd/255.0 alpha: 1.0], (CGFloat)1.0,
            nil];
    [grad drawInRect: NSMakeRect(NSMaxX(tabRect)-1, NSMinY(tabRect)+1, 1, NSHeight(tabRect)-2) angle: 90];
    [grad drawInRect: NSMakeRect(NSMinX(tabRect)+1, NSMinY(tabRect)+1, 1, NSHeight(tabRect)-2) angle: 90];
    [grad release];    
  }
  tabRect.origin.y = floor(NSMinY(tabRect) + 1 + (NSHeight(tabRect)-labelSize.height)/2);
  tabRect.size.height = labelSize.height;
  if (labelSize.width < mMinTabWidth - 4)
    [label drawInRect: tabRect withAttributes: mLabelAttributes];
  else
    [label drawInRect: NSMakeRect(NSMinX(tabRect)+2, NSMinY(tabRect), mMinTabWidth - 4, NSHeight(tabRect))
       withAttributes: mLabelAttributes];
  
  return NSWidth(tabRect);
}

/**
 * Editor tabs style.
 */
- (float)drawEditorTabStyleInFrame: (NSRect)rect
                           forItem: (NSTabViewItem*)item
{
  NSString *label = [item label];
  NSSize labelSize = [label sizeWithAttributes: mLabelAttributes];
  NSRect tabRect;
  tabRect.origin = rect.origin;
  tabRect.size = [self _sizeOfTabViewItem: item];

  if (NSWidth(tabRect) > NSWidth(rect))
    return 0;
  
  NSImage *icon = [mDelegate respondsToSelector: @selector(tabView:iconForItem:)] ? [mDelegate tabView: mTabView iconForItem: item] : nil;
  NSImage *closePart = nil;
  NSRect r;

  bool white = false;
  // check whether this tab is set to have a white background (like the admin tab in the sql editor)
  if ([[item view] respondsToSelector: @selector(backgroundColor)])
  {
    NSColor *color = [[item view] backgroundColor];
    if ([color redComponent] == 1 && [color greenComponent] == 1 && [color blueComponent] == 1)
      white = true;
  }
  if ([item tabState] == NSSelectedTab)
    draw_tab_images([EditorTabImages objectForKey: white ? @"left_white" : @"left"],
                    [EditorTabImages objectForKey: white ? @"middle_white" : @"middle"],
                    [EditorTabImages objectForKey: white ? @"right_white" : @"right"],
                    NSMinX(tabRect), NSMaxX(tabRect));

  if (mHoverItem == item && mCloseHighlighted)
  {
    if (mClosePressed)
      closePart = [MainTabImages objectForKey: @"close_down"];
    else
      closePart = [MainTabImages objectForKey: @"close_high"];
  }
  else
    closePart = [MainTabImages objectForKey: @"close"];

  r.size = [closePart size];
  r.origin.x = NSMaxX(tabRect) - NSWidth(r) - [[EditorTabImages objectForKey: @"right"] size].width - 4;
  r.origin.y = floor((NSHeight(tabRect) - NSHeight(r)) / 2 - 2);
  if (item != mBusyTab)
    [closePart drawAtPoint: r.origin
                  fromRect: NSZeroRect
                 operation: NSCompositeSourceOver
                  fraction: 1.0];
  [mCloseButtonRects setObject: [NSValue valueWithRect: NSInsetRect(r, -4, -4)] forKey: [item identifier]];
  
  [icon drawAtPoint: NSMakePoint(NSMinX(tabRect) + 5, NSMinY(tabRect) + floor((NSHeight(tabRect) - 4 - [icon size].height)/2))
           fromRect: NSZeroRect 
          operation: NSCompositeSourceOver
           fraction: [item tabState] == NSSelectedTab ? 1.0 : 0.5];

  if ([item tabState] == NSSelectedTab)
    [label drawInRect: NSMakeRect(NSMinX(tabRect) - 2, NSMinY(tabRect) + 2,
                                  NSWidth(tabRect), labelSize.height)
        withAttributes: mLabelShadowAttributes];
  [label drawInRect: NSMakeRect(NSMinX(tabRect) - 2, NSMinY(tabRect) + 3,
                                NSWidth(tabRect), labelSize.height)
      withAttributes: [item tabState] == NSSelectedTab ? mLabelAttributes : mLabelDisabledAttributes];
  
  return NSWidth(tabRect);
}

//
// Bottom tabs style.
//
- (float)drawEditorBottomTabStyleInFrame:(NSRect)rect forItem:(NSTabViewItem*)item
{
  NSString *label = [item label];
  NSSize labelSize = [label sizeWithAttributes: mLabelAttributes];
  NSRect tabRect;
  tabRect.origin = rect.origin;
  tabRect.size = [self _sizeOfTabViewItem: item];
  
  if ([item tabState] == NSSelectedTab)
  {
    NSBezierPath *path, *shadowPath;
    float selectedItemMinX = NSMinX(tabRect) + 0.5;
    float selectedItemMaxX = NSMaxX(tabRect) + 0.5;
    float height = NSHeight(tabRect);
    
    path = [NSBezierPath bezierPath];
    [path moveToPoint: NSMakePoint(selectedItemMinX, height)];
    [path lineToPoint: NSMakePoint(selectedItemMinX, 7+7.5)];
    [path appendBezierPathWithArcWithCenter: NSMakePoint(selectedItemMinX+7, 8+7.5)
                                     radius: 7
                                 startAngle: 180
                                   endAngle: 270
                                  clockwise: NO];
    [path lineToPoint: NSMakePoint(selectedItemMaxX-7, 8.5)];
    [path appendBezierPathWithArcWithCenter: NSMakePoint(selectedItemMaxX-7, 8+7.5)
                                     radius: 7
                                 startAngle: 270
                                   endAngle: 0
                                  clockwise: NO];
    [path lineToPoint: NSMakePoint(selectedItemMaxX, height)];
  
    [[NSColor whiteColor] set];
    [path fill];

    [[NSColor colorWithDeviceWhite: 150.0/256 alpha: 0.8] set];
//    [NSBezierPath strokeLineFromPoint: NSMakePoint(selectedItemMinX+2, 9.5) toPoint: NSMakePoint(selectedItemMaxX-2, 9.5)];
    
    shadowPath = [NSBezierPath bezierPath];
    [shadowPath moveToPoint: NSMakePoint(selectedItemMinX-1, height-1)];
    [shadowPath lineToPoint: NSMakePoint(selectedItemMinX-1, 7+7.5)];
    [shadowPath appendBezierPathWithArcWithCenter: NSMakePoint(selectedItemMinX-1+7, 7+7.5)
                                     radius: 7
                                 startAngle: 180
                                   endAngle: 270
                                  clockwise: NO];
    [shadowPath lineToPoint: NSMakePoint(selectedItemMaxX+1-7, 7.5)];
    [shadowPath appendBezierPathWithArcWithCenter: NSMakePoint(selectedItemMaxX+1-7, 8+7.5)
                                     radius: 7
                                 startAngle: 270
                                   endAngle: 0
                                  clockwise: NO];
    [shadowPath lineToPoint: NSMakePoint(selectedItemMaxX+1, height-1)];
    [shadowPath setLineWidth: 1.0];
    [[NSColor colorWithDeviceWhite: 215.0/256 alpha: 0.8] set];
    [shadowPath stroke];
    [NSBezierPath strokeLineFromPoint: NSMakePoint(selectedItemMinX+2, 10) toPoint: NSMakePoint(selectedItemMaxX-2, 10)];
    
    [[NSColor colorWithDeviceWhite: 184.0/256 alpha: 1.0] set];
    [path setLineWidth: 1.0];
    [path stroke];
  }
  
  [label drawAtPoint: NSMakePoint(NSMinX(tabRect) + (NSWidth(tabRect) - labelSize.width) / 2, NSMinY(tabRect) + 9 + (NSHeight(tabRect)-8-labelSize.height)/2)
      withAttributes: mLabelAttributes];

  {
    NSImage *image = mClosePressed ? CloseButtonImagePressed : CloseButtonImageUnpressed;
    NSRect closeRect;
    closeRect = NSMakeRect(NSMaxX(tabRect)-14, (NSHeight(tabRect) - [image size].height) / 2 + 5, [image size].width, [image size].height);
    if (mHoverItem == item)
      [image drawInRect: closeRect fromRect: NSMakeRect(0, 0, [image size].width, [image size].height)
              operation: NSCompositeSourceOver fraction: 1.0];
    [mCloseButtonRects setObject: [NSValue valueWithRect: NSInsetRect(closeRect, -4, -4)] forKey: [item identifier]];
  }
  
  return NSWidth(tabRect);
}

- (void)drawExtenderInRect: (NSRect)rect
{
  NSPoint point = NSMakePoint(NSMinX(rect) + 4, (NSHeight(rect) - TabExtender.size.height) / 2.0 - 2);
  [TabExtender drawAtPoint: point
                  fromRect: NSZeroRect
                 operation: NSCompositeSourceOver
                  fraction: 1];
  mExternderButtonRect = NSMakeRect(point.x, point.y, TabExtender.size.width, TabExtender.size.height);
}

- (void)drawRect:(NSRect)rect
{
  rect = [self bounds];
  float padding = [self _sizeOfTabViewItem:nil].width;
  
  switch (mStyle)
  {
    case MSectionTabSwitcher:
    {
      [[[[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 225/255.0 alpha:1.0]
                                      endingColor: [NSColor colorWithDeviceWhite: 247/255.0 alpha:1.0]] autorelease]
       drawInBezierPath: [NSBezierPath bezierPathWithRect: rect]
       angle: 270];
      
      if (!mUnselected)
      {
        NSRect tabArea = rect;
        tabArea.origin.x = floor(NSMinX(rect)) + padding;
        tabArea.size.width -= 2 * padding;
        for (NSTabViewItem *item in [mTabView tabViewItems])
        {
          float w = [self drawSectionTabStyleInFrame: tabArea forItem: item];
          tabArea.origin.x += w;
          tabArea.size.width -= w;
        }
      }
      [[NSColor colorWithDeviceWhite: 236/256.0 alpha: 1.0] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMaxY(rect)-0.5) toPoint: NSMakePoint(NSMaxX(rect), NSMaxY(rect)-0.5)];

      [[NSColor colorWithDeviceWhite: 175/256.0 alpha: 1.0] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMinY(rect)+0.5) toPoint: NSMakePoint(NSMaxX(rect), NSMinY(rect)+0.5)];
      break;
    }
      
    case MPaletteTabSwitcher:
    case MPaletteTabSwitcherSmallText:
    {
      [[[[NSGradient alloc] initWithColorsAndLocations:
          [NSColor colorWithDeviceWhite: 0xbb/255.0 alpha:1.0], (CGFloat)0.0,
          [NSColor colorWithDeviceWhite: 0xc9/255.0 alpha:1.0], (CGFloat)0.53,
          [NSColor colorWithDeviceWhite: 0xd4/255.0 alpha:1.0], (CGFloat)0.77,
          [NSColor colorWithDeviceWhite: 0xdb/255.0 alpha:1.0], (CGFloat)1.0,
          nil] autorelease]
       drawInBezierPath: [NSBezierPath bezierPathWithRect: rect]
       angle: 90];
      
      NSRect tabArea = rect;
      tabArea.origin.x = floor(NSMinX(rect)) + padding;
      tabArea.size.width -= 2 * padding;
      for (NSTabViewItem *item in [mTabView tabViewItems])
      {
        if (![[item view] isHidden])
        {
          float w = [self drawPaletteTabStyleInFrame: tabArea forItem: item];
          tabArea.origin.x += w;
          tabArea.size.width -= w;
        }
      }
      [[NSColor colorWithDeviceWhite: 0x7e/256.0 alpha: 1.0] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMinY(rect)+0.5) toPoint: NSMakePoint(NSMaxX(rect), NSMinY(rect)+0.5)];
      break;
    }
      
    case MEditorTabSwitcher:
    {
      { // draw background
        NSImage *image = [EditorTabImages objectForKey: @"bg"];
        NSRect r = rect;
        r.size = [image size];
        for (r.origin.x = 0.0; r.origin.x < NSWidth(rect); r.origin.x += [image size].width)
          [image drawInRect: r fromRect: NSZeroRect operation: NSCompositeSourceOver fraction: 1.0];
      }

      NSTabViewItem *activeTab = nil;
      NSRect activeTabRect;
      NSRect tabArea = rect;
      tabArea.origin.x = floor(NSMinX(rect)) + padding;
      tabArea.size.width -= 2 * padding + mReservedSpace + TabExtender.size.width;
      
      NSArray *items = [mTabView tabViewItems];
      int i = mFirstVisibleTabIndex;
      mExternderButtonRect = NSZeroRect;
      NSTabViewItem *current = items.count > 0 ? [items objectAtIndex: i] : nil;
      while (current != nil)
      {
        if (![current.view isHidden])
        {
          if ((mHoverItem != current || !mDraggingTab) && [current tabState] != NSSelectedTab)
          {
            float w = [self drawEditorTabStyleInFrame: tabArea
                                              forItem: current];
            if (w == 0)
            {
              // Tab did not fit anymore and wasn't drawn. Instead draw the extender.
              [self drawExtenderInRect: tabArea];
              break;
            }
            tabArea.origin.x += w;
            tabArea.size.width -= w;
          }
          else
          {
            // This tab is being dragged or is the active one, so we draw it later.
            float w = [self _sizeOfTabViewItem: current].width;
            activeTabRect = tabArea;
            tabArea.origin.x += w;
            tabArea.size.width -= w;
            activeTab = current;
          }
        }
        current = ++i < [items count] ? [items objectAtIndex: i] : nil;
      }

      mLastVisibleTabIndex = --i;

      if (activeTab)
      {
        if (mDraggingTab)
          activeTabRect.origin.x = mTabDragPosition.x - mClickTabOffset.x;
        float w = [self drawEditorTabStyleInFrame: activeTabRect forItem: activeTab];
        if (w == 0 && !mDraggingTab)
        {
          // Can only be the case if the active tab is at the right end of the tab area.
          // It has been counted above as visible tab, so we need to correct this.
          [self drawExtenderInRect: activeTabRect];
          --mLastVisibleTabIndex;
        }
      }

      // Draw extender also if we came to last tab but there are invisible tabs at the left.
      if (mLastVisibleTabIndex == mTabView.numberOfTabViewItems - 1 && mFirstVisibleTabIndex > 0)
        [self drawExtenderInRect: tabArea];
      break;
    }

    case MEditorBottomTabSwitcher:
    {
      [[NSColor colorWithDeviceWhite: 230/256.0 alpha: 1.0] set];
      NSRectFill(rect);

      [[NSColor colorWithDeviceWhite: 187/256.0 alpha: 1.0] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMaxY(rect)-0.5) toPoint: NSMakePoint(NSMaxX(rect), NSMaxY(rect)-0.5)];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), NSMinY(rect)+0.5) toPoint: NSMakePoint(NSMaxX(rect), NSMinY(rect)+0.5)];
      
      NSRect tabArea = rect;
      tabArea.origin.x = floor(NSMinX(rect)) + padding;
      tabArea.size.width -= 2 * padding - mReservedSpace;
      for (NSTabViewItem *item in [mTabView tabViewItems])
      {
        if (mHoverItem != item || !mDraggingTab)
        {
          float w = [self drawEditorBottomTabStyleInFrame: tabArea forItem: item];
          tabArea.origin.x += w;
          tabArea.size.width -= w;
        }
        else
        {
          float w = [self _sizeOfTabViewItem: item].width;
          tabArea.origin.x += w;
          tabArea.size.width -= w;
        }
      }

      if (mHoverItem && mDraggingTab)
      {
        tabArea.origin.x = mTabDragPosition.x - mClickTabOffset.x;
        tabArea.size.width = [self _sizeOfTabViewItem: mHoverItem].width;
        [self drawEditorBottomTabStyleInFrame: tabArea forItem: mHoverItem];
      }
      break;
    }

    case MMainTabSwitcher:
    {
      [super drawRect: rect];
      
      NSRect selectedTabRect = NSZeroRect;
      NSRect tabArea = rect;
      tabArea.origin.x = floor(NSMinX(rect) + padding);
      tabArea.size.width -= 2 * padding - mReservedSpace;

      if (![[mTabView selectedTabViewItem] label])
        tile_image([[self window] isMainWindow] ? [MainTabImages objectForKey: @"home_bg"] : [MainTabInactiveImages objectForKey: @"home_bg"], 0, NSMinX(rect), NSMaxX(rect));
      else
        tile_image([[self window] isMainWindow] ? [MainTabImages objectForKey: @"bg"] : [MainTabInactiveImages objectForKey: @"bg"], 0, NSMinX(rect), NSMaxX(rect));

      NSArray *items = [mTabView tabViewItems];
      int i = mFirstVisibleTabIndex;
      mExternderButtonRect = NSZeroRect;
      NSTabViewItem *current = items.count > 0 ? [items objectAtIndex: i] : nil;
      while (current != nil)
      {
        float w;

        if ([current tabState] == NSSelectedTab)
          selectedTabRect = tabArea;
        if (mHoverItem != current || !mDraggingTab)
        {
          w = [self drawMainTabStyleInFrame: tabArea forItem: current];

          if (w == 0)
          {
            // Tab did not fit anymore and wasn't drawn. Instead draw the extender.
            [self drawExtenderInRect: tabArea];
            break;
          }
          tabArea.origin.x += w;
          tabArea.size.width -= w;
        }
        else
        {
          w = [self _sizeOfTabViewItem: current].width;
          tabArea.origin.x += w;
          tabArea.size.width -= w;
        }
        if ([current tabState] == NSSelectedTab)          
          selectedTabRect.size.width = w;

        current = ++i < items.count ? [items objectAtIndex: i] : nil;
      }
      
      mLastVisibleTabIndex = --i;

      if (mHoverItem && mDraggingTab)
      {
        tabArea.origin.x = mTabDragPosition.x - mClickTabOffset.x;
        tabArea.size.width = [self _sizeOfTabViewItem: mHoverItem].width;
        [self drawMainTabStyleInFrame: tabArea forItem: mHoverItem];
      }

      // Draw extender also if we came to last tab but there are invisible tabs at the left.
      if (mLastVisibleTabIndex == mTabView.numberOfTabViewItems - 1 && mFirstVisibleTabIndex > 0)
        [self drawExtenderInRect: tabArea];

      break;
    }
  }
}

- (void)makeUnselected
{
  mUnselected = YES;
}


- (void)didAddSubview:(NSView *)subview
{
  [self resizeSubviewsWithOldSize: NSZeroSize];
  [self setNeedsDisplay: YES];
}

- (void)didRemoveSubview:(NSView *)subview
{
  [self resizeSubviewsWithOldSize: NSZeroSize];
  [self setNeedsDisplay: YES];
}

- (void)tile
{
  [self resizeSubviewsWithOldSize: [self frame].size];
}


- (void)frameDidChange:(NSNotification*)notif
{
  [self removeTrackingArea: mTrack];
  [mTrack release];
  mTrack = [[NSTrackingArea alloc] initWithRect: [self bounds]
                                        options: NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways
                                          owner: self
                                       userInfo: nil];
  [self addTrackingArea: mTrack];

  if (mStyle != MEditorTabSwitcher)
    return;
  
  for (id tag in mToolTipTags)
    [self removeToolTip: [tag intValue]];
  [mToolTipTags removeAllObjects];

  int skip = mFirstVisibleTabIndex;
  for (NSTabViewItem *item in [mTabView tabViewItems])
  {
    if ([[item view] isHidden])
      continue;
    if (skip > 0)
    {
      skip--;
      continue;
    }
    [mToolTipTags addObject: [NSNumber numberWithInteger: [self addToolTipRect: [self _tabItemRect: item] owner: self userData: item]]];
  }

  [self setNeedsDisplay: YES];
}


- (NSString *)view:(NSView *)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void *)userData
{
  NSTabViewItem *item = [self tabViewItemAtPoint: point];
  if (item && [mDelegate respondsToSelector: @selector(tabView:toolTipForItem:)])
    return [mDelegate tabView: mTabView toolTipForItem: item];
  return nil;
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldSize
{ 
  // layout the additional views (like embedded Apply/Cancel buttons)
  mReservedSpace = 0;
  int count = 0;
  for (NSView *item in [self subviews])
  {
    if (![item isHidden] && item != mBusyTabIndicator)
    {
      count++;
      mReservedSpace += NSWidth([item frame]);
    }
  }
  if (count > 0)
  {
    mReservedSpace += 6 * (count - 1) + 16;
    
    float x = NSWidth([self frame]) - 16;
    for (NSView *item in [[self subviews] reverseObjectEnumerator])
    {
      if (![item isHidden] && item != mBusyTabIndicator)
      {
        NSRect r = [item frame];
        x -= NSWidth(r);
        r.origin.x = x;
        r.origin.y = (NSHeight([self frame]) - NSHeight(r)) / 2;
        [item setFrame: r];
        x -= 6;
      }
    }
  }
//  [self frameDidChange: nil];
}

#pragma mark Event Handlers


- (void)mouseDown:(NSEvent *)theEvent
{
  NSPoint clickPos = [self convertPoint: [theEvent locationInWindow] fromView: nil];
  NSTabViewItem *item = [self tabViewItemAtPoint: clickPos];
  mClosePressed = NO;
  mClickedItem = item;
  if (item)
  {
    NSRect closeRect = [[mCloseButtonRects objectForKey: [item identifier]] rectValue];

    if (mHoverItem && item != mBusyTab && NSPointInRect(clickPos, closeRect) &&
        ([mDelegate respondsToSelector: @selector(tabView:itemHasCloseButton:)] &&
         [mDelegate tabView: mTabView itemHasCloseButton: item]))
      mClosePressed = YES;
    else
      [mTabView selectTabViewItem: item];

    NSRect tabRect = [self _tabItemRect: item];
    mClickTabOffset = clickPos;
    mClickTabOffset.x -= tabRect.origin.x;

    mTabDragPosition = clickPos;

    [self display];
  }
}

- (void)mouseUp: (NSEvent *)theEvent
{
  NSPoint position = [self convertPoint: [theEvent locationInWindow] fromView: nil];
  if (NSPointInRect(position, mExternderButtonRect))
  {
    NSMenu *menu = [[[NSMenu alloc] initWithTitle: @"Tabs Menu"] autorelease];

    // Two loops, one for items before the first visible one and the other for items after the last visible one.
    int i = 0;
    while (i < mFirstVisibleTabIndex)
    {
      NSTabViewItem *tabItem = [mTabView tabViewItemAtIndex: i++];
      NSMenuItem *item = [menu addItemWithTitle: tabItem.label != nil ? tabItem.label : @"Home Screen"
                                         action: @selector(makeTabVisibleAndSelect:)
                                  keyEquivalent: @""];
      item.target = self;
      item.representedObject = tabItem;
    }

    i = mLastVisibleTabIndex + 1;
    while (i < mTabView.numberOfTabViewItems)
    {
      NSTabViewItem *tabItem = [mTabView tabViewItemAtIndex: i++];
      NSMenuItem *item = [menu addItemWithTitle: tabItem.label
                                         action: @selector(makeTabVisibleAndSelect:)
                                  keyEquivalent: @""];
      item.target = self;
      item.representedObject = tabItem;
    }

    [NSMenu popUpContextMenu: menu withEvent: theEvent forView: nil];
    return;
  }

  if (!mInside)
    mHoverItem = nil;

  if (mClosePressed && mHoverItem && NSPointInRect(position, [[mCloseButtonRects objectForKey: [mHoverItem identifier]] rectValue]))
  {
    NSTabViewItem *item = mHoverItem;
    [self setNeedsDisplay: YES];
    if ([mDelegate respondsToSelector: @selector(tabView:willCloseTabViewItem:)] &&
        [mDelegate tabView: mTabView willCloseTabViewItem: item] && item != mBusyTab)
    {
      if ([mTabView indexOfTabViewItem: item] != NSNotFound)
        [mTabView removeTabViewItem: item];
      mHoverItem = nil;
    }
  }
  if (mBusyTab)
  {
    [mBusyTabIndicator setHidden: NO];
    [self setBusyTab: mBusyTab]; // force update of indicator pos
  }
  mDraggingTab = NO;
  mClosePressed = NO;
  [self setNeedsDisplay: YES];
}

- (void)makeTabVisibleAndSelect: (id)sender
{
  NSTabViewItem *item = [sender representedObject];

  [mTabView selectTabViewItem: item];
  int index = [mTabView indexOfTabViewItem: item];
  if (index < mFirstVisibleTabIndex)
  {
    mFirstVisibleTabIndex = index;
    [self setNeedsDisplay: YES]; // Also recomputes the last visible entry.
  }
  else
    if (index > mLastVisibleTabIndex)
    {
      // Compute right border of the given item.
      CGFloat padding = [self _sizeOfTabViewItem: nil].width;
      CGFloat right = padding;
      for (NSTabViewItem *item in mTabView.tabViewItems)
      {
        if (![item.view isHidden])
          right += [self _sizeOfTabViewItem: item].width; // TODO: tab widths should be cached.
        if (item == [sender representedObject])
          break;
      }

      // Compute offset we need to shift tabs to the left.
      NSRect tabArea = self.bounds;
      tabArea.origin.x = floor(NSMinX(self.bounds)) + padding;
      tabArea.size.width -= 2 * padding + mReservedSpace + TabExtender.size.width;
      CGFloat offset = right - NSMaxX(tabArea);

      // Finally convert that offset into a tab index we use as first visible tab.
      mFirstVisibleTabIndex = 0;
      right = padding;
      for (NSTabViewItem *item in mTabView.tabViewItems)
      {
        if (right >= offset)
          break;

        if (![item.view isHidden])
          right += [self _sizeOfTabViewItem: item].width;
        ++mFirstVisibleTabIndex;
      }
    }
  // Else nothing to do.
}

- (void)closeTabViewItem: (NSTabViewItem*)item
{
  if ([mDelegate respondsToSelector: @selector(tabView:willCloseTabViewItem:)] &&
      [mDelegate tabView: mTabView willCloseTabViewItem: item])
  {
    [mTabView removeTabViewItem: item];
    [self setNeedsDisplay: YES];
  }
}


- (IBAction)handleMenuAction:(id)sender
{
  switch ([sender tag])
  {
    case 1000: // close tab

      if (mClickedItem &&
          (![mDelegate respondsToSelector: @selector(tabView:itemHasCloseButton:)] ||
           [mDelegate tabView: mTabView itemHasCloseButton: mClickedItem]))
      {
        [self closeTabViewItem: mClickedItem];
        mClickedItem = nil;
        mHoverItem = nil;
      }
      break;
    case 1001: // close other tabs
      for (NSTabViewItem *item in [[mTabView tabViewItems] reverseObjectEnumerator])
      {
        if (item != mClickedItem)
        {
          if ((![mDelegate respondsToSelector: @selector(tabView:itemHasCloseButton:)] ||
               [mDelegate tabView: mTabView itemHasCloseButton: item]))
            [self closeTabViewItem: item];
        }
      }
      [self setNeedsDisplay: YES];
      break;
  }
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
  NSPoint clickPos = [self convertPoint: [theEvent locationInWindow] fromView: nil];
  NSTabViewItem *item = [self tabViewItemAtPoint: clickPos];
  mClickedItem = item;
  if (item)
  {
    NSMenu *menu = [self menu];
    
    if (![mDelegate respondsToSelector: @selector(tabView:itemHasCloseButton:)] ||
        [mDelegate tabView: mTabView itemHasCloseButton: item])
      [[menu itemWithTag: 1000] setEnabled: YES];
    else
      [[menu itemWithTag: 1000] setEnabled: NO];
    if ([mTabView numberOfTabViewItems] > 1)
      [[menu itemWithTag: 1001] setEnabled: YES];
    else
      [[menu itemWithTag: 1001] setEnabled: NO];
    
    if ([mDelegate respondsToSelector: @selector(tabView:willDisplayMenu:forTabViewItem:)])
      [mDelegate tabView: mTabView willDisplayMenu: menu forTabViewItem: item];
    
    [NSMenu popUpContextMenu: menu withEvent: theEvent forView: self];
  }
}


- (NSTabViewItem*)clickedItem
{
  return mClickedItem;
}
       
- (void)mouseMoved:(NSEvent *)theEvent
{
  if (!mDraggingTab)
  {
    NSPoint pos = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    NSTabViewItem *item = [self tabViewItemAtPoint: pos];

    mCloseHighlighted = NO;
    if (![mDelegate respondsToSelector: @selector(tabView:itemHasCloseButton:)] ||
        ![mDelegate tabView: mTabView itemHasCloseButton: item])
      item = nil;
    else if (NSPointInRect(pos, [[mCloseButtonRects objectForKey: [item identifier]] rectValue]))
      mCloseHighlighted = YES;
    mHoverItem = item;
    [self setNeedsDisplay: YES];
  }
}


- (void)mouseDragged:(NSEvent *)theEvent
{
  mClosePressed = NO;
  if (mHoverItem)
  {
    NSPoint clickPos = [self convertPoint: [theEvent locationInWindow] fromView: nil];
    if (!(mDraggingTab || fabs(clickPos.x - mTabDragPosition.x > 3) || fabs(clickPos.y - mTabDragPosition.y) > 3))
      return;
    NSTabViewItem *item = [self tabViewItemAtPoint: NSMakePoint(clickPos.x, mClickTabOffset.y)];
    
    if (mAllowTabReordering && [mTabView numberOfTabViewItems] > 1 && item)
    {
      [mBusyTabIndicator setHidden: YES];
      mDraggingTab = YES;
    }
    mTabDragPosition = clickPos;

    if (mHoverItem != item && mDraggingTab) // handle reordering
    {
      NSTabViewItem *draggedItem = mHoverItem;
      BOOL passedThreshold = NO;
      
      if ([mTabView indexOfTabViewItem: draggedItem] > [mTabView indexOfTabViewItem: item])
      {
        if (clickPos.x < NSMaxX([self _tabItemRect: item]) - 20)
          passedThreshold = YES;
      }
      else
      {
        if (clickPos.x > NSMinX([self _tabItemRect: item]) + 20)
          passedThreshold = YES;
      }
      
      if (passedThreshold)
      {
        mReorderingTab = YES;
        [[draggedItem retain] autorelease];
        if (item)
        {
          int idx = [mTabView indexOfTabViewItem: item];
          if (![mDelegate respondsToSelector: @selector(tabView:willReorderTabViewItem:toIndex:)] ||
              [mDelegate tabView: mTabView willReorderTabViewItem: draggedItem toIndex: idx])
          {
            [mTabView removeTabViewItem: draggedItem];
            [mTabView insertTabViewItem: draggedItem atIndex: idx];
          }
        }
        else
        {
          int idx;
          if (clickPos.x < [self _sizeOfTabViewItem: nil].width)
            idx = 0;
          else
            idx = [mTabView numberOfTabViewItems];
          if (![mDelegate respondsToSelector: @selector(tabView:willReorderTabViewItem:toIndex:)] ||
              [mDelegate tabView: mTabView willReorderTabViewItem: draggedItem toIndex: idx])
          {
            [mTabView removeTabViewItem: draggedItem];
            if (idx == 0)
              [mTabView insertTabViewItem: draggedItem atIndex: 0];
            else
              [mTabView addTabViewItem: draggedItem];
          }
        }
        [mTabView selectTabViewItem: draggedItem]; // reselect the tab since it gets unselected when removed
        mReorderingTab = NO;
      }
    }
    [self setNeedsDisplay: YES];
  }
}


- (void)mouseEntered:(NSEvent *)theEvent
{
  mInside = YES;
  if (!mDraggingTab)
  {
    NSTabViewItem *item = [self tabViewItemAtPoint: [self convertPoint: [theEvent locationInWindow] fromView: nil]];
    if ([mDelegate respondsToSelector: @selector(tabView:itemHasCloseButton:)] &&
        [mDelegate tabView: mTabView itemHasCloseButton: item])
    {
      mHoverItem = item;
      [self setNeedsDisplay: YES];
    }
  }
}


- (void)mouseExited:(NSEvent *)theEvent
{
  mInside = NO;
  if (!mDraggingTab)
  {
    [self setNeedsDisplay: YES];
    mHoverItem = nil;
  }
}


- (void)setBusyTab: (NSTabViewItem*)tab
{
  mBusyTab = tab;
  if (tab)
  {
    NSRect rect = [self _tabItemRect: tab];
    switch (mStyle)
    {
      case MEditorTabSwitcher:
        if (mBusyTabIndicator)
          [mBusyTabIndicator setFrameOrigin: NSMakePoint(NSMaxX(rect) - 23, 2)];
        else
        {
          mBusyTabIndicator = [[[NSProgressIndicator alloc] initWithFrame: NSMakeRect(NSMaxX(rect) - 23, 2, 16, 16)] autorelease];
          [mBusyTabIndicator setControlSize: NSSmallControlSize];
          [mBusyTabIndicator setStyle: NSProgressIndicatorSpinningStyle];
          [mBusyTabIndicator setIndeterminate: YES];
          [mBusyTabIndicator startAnimation: nil];
          [self addSubview: mBusyTabIndicator];
        }
        [self setNeedsDisplay: YES];
        break;
      default:
        break;
    }
  }
  else
  {
    [mBusyTabIndicator stopAnimation: nil];
    [mBusyTabIndicator removeFromSuperview];
    mBusyTabIndicator = nil;
  }
}

#pragma mark Delegate Methods

- (void)tabViewDidChangeNumberOfTabViewItems:(NSTabView *)aTabView
{
  [self frameDidChange: nil];
  [self setNeedsDisplay: YES];
  if ([mDelegate respondsToSelector: @selector(tabViewDidChangeNumberOfTabViewItems:)])
    [mDelegate tabViewDidChangeNumberOfTabViewItems: aTabView];

  if ([aTabView numberOfTabViewItems] < [mCloseButtonRects count])
  {
    for (id item in [mCloseButtonRects allKeys])
    {
      if ([aTabView indexOfTabViewItemWithIdentifier: item] != NSNotFound)
        [mCloseButtonRects removeObjectForKey: item];
    }
  }

  if (!mReorderingTab)
  {
    if ([mTabView indexOfTabViewItem: mClickedItem] == NSNotFound)
      mClickedItem = nil;
    if ([mTabView indexOfTabViewItem: mHoverItem] == NSNotFound)
      mHoverItem = nil;
  }
}

- (void)tabView:(NSTabView *)aTabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  mUnselected = NO;
  [self setNeedsDisplay: YES];
  if ([mDelegate respondsToSelector: @selector(tabView:didSelectTabViewItem:)])
    [mDelegate tabView: aTabView didSelectTabViewItem: tabViewItem];
}

/* unfinished
#pragma mark IB Support

- (id)initWithCoder:(NSCoder *)coder
{
  self = [super initWithCoder:coder];
  if (self)
  {
    if ([coder allowsKeyedCoding])
    {
      [self setTabStyle: [[coder decodeObjectForKey: @"tabStyle"] intValue]];
    }
    else
    {
      [self setTabStyle: [[coder decodeObject] intValue]];
    }
    NSRect rect = [self frame];
    rect.origin = NSZeroPoint;
    mTrack = [[NSTrackingArea alloc] initWithRect: rect
                                          options: NSTrackingMouseEnteredAndExited|NSTrackingMouseMoved|NSTrackingActiveAlways
                                            owner: self
                                         userInfo: nil];
    [self addTrackingArea: mTrack];
  }
  return self;
}


- (void)encodeWithCoder:(NSCoder *)coder
{
  [super encodeWithCoder:coder];
  if ([coder allowsKeyedCoding])
  {
    [coder encodeObject: [NSNumber numberWithInt: mStyle]
                 forKey: @"tanStyle"];
  }
  else
  {
    [coder encodeObject: [NSNumber numberWithInt: mStyle]];
  }
}
*/

@end
