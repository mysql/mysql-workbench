/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#import "ResponderLayer.h"

typedef NS_ENUM(NSInteger, WBTabDirection) {
  WBTabDirectionUndefined = 0,
  WBTabDirectionUp = 1,
  WBTabDirectionDown = 2
};

typedef NS_ENUM(NSInteger, WBTabPlacement) {
  WBTabPlacementUndefined = 0,
  WBTabPlacementTop = 1,
  WBTabPlacementBottom = 2
};

typedef NS_ENUM(NSInteger, WBTabSize) { WBTabSizeUndefined = 0, WBTabSizeSmall = 1, WBTabSizeLarge = 2 };

@class WBTabItem;
@protocol WBTabItemDelegateProtocol
- (void)selectTab:(WBTabItem*)sender;
- (void)closeTab:(WBTabItem*)sender;
- (CGRect)tabItem:(WBTabItem*)sender draggedToFrame:(CGRect)r;
@end

@interface WBTabMenuLayer : ResponderLayer {
}
@end

@interface WBTabDraggerLayer : ResponderLayer {
  CGPoint mLocation;
  BOOL mDragged;
}

- (void)mouseDraggedToPoint:(CGPoint)mouse;
- (void)mouseUp;

@end

@interface WBTabArrow : ResponderLayer {
  BOOL mEnabled;
}

- (void)setEnabled:(BOOL)yn;

@end

@interface WBCustomTabItemView : NSView {
  NSColor* mBackgroundColor;
}

- (void)setBackgroundColor:(NSColor*)backgroundColor;

@end

@interface WBTabItem : ResponderLayer {
  id mIdentifier;
  NSCellStateValue mState;
  BOOL mEnabled;
  WBTabSize mTabSize;
  WBTabPlacement mTabPlacement;
  WBTabDirection mTabDirection;

  // Closing the tab.
  BOOL mCanClose;
  CALayer* mCloseButton;
  BOOL mClickInCloseBox;
  BOOL mMouseInCloseBox;
  NSImage* mCloseButtonImage;

  // Tab icon.
  BOOL mHasIcon;
  CALayer* mIcon;
  NSImage* mDocumentIconImage;

  // Label
  NSString* mLabel;
  CATextLayer* mTitleLayer;

  // Colors.
  CGColorRef mColorActiveSelected;
  CGColorRef mColorActiveNotSelected;
  CGColorRef mColorNotActiveSelected;
  CGColorRef mColorNotActiveNotSelected;

  NSGradient* mGradient;
}

- (void)mouseDraggedToPoint:(CGPoint)mouse;
- (void)mouseUp;

- (void)updateAppearance;
- (void)setState:(NSCellStateValue)value;
- (void)setEnabled:(BOOL)enabled;
- (void)setLabel:(NSString*)newLabel;
- (void)setIconImage:(NSImage*)image;
@property(readonly, strong) id identifier;
- (void)setColorActiveSelected:(CGColorRef)colorActiveSelected
        colorActiveNotSelected:(CGColorRef)colorActiveNotSelected
        colorNotActiveSelected:(CGColorRef)colorNotActiveSelected
     colorNotActiveNotSelected:(CGColorRef)colorNotActiveNotSelected;

- (instancetype)initWithIdentifier:(id)identifier
                             label:(NSString*)label
                         direction:(WBTabDirection)tabDirection
                         placement:(WBTabPlacement)tabPlacement
                              size:(WBTabSize)tabSize
                           hasIcon:(BOOL)hasIcon
                          canClose:(BOOL)canClose NS_DESIGNATED_INITIALIZER;

+ (WBTabItem*)tabItemWithIdentifier:(id)identifier
                              label:(NSString*)label
                          direction:(WBTabDirection)tabDirection
                          placement:(WBTabPlacement)tabPlacement
                               size:(WBTabSize)tabSize
                            hasIcon:(BOOL)hasIcon
                           canClose:(BOOL)canClose;

@property(readonly) CGFloat preferredWidth;

@end
