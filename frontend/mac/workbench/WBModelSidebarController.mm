/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#import "WBModelSidebarController.h"
#import "GRTListDataSource.h"
#import "MCanvasViewer.h"
#import "WBObjectPropertiesController.h"
#import "MOutlineView.h"
#import "MTextImageCell.h"
#import "GRTIconCache.h"

#include "wb_context.h"

#include "model/wb_history_tree.h"
#include "model/wb_model_diagram_form.h"
#include "model/wb_user_datatypes.h"
#include "model/wb_context_model.h"

#import "mforms/../cocoa/MFView.h"
#import "MCPPUtilities.h"

#include "grtdb/db_object_helpers.h"


@implementation WBModelSidebarController

- (void)setupWithDiagramForm:(wb::ModelDiagramForm*)form
{
  _catalog_tree = form->get_catalog_tree();
  [catalogTreeTab setView: nsviewForView(_catalog_tree)];

  _udtlist = form->get_wb()->get_model_context()->create_user_type_list();
  [userTypesTab setView: nsviewForView(_udtlist)];

  _history = form->get_wb()->get_model_context()->create_history_tree();
  [historyTab setView: nsviewForView(_history)];
}


- (void)setupWithContext:(wb::WBContextModel*)context
{
  _udtlist = context->create_user_type_list();
  [userTypesTab setView: nsviewForView(_udtlist)];

  _history = context->create_history_tree();
  [historyTab setView: nsviewForView(_history)];
}


- (void)dealloc
{
  delete _udtlist;
  delete _history;
  delete _catalog_tree;
  
  [super dealloc];
}


- (void)invalidate
{
  // Since performSelectorOnMainThread could have pending refreshes reset wbui.
}

//--------------------------------------------------------------------------------------------------

/**
 * Triggered if the underlying outline view was told the backend does not handle the specific
 * menu command (usually the case for pure UI related tasks). Thus handle them here.
 */
- (void)menuAction: (NSNotification*) notification
{
  /*
  id sender = [notification object];
  NSString* command = [sender representedObject];
  */
  // XXX: still needed?
}

//--------------------------------------------------------------------------------------------------

@end
