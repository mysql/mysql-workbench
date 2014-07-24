/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "base/geometry.h"
#include "base/string_utilities.h"
#import "base/log.h"

#include "workbench/wb_context.h"
#include "workbench/wb_command_ui.h"

#import "WBSQLQueryPanel.h"

#import "WBMainController.h"
#import "WBMainWindow.h"
#import "MResultsetViewer.h"

#include "mforms/code_editor.h"

#include "workbench/wb_context_ui.h"

#import "WBSQLQueryUI.h"

DEFAULT_LOG_DOMAIN(DOMAIN_WQE_NATIVE)

static WBBasePanel *createQueryPanel(WBMainWindow *mainwin, boost::shared_ptr<bec::UIForm> form)
{
  SqlEditorForm::Ref editor= boost::dynamic_pointer_cast<SqlEditorForm>(form);
  
  if (!editor)
    throw std::logic_error("invalid backend object");

  return [[[WBSQLQueryPanel alloc] initWithBE: editor] autorelease];
}


void setupSQLQueryUI(WBMainController *main, WBMainWindow *mainwin, wb::WBContextUI *wbui)
{  
  log_debug("Setting up UI\n");
  // other commands in wb_context_sqlide.cpp

  [main registerFormPanelFactory:createQueryPanel forFormType: WB_MAIN_VIEW_DB_QUERY];
}

