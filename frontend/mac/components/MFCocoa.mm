//
//  MFCocoa.mm
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 23/Apr/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import "MFCocoa.h"

#include <mforms/mforms.h>


NSView *NSViewForMFormsView(mforms::View *view)
{
  return view->get_data();
}
