/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#import "WBTabItem.h"
#import "CGColorUtilities.h"

#import "WBTabView.h"

#define TAB_ITEM_WIDTH (150)
#define TAB_ITEM_SMALL_WIDTH (70)

// Defined in MTabSwitcher.mm.
extern NSDictionary<NSString *, NSColor *> *activeColorsDark;
extern NSDictionary<NSString *, NSColor *> *inactiveColorsDark;
extern NSDictionary<NSString *, NSColor *> *activeColorsLight;
extern NSDictionary<NSString *, NSColor *> *inactiveColorsLight;

extern NSDictionary<NSString *, NSGradient *> *activeGradientsDark;
extern NSDictionary<NSString *, NSGradient *> *inactiveGradientsDark;
extern NSDictionary<NSString *, NSGradient *> *activeGradientsLight;
extern NSDictionary<NSString *, NSGradient *> *inactiveGradientsLight;

//----------------------------------------------------------------------------------------------------------------------

@implementation WBTabMenuLayer

//----------------------------------------------------------------------------------------------------------------------

- (BOOL) acceptsMouseDownAtPoint: (CGPoint) mouse {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (ResponderLayer*)mouseDownAtPoint: (CGPoint)mouse {
  SEL selector = NSSelectorFromString(@"tabViewMenuAction:");
  if ([self.delegate respondsToSelector: selector])
    ((void (*)(id, SEL, id))[(id)self.delegate methodForSelector: selector])(self.delegate, selector, self);

  return self;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBTabArrow

- (BOOL)acceptsMouseDownAtPoint: (CGPoint)mouse {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (ResponderLayer*)mouseDownAtPoint: (CGPoint)mouse {
  SEL selector = NSSelectorFromString(@"tabViewArrowAction:");
  if ([self.delegate respondsToSelector: selector])
    ((void (*)(id, SEL, id))[(id)self.delegate methodForSelector: selector])(self.delegate, selector, self);

  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setEnabled: (BOOL)yn {
  CALayer* iconLayer = self.sublayers[0];
  iconLayer.opacity = (yn ? 1 : 0.7);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setFrame: (CGRect)r {
  super.frame = r;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBCustomTabItemView

//----------------------------------------------------------------------------------------------------------------------

- (void) drawRect: (NSRect) rect {
  if (mBackgroundColor == nil) {
    mBackgroundColor = [NSColor colorWithCalibratedRed: 0.88
                                                 green: 0.88
                                                  blue: 0.88
                                                 alpha: 1];
  }
  
  [super drawRect: rect];
  
  if (mBackgroundColor != nil) {
    [mBackgroundColor set];
    [NSBezierPath fillRect: rect];
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setBackgroundColor: (NSColor*)backgroundColor {
  mBackgroundColor = backgroundColor;
  [self setNeedsDisplay: YES];
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation WBTabItem

- (NSDictionary<NSString *, NSColor *> *)currentColors {
  BOOL isActive = owner.window.mainWindow && mEnabled;
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = owner.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  return isActive
    ? (isDark ? activeColorsDark : activeColorsLight)
    : (isDark ? inactiveColorsDark : inactiveColorsLight);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)updateAppearance {
  mIcon.opacity = (mEnabled ? 1.0 : 0.6);
  mCloseButton.opacity = (mEnabled ? 1.0 : 0.6);
  mTitleLayer.opacity = (mEnabled ? 1.0 : 0.6);
  
  if (mState == NSControlStateValueOn) {
    self.backgroundColor = self.currentColors[@"tabBackgroundSelected"].CGColor;
    mTitleLayer.foregroundColor = self.currentColors[@"tabLabelSelected"].CGColor;
  } else {
    self.backgroundColor = self.currentColors[@"tabViewBackground"].CGColor;
    mTitleLayer.foregroundColor = self.currentColors[@"tabLabelUnselected"].CGColor;
  }

  [self setCloseButtonState: NSControlStateValueOff];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setState: (NSControlStateValue)value {
  NSAssert( (value == NSControlStateValueOff || value == NSControlStateValueOn), @"Bad argument for setState, should be NSControlStateValueOff or NSControlStateValueOn.");
  
  mState = value;
  [self updateAppearance];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setEnabled: (BOOL)enabled {
  mEnabled = enabled;
  [self updateAppearance];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setLabel: (NSString*)label {
  mLabel = label;
  mTitleLayer.string = label;

  if (mTabSize == WBTabSizeLarge) {
    CGRect titleFrame = mTitleLayer.frame;
    titleFrame.size.width = self.preferredWidth - 30;
    mTitleLayer.frame = titleFrame;
    
    CGRect frame = self.frame;
    frame.size.width = self.preferredWidth;
    self.frame = frame;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setCloseButtonState: (NSControlStateValue)state {
  BOOL isDark = NO;
  if (@available(macOS 10.14, *)) {
    isDark = owner.window.effectiveAppearance.name == NSAppearanceNameDarkAqua;
  }

  NSBundle * bundle = [NSBundle bundleForClass: self.class];
  if (state == NSControlStateValueOn) {
    mCloseButtonImage = [bundle imageForResource: isDark ? @"TabClose_PressedDark" : @"TabClose_PressedLight"];
    mCloseButton.contents = mCloseButtonImage;
  } else if (state == NSControlStateValueOff) {
    mCloseButtonImage = [bundle imageForResource: isDark ? @"TabClose_UnpressedDark" : @"TabClose_UnpressedLight"];
    mCloseButton.contents = mCloseButtonImage;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setDelegate: (id)delegate {
  NSAssert( [delegate conformsToProtocol: @protocol(WBTabItemDelegateProtocol)], @"Delegate must conform to TabItemDelegateProtocol.");
  
  super.delegate = delegate;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)setIconImage: (NSImage*)image {
  mDocumentIconImage = image;
  
  if (image == nil) {
    [mIcon removeFromSuperlayer];
    mIcon = nil;
  
    CGRect titleFrame = mTitleLayer.frame;
    titleFrame.origin.x = 8;
    mTitleLayer.frame = titleFrame;
    [self setLabel: mLabel];
  } else {
    if (!mIcon) {
      mIcon = [CALayer layer];
      mIcon.contentsScale = owner.window.backingScaleFactor;
      CGRect rect = mIcon.frame;
      mIcon.contents = mDocumentIconImage;
      rect.size = NSSizeToCGSize(image.size);
      mIcon.frame = rect;
      [self addSublayer: mIcon];

      CGRect titleFrame = mTitleLayer.frame;
      titleFrame.origin.x = CGRectGetMaxX(rect) + 6;
      mTitleLayer.frame = titleFrame;      
    } else {
      CGRect rect = mIcon.frame;
      mIcon.contents = mDocumentIconImage;
      rect.size = NSSizeToCGSize(image.size);
      mIcon.frame = rect;
      
      CGRect titleFrame = mTitleLayer.frame;
      titleFrame.origin.x = CGRectGetMaxX(rect) + 6;
      mTitleLayer.frame = titleFrame;      
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (id)identifier {
  return mIdentifier;
}

//----------------------------------------------------------------------------------------------------------------------

- (CGFloat)preferredWidth {
  return 100;
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark User interaction

- (BOOL)acceptsMouseDownAtPoint: (CGPoint)mouse {
  return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (ResponderLayer*)mouseDownAtPoint: (CGPoint)mouse {
  mMouseDownPoint = mouse;
  
  if (CGRectContainsPoint(mCloseButton.frame, mouse)) {
    mClickInCloseBox = YES;
    mMouseInCloseBox = YES;
    [self setCloseButtonState: NSControlStateValueOn];
  } else {
    mClickInCloseBox = NO;
    mMouseInCloseBox = NO;
    [(id)[self delegate] selectTab: self];
  }
  
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseDraggedToPoint: (CGPoint)mouse {
  if (mClickInCloseBox) {
    // Track mouse around close box.
    BOOL inside = (CGRectContainsPoint(mCloseButton.frame, mouse));
    if (mMouseInCloseBox != inside) {
      [self setCloseButtonState: (inside ? NSControlStateValueOn : NSControlStateValueOff)];
    }
    mMouseInCloseBox = inside;
  }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseUp {
  if (mMouseInCloseBox) {
    [(id)[self delegate] closeTab: self];
    [self setCloseButtonState: NSControlStateValueOff];
  }
}

//----------------------------------------------------------------------------------------------------------------------

#pragma mark Creation & Destruction

- (void)layout {
}

//----------------------------------------------------------------------------------------------------------------------

- (instancetype)initWithOwner: (WBTabView *)owner
                   identifier: (id) identifier
                        label: (NSString*) label
                    direction: (WBTabDirection) tabDirection
                    placement: (WBTabPlacement) tabPlacement
                         size: (WBTabSize) tabSize
                      hasIcon: (BOOL) hasIcon
                     canClose: (BOOL) canClose {
  self = [super init];
  
  if (self != nil) {
    self->owner = owner;
    mIdentifier = identifier;
    
    mTabDirection = tabDirection;
    mTabPlacement = tabPlacement;
    mTabSize = tabSize;
    
    mHasIcon = hasIcon;
    mCanClose = canClose;
    
    mState = -1;
    
    CGRect frame = CGRectZero;
    if (tabSize == WBTabSizeLarge)
      frame.size = CGSizeMake(self.preferredWidth, 44);
    else
      frame.size = CGSizeMake(TAB_ITEM_SMALL_WIDTH, 32);
    self.frame = frame;
    
    CGFloat horizon = frame.size.height / 2;
    NSBundle* bundle = [NSBundle bundleForClass: self.class];

    if (mCanClose) {
      // Close button layer.
      mCloseButtonImage = [bundle imageForResource: @"TabClose_UnpressedLight"];
      
      mCloseButton = [CALayer layer];
      mCloseButton.contentsScale = owner.window.backingScaleFactor;
      CGRect r = CGRectZero;
      r.size = NSSizeToCGSize(mCloseButtonImage.size);
      r.origin.x = frame.size.width - r.size.width - 4;
      if (mTabDirection == WBTabDirectionUp)
        r.origin.y = floor(horizon + (horizon / 2) - (r.size.height / 2)) - 1;
      else 
        r.origin.y = floor((horizon / 2) - (r.size.height / 2)) - 1;
      mCloseButton.frame = r;
      mCloseButton.contents = mCloseButtonImage;
      mCloseButton.autoresizingMask = kCALayerMinXMargin;
      [self addSublayer: mCloseButton];
    }
    
    if (mHasIcon) {
      // Icon layer.
      mDocumentIconImage = [bundle imageForResource: @"TabDocument"];
      
      mIcon = [CALayer layer];
      mIcon.contentsScale = owner.window.backingScaleFactor;
      CGRect r = CGRectZero;
      r.size = NSSizeToCGSize(mDocumentIconImage.size);
      r.origin.x = 9;
      if (mTabDirection == WBTabDirectionUp)
        r.origin.y = floor(horizon + (horizon / 2) - (r.size.height / 2));
      else
        r.origin.y = floor((horizon / 2) - (r.size.height / 2));
      mIcon.frame = r;
      mIcon.contents = mDocumentIconImage;
      [self addSublayer: mIcon];
    }
    
    {
      // Title layer.
      CGRect titleFrame = CGRectZero;
      if (mHasIcon) {
        CGRect r = mIcon.frame;
        titleFrame.origin.x = CGRectGetMaxX(r) + 6;
      } else {
        titleFrame.origin.x = 5;
      }
      
      titleFrame.size.height = 15;
      
      if (mHasIcon) {
        titleFrame.origin.y = mIcon.frame.origin.y;
      } else {
        if (mTabDirection == WBTabDirectionUp) {
          titleFrame.origin.y = horizon;
        } else {
          titleFrame.origin.y = horizon - 16;
        }
      }
      
      if (tabSize == WBTabSizeLarge)
        titleFrame.size.width = self.preferredWidth - 50;
      else
        titleFrame.size.width = 100;
      
      mTitleLayer = [CATextLayer layer];
      mTitleLayer.contentsScale = owner.window.backingScaleFactor;
      mTitleLayer.frame = titleFrame;
      mTitleLayer.autoresizingMask = (kCALayerMaxXMargin | kCALayerMaxYMargin);
      
      NSFont* font = [NSFont boldSystemFontOfSize: 12];
      mTitleLayer.font = (__bridge CFTypeRef _Nullable)(font);
      if (mTabSize == WBTabSizeLarge) {
        mTitleLayer.fontSize = 12;
      } else {
        mTitleLayer.fontSize = 9;
      }
      
      [self addSublayer: mTitleLayer];
    }
    
    [self setLabel: label];
    
    frame.size.width = self.preferredWidth;
    self.frame = frame;
    
    [self setState: NSControlStateValueOff];
    [self setEnabled: YES];
  }
  
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

+ (WBTabItem*)tabItemWithOwner: (WBTabView *)owner
                    identifier: (id)identifier
                         label: (NSString*)label
                     direction: (WBTabDirection)tabDirection
                     placement: (WBTabPlacement)tabPlacement
                          size: (WBTabSize)tabSize
                       hasIcon: (BOOL)hasIcon
                      canClose: (BOOL)canClose {
  return [[WBTabItem alloc] initWithOwner: (WBTabView *)owner
                               identifier: identifier
                                    label: label
                                direction: tabDirection
                                placement: tabPlacement
                                     size: tabSize
                                  hasIcon: hasIcon
                                 canClose: canClose];
}

@end

//----------------------------------------------------------------------------------------------------------------------
