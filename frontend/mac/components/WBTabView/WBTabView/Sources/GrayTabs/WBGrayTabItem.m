
/*!
 Copyright 2009 Sun Microsystems, Inc.
 
 @author
 jak
 
 @class
 WBGrayTabItem
 
 @abstract
 Implements the custom tabs. Tabs at the top of the tab view, pointing upwards.
 
 @ingroup
 Custom Tab Views
 */



#import "WBGrayTabItem.h"
#import "CGColorUtilities.h"



@implementation WBGrayTabItem




- (CGFloat) preferredWidth;
{
  CGFloat preferredWidth = 0;
  
  NSFont* font = [NSFont boldSystemFontOfSize: 11.5];
  NSDictionary* attributes = [NSDictionary dictionaryWithObject: font
                                                         forKey: NSFontAttributeName];
  CGFloat labelWidth = ceil([mLabel sizeWithAttributes:attributes].width);
  preferredWidth = 25 + [mDocumentIconImage size].width + labelWidth + [mCloseButtonImage size].width;
  
  //  if (preferredWidth < 150)
  //    preferredWidth = 150;
  
  return preferredWidth;
  //	return 150;
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
		CGColorRef colorActiveSelected = WB_CGColorCreateCalibratedRGB(0.93, 0.93, 0.93, 1);
		CGColorRef colorActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.5, 0.5, 0.5, 1);
		CGColorRef colorNotActiveSelected = WB_CGColorCreateCalibratedRGB(0.93, 0.93, 0.93, 1);
		CGColorRef colorNotActiveNotSelected = WB_CGColorCreateCalibratedRGB(0.8, 0.8, 0.8, 1);
		[self setColorActiveSelected: colorActiveSelected
					colorActiveNotSelected: colorActiveNotSelected
					colorNotActiveSelected: colorNotActiveSelected
		   colorNotActiveNotSelected: colorNotActiveNotSelected];
		CGColorRelease(colorActiveSelected);
		CGColorRelease(colorActiveNotSelected);
		CGColorRelease(colorNotActiveSelected);
		CGColorRelease(colorNotActiveNotSelected);
		
		// Border.
		[self setCornerRadius: 6];
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
	return [[[self alloc] initWithIdentifier: identifier
																		 label: label
																 direction: WBTabDirectionUp
																 placement: WBTabPlacementTop
																			size: WBTabSizeLarge
																	 hasIcon: YES
																	canClose: YES] autorelease];
}



@end


