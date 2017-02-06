//
//  GreenhouseWindowController.h
//
//  Created by Jacob Engstrand on 2008-11-26.
//  Copyright 2008 Sun Microsystems, Inc. All rights reserved.
//

@class WBPaletteContainer;

@interface GreenhouseWindowController : NSWindowController {
  IBOutlet NSToolbar* mToolbar;
  IBOutlet WBPaletteContainer* mPaletteContainer;
  IBOutlet id mTabView;
  IBOutlet id mNewView;

  IBOutlet NSTabView* mEditorTabView;
}

@end
