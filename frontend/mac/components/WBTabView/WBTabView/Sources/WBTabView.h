
/*!
 Copyright 2009 Sun Microsystems, Inc.
 */



#import "WBTabItem.h"



@class ResponderLayer;
@class WBRightClickThroughView;



@interface WBTabView : NSTabView <WBTabItemDelegateProtocol>
{
  NSTabView* mTabView;
  ResponderLayer* mTabRowLayer;
  WBRightClickThroughView* mTabRowView;
  NSMutableArray* mTabItems;
  WBTabItem* mSelectedTab;
  ResponderLayer* mMouseDownLayer;
  BOOL mEnabled;
  NSString* mLabel;
  WBTabSize mTabSize;
  WBTabDirection mTabDirection;
  WBTabPlacement mTabPlacement;
  
  WBTabArrow* mLeftArrow;
  NSImage* mLeftArrowIconImage;
  WBTabArrow* mRightArrow;
  NSImage* mRightArrowIconImage;
  WBTabMenuLayer* mTabMenu;
  NSImage* mTabMenuIconImage;
  WBTabDraggerLayer* mDragger;
  NSImage* mDraggerIconImage;
  
  CGColorRef mColorActiveSelected;
  CGColorRef mColorActiveNotSelected;
  CGColorRef mColorNotActiveSelected;
  CGColorRef mColorNotActiveNotSelected;
  
  BOOL mDoneCustomizing;
  BOOL mEnablAnimations;
  //	BOOL mAllowsTabReordering;
  NSInteger mFirstVisibleTabIndex;
  NSInteger mLastVisibleTabIndex;
  NSInteger mLastSelectedTabIndex;
  NSInteger mTabScrollOffset;
}

- (CGFloat) tabAreaHeight;
//- (void) setAllowsTabReordering: (BOOL) yn;
- (NSSize) contentSize;
- (CALayer*) shadowLayer;
- (void) doCustomize;
- (void) updateLabelForTabViewItem: (id) identifier;

- (void)createDragger;

- (void)setIcon:(NSImage*)icon forTabViewItem:(id)identifier;
- (void) setColorActiveSelected: (CGColorRef) colorActiveSelected
         colorActiveNotSelected: (CGColorRef) colorActiveNotSelected
         colorNotActiveSelected: (CGColorRef) colorNotActiveSelected
      colorNotActiveNotSelected: (CGColorRef) colorNotActiveNotSelected;

@end



@protocol WBTabViewDelegateProtocol

- (BOOL) tabView: (NSTabView*) tabView 
willCloseTabViewItem: (NSTabViewItem*) tabViewItem;

- (void) tabView: (NSTabView*) tabView 
draggedHandleAtOffset: (NSPoint) offset;

- (void) tabViewDraggerClicked: (NSTabView*) tabView;

@end

