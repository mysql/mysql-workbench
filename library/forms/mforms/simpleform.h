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

#pragma once

#include <mforms/form.h>
#include <string>
#include <list>
#include <map>

namespace mforms {

  class Label;
  class View;
  class Button;
  class Panel;
  class Box;
  class Table;

  class MFORMS_EXPORT SimpleForm : public Form {
  public:
    SimpleForm(const std::string &title, const std::string &ok_caption);
    ~SimpleForm();

    void parse_definition(const std::string &definition);

    // void begin_group();
    // void end_group();

    void add_label(const std::string &text, bool bold);

    void add_file_picker(const std::string &name, const std::string &caption, const std::string &default_value = "");

    void add_text_entry(const std::string &name, const std::string &caption, const std::string &default_value = "");

    void add_text_area(const std::string &name, const std::string &caption, int rows,
                       const std::string &default_value = "");

    void add_checkbox(const std::string &name, const std::string &caption, bool default_value = false);

    void add_select(const std::string &name, const std::string &caption, const std::list<std::string> &items,
                    int default_index = -1);

    bool show();

    std::map<std::string, View *> get_views();
    std::string get_string_view_value(const std::string &name);

    bool get_bool_view_value(const std::string &name);
    int get_int_view_value(const std::string &name);

  private:
// Visual Studio produces a warning about this structure and its use in the list below. It complains about having no
// DLL interface for it. Huh? This is a private structure and never given out to anyone.
// If that ever changes then remove the pragma and export the list type properly!
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif // _MSC_VER
    struct Row {
      Label *caption;
      View *view;
      int spacing;    // TODO: this spacing has no function in a table. Investigate if it is still necessary.
      bool fullwidth; // TODO: similar this value
    };
    std::list<Row> _rows;
    std::string _ok_caption; // STL type, produces the same DLL-interface warning.
#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

    Table *_content;
    Box *_button_box;
    Button *_ok_button;
    Button *_cancel_button;
    int _caption_width;
    int _view_width;
    int _title_width;
  };
};
