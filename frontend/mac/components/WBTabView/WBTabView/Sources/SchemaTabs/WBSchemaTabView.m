
/*!
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
 
 @class
 WBSchemaTabView
 
 @abstract
 Implements custom tabs. Big tabs at the top of the view, pointing upwards.
 
 @ingroup
 Custom Tab Views
 */



#import "WBSchemaTabView.h"
#import "WBSchemaTabItem.h"
#import "CGColorUtilities.h"



@implementation WBSchemaTabView




- (CGFloat) tabAreaHeight;
{
  return 42.0;
}



- (CGColorRef) tabRowActiveBackgroundColorCreate;
{
  return WB_CGColorCreateCalibratedRGB(0.925, 0.949, 0.973, 1);
  //	return WB_CGColorCreateCalibratedRGB(0.91, 0.91, 0.91, 1);
}



- (CGColorRef) tabRowInactiveBackgroundColorCreate;
{
  return WB_CGColorCreateCalibratedRGB(0.925, 0.949, 0.973, 1);
}



- (CALayer*) shadowLayer;
{
  // Create a line between non-selected tabs and the tabviewcontants.
  CALayer* lineLayer = [CALayer layer];
  
  [lineLayer setBorderWidth: 1];
  CGColorRef c = WB_CGColorCreateCalibratedRGB(0.68, 0.68, 0.68, 1);
  [lineLayer setBorderColor: c];
  CGColorRelease(c);
  
  CGRect r = CGRectZero;
  r.origin.x = 0;
  r.origin.y = [[self layer] frame].size.height - [self tabAreaHeight];
  r.size.width = [[self layer] frame].size.width;
  r.size.height = 1;
  [lineLayer setFrame: r];
  [lineLayer setAutoresizingMask: (kCALayerWidthSizable | kCALayerMinYMargin)];
  
  [lineLayer setZPosition: -2];
  [[self layer] addSublayer: lineLayer];
  
  return lineLayer;
}



- (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label;
{
  WBTabItem* item = [WBSchemaTabItem tabItemWithIdentifier: identifier
                                                     label: label];
  
  return item;
}



- (void) doCustomize;
{
  mTabPlacement = WBTabPlacementTop;
  mTabDirection = WBTabDirectionUp;
  mTabSize = WBTabSizeLarge;
  
  [super doCustomize];
}



@end


