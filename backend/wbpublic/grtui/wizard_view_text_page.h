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
#ifndef _WIZARD_VIEW_TEXT_PAGE_H_
#define _WIZARD_VIEW_TEXT_PAGE_H_

#include "mforms/code_editor.h"
#include "mforms/box.h"
#include "mforms/button.h"

namespace grtui {

  class WBPUBLICBACKEND_PUBLIC_FUNC ViewTextPage : public WizardPage {
  public:
    enum Buttons { SaveButton = (1 << 0), CopyButton = (1 << 1) };

    ViewTextPage(WizardForm *form, const char *name = "preview", Buttons buttons = (Buttons)0,
                 const std::string &filetype = "");

    void set_text(const std::string &text);
    std::string get_text();

    void set_editable(bool flag = true);

    void save_text_to(const std::string &path);

  protected:
    mforms::CodeEditor _text;

    mforms::Box _button_box;
    mforms::Button _save_button;
    mforms::Button _copy_button;

    std::string _filetype;
    bool _editable;

    void save_clicked();
    void copy_clicked();
  };
};

#endif /* _WIZARD_VIEW_TEXT_PAGE_H_ */
