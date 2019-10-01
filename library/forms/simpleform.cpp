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

#include "mforms/mforms.h"
#include <sstream>
#include "base/string_utilities.h"

#define MIN_VIEW_WIDTH 200
#define FORM_MARGIN 12

using namespace std;
using namespace mforms;

SimpleForm::SimpleForm(const std::string &title, const std::string &ok_caption) : Form(NULL) {
  set_name("Simple Form");
  setInternalName("simple form");
  _caption_width = 0;
  _view_width = 0;
  _title_width = 0;

  _button_box = 0;
  _ok_button = 0;
  _cancel_button = 0;

  _ok_caption = ok_caption;

  set_title(title);

  _content = new Table();
  _content->set_padding(12);
  _content->set_row_spacing(8);
  _content->set_column_spacing(4);
  _content->set_column_count(2);
}

SimpleForm::~SimpleForm() {
  for (std::list<Row>::iterator iter = _rows.begin(); iter != _rows.end(); ++iter) {
    delete iter->caption;
    delete iter->view;
  }
  delete _ok_button;
  delete _cancel_button;
  delete _button_box;
}

static std::string parse_newlines(const std::string &str) {
  std::string result;
  const char *begin = str.data();
  const char *pos;

  while ((pos = strstr(begin, "\\n")) != NULL) {
    result.append(begin, pos).append("\n");
    begin = pos + 2;
  }
  result.append(begin);

  return result;
}

/*
 * Example:
 *   label;Stuff Here\nOptionally in multiple\nlines.
 *   textentry;name;caption;default
 *   checkbox;name;caption;1/0
 *   textarea;name;caption;rows;default
 *   select;name;caption;item1,item2,item3;default_index
 */
void SimpleForm::parse_definition(const std::string &definition) {
  char line[4 * 1024];
  char arg[4 * 1024];

  std::stringstream strm(definition, std::stringstream::in);

  while (!strm.eof()) {
    char type[20];

    strm.getline(line, sizeof(line));

    if (!*line)
      continue;

    std::stringstream toks(line, std::stringstream::in);

    toks.getline(type, sizeof(type), ';');
    if (strcmp(type, "label") == 0) {
      toks.getline(arg, sizeof(arg));
      add_label(parse_newlines(arg), false);
    } else if (strcmp(type, "textentry") == 0) {
      std::string name, caption, defaultv;

      toks.getline(arg, sizeof(arg), ';');
      name = arg;
      toks.getline(arg, sizeof(arg), ';');
      caption = arg;
      toks.getline(arg, sizeof(arg), ';');
      defaultv = arg;

      add_text_entry(name, caption, defaultv);
    } else if (strcmp(type, "checkbox") == 0) {
      std::string name, caption, defaultv;

      toks.getline(arg, sizeof(arg), ';');
      name = arg;
      toks.getline(arg, sizeof(arg), ';');
      caption = arg;
      toks.getline(arg, sizeof(arg), ';');
      defaultv = arg;

      add_checkbox(name, caption, defaultv == "1");
    } else if (strcmp(type, "textarea") == 0) {
      std::string name, rows, caption, defaultv;

      toks.getline(arg, sizeof(arg), ';');
      name = arg;
      toks.getline(arg, sizeof(arg), ';');
      caption = arg;
      toks.getline(arg, sizeof(arg), ';');
      rows = arg;
      toks.getline(arg, sizeof(arg), ';');
      defaultv = parse_newlines(arg);

      add_text_area(name, caption, base::atoi<int>(rows, 0), defaultv);
    } else if (strcmp(type, "select") == 0) {
      std::string name, caption, items, defaulti;
      std::list<std::string> items_list;

      toks.getline(arg, sizeof(arg), ';');
      name = arg;
      toks.getline(arg, sizeof(arg), ';');
      caption = arg;
      toks.getline(arg, sizeof(arg), ';');
      items = arg;
      toks.getline(arg, sizeof(arg), ';');
      defaulti = parse_newlines(arg);

      std::stringstream is(items, std::stringstream::in);
      char *s = new char[items.size()];
      while (!is.eof()) {
        is.getline(s, items.size(), ',');
        items_list.push_back(s);
      }
      delete[] s;

      add_select(name, caption, items_list, defaulti.empty() ? -1 : base::atoi<int>(defaulti, 0));
    } else
      throw std::runtime_error(std::string("invalid simpleform view type: ").append(type));
  }
}

void SimpleForm::add_label(const std::string &text, bool bold) {
  Label *l = new Label(text);
  _content->set_row_count((int)_rows.size() + 1);
  _content->add(l, 0, 2, (int)_rows.size(), (int)_rows.size() + 1, HFillFlag | HExpandFlag | VExpandFlag);

  Row row;
  row.caption = l;
  row.view = 0;
  row.spacing = 12;
  row.fullwidth = false;
  _rows.push_back(row);
}

void SimpleForm::add_file_picker(const std::string &name, const std::string &caption,
                                 const std::string &default_value) {
}

void SimpleForm::add_text_entry(const std::string &name, const std::string &caption, const std::string &default_value) {
  Label *l = 0;

  _content->set_row_count((int)_rows.size() + 1);
  if (!caption.empty()) {
    l = new Label(caption);
    l->set_text_align(MiddleRight);
    _content->add(l, 0, 1, (int)_rows.size(), (int)_rows.size() + 1, HFillFlag | HExpandFlag | VExpandFlag);
  }

  TextEntry *t = new TextEntry();
  t->set_value(default_value);
  t->set_name(name);
  _content->add(t, 1, 2, (int)_rows.size(), (int)_rows.size() + 1, HFillFlag | HExpandFlag | VExpandFlag);

  Row row;
  row.caption = l;
  row.view = t;
  row.spacing = 4;
  row.fullwidth = false;
  _rows.push_back(row);
}

void SimpleForm::add_text_area(const std::string &name, const std::string &caption, int rows,
                               const std::string &default_value) {
  Label *l = 0;

  _content->set_row_count((int)_rows.size() + 2);
  if (!caption.empty()) {
    l = new Label(caption);
    l->set_text_align(BottomRight);
    _content->add(l, 0, 1, (int)_rows.size(), (int)_rows.size() + 1, 0);
  }

  TextBox *t = new TextBox(BothScrollBars);
  t->set_value(default_value);
  t->set_name(name);
  //  t->set_size(t->get_preferred_width(), t->get_preferred_height()*rows);
  _content->add(t, caption.empty() ? 0 : 1, 2, (int)_rows.size(), (int)_rows.size() + 1,
                HFillFlag | HExpandFlag | VFillFlag | VExpandFlag);

  Row row;

  row.caption = l;
  row.view = 0;
  row.spacing = 2;
  row.fullwidth = false;
  _rows.push_back(row);

  row.caption = 0;
  row.view = t;
  row.spacing = 12;
  row.fullwidth = true;
  _rows.push_back(row);
}

void SimpleForm::add_checkbox(const std::string &name, const std::string &caption, bool default_value) {
  CheckBox *t = new CheckBox();
  t->set_text(caption);
  t->set_active(default_value);
  t->set_name(name);

  _content->set_row_count((int)_rows.size() + 1);
  _content->add(t, 0, 2, (int)_rows.size(), (int)_rows.size() + 1, 0);

  Row row;
  row.caption = 0;
  row.view = t;
  row.spacing = 4;
  row.fullwidth = false;
  _rows.push_back(row);
}

void SimpleForm::add_select(const std::string &name, const std::string &caption, const std::list<std::string> &items,
                            int default_index) {
  Label *l = 0;

  _content->set_row_count((int)_rows.size() + 1);
  if (!caption.empty()) {
    l = new Label(caption);
    l->set_text_align(MiddleRight);
    _content->add(l, 0, 1, (int)_rows.size(), (int)_rows.size() + 1, 0);
  }

  Selector *t = new Selector();
  t->set_selected(default_index);
  t->add_items(items);
  t->set_name(name);
  _content->add(t, 1, 2, (int)_rows.size(), (int)_rows.size() + 1, HFillFlag | HExpandFlag);

  Row row;
  row.caption = l;
  row.view = t;
  row.spacing = 4;
  row.fullwidth = false;
  _rows.push_back(row);
}

bool SimpleForm::show() {
  if (!_button_box) {
    set_content(_content);
    center();

    _button_box = new Box(true);
    _button_box->set_spacing(8);

    _content->set_row_count((int)_rows.size() + 2);
    _content->add(manage(new Label("")), 0, 2, (int)_rows.size() - 1, (int)_rows.size(), HFillFlag | HExpandFlag);
    _content->add(_button_box, 0, 2, (int)_rows.size(), (int)_rows.size() + 1, HFillFlag | HExpandFlag | VFillFlag | VExpandFlag);

    _ok_button = new Button();
    _ok_button->set_text(_ok_caption);
    _button_box->add_end(_ok_button, true, true);

    _cancel_button = new Button();
    _cancel_button->set_text("Cancel");
    _button_box->add_end(_cancel_button, true, true);
  }

  return run_modal(_ok_button, _cancel_button);
}

std::map<std::string, View *> SimpleForm::get_views() {
  std::map<std::string, View *> views;

  for (std::list<Row>::const_iterator iter = _rows.begin(); iter != _rows.end(); ++iter) {
    if (iter->view)
      views[iter->view->getInternalName()] = iter->view;
  }

  return views;
}

std::string SimpleForm::get_string_view_value(const std::string &name) {
  View *view = dynamic_cast<View *>(_content->find_subview(name));
  if (view)
    return view->get_string_value();

  return "";
}

bool SimpleForm::get_bool_view_value(const std::string &name) {
  View *view = dynamic_cast<View *>(_content->find_subview(name));
  if (view)
    return view->get_bool_value();

  return false;
}

int SimpleForm::get_int_view_value(const std::string &name) {
  View *view = dynamic_cast<View *>(_content->find_subview(name));
  if (view)
    return view->get_int_value();

  return 0;
}
