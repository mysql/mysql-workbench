
/*!
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
 
 @class
 WBEditorTabItem
 
 @abstract
 Implements the custom tabs used in editors. Small tabs at the bottom of the tab view, pointing downwards.
 
 @ingroup
 Custom Tab Views
 */



#import "WBEditorTabItem.h"
#import "CGColorUtilities.h"



@implementation WBEditorTabItem



- (void) updateAppearance;
{
	[super updateAppearance];
	
	if ( (mState == NSOnState) && mEnabled ) {
		[mTitleLayer setOpacity: 1.0];
  }
	else {
		[mTitleLayer setOpacity: 0.8];
  }

  // Set the shadow.
  [self setShadowOpacity: 0];

//	if (mState == NSOnState) {
////    [self setShadowOpacity: 0.2];
////    [self setShadowRadius: 1.0];
////    NSFont* font = [NSFont boldSystemFontOfSize: 0];
//    NSFont* font = [NSFont systemFontOfSize: 0];
//    [mTitleLayer setFont: font];
//  }
//  else {
////    [self setShadowOpacity: 0];
//    NSFont* font = [NSFont systemFontOfSize: 0];
//    [mTitleLayer setFont: font];
//  }    
}	



- (CGFloat) preferredWidth;
{
	CGFloat preferredWidth = 0;
	
	NSFont* font = [NSFont boldSystemFontOfSize: 9];
	NSDictionary* attributes = [NSDictionary dictionaryWithObject: font
                                                         forKey: NSFontAttributeName];
	CGFloat labelWidth = ceil([mLabel sizeWithAttributes: attributes].width);
	preferredWidth = 5 + labelWidth + 5;
  preferredWidth = MAX(preferredWidth, 86);
  
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
    double gray = (230.0 / 255.0);
    CGColorRef colorActiveSelected = WB_CGColorCreateCalibratedRGB(gray, gray, gray, 1);
		CGColorRef colorActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.91, 0.91, 0.91, 1);
		CGColorRef colorNotActiveSelected = WB_CGColorCreateCalibratedRGB(gray, gray, gray, 1);
		CGColorRef colorNotActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.91, 0.91, 0.91, 1);
    
    [self setColorActiveSelected: colorActiveSelected
          colorActiveNotSelected: colorActiveNotSelected
          colorNotActiveSelected: colorNotActiveSelected
		   colorNotActiveNotSelected: colorNotActiveNotSelected];
		CGColorRelease(colorActiveSelected);
		CGColorRelease(colorActiveNotSelected);
		CGColorRelease(colorNotActiveSelected);
		CGColorRelease(colorNotActiveNotSelected);
		
		// Border.
		[self setCornerRadius: 5];
		[self setBorderWidth: 1];
    gray = (130.0 / 255.0);
		CGColorRef c = WB_CGColorCreateCalibratedRGB(gray, gray, gray, 0.7);
		[self setBorderColor: c];
		CGColorRelease(c);

    c = WB_CGColorCreateCalibratedRGB(0, 0, 0, 1.0);
    [mTitleLayer setForegroundColor: c];
    CGColorRelease(c);
    NSFont* font = [NSFont systemFontOfSize: 0];
    [mTitleLayer setFont: font];
//    [mTitleLayer setShadowOpacity: 0];

    // Center the title.
    CGRect f = [mTitleLayer frame];
    f.origin.x = 0;
    f.size.width = [self frame].size.width;
    f.origin.y -= 1;
    [mTitleLayer setFrame: f];
    [mTitleLayer setAlignmentMode: kCAAlignmentCenter];
  }
	
	return self;
}



+ (WBTabItem*) tabItemWithIdentifier: (id) identifier
                               label: (NSString*) label;
{
	return [[[WBEditorTabItem alloc] initWithIdentifier: identifier
                                                label: label
                                            direction: WBTabDirectionDown
                                            placement: WBTabPlacementBottom
                                                 size: WBTabSizeLarge
                                              hasIcon: NO
                                             canClose: NO] autorelease];
}



@end


