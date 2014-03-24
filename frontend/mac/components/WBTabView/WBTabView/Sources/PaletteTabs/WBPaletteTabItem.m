
/*!
 
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
 
 @class
 WBPaletteTabItem
 
 @abstract
 Implements the custom tabs. Small tabs at the top of the tab view, pointing upwards.
 
 @ingroup
 Custom Tab Views
 
 */



#import "WBPaletteTabItem.h"
#import "CGColorUtilities.h"



@implementation WBPaletteTabItem



- (void) updateAppearance;
{
	[super updateAppearance];
	
	if ( (mState == NSOnState) && mEnabled )
		[mTitleLayer setOpacity: 1.0];
	else
		[mTitleLayer setOpacity: 0.7];
}	



- (CGFloat) preferredWidth;
{
	CGFloat preferredWidth = 0;
	
	NSFont* font = [NSFont boldSystemFontOfSize: 9];
	NSDictionary* attributes = [NSDictionary dictionaryWithObject: font
                                                         forKey: NSFontAttributeName];
	CGFloat labelWidth = ceil([mLabel sizeWithAttributes:attributes].width);
	preferredWidth = 5 + labelWidth + 5;
	
	return preferredWidth;
}



- (id) initWithIdentifier: (id) identifier
                    label: (NSString*) label
                direction: (WBTabDirection) tabDirection
                placement: (WBTabPlacement) tabPlacement
                     size: (WBTabSize) tabSize
                  hasIcon: (BOOL) hasIcon
                 canClose: (BOOL) canClose;
{
	self = [super initWithIdentifier: identifier
                             label: label
                         direction: tabDirection
                         placement: tabPlacement
                              size: tabSize
                           hasIcon: hasIcon
                          canClose: canClose];
	
	if (self != nil) {
		CGColorRef colorActiveSelected = WB_CGColorCreateCalibratedRGB(0.909, 0.909, 0.909, 1);
		CGColorRef colorActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.85, 0.85, 0.85, 1);
		CGColorRef colorNotActiveSelected = WB_CGColorCreateCalibratedRGB(0.909, 0.909, 0.909, 1);
		CGColorRef colorNotActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.85, 0.85, 0.85, 1);
		[self setColorActiveSelected: colorActiveSelected
          colorActiveNotSelected: colorActiveNotSelected
          colorNotActiveSelected: colorNotActiveSelected
		   colorNotActiveNotSelected: colorNotActiveNotSelected];
		CGColorRelease(colorActiveSelected);
		CGColorRelease(colorActiveNotSelected);
		CGColorRelease(colorNotActiveSelected);
		CGColorRelease(colorNotActiveNotSelected);
		
		// Border.
		[self setCornerRadius: 4];
		[self setBorderWidth: 1];
		CGColorRef c = WB_CGColorCreateCalibratedRGB(0.3, 0.3, 0.3, 0.7);
		[self setBorderColor: c];
		CGColorRelease(c);
	}
	
	return self;
}



+ (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label;
{
	return [[[WBPaletteTabItem alloc] initWithIdentifier: identifier
                                                 label: label
                                             direction: WBTabDirectionUp
                                             placement: WBTabPlacementTop
                                                  size: WBTabSizeSmall
                                               hasIcon: NO
                                              canClose: NO] autorelease];
}



@end


