/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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


