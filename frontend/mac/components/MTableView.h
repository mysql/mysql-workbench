//
//  MTableView.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 24/May/09.
//  Copyright 2009 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>

// NSTableView subclass that handles context menus from bec::TreeModel
@interface MTableView : NSTableView {

}

- (BOOL) canDeleteItem: (id)sender;
- (void) deleteItem: (id)sender;
- (std::vector<bec::NodeId>) selectedNodeIds;

@end
