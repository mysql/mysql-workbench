/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "wb_context.h"
#include "model/wb_model_diagram_form.h"

#import "WBModelSidebarController.h"
#import "mforms/../cocoa/MFView.h"

@implementation WBModelSidebarController

- (void)setupWithDiagramForm: (wb::ModelDiagramForm*)form
{
  _catalog_tree = form->get_catalog_tree();
  catalogTreeTab.view = nsviewForView(_catalog_tree);

  _udtlist = form->get_wb()->get_model_context()->create_user_type_list();
  userTypesTab.view = nsviewForView(_udtlist);

  _history = form->get_wb()->get_model_context()->create_history_tree();
  historyTab.view = nsviewForView(_history);
}

- (void)setupWithContext:(wb::WBContextModel*)context
{
  _udtlist = context->create_user_type_list();
  userTypesTab.view = nsviewForView(_udtlist);

  _history = context->create_history_tree();
  historyTab.view = nsviewForView(_history);
}

- (void)dealloc
{
  delete _udtlist;
  delete _history;
  delete _catalog_tree;
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
