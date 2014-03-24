
/*!
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
 
 @class
 WBGrayTabView
 
 @abstract
 Implements the custom tabs. Tabs at the top of the tab view, pointing upwards.
 
 @ingroup
 Custom Tab Views
 */



#import "WBGrayTabView.h"
#import "WBGrayTabItem.h"
#import "CGColorUtilities.h"



@implementation WBGrayTabView

- (void) dealloc
{
  [mPlaqueImage release];
  [super dealloc];
}


- (void) drawRect: (NSRect) rect;
{
  [super drawRect: rect];
  
  // fills area inside a tab
  [[NSColor colorWithCalibratedRed: 0.93
                             green: 0.93
                              blue: 0.93
                             alpha: 1] set];
  [NSBezierPath fillRect: rect];
}


- (NSRect)contentRect
{
  NSRect rect= [super contentRect];
  
  rect.size.height= NSHeight([self frame]) - [self tabAreaHeight];
  
  return rect;
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


- (CALayer*) plaqueLayer;
{
  // Create a gradient plaque in the tab row area, behind the tabs.
  CALayer* plaqueLayer = [CALayer layer];
  
  CGRect r = [mTabRowLayer bounds];
  r.origin.y -= 1;
  r.size.height = [self tabAreaHeight] + 1;
  [plaqueLayer setFrame: r];
  [plaqueLayer setAutoresizingMask: (kCALayerWidthSizable | kCALayerMaxYMargin)];
  [plaqueLayer setZPosition: -4];
  
  NSBundle* b = [NSBundle bundleForClass: [self class]];
  NSString* path = [b pathForResource: @"tab_header_background2"
                               ofType: @"png"];
  mPlaqueImage = [[NSImage alloc] initWithContentsOfFile: path];
  NSImageRep* rep = [[mPlaqueImage representations] objectAtIndex: 0];
  CGImageRef img = [(id)rep CGImage];
  [plaqueLayer setContents: (id)img];
  
  return plaqueLayer;
}


- (WBTabItem*) tabItemWithIdentifier: (id) identifier
							   label: (NSString*) label;
{
	WBTabItem* item = [WBGrayTabItem tabItemWithIdentifier: identifier
                                                   label: label];
	
	
	return item;
}



- (void) doCustomize;
{
  mTabPlacement = WBTabPlacementTop;
  mTabDirection = WBTabDirectionUp;
  mTabSize = WBTabSizeLarge;

  
  [super doCustomize];
  
  NSRect r = [mTabView frame];
  r.size.height -= 5;
  [mTabView setFrame: r];
}

@end


@implementation WBPaddedGrayTabView

- (void) doCustomize
{
  [super doCustomize];
  NSRect r = [mTabView frame];
  [mTabView setFrame: r];
}

@end


@implementation WBUnpaddedGrayTabView

- (void) doCustomize
{
  [super doCustomize];
  NSRect r = [mTabView frame];
  r = NSInsetRect(r, 5, 0);
  r.size.height += 5;
  [mTabView setFrame: r];
}

@end

