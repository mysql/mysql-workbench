
/*!
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
 
 @class
 WBPaletteTabView
 
 @abstract
 Implements the custom tabs. Small tabs at the top of the tab view, pointing upwards.
 
 @ingroup
 Custom Tab Views
 */



#import "WBPaletteTabView.h"
#import "WBPaletteTabItem.h"



@implementation WBPaletteTabView



- (CALayer*) shadowLayer;
{
  return nil;
}



- (CALayer*) plaqueLayer;
{
  // Create a gradient plaque in the tab row area, behind the tabs.
  CALayer* plaqueLayer = [CALayer layer];
  
  CGRect r = [mTabRowLayer bounds];
  r.size.height = [self tabAreaHeight] - 1;
  [plaqueLayer setFrame: r];
  [plaqueLayer setAutoresizingMask: (kCALayerWidthSizable | kCALayerMaxYMargin)];
  [plaqueLayer setZPosition: -4];
  
  NSBundle* b = [NSBundle bundleForClass: [self class]];
  NSString* path = [b pathForResource: @"TabRowGradient"
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
	WBTabItem* item = [WBPaletteTabItem tabItemWithIdentifier: identifier
                                                      label: label];
	
	return item;
}



- (void) doCustomize;
{
	mTabPlacement = WBTabPlacementTop;
	mTabDirection = WBTabDirectionUp;
	mTabSize = WBTabSizeSmall;
	
	[super doCustomize];
  
  NSRect r = [mTabView frame];
  r = NSInsetRect(r, 3, 3);
  [mTabView setFrame: r];
}



- (void) dealloc
{
  [mPlaqueImage release];
  
  [super dealloc];
}

@end


