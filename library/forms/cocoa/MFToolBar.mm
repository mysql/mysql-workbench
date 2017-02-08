/* 
 * Copyright (c) 2011, 2016, Oracle and/or its affiliates. All rights reserved.
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

#import "MFToolBar.h"
#import "MFBase.h"
#include "mforms/toolbar.h"
#import "MFMForms.h"


static struct
{
  const float height;
  const float xpadding;
  const float ypadding;
  const float item_width;
  const float spacing;
} layout_info[] = 
{
  {32, 12, 5, 36 - 2 * 7, 9}, // Main
  {26, 12, 2, 25 - 2 * 7, 7}, // Secondary
  {36, 12, 7, 36 - 2 * 7, 7}, // ToolPicker
  {20, 8, 1, 20-2*1, 4}, // Options
  {26, 4, 3, 26-2*3, 2} // Palette
};

static const float EMPTY_TOOLBAR_HEIGHT = 20;

using namespace mforms;


@interface MFColorMenuItem : NSMenuItem
{
}
@end

@implementation MFColorMenuItem

static NSColor* colorFromHexString(const char* hexcolor)
{
  int r, g, b;
  
  if (sscanf(hexcolor, "#%02x%02x%02x", &r, &g, &b) != 3)
    return nil;
  
  return [NSColor colorWithDeviceRed:r / 255.0 green:g / 255.0 blue:b / 255.0 alpha: 1.0];
}

- (instancetype)initWithColorName: (NSString*)color
{
  self = [super init];
  if (self)
  {
    NSImage *image= [[NSImage alloc] initWithSize: NSMakeSize(24, 16)];
    [image lockFocus];
    [[NSColor lightGrayColor] set];
    NSFrameRect(NSMakeRect(1, 1, 22, 14));
    [colorFromHexString(color.UTF8String) set];
    NSRectFill(NSMakeRect(2, 2, 20, 12));
    [image unlockFocus];
    self.image = image;
    self.title = @"";
    self.representedObject = color;
  }
  return self;
}

@end



@interface MFToolBarSeparatorImpl : NSView
{
  ToolBarItem *mOwner;
  BOOL mHorizontal;
}

- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;

@property (readonly) mforms::ToolBarItemType itemType;
@property (readonly) mforms::ToolBarItem *toolBarItem;

- (void)setHorizontal:(BOOL)flag;

@end


@implementation MFToolBarSeparatorImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item
{
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, 1, layout_info[0].height)];
  if (self)
  {
    mOwner = item;
    mOwner->set_data(self);
  }
  return self;
}

- (instancetype)initWithFrame: (NSRect)frame
{
  return [self initWithItemObject: nil];
}

- (instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithItemObject: nil];
}

- (ToolBarItem*)toolBarItem
{
  return mOwner;
}

- (void)setHorizontal:(BOOL)flag
{
  mHorizontal= flag;
  if (flag)
  {
    NSRect frame = self.frame;
    float tmp = frame.size.height;
    frame.size.height = frame.size.width;
    frame.size.width = tmp;
    self.frame = frame;
  }
}

- (ToolBarItemType)itemType
{
  return mOwner->get_type();
}

- (void)drawRect:(NSRect)rect
{
  if (mOwner->get_type() == SeparatorItem)
  {
    MFToolBarImpl *toolbar = (MFToolBarImpl*)self.superview;
    
    if (toolbar.type == SecondaryToolBar)
    {
      [[NSColor colorWithDeviceWhite: 140/255.0 alpha: 1.0] set];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(NSMinX(rect), 2) toPoint: NSMakePoint(NSMinX(rect), NSHeight(rect)-1)];
    }
    else
    {
      NSBezierPath *vdotted;
      
      CGFloat pattern[] = {1.0, 2.0};
      vdotted = [NSBezierPath bezierPath];
      vdotted.lineWidth = 1.0;
      [vdotted setLineDash: pattern count: 2 phase: 0.0];
      
      if (mHorizontal)
      {
        [vdotted moveToPoint: NSMakePoint(0, 0)];
        [vdotted lineToPoint: NSMakePoint(NSWidth(self.frame), 0)];
      }
      else
      {
        [vdotted moveToPoint: NSMakePoint(0, 0)];
        [vdotted lineToPoint: NSMakePoint(0, NSHeight(self.frame))];
      }
      
      [[NSColor darkGrayColor] set];
      [vdotted stroke];
    }
  }
}

@end

//--------------------------------------------------------------------------------------------------

@interface MFToolBarActionItemImpl : NSButton 
{
  ToolBarItem *mOwner;
  BOOL mToolPicker;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;
@property (readonly) mforms::ToolBarItem *toolBarItem;
@end


@implementation MFToolBarActionItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item
{
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, layout_info[0].item_width, layout_info[0].item_width)];
  if (self)
  {
    mOwner = item;
    mOwner->set_data(self);
    switch (item->get_type())
    {
      case ToggleItem:
        [self setButtonType: NSToggleButton];
        [self.cell setHighlightsBy: NSChangeBackgroundCellMask];
        self.bordered = NO;
        break;

      case TextActionItem:
        self.imagePosition = NSNoImage;
        self.bordered = NO;
        break;

      case SegmentedToggleItem:
        [self setButtonType: NSToggleButton];
        self.imagePosition = NSImageOnly;
        [self.cell setHighlightsBy: NSChangeBackgroundCellMask];
        self.bordered = NO;
        break;

      case SwitcherItem:
        self.bezelStyle = NSRecessedBezelStyle;
        self.showsBorderOnlyWhileMouseInside = YES;
        self.buttonType = NSMomentaryPushInButton;
        self.bordered = YES;
        break;

      default: // ActionItem etc.
        self.bezelStyle = NSTexturedRoundedBezelStyle;
        self.buttonType = NSMomentaryPushInButton;
        //[self.cell setImageScaling: NSImageScaleProportionallyDown];
        self.bordered = NO;

        break;
    }
    self.target = self;
    self.action = @selector(perform:);
    self.title = @"";
  }
  return self;
}

-(instancetype)initWithFrame: (NSRect)frame
{
  return [self initWithItemObject: nil];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithItemObject: nil];
}

- (void)dealloc
{
  mOwner = NULL;
}

- (ToolBarItem*)toolBarItem
{
  return mOwner;
}

- (void)perform: (id)sender
{
  mOwner->callback();
  if (mToolPicker)
    self.bordered = self.state == NSOnState;  
}


- (void)setState:(NSInteger)value
{
  super.state = value;
  if (mToolPicker)
    self.bordered = value == NSOnState;
}

- (void)setImage: (NSImage *)image
{
  super.image = image;
  if (self.title.length > 0)
    self.imagePosition = NSImageLeft;
  else
    self.imagePosition = NSImageOnly;

  [self sizeToFit];
}

- (void)viewDidMoveToSuperview
{
  if (mOwner == NULL)
    return;

  MFToolBarImpl *toolbar = (MFToolBarImpl*)self.superview;
  if (toolbar == nil)
    return;

  if (!self.alternateImage && mOwner->get_type() == ToggleItem)
  {
    mToolPicker = YES;
    [self setButtonType: NSOnOffButton];
    [self setBordered: NO];
    self.bezelStyle = NSTexturedSquareBezelStyle;
    [self.cell setBackgroundColor: toolbar.backgroundColor];

    if (toolbar.type == mforms::SecondaryToolBar)
    {
      NSSize tsize = toolbar.frame.size;
      [self setFrameSize: NSMakeSize(tsize.height, tsize.height)];
    }
  }
}

- (void)setStringValue: (NSString*)value
{
  self.title = value;
  if (self.image != nil)
    self.imagePosition = NSImageLeft;
  else
    self.imagePosition = NSImageOnly;

  [self sizeToFit];
}

- (NSString*)stringValue
{
  return self.title;
}

@end

//--------------------------------------------------------------------------------------------------

@interface MFToolBarLabelItemImpl : NSTextField
{
  MFToolBarImpl *mToolbar;
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;
- (void)setToolbar:(MFToolBarImpl*)toolbar;
@property (readonly) mforms::ToolBarItem *toolBarItem;
@end

@implementation MFToolBarLabelItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item
{
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, 200, layout_info[0].item_width)];
  if (self)
  {
    mOwner = item;
    mOwner->set_data(self);
    [self setBordered: NO];
    [self setEditable: NO];
    [self setDrawsBackground: NO];
    if (item->get_type() == mforms::TitleItem)
    {
      self.font = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
      self.textColor = [NSColor colorWithDeviceWhite: 0x33/255.0 alpha:1.0];
    }
    else
      self.font = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
  }
  return self;
}

-(instancetype)initWithFrame: (NSRect)frame
{
  return [self initWithItemObject: nil];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithItemObject: nil];
}

- (void)setToolbar:(MFToolBarImpl *)toolbar
{
  mToolbar = toolbar;
}


- (ToolBarItem*)toolBarItem
{
  return mOwner;
}

- (void) setStringValue:(NSString *)aString
{
  super.stringValue = aString;
  [self sizeToFit];
  [mToolbar resizeSubviewsWithOldSize: NSZeroSize];
}

@end

//--------------------------------------------------------------------------------------------------


@interface MFToolBarImageItemImpl : NSImageView
{
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;
@property (readonly) mforms::ToolBarItem *toolBarItem;
@end

@implementation MFToolBarImageItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item
{
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, layout_info[0].item_width, layout_info[0].item_width)];
  if (self)
  {
    mOwner = item;
    mOwner->set_data(self);
    self.imageFrameStyle = NSImageFrameNone;
    self.imageScaling = NSImageScaleNone;
  }
  return self;
}

-(instancetype)initWithFrame: (NSRect)frame
{
  return [self initWithItemObject: nil];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithItemObject: nil];
}

- (ToolBarItem*)toolBarItem
{
  return mOwner;
}

- (void)setImage:(NSImage*)image
{
  super.image = image;
  [self setFrameSize: image.size];
}
@end

//--------------------------------------------------------------------------------------------------

@interface MFToolBarSearchItemImpl : NSSearchField
{
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;
@property (readonly) mforms::ToolBarItem *toolBarItem;
@end

@implementation MFToolBarSearchItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item
{
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, 200, layout_info[0].item_width)];
  if (self)
  {
    mOwner = item;
    mOwner->set_data(self);
    [self.cell setSendsSearchStringImmediately: NO];
 //   [[self cell] setSendsActionOnEndEditing: YES];
    self.target = self;
    self.action = @selector(perform:);
  }
  return self;
}

-(instancetype)initWithFrame: (NSRect)frame
{
  return [self initWithItemObject: nil];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithItemObject: nil];
}

- (ToolBarItem*)toolBarItem
{
  return mOwner;
}

- (void)perform:(id)sender
{
  mOwner->callback();
}
@end

//--------------------------------------------------------------------------------------------------

@interface MFToolBarTextItemImpl : NSTextField
{
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;
@property (readonly) mforms::ToolBarItem *toolBarItem;
@end

@implementation MFToolBarTextItemImpl

- (instancetype)initWithItemObject: (ToolBarItem*)item
{
  if (item == nil)
    return nil;

  self = [super initWithFrame: NSMakeRect(0, 0, 200, layout_info[0].item_width)];
  if (self)
  {
    mOwner = item;
    mOwner->set_data(self);
  }
  return self;
}

-(instancetype)initWithFrame: (NSRect)frame
{
  return [self initWithItemObject: nil];
}

-(instancetype)initWithCoder: (NSCoder *)coder
{
  return [self initWithItemObject: nil];
}

- (ToolBarItem*)toolBarItem
{
  return mOwner;
}

@end

//--------------------------------------------------------------------------------------------------

@interface MFToolBarSelectorItemImpl : NSPopUpButton
{
  ToolBarItem *mOwner;
}
- (instancetype)initWithItemObject:(ToolBarItem*)item NS_DESIGNATED_INITIALIZER;
@property (readonly) mforms::ToolBarItem *toolBarItem;
@end

@implementation MFToolBarSelectorItemImpl
- (instancetype)initWithItemObject:(ToolBarItem*)item
{
  self = [super initWithFrame: NSMakeRect(0, 0, 150, layout_info[0].item_width)];
  if (self)
  {
    mOwner = item;
    mOwner->set_data(self);
    self.target = self;
    self.action = @selector(perform:);
    self.cell.controlSize = NSSmallControlSize;
    self.font = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
  }
  return self;
}

- (ToolBarItem*)toolBarItem
{
  return mOwner;
}

- (void) viewDidMoveToSuperview
{
  switch (((MFToolBarImpl*)self.superview).type)
  {
    case OptionsToolBar:
      [self setBordered: NO];
      break;
    default:
      break;
  }
}

- (void)perform:(id)sender
{
  mOwner->callback();
}


- (NSString*)stringValue
{
  if (mOwner->get_type() == ColorSelectorItem)
    return self.selectedItem.representedObject;
  else
    return self.selectedItem.title;
}


- (void)setStringValue:(NSString*)value
{
  if (mOwner->get_type() == ColorSelectorItem)
    [self selectItemAtIndex: [self indexOfItemWithRepresentedObject: value]];
  else
    [self selectItemWithTitle: value];
}

@end

//--------------------------------------------------------------------------------------------------

@implementation MFToolBarImpl

- (instancetype)initWithObject:(ToolBar*)owner type:(ToolBarType)type
{
  switch (type) 
  {
    case MainToolBar:
      self = [super initWithFrame:NSMakeRect(0, 0, 100, EMPTY_TOOLBAR_HEIGHT)];
      break;

    case SecondaryToolBar:
      self = [super initWithFrame:NSMakeRect(0, 0, 100, layout_info[SecondaryToolBar].height)];
      break;
      
    case ToolPickerToolBar: // this is vertical
      self = [super initWithFrame:NSMakeRect(0, 0, layout_info[ToolPickerToolBar].height, 100)];
      break;
      
    case OptionsToolBar:
      self = [super initWithFrame:NSMakeRect(0, 0, 100, layout_info[OptionsToolBar].height)];
      break;
      
    case PaletteToolBar:
      self = [super initWithFrame:NSMakeRect(0, 0, 100, layout_info[PaletteToolBar].height)];
      break;      
  }
      
  if (self)
  {
    mOwner = owner;
    mOwner->set_data(self);
    
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(windowKeyChanged:)
                                                 name: NSWindowDidBecomeMainNotification
                                               object: nil];
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(windowKeyChanged:)
                                                 name: NSWindowDidResignMainNotification
                                               object: nil];    
  }
  return self;
}


- (NSSize)minimumSize
{
  NSSize requiredSize;
  switch (mOwner->get_type())
  {
    case MainToolBar:
      if (self.subviews.count == 0)
        requiredSize = NSMakeSize(100, EMPTY_TOOLBAR_HEIGHT);
      else
        requiredSize = NSMakeSize(100, layout_info[MainToolBar].height);

    case SecondaryToolBar:
      requiredSize = NSMakeSize(100, layout_info[SecondaryToolBar].height);

    case ToolPickerToolBar:
      requiredSize = NSMakeSize(layout_info[ToolPickerToolBar].height, 100);
      
    case OptionsToolBar:
      requiredSize = NSMakeSize(100, layout_info[OptionsToolBar].height);
      
    case PaletteToolBar:
      requiredSize = NSMakeSize(100, layout_info[PaletteToolBar].height);

    default:
      break;
  }

  NSSize minSize = super.minimumSize;
  return { MAX(requiredSize.width, minSize.width), MAX(requiredSize.height, minSize.height) };
}


- (BOOL)expandsOnLayoutVertically:(BOOL)vertically
{
  if (vertically)
    return NO;
  return YES;
}

- (void)insertItem:(id)item
            atIndex:(NSInteger)index
{
  BOOL wasEmpty = NO;
  if (mOwner->get_type() == ToolPickerToolBar && [item respondsToSelector: @selector(setHorizontal:)])
    [item setHorizontal: YES];

  if ([item isKindOfClass: [MFToolBarSearchItemImpl class]])
  {
    if (mOwner->get_type() == SecondaryToolBar)
    {
      [item cell].controlSize = NSSmallControlSize;
      [item setFont: [NSFont systemFontOfSize: [NSFont smallSystemFontSize]]];
      [item setFrame: NSMakeRect(0, 0, 120, 19)];
    }
  }

  if ([item respondsToSelector: @selector(setToolbar:)])
    [item setToolbar: (id)self];
  
  if (self.subviews.count == 0)
    wasEmpty = YES;

  NSView *view = item;
  if (index >= (int)self.subviews.count)
    [self addSubview: view];
  else
    [self addSubview: view positioned: NSWindowBelow relativeTo: self.subviews[index]];
  if (wasEmpty)
  {
    // main toolbar starts with a reduced height if it's empty. so when something is added, it has to
    // be resized to the proper height
    if (mOwner->get_type() == MainToolBar)
    {
      NSRect rect = self.frame;
      rect.size.height = layout_info[MainToolBar].height;
      self.frame = rect;
    }
  }
    
  [self relayout];
}

- (void) dealloc
{
  mOwner = NULL;
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

- (BOOL) isFlipped
{
  return YES;
}

- (void)removeItem:(id)item
{
  [item removeFromSuperview];
  [self resizeSubviewsWithOldSize: NSZeroSize];
}


- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize
{
  NSRect rect = self.bounds;
  
  if (mOwner->get_type() == ToolPickerToolBar)
  {
    float spacing = 0;
    float xpadding = 1;
    float ypadding = 4;
    float x = NSMinX(rect) + xpadding;
    float y = NSMinY(rect) + ypadding;
    float w = layout_info[ToolPickerToolBar].height - 2 * ypadding;
    
    int item_count = 0;
    int expander_count = 0;
    float total_height = ypadding*2;
    
    for (id item in self.subviews)
    {
      total_height += NSHeight([item frame]);
      if ([item isKindOfClass: [MFToolBarSeparatorImpl class]] && [item itemType] == ExpanderItem)
        expander_count++;
      else
        item_count++;
    }
    total_height += spacing * item_count;
    
    for (id item in self.subviews)
    {
      NSRect r = [item frame];
      r.origin = NSMakePoint(x, y);
      r.size.width = w;
      if ([item isKindOfClass: [MFToolBarActionItemImpl class]])
        r.size.height = w;
      if ([item isKindOfClass: [MFToolBarSeparatorImpl class]] && [item itemType] == ExpanderItem)
      {
        r.size.height = (NSHeight(rect) - total_height) / expander_count;
        if (r.size.height < 0)
          r.size.height = 0;
      }
      else
        [item setFrame: r];
      
      y += NSHeight(r) + spacing;
    }
  }
  else
  {
    ToolBarType type = mOwner->get_type();
    float spacing = layout_info[type].spacing;
    float xpadding = layout_info[type].xpadding;
    float ypadding = layout_info[type].ypadding;
    float x = NSMinX(rect) + xpadding;
    float y = NSMinY(rect) + ypadding;    
    float h = layout_info[type].height - 2 * ypadding;
    
    int spaced_item_count = 0;
    int item_count = 0;
    int expander_count = 0;
    float total_width = xpadding*2;
    
    for (id item in self.subviews)
    {
      if ([item isKindOfClass: [MFToolBarSeparatorImpl class]] && [item itemType] == ExpanderItem)
        expander_count++;
      else
      {
        mforms::ToolBarItem *titem = [item toolBarItem];
        if (titem && titem->get_expandable())
          expander_count++;
        else
          total_width += NSWidth([item frame]);
        spaced_item_count++;
        item_count++;
      }
    }
    total_width += spacing * spaced_item_count;
    
    for (NSView *item in self.subviews)
    {
      NSRect r = [item frame];
      r.origin = NSMakePoint(x, y);
      mforms::ToolBarItem *titem = [(id)item toolBarItem];
      if (titem && titem->get_expandable())
        r.size.width = (NSWidth(rect) - total_width) / expander_count;
      if ([item isKindOfClass: [MFToolBarSeparatorImpl class]] && [(id)item itemType] == ExpanderItem)
      {
        r.size.height = h;
        r.size.width = (NSWidth(rect) - total_width) / expander_count;
        if (r.size.width < 0)
          r.size.width = 0;
      }
      else if ([item isKindOfClass: [MFToolBarImageItemImpl class]])
      {
        r.origin.y = NSMinY(rect) + (NSHeight(rect) - r.size.height)/2;
        [item setFrame: r];
      }
      else if ([item isKindOfClass: [MFToolBarLabelItemImpl class]])
      {
        r.origin.y = (NSHeight(rect) - r.size.height)/2;
        [item setFrame: r];
      }
      else if ([item isKindOfClass: [MFToolBarSearchItemImpl class]]
               && mOwner->get_type() == SecondaryToolBar)
      {
        r.origin.y = (NSHeight(rect) - r.size.height)/2;
        [item setFrame: r];
      }
      else
      {
        r.origin.y = (NSHeight(rect) - r.size.height)/2;
        r.size.height = h;

        r.size = item.frame.size;
        [item setFrame: r];
      }

      x += NSWidth(r) + spacing;
    }
  }
}

- (void)windowKeyChanged:(NSNotification*)notif
{
  [self setNeedsDisplay: YES];
}


- (void)drawRect:(NSRect)rect
{
  NSGradient *grad;
  rect = self.bounds;

  switch (mOwner->get_type())
  {
    case ToolPickerToolBar:
    case PaletteToolBar:
    case SecondaryToolBar:
      [self.backgroundColor set];
      NSRectFill(rect);
      [[NSColor colorWithDeviceWhite: 202/255.0 alpha: 1.0] set];
      [NSBezierPath setDefaultLineWidth: 0.0];
      [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(rect)-0.5) toPoint: NSMakePoint(NSWidth(rect), NSHeight(rect)-0.5)];
      break;

    default:
      if (mOwner->get_type() == OptionsToolBar)
      {
        grad = [[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 243/255.0 alpha:1.0] 
                                             endingColor: [NSColor colorWithDeviceWhite: 230/255.0 alpha:1.0]];
      }
      else
      {
        if (self.window.mainWindow)
          grad = [[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 206/255.0 alpha:1.0] 
                                               endingColor: [NSColor colorWithDeviceWhite: 188/255.0 alpha:1.0]];
        else
          grad = [[NSGradient alloc] initWithStartingColor: [NSColor colorWithDeviceWhite: 233/255.0 alpha:1.0] 
                                               endingColor: [NSColor colorWithDeviceWhite: 216/255.0 alpha:1.0]];
      }
      [grad drawInRect: rect angle: 90.0];
      
      [[NSColor colorWithDeviceWhite: 202/255.0 alpha: 1.0] set];
      [NSBezierPath setDefaultLineWidth: 0.0];
      if (mOwner->get_type() != OptionsToolBar && self.window.mainWindow)
      {
        [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(rect)-1.5) toPoint: NSMakePoint(NSWidth(rect), NSHeight(rect)-1.5)];
        [[NSColor colorWithDeviceWhite: 117/255.0 alpha: 1.0] set];
      }
      [NSBezierPath strokeLineFromPoint: NSMakePoint(0, NSHeight(rect)-0.5) toPoint: NSMakePoint(NSWidth(rect), NSHeight(rect)-0.5)];
      break;
  }
}

- (NSColor*)backgroundColor
{
  return [NSColor colorWithDeviceWhite: 232/255.0 alpha:1.0];
}

- (mforms::ToolBarType)type
{
  return mOwner->get_type();
}

- (BOOL)mouseDownCanMoveWindow
{
  return NO;
}


- (void)destroy
{
  [self removeFromSuperview];
}

@end


//--------------------------------------------------------------------------------------------------


static bool create_tool_bar(ToolBar *tb, ToolBarType type)
{
  return [[MFToolBarImpl alloc] initWithObject: tb type: type] != nil;
}

static void insert_item(ToolBar *toolbar, int index, ToolBarItem *item)
{
  [toolbar->get_data() insertItem: item->get_data() atIndex: index];
}

static void remove_item(ToolBar *toolbar, ToolBarItem *item)
{
  [toolbar->get_data() removeItem: item->get_data()];
}

static bool create_tool_item(ToolBarItem *item, ToolBarItemType type)
{
  switch (type)
  {
    case ActionItem:
    case ToggleItem:
    case TextActionItem:
    case SegmentedToggleItem:
    case SwitcherItem:
      return [[MFToolBarActionItemImpl alloc] initWithItemObject: item] != nil;

    case SeparatorItem:
    case ExpanderItem:
      return [[MFToolBarSeparatorImpl alloc] initWithItemObject: item] != nil;

    case SearchFieldItem:
      return [[MFToolBarSearchItemImpl alloc] initWithItemObject: item] != nil;

    case FlatSelectorItem:
    case SelectorItem:
      return [[MFToolBarSelectorItemImpl alloc] initWithItemObject: item] != nil;

    case ColorSelectorItem:
      return [[MFToolBarSelectorItemImpl alloc] initWithItemObject: item] != nil;

    case LabelItem:
    case TitleItem:
      return [[MFToolBarLabelItemImpl alloc] initWithItemObject: item] != nil;

    case ImageBoxItem:
      return [[MFToolBarImageItemImpl alloc] initWithItemObject: item] != nil;

    case TextEntryItem:
      return [[MFToolBarTextItemImpl alloc] initWithItemObject: item] != nil;

    default:
      return false;
  }

}

static void set_item_icon(ToolBarItem *item, const std::string &image)
{
  id tbitem = item->get_data();
  NSImage *i = [[NSImage alloc] initWithContentsOfFile: wrap_nsstring(image)];
  if (!i || !i.valid)
    NSLog(@"invalid icon for toolbar %s", image.c_str());
  else
    [tbitem setImage: i];
}

static void set_item_alt_icon(ToolBarItem *item, const std::string &image)
{
  id tbitem = item->get_data();
  NSImage *i = [[NSImage alloc] initWithContentsOfFile: wrap_nsstring(image)];
  if (!i || !i.valid)
    NSLog(@"invalid icon for toolbar %s", image.c_str());
  else
    [tbitem setAlternateImage: i];
}

static void set_item_text(ToolBarItem *item, const std::string &text)
{
  id tbitem = item->get_data();
  [tbitem setStringValue: wrap_nsstring(text)];
}

static std::string get_item_text(ToolBarItem *item)
{
  id tbitem = item->get_data();
  return [tbitem stringValue].UTF8String ?: "";
}

static void set_item_name(ToolBarItem *item, const std::string &name)
{
  // This is dummy function to silent warnings
}

static void set_item_enabled(ToolBarItem *item, bool flag)
{
  id tbitem = item->get_data();
  [tbitem setEnabled: flag];
}

static bool get_item_enabled(ToolBarItem *item)
{
  id tbitem = item->get_data();
  return [tbitem isEnabled];
}

static void set_item_checked(ToolBarItem *item, bool flag)
{
  id tbitem = item->get_data();
  if ([tbitem state] != (flag ? NSOnState : NSOffState))
    [tbitem setState: flag ? NSOnState : NSOffState];
}

static bool get_item_checked(ToolBarItem *item)
{
  id tbitem = item->get_data();
  return [tbitem state] == NSOnState;
}

static void set_item_tooltip(ToolBarItem *item, const std::string &text)
{
  id tbitem = item->get_data();
  [tbitem setToolTip: wrap_nsstring(text)];
}


static void set_selector_items(ToolBarItem *item, const std::vector<std::string> &items)
{
  id tbitem = item->get_data();
  if ([tbitem isKindOfClass: [MFToolBarSelectorItemImpl class]])
  {
    if (item->get_type() == ColorSelectorItem)
    {   
      NSMenu *menu= [[NSMenu alloc] initWithTitle: @""];
      [tbitem cell].controlSize = NSSmallControlSize;
      for (std::vector<std::string>::const_iterator color= items.begin(); color != items.end(); ++color)
      {
        [menu addItem: [[MFColorMenuItem alloc] initWithColorName: wrap_nsstring(*color)]];
      }
      [tbitem setMenu: menu];
      [tbitem sizeToFit];
    }
    else
    {
      NSMutableArray *array = [NSMutableArray arrayWithCapacity: items.size()];
      for (std::vector<std::string>::const_iterator iter = items.begin();
           iter != items.end(); ++iter)
        [array addObject: wrap_nsstring(*iter)];
      [tbitem removeAllItems];
      [tbitem addItemsWithTitles: array];
      [tbitem sizeToFit];
    }
  }
}


void cf_toolbar_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  
  f->_tool_bar_impl.create_tool_bar = create_tool_bar;
  f->_tool_bar_impl.insert_item = insert_item;
  f->_tool_bar_impl.remove_item = remove_item;
  
  f->_tool_bar_impl.create_tool_item = create_tool_item;
  f->_tool_bar_impl.set_item_icon = set_item_icon;
  f->_tool_bar_impl.set_item_alt_icon = set_item_alt_icon;
  f->_tool_bar_impl.set_item_text = set_item_text;
  f->_tool_bar_impl.get_item_text = get_item_text;
  f->_tool_bar_impl.set_item_name = set_item_name;
  f->_tool_bar_impl.set_item_enabled = set_item_enabled;
  f->_tool_bar_impl.get_item_enabled = get_item_enabled;
  f->_tool_bar_impl.set_item_checked = set_item_checked;
  f->_tool_bar_impl.get_item_checked = get_item_checked;
  f->_tool_bar_impl.set_item_tooltip = set_item_tooltip;
  f->_tool_bar_impl.set_selector_items = set_selector_items;
}

