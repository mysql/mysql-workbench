/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "wizard_object_filter_page.h"
/**
 * @file  wizard_object_filter_page.cpp
 * @brief
 */

using namespace grtui;

WizardObjectFilterPage::WizardObjectFilterPage(WizardForm *form, const char *name)
  : WizardPage(form, name), _scroll_panel(), _box(false) {
  set_padding(8);
  set_spacing(8);
  add(&_top_label, false, true);
  add(&_scroll_panel, true, true);
  _scroll_panel.add(&_box);
}

WizardObjectFilterPage::~WizardObjectFilterPage() {
  reset();
}

DBObjectFilterFrame *WizardObjectFilterPage::add_filter(const std::string &class_name, const std::string &caption_fmt,
                                                        bec::GrtStringListModel *model,
                                                        bec::GrtStringListModel *excl_model, bool *enabled_flag) {
  DBObjectFilterFrame *filter;

  filter = new DBObjectFilterFrame();
  filter->set_object_class(class_name, caption_fmt);
  filter->set_models(model, excl_model, enabled_flag);
  _box.add(mforms::manage(filter), false, true);
  _filters.push_back(filter);
  //  filter->set_active(true);

  return filter;
}

void WizardObjectFilterPage::reset() {
  for (std::vector<DBObjectFilterFrame *>::const_iterator iter = _filters.begin(); iter != _filters.end(); ++iter) {
    _box.remove(*iter);
    //   (*iter)->release();
  }
  _filters.clear();
}
