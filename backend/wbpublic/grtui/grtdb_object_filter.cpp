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

#include "grtdb_object_filter.h"
#include "grt/grt_string_list_model.h"
#include "text_input_dialog.h"
#include "base/string_utilities.h"

using namespace grtui;
using namespace base;

//--------------------------------------------------------------------------------------------------

static void refill_list(mforms::ListBox &list, bec::GrtStringListModel *model) {
  list.clear();
  for (size_t c = model->count(), i = 0; i < c; i++) {
    std::string item;
    model->get_field((int)i, 0, item);
    list.add_item(item);
  }
}

//--------------------------------------------------------------------------------------------------

DBObjectFilterFrame::DBObjectFilterFrame()
  : mforms::Panel(mforms::BorderedPanel), _enabled_flag(NULL), _box(false), _object_list(true), _mask_list(true) {
  set_padding(8);
  set_name("Object Filter");
  setInternalName("objectFilterFrame");

  _box.set_spacing(8);
  _box.set_name("Filter Contents");
  _box.setInternalName("filterContentBox");

  // summary view
  _summary_table.set_name("Summary");
  _summary_table.setInternalName("summaryTable");
  _box.add(&_summary_table, false, true);

  _summary_table.set_row_count(2);
  _summary_table.set_column_count(3);

  _summary_table.set_row_spacing(4);
  _summary_table.set_column_spacing(8);

  _icon.set_size(48, 48);
  _icon.set_name("Filter");
  _icon.setInternalName("filterIcon");

  _check.set_name("Filter");
  _check.setInternalName("filterCheckBox");
  _check.set_text("Include Objects of This Type"); // this text will be updated later on
  scoped_connect(_check.signal_clicked(), std::bind(&DBObjectFilterFrame::toggle_enabled, this));

  _summary_table.add(&_icon, 0, 1, 0, 2, 0);

  _summary_label.set_name("Filter Summary");
  _summary_label.setInternalName("filterSummaryLabel");
  _summary_label.set_text(_("Selected/Total Objects:"));
  _summary_label.set_style(mforms::SmallStyle);
  _summary_label.set_name("Summary");
  _summary_label.setInternalName("summaryLabel");

  _summary_table.add(&_check, 1, 2, 0, 1, mforms::HExpandFlag | mforms::HFillFlag | mforms::VFillFlag);
  _summary_table.add(&_summary_label, 1, 2, 1, 2, mforms::HFillFlag | mforms::VFillFlag);
  _summary_label.set_text_align(mforms::MiddleLeft);

  _show_button.set_name("Filter Show Lists");
  _show_button.setInternalName("filterShowListsButton");
  _show_button.set_text(_("Show Filter"));
  scoped_connect(_show_button.signal_clicked(), std::bind(&DBObjectFilterFrame::toggle_detailed, this));

  _summary_table.add(&_show_button, 2, 3, 0, 2, mforms::HFillFlag);

  // detailed view
  _detailed_table.set_name("Filter Details");
  _detailed_table.setInternalName("filterDetailTable");
  _box.add(&_detailed_table, true, true);
  _detailed_table.show(false);

  _detailed_table.set_row_count(9);
  _detailed_table.set_column_count(3);

  _detailed_table.set_row_spacing(8);
  _detailed_table.set_column_spacing(12);

  // not implemented yet
  //_search_label.set_text(_("Search:"));
  //_detailed_table.add(&_search_label, 0, 1, 0, 1, 0);

  //_detailed_table.add(&_search_text, 1, 2, 0, 1);

  /*
  _filter_label.set_text(_("Filter:"));
  _detailed_table.add(&_filter_label, 2, 3, 0, 1, 0);


  _filter_combo.set_tooltip(_("Select a preset filter."));
  _detailed_table.add(&_filter_combo, 3, 4, 0, 1);
  */

  _object_list.set_name("Filter Sources");
  _object_list.setInternalName("filterSourceList");
  _object_list.set_heading(_("Objects to Process"));
#ifdef _MSC_VER
  _object_list.set_size(120, -1); // Need a minimum size for Windows, or layouting does not work properly.
#endif
  _detailed_table.add(&_object_list, 0, 1, 1, 8, mforms::VFillFlag | mforms::HFillFlag | mforms::HExpandFlag);
  scoped_connect(_object_list.signal_changed(), std::bind(&DBObjectFilterFrame::update_button_enabled, this));

  _add1_button.set_name("Filter Add Selected Object");
  _add1_button.setInternalName("filterAddSelectedObjectButton");
  _add1_button.set_text(">");
  scoped_connect(_add1_button.signal_clicked(), std::bind(&DBObjectFilterFrame::add_clicked, this, false));

  _del1_button.set_name("Filter Remove Selected Object");
  _del1_button.setInternalName("filterRemoveSelectedObjectButton");
  _del1_button.set_text("<");
  scoped_connect(_del1_button.signal_clicked(), std::bind(&DBObjectFilterFrame::del_clicked, this, false));

  _add1_button.set_name("Filter Add All Objects");
  _add1_button.setInternalName("filterAddAllObjectsButton");
  _add2_button.set_text(">>");
  scoped_connect(_add2_button.signal_clicked(), std::bind(&DBObjectFilterFrame::add_clicked, this, true));

  _del2_button.set_name("Filter Remove All Objects");
  _del2_button.setInternalName("filterRemoveAllObjectsButton");
  _del2_button.set_text("<<");
  scoped_connect(_del2_button.signal_clicked(), std::bind(&DBObjectFilterFrame::del_clicked, this, true));

  _mask_button.set_name("Filter Add Pattern");
  _mask_button.setInternalName("filterAddPatternButton");
  _mask_button.set_text("+");
  scoped_connect(_mask_button.signal_clicked(), std::bind(&DBObjectFilterFrame::add_mask, this));

  _detailed_table.add(&_add1_button, 1, 2, 2, 3, mforms::HFillFlag);
  _detailed_table.add(&_del1_button, 1, 2, 3, 4, mforms::HFillFlag);
  _detailed_table.add(&_add2_button, 1, 2, 4, 5, mforms::HFillFlag);
  _detailed_table.add(&_del2_button, 1, 2, 5, 6, mforms::HFillFlag);
  _detailed_table.add(&_mask_button, 1, 2, 6, 7, mforms::HFillFlag);

  _mask_list.set_name("Filter Target List");
  _mask_list.setInternalName("filterTargetList");

#ifdef _MSC_VER
  _mask_list.set_size(120, -1);
#endif

  _mask_list.set_heading(_("Excluded Objects"));
  _detailed_table.add(&_mask_list, 2, 3, 1, 8, mforms::HFillFlag | mforms::VFillFlag | mforms::HExpandFlag);
  scoped_connect(_mask_list.signal_changed(), std::bind(&DBObjectFilterFrame::update_button_enabled, this));

  _filter_help_label.set_name("Filter Help");
  _filter_help_label.setInternalName("filterHelpLabel");
  _filter_help_label.set_style(mforms::SmallHelpTextStyle);
  _filter_help_label.set_text(_("Use the + button to exclude objects matching wildcards such as * and ?"));
  _detailed_table.add(&_filter_help_label, 0, 3, 8, 9, mforms::HFillFlag | mforms::VFillFlag);

  add(&_box);
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::set_object_class(const std::string &oclass, const std::string &caption_format) {
  _filter_be.set_object_type_name(oclass);

  _summary_label.set_text(strfmt(_("%i Total Objects, %i Selected"), 0, 0));

  _check.set_text(strfmt(caption_format.c_str(), _filter_be.get_full_type_name().c_str()));

  bec::IconId icon = _filter_be.icon_id(bec::Icon32);

  if (icon != 0) {
    std::string icon_path = bec::IconManager::get_instance()->get_icon_path(icon);
    if (!icon_path.empty())
      _icon.set_image(icon_path);
  }
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::set_models(bec::GrtStringListModel *model, bec::GrtStringListModel *excl_model,
                                     bool *enabled_flag) {
  _model = model;
  _exclude_model = excl_model;
  _enabled_flag = enabled_flag;
  _filter_be.filter_model(_exclude_model);

  if (_model->total_items_count() == 0)
    set_active(false);
  else
    set_active(true);

  refresh(-1, -1);
}

//--------------------------------------------------------------------------------------------------

/**
 * Reloads the models and the list boxes. Selects the given indices if > -1.
 */
void DBObjectFilterFrame::refresh(ssize_t object_list_selection, ssize_t mask_list_selection) {
  _model->refresh();
  _exclude_model->refresh();

  refill_list(_object_list, _model);
  if (object_list_selection > -1 && object_list_selection < (ssize_t)_model->count())
    _object_list.set_selected(object_list_selection);

  refill_list(_mask_list, _exclude_model);
  if (mask_list_selection > -1 && mask_list_selection < (ssize_t)_exclude_model->count())
    _mask_list.set_selected(mask_list_selection);

  std::stringstream out;
  out << _model->total_items_count() << " Total Objects, " << _model->active_items_count() << " Selected";
  _summary_label.set_text(out.str());

  update_button_enabled();
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::update_button_enabled() {
  _add1_button.set_enabled(!_object_list.get_selected_indices().empty());
  _del1_button.set_enabled(!_mask_list.get_selected_indices().empty());
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::toggle_enabled() {
  if (_enabled_flag)
    *_enabled_flag = get_active();
  //  _box.set_enabled(get_active());
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::set_active(bool flag) {
  _check.set_active(flag);
}

//--------------------------------------------------------------------------------------------------

bool DBObjectFilterFrame::get_active() {
  return _check.get_active();
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::toggle_detailed() {
  if (_detailed_table.is_shown()) {
    _show_button.set_text(_("Show Filter"));
    _detailed_table.show(false);
  } else {
    _show_button.set_text(_("Hide Filter"));
    _detailed_table.show(true);
  }
  get_parent()->relayout();
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::add_mask() {
  TextInputDialog dlg(get_parent_form());

  dlg.set_description("Pattern mask for objects to be ignored.\nYou may use wildcards such as * and ?");
  dlg.set_caption("Enter Pattern Mask:");

  if (dlg.run()) {
    _exclude_model->add_item(dlg.get_value(), -1);
    _model->invalidate();

    refresh(-1, -1);
  }
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::add_clicked(bool all) {
  _filter_combo.set_selected(-1);

  std::vector<size_t> indices;
  ssize_t new_selection = -1;
  if (all) {
    for (size_t i = 0; i < _model->count(); ++i)
      indices.push_back(i);
  } else {
    indices = _object_list.get_selected_indices();
    new_selection = indices[0] - 1;
    if (new_selection < 0)
      new_selection = 0;
  }

  _model->copy_items_to_val_masks_list(indices);
  _model->invalidate(); // Weird work flow here. Need to mark the model as invalidate or it will refuse to refresh.
  refresh(new_selection, -1);
}

//--------------------------------------------------------------------------------------------------

void DBObjectFilterFrame::del_clicked(bool all) {
  _filter_combo.set_selected(-1);

  std::vector<size_t> indices;
  ssize_t new_selection = -1;
  if (all) {
    for (size_t c = _exclude_model->count(), i = 0; i < c; i++)
      indices.push_back(i);
  } else {
    indices = _mask_list.get_selected_indices();
    new_selection = indices[0] - 1;
    if (new_selection < 0)
      new_selection = 0;
  }

  _exclude_model->remove_items(indices);
  _model->invalidate();
  refresh(-1, new_selection);
}

//--------------------------------------------------------------------------------------------------
