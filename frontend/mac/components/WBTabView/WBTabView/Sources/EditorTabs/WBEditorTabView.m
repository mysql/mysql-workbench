
/*!
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
 
 @class
 WBEditorTabView
 
 @abstract
 Implements the custom tabs used in editors. Small tabs at the bottom of the tab view, pointing downwards.
 
 @ingroup
 Custom Tab Views
 */



#import "WBEditorTabView.h"
#import "WBEditorTabItem.h"
#import "CGColorUtilities.h"



@implementation WBEditorTabView



- (void) drawRect: (NSRect) frame;
{
  [super drawRect: frame];
  
  NSRect r = [self bounds];
  r.origin.y += [self tabAreaHeight];
  
  double gray = (214.0 / 255.0);
  NSColor* c = [NSColor colorWithCalibratedRed: gray
                                         green: gray
                                          blue: gray
                                         alpha: 1];

  [c set];
  [NSBezierPath fillRect: r];
  
  gray = (130.0 / 255.0);
  c = [NSColor colorWithCalibratedRed: gray
                                green: gray
                                 blue: gray
                                alpha: 1];
  [c set];
  [NSBezierPath strokeLineFromPoint: r.origin
                            toPoint: NSMakePoint(r.origin.x, r.size.height - 2)];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(r.origin.x, r.size.height - 2)
                            toPoint: NSMakePoint(r.origin.x + 2, r.size.height)];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(r.origin.x + 2, r.size.height)
                            toPoint: NSMakePoint(r.size.width - 2, r.size.height)];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(r.size.width - 2, r.size.height)
                            toPoint: NSMakePoint(r.size.width, r.size.height - 2)];
  [NSBezierPath strokeLineFromPoint: NSMakePoint(r.size.width, r.size.height - 2)
                            toPoint: NSMakePoint(r.size.width, r.origin.y)];
}



- (float) contentPadding;
{
  return 2;
}



- (CGColorRef) tabRowActiveBackgroundColorCreate;
{
//	return WB_CGColorCreateCalibratedRGB(1, 0.2, 0.2, 1);
	return WB_CGColorCreateCalibratedRGB(0.93, 0.93, 0.93, 1);
}



- (CGColorRef) tabRowInactiveBackgroundColorCreate;
{
	return nil;
}



- (CALayer*) shadowLayer;
{
  return nil;
}



- (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label;
{
	WBTabItem* item = [WBEditorTabItem tabItemWithIdentifier: identifier
                                                        label: label];
	
	return item;
}


- (void) doCustomize;
{
	mTabPlacement = WBTabPlacementBottom;
	mTabDirection = WBTabDirectionDown;
	mTabSize = WBTabSizeLarge;
	
	[super doCustomize];

	CGColorRef c = WB_CGColorCreateCalibratedRGB(0.93, 0.93, 0.93, 1);
	[mTabRowLayer setBackgroundColor: c];
	CGColorRelease(c);
}



@end


