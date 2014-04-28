//
//  WBDiagramSizeController.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 21/Feb/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>

namespace wb {
  class DiagramOptionsBE;
  class WBContextUI;
}

@class MCanvasViewer;

@interface WBDiagramSizeController : NSObject {
  IBOutlet NSPanel *panel;
  IBOutlet MCanvasViewer *canvas;
  IBOutlet NSTextField *nameField;
  IBOutlet NSTextField *widthField;
  IBOutlet NSTextField *heightField;
  
  wb::DiagramOptionsBE *_be;
}

- (id)initWithWBContext:(wb::WBContextUI*)wbui;

- (IBAction)okClicked:(id)sender;
- (void)showModal;

@end
