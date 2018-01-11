/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _GRTDB_OBJECT_FILTER_H_
#define _GRTDB_OBJECT_FILTER_H_

#include "grt/grt_manager.h"
#include "grtdb/db_object_filter.h"

#include "mforms/panel.h" // TODO: use class pointers and forward decls to avoid including all these headers.
#include "mforms/box.h"
#include "mforms/table.h"
#include "mforms/imagebox.h"
#include "mforms/checkbox.h"
#include "mforms/label.h"
#include "mforms/button.h"
#include "mforms/textentry.h"
#include "mforms/selector.h"
#include "mforms/listbox.h"
#include "mforms/form.h"

namespace grtui {

  class WBPUBLICBACKEND_PUBLIC_FUNC DBObjectFilterFrame : public mforms::Panel {
  public:
    DBObjectFilterFrame();

    void set_object_class(const std::string &oclass, const std::string &caption_format);
    void set_models(bec::GrtStringListModel *model, bec::GrtStringListModel *excl_model, bool *enabled_flag);

    void set_active(bool flag);
    bool get_active();

  protected:
    bec::DBObjectFilterBE _filter_be;

    bec::GrtStringListModel *_model;
    bec::GrtStringListModel *_exclude_model;
    bool *_enabled_flag;

    mforms::Box _box;

    mforms::Table _summary_table;

    mforms::ImageBox _icon;

    mforms::CheckBox _check;

    mforms::Label _summary_label;

    mforms::Button _show_button;

    mforms::Table _detailed_table;

    mforms::Label _filter_help_label;
    mforms::Label _search_label;
    mforms::Label _filter_label;
    mforms::TextEntry _search_text;
    mforms::Selector _filter_combo;
    mforms::Button _add_button;
    mforms::Button _remove_button;

    mforms::ListBox _object_list;
    mforms::ListBox _mask_list;

    mforms::Button _add1_button;
    mforms::Button _add2_button;
    mforms::Button _del1_button;
    mforms::Button _del2_button;
    mforms::Button _mask_button;

    void toggle_enabled();
    void toggle_detailed();

    void update_button_enabled();

    void refresh(ssize_t object_list_selection, ssize_t mask_list_selection);

    void add_mask();
    void add_clicked(bool all);
    void del_clicked(bool all);
  };
};

#endif /* _GRTDB_OBJECT_FILTER_H_ */
