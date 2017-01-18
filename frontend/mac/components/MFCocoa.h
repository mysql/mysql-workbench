//
//  MFCocoa.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 23/Apr/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>

namespace mforms {
  class View;
};

NSView *NSViewForMFormsView(mforms::View *view);
