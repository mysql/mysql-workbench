
/*!
 Copyright 2009 Sun Microsystems, Inc.
 */



#import "ResponderLayer.h"


typedef enum {
  WBTabDirectionUndefined = 0,
  WBTabDirectionUp = 1,
  WBTabDirectionDown = 2
} WBTabDirection;


typedef enum {
  WBTabPlacementUndefined = 0,
  WBTabPlacementTop = 1,
  WBTabPlacementBottom = 2
} WBTabPlacement;


typedef enum {
  WBTabSizeUndefined = 0,
  WBTabSizeSmall = 1,
  WBTabSizeLarge = 2
} WBTabSize;



@class WBTabItem;
@protocol WBTabItemDelegateProtocol
- (void) selectTab: (WBTabItem*) sender;
- (void) closeTab: (WBTabItem*) sender;
- (CGRect) tabItem: (WBTabItem*) sender
    draggedToFrame: (CGRect) r;
@end


@interface WBTabMenuLayer : ResponderLayer
{
}
@end


@interface WBTabDraggerLayer : ResponderLayer
{
  CGPoint mLocation;
  BOOL mDragged;
}
@end


@interface WBTabArrow : ResponderLayer
{
  BOOL mEnabled;
}

- (void) setEnabled: (BOOL) yn;

@end






@interface WBCustomTabItemView : NSView
{
  NSColor* mBackgroundColor;
}

- (void) setBackgroundColor: (NSColor*) backgroundColor;

@end



@interface WBTabItem : ResponderLayer
{
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
 
  NSGradient *mGradient;
}



- (void) updateAppearance;
- (void) setState: (NSCellStateValue) value;
- (void) setEnabled: (BOOL) enabled;
- (void) setLabel: (NSString*) newLabel;
- (void) setIconImage: (NSImage*) image;
- (id) identifier;
- (void) setColorActiveSelected: (CGColorRef) colorActiveSelected
         colorActiveNotSelected: (CGColorRef) colorActiveNotSelected
         colorNotActiveSelected: (CGColorRef) colorNotActiveSelected
      colorNotActiveNotSelected: (CGColorRef) colorNotActiveNotSelected;

- (id) initWithIdentifier: (id) identifier
                    label: (NSString*) label
                direction: (WBTabDirection) tabDirection
                placement: (WBTabPlacement) tabPlacement
                     size: (WBTabSize) tabSize
                  hasIcon: (BOOL) hasIcon
                 canClose: (BOOL) canClose;

+ (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label
                           direction: (WBTabDirection) tabDirection
                           placement: (WBTabPlacement) tabPlacement
                                size: (WBTabSize) tabSize
                             hasIcon: (BOOL) hasIcon
                            canClose: (BOOL) canClose;


- (CGFloat) preferredWidth;

@end

