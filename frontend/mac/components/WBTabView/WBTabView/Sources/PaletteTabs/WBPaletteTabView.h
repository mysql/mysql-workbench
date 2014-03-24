
/*!
 Copyright 2009 Sun Microsystems, Inc.
 */




#import "WBTabView.h"



@interface WBPaletteTabView : WBTabView
{
  NSImage* mPlaqueImage; // Must retain this for some reason, to make the NSImageRep work.
}


@end


