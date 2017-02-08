//
//  ResponderLayer.h
//
//  Created by Jacob Engstrand on 2008-08-12.
//  This source code is public domain. No rights reserved.
//

/**

 @class
ResponderLayer

 @abstract
 WBTabView implements a customized look for tab views, while trying to be semantically compatible with NSTabView.
 WBTabView is the base class of several customized classes each implementing a different look.

 @discussion
 An WBTabView inherits from NSTabView. The method -doCustomize does the following:
 1. Adds a tabless NSTabView as subview inside self, and moves any existing tab view item to it.
 2. Creates a view mTabRowView in which it draws its customized tabs.
 */

#import <QuartzCore/QuartzCore.h>

@interface ResponderLayer : CALayer {
  CGPoint mMouseDownPoint;
}

- (ResponderLayer*)mouseDownAtPoint:(CGPoint)mouse;
- (void)mouseDraggedToPoint:(CGPoint)mouse;
- (void)mouseUp;
- (ResponderLayer*)responderLayerAtPoint:(CGPoint)mouse;

@end
