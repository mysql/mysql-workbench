//
//  ResponderLayer.m
//
//  Created by Jacob Engstrand on 2008-08-12.
//  This source code is public domain. No rights reserved.
//



#import "ResponderLayer.h"
#import "WBTabItem.h"


@implementation ResponderLayer



- (BOOL) acceptsMouseDownAtPoint: (CGPoint) mouse;
{
  return NO;
}



- (ResponderLayer*) mouseDownAtPoint: (CGPoint) mouse;
{
	ResponderLayer* mouseHandledByLayer = nil;
	
	mMouseDownPoint = mouse;
  
	// Send mouse down message to all sublayers.
	NSArray* sublayers = self.sublayers;
	for (ResponderLayer* lay in sublayers) {
		if ([lay isKindOfClass: [ResponderLayer class]]) {
			if (CGRectContainsPoint(lay.frame, mouse)) {
				CGPoint localPoint = [self convertPoint: mouse
																				toLayer: lay];
        BOOL acceptsMouse = [(ResponderLayer*)lay acceptsMouseDownAtPoint: localPoint];
        if (acceptsMouse) {
          // If several layers in the hierarchy accepts the mouse, then pick the frontmost
          // layer (the one with the highes z-coordinate).
          if (mouseHandledByLayer == nil) {
            // This is the first layer in the layer hierarchy that will accept the mouse.
            mouseHandledByLayer = lay;
          }
          else {
            if (lay.zPosition > mouseHandledByLayer.zPosition) {
              mouseHandledByLayer = lay;
            }
          }
        }
			}
		}		
	}
	
  if (mouseHandledByLayer != nil) {
    CGPoint localPoint = [self convertPoint: mouse
                                    toLayer: mouseHandledByLayer];
    mouseHandledByLayer = [mouseHandledByLayer mouseDownAtPoint: localPoint];
//    NSLog(@"mouseHandledByLayer z: %f", [mouseHandledByLayer zPosition]);
  }
  
	return mouseHandledByLayer;
}



- (void) mouseDraggedToPoint: (CGPoint) mouse;
{
  id delegate = self.delegate;
  if ([delegate respondsToSelector: @selector(mouseDraggedToPoint:)]) {
    [delegate mouseDraggedToPoint: mouse];
	}
}



- (void) mouseUp;
{
  id delegate = self.delegate;
	if ([delegate respondsToSelector: @selector(mouseUp)]) {
		[delegate mouseUp];
	}
}



- (ResponderLayer*) responderLayerAtPoint: (CGPoint) mouse;
{
	ResponderLayer* layerAtPoint = nil;
	
	// Send mouse down message to all sublayers.
	NSArray* sublayers = self.sublayers;
	for (CALayer* lay in sublayers) {
		if ([lay respondsToSelector: @selector(responderLayerAtPoint:)]) {
			if (CGRectContainsPoint(lay.frame, mouse)) {
				CGPoint localPoint = [self convertPoint: mouse
																				toLayer: lay];
				layerAtPoint = [(ResponderLayer*)lay responderLayerAtPoint: localPoint];
			}
		}
		
		if (layerAtPoint == nil) {
      // We return self since we are the last sublayer that responds to responderLayerAtPoint:.
			layerAtPoint = self;
		}
	}
	
	return layerAtPoint;
}



@end


