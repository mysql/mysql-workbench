
/*!
 Copyright 2009 Sun Microsystems, Inc.
 */




#import "WBTabItem.h"



@interface WBSchemaTabItem : WBTabItem
{
	CATextLayer* mTitleLayerB;
	CALayer* mSideLeft;
	CALayer* mSideRight;
	CALayer* mBackgroundGradient;
  
  NSImage* mIconImage; // Must retain this for some reason, to make the NSImageRep work.
  NSImage* mSideLineImage; // Must retain this for some reason, to make the NSImageRep work.
  NSImage* mAlphaGradientImage; // Must retain this for some reason, to make the NSImageRep work.
}



+ (WBTabItem*) tabItemWithIdentifier: (id) identifier
							   label: (NSString*) label;



@end


