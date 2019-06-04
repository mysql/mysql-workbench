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

#ifndef _LF_MFORMS_H_
#define _LF_MFORMS_H_

#include "mforms/view.h"
#include "mforms/form.h"
#include "mforms/button.h"
#include "mforms/checkbox.h"
#include "mforms/textentry.h"
#include "mforms/textbox.h"
#include "mforms/label.h"
#include "mforms/selector.h"
#include "mforms/listbox.h"
#include "mforms/tabview.h"
#include "mforms/box.h"
#include "mforms/panel.h"
#include "mforms/filechooser.h"
#include "mforms/radiobutton.h"
#include "mforms/imagebox.h"
#include "mforms/progressbar.h"
#include "mforms/table.h"
#include "mforms/scrollpanel.h"
#include "mforms/treeview.h"
#include "mforms/wizard.h"
#include "mforms/drawbox.h"
#include "mforms/tabswitcher.h"
#include "mforms/app.h"
#include "mforms/appview.h"
#include "mforms/utilities.h"
#include "mforms/uistyle.h"
#include "mforms/appview.h"
#include "mforms/sectionbox.h"
#include "mforms/widgets.h"
#include "mforms/menu.h"
#include "mforms/splitter.h"
#include "mforms/popup.h"
#include "mforms/code_editor.h"
#include "mforms/menubar.h"
#include "mforms/toolbar.h"
#include "mforms/task_sidebar.h"
#include "mforms/hypertext.h"
#include "mforms/popover.h"
#include "mforms/fs_object_selector.h"
#include "mforms/simpleform.h"
#include "mforms/find_panel.h"
#include "mforms/canvas.h"

#include "mforms/mforms.h"

#include "lf_base.h"

namespace mforms {
  namespace gtk {
    extern bool force_sys_colors;
    void init(bool force_sys_colors);
    void check();
  };
};

#endif
