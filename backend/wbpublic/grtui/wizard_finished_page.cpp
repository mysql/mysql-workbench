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

#include "wizard_finished_page.h"
#include "grt/common.h"
#include "base/string_utilities.h"

using namespace grtui;

WizardFinishedPage::WizardFinishedPage(WizardForm *form, const std::string &top_title) : WizardPage(form, "finish") {
  set_short_title(_("Results"));

  _label1.set_style(mforms::BoldStyle);
  _label1.set_wrap_text(true);

  set_spacing(12);
  set_padding(24);

  _top_title = top_title;
  _label2.set_text_align(mforms::TopLeft);
  _label2.set_wrap_text(true);

  _label1.set_text("");
  _label2.set_text("");

  add(&_label1, false, true);
  add(&_label2, true, true);
}

void WizardFinishedPage::set_heading(const std::string &title) {
  _label1.set_text(title);
}

void WizardFinishedPage::set_summary(const std::string &text) {
  _label2.set_text(text);
}

std::string WizardFinishedPage::next_button_caption() {
#ifdef __APPLE__
  return _("Close");
#elif defined(_WIN32)
  return _("Finish");
#else
  return _("_Close");
#endif
}
