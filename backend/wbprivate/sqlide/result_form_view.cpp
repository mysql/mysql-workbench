/*
 * Copyright (c) 2013, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_sql_editor_form.h"
#include "wb_sql_editor_result_panel.h"
#include "result_form_view.h"

#include "grtdb/db_helpers.h"
#include "grtui/inserts_export_form.h"
#include "grtui/geom_draw_box.h"
#include "sqlide/recordset_cdbc_storage.h"

#include "grt/spatial_handler.h"

#include "base/sqlstring.h"
#include "grt/parse_utils.h"
#include "base/log.h"
#include "base/boost_smart_ptr_helpers.h"

#include "mforms/gridview.h"
#include "mforms/utilities.h"
#include "mforms/treeview.h"
#include "mforms/textbox.h"
#include "mforms/label.h"
#include "mforms/tabview.h"
#include "mforms/tabswitcher.h"
#include "mforms/menubar.h"

#include "mforms/button.h"
#include "mforms/selector.h"
#include "mforms/textentry.h"

#include <algorithm>

using namespace base;

DEFAULT_LOG_DOMAIN("SqlResult")

//----------------------------------------------------------------------------------------------------------------------

class ResultFormView::FieldView {
  mforms::Label _label;

protected:
  std::function<void(const std::string &s)> _change_callback;

public:
  FieldView(const std::string &name, const std::function<void(const std::string &s)> &change_callback)
    : _label(name), _change_callback(change_callback) {
    _label.set_text_align(mforms::TopRight);
  }

  virtual ~FieldView() {
  }

  static ResultFormView::FieldView *create(const Recordset_cdbc_storage::FieldInfo &field, const std::string &full_type,
                                           bool editable, const std::function<void(const std::string &s)> &callback,
                                           const std::function<void()> &view_blob_callback);

  mforms::Label *label() {
    return &_label;
  }
  virtual mforms::View *value() = 0;
  virtual bool expands() {
    return false;
  }

  virtual void set_value(const std::string &value, bool is_null) = 0;
};

//----------------------------------------------------------------------------------------------------------------------

class StringFieldView : public ResultFormView::FieldView {
  mforms::TextEntry *_entry;
  bool _expands;

  void changed() {
    _change_callback(_entry->get_string_value());
  }

public:
  StringFieldView(const std::string &name, int max_length, bool editable,
                  const std::function<void(const std::string &s)> &change_callback)
    : FieldView(name, change_callback), _expands(false) {
    _entry = new mforms::TextEntry();
    _entry->set_enabled(editable);
    _entry->signal_changed()->connect(std::bind(&StringFieldView::changed, this));
    if (max_length > 64)
      _expands = true;
    else
      _entry->set_size(std::max(max_length * 10, 60), -1);
  }

  virtual bool expands() {
    return _expands;
  }

  virtual ~StringFieldView() {
    _entry->release();
  }

  virtual mforms::View *value() {
    return _entry;
  }

  virtual void set_value(const std::string &value, bool is_null) {
    _entry->set_value(value);
  }
};

//----------------------------------------------------------------------------------------------------------------------

class SelectorFieldView : public ResultFormView::FieldView {
  mforms::Selector _selector;

  void changed() {
    _change_callback(_selector.get_string_value());
  }

public:
  SelectorFieldView(const std::string &name, const std::list<std::string> &items, bool editable,
                    const std::function<void(const std::string &s)> &change_callback)
    : FieldView(name, change_callback) {
    _selector.add_items(items);
    _selector.set_enabled(editable);
    _selector.signal_changed()->connect(std::bind(&SelectorFieldView::changed, this));
  }

  virtual ~SelectorFieldView() {
  }

  virtual mforms::View *value() {
    return &_selector;
  }

  virtual void set_value(const std::string &value, bool is_null) {
    _selector.set_value(value);
  }
};

class SetFieldView : public ResultFormView::FieldView {
  mforms::TreeView _tree;

  void changed() {
    std::string value;

    for (int c = _tree.count(), i = 0; i < c; i++) {
      mforms::TreeNodeRef node = _tree.node_at_row(i);
      if (node->get_bool(0)) {
        if (!value.empty())
          value.append(",");
        value.append(node->get_string(1));
      }
    }
    _change_callback(value);
  }

public:
  SetFieldView(const std::string &name, const std::list<std::string> &items, bool editable,
               const std::function<void(const std::string &s)> &change_callback)
    : FieldView(name, change_callback), _tree(mforms::TreeFlatList | mforms::TreeNoHeader) {
    _tree.add_column(mforms::CheckColumnType, "", 30, true);
    _tree.add_column(mforms::StringColumnType, "", 200, false);
    _tree.end_columns();

    for (std::list<std::string>::const_iterator i = items.begin(); i != items.end(); ++i) {
      mforms::TreeNodeRef node = _tree.add_node();
      node->set_string(1, *i);
    }

    size_t height = items.size() * 20;
    _tree.set_size(250, height > 100 ? 100 : (int)height);

    _tree.set_enabled(editable);
    _tree.signal_changed()->connect(std::bind(&SetFieldView::changed, this));
  }

  virtual mforms::View *value() {
    return &_tree;
  }

  virtual void set_value(const std::string &value, bool is_null) {
    std::vector<std::string> l(base::split_token_list(value, ','));

    for (int c = _tree.count(), i = 0; i < c; i++) {
      mforms::TreeNodeRef node = _tree.node_at_row(i);
      if (std::find(l.begin(), l.end(), node->get_string(1)) != l.end())
        node->set_bool(0, true);
      else
        node->set_bool(0, false);
    }
  }
};

//----------------------------------------------------------------------------------------------------------------------

class TextFieldView : public ResultFormView::FieldView {
  mforms::TextBox *_tbox;

  void changed() {
    _change_callback(_tbox->get_string_value());
  }

public:
  TextFieldView(const std::string &name, bool editable,
                const std::function<void(const std::string &s)> &change_callback)
    : FieldView(name, change_callback) {
    _tbox = new mforms::TextBox(mforms::VerticalScrollBar);
    _tbox->set_enabled(editable);
    _tbox->signal_changed()->connect(std::bind(&TextFieldView::changed, this));
    _tbox->set_size(-1, 60);
  }

  virtual bool expands() {
    return true;
  }

  virtual ~TextFieldView() {
    _tbox->release();
  }

  virtual mforms::View *value() {
    return _tbox;
  }

  virtual void set_value(const std::string &value, bool is_null) {
    _tbox->set_value(value);
  }
};

//----------------------------------------------------------------------------------------------------------------------

class BlobFieldView : public ResultFormView::FieldView {
  mforms::Box _box;
  mforms::Label _blob;
  std::string _type_desc;

  void changed() {
  }

public:
  BlobFieldView(const std::string &name, const std::string &type, bool editable,
                const std::function<void(const std::string &s)> &change_callback,
                const std::function<void()> &view_callback)
    : FieldView(name, change_callback), _box(true), _blob(type), _type_desc(type) {
    _box.set_spacing(8);
    _box.add(&_blob, false, true);
    mforms::Button *b = mforms::manage(new mforms::Button());
    b->enable_internal_padding(false);
    b->signal_clicked()->connect(view_callback);
    b->set_text("View...");
    _box.add(b, false, true);
  }

  virtual mforms::View *value() {
    return &_box;
  }

  virtual void set_value(const std::string &value, bool is_null) {
    _blob.set_text(is_null ? "NULL" : _type_desc);
  }
};

//----------------------------------------------------------------------------------------------------------------------

class GeomFieldView : public ResultFormView::FieldView {
  mforms::Box _box;
  mforms::Box _imageBox;
  mforms::Label _srid;
  mforms::TextBox _text;
  GeomDrawBox _image;
  std::string _raw_data;
  int _view_type;

  virtual bool expands() {
    return true;
  }

  void update() {
    std::string text;
    spatial::Importer importer;
    importer.import_from_mysql(_raw_data);
    switch (_view_type) {
      case 0:
        text = importer.as_wkt();
        break;
      case 1:
        text = importer.as_json();
        break;
      case 2:
        text = importer.as_gml();
        break;
      case 3:
        text = importer.as_kml();
        break;
    }
    _text.set_value(text);
  }

public:
  GeomFieldView(const std::string &name, const std::string &type, bool editable,
                const std::function<void(const std::string &s)> &change_callback,
                const std::function<void()> &view_callback)
    : FieldView(name, change_callback), _box(true), _imageBox(false), _text(mforms::VerticalScrollBar) {
    _view_type = 0;
    _box.set_spacing(8);
    _imageBox.set_spacing(8);
    _image.set_size(150, 150);

    _imageBox.add(&_image, false, true);
    _imageBox.add_end(&_srid, false, false);
    _box.add(&_imageBox, false, true);
    _box.add(&_text, true, true);
  }

  virtual mforms::View *value() {
    return &_box;
  }

  virtual void set_value(const std::string &value, bool is_null) {
    _image.set_data(value);
    _srid.set_text("SRID: " + std::to_string(_image.getSrid()));
    _text.set_read_only(false);
    _raw_data = value;
    update();
    _text.set_read_only(true);
  }

  void set_view_type(const std::string &type) {
    if (type.find("WKT") != std::string::npos)
      _view_type = 0;
    else if (type.find("JSON") != std::string::npos)
      _view_type = 1;
    else if (type.find("GML") != std::string::npos)
      _view_type = 2;
    else if (type.find("KML") != std::string::npos)
      _view_type = 3;
    update();
  }
};

//----------------------------------------------------------------------------------------------------------------------

static std::list<std::string> parse_enum_definition(const std::string &full_type) {
  std::list<std::string> l;
  std::string::size_type b, e;

  b = full_type.find('(');
  e = full_type.rfind(')');
  if (b != std::string::npos && e != std::string::npos && e > b) {
    bec::tokenize_string_list(full_type.substr(b + 1, e - b - 1), '\'', true, l);
    for (std::list<std::string>::iterator i = l.begin(); i != l.end(); ++i) {
      // strip quotes
      *i = i->substr(1, i->size() - 2);
    }
  }
  return l;
}

//----------------------------------------------------------------------------------------------------------------------

inline std::string format_label(const std::string &label) {
  std::string flabel = label + ":";

  if (g_ascii_isalpha(flabel[0]))
    flabel = g_ascii_toupper(flabel[0]) + flabel.substr(1);

  return flabel;
}

//----------------------------------------------------------------------------------------------------------------------

ResultFormView::FieldView *ResultFormView::FieldView::create(const Recordset_cdbc_storage::FieldInfo &field,
                                                             const std::string &full_type, bool editable,
                                                             const std::function<void(const std::string &s)> &callback,
                                                             const std::function<void()> &view_blob_callback) {
  if (field.type == "VARCHAR") {
    if (field.display_size > 40) {
      TextFieldView *text = new TextFieldView(format_label(field.field), editable, callback);
      if (field.display_size > 1000)
        text->value()->set_size(-1, 200);
      return text;
    } else
      return new StringFieldView(format_label(field.field), field.display_size, editable, callback);
  } else if (field.type == "TEXT") {
    return new TextFieldView(format_label(field.field), editable, callback);
  } else if (field.type == "BLOB") {
    return new BlobFieldView(format_label(field.field), field.type, editable, callback, view_blob_callback);
  } else if (field.type == "GEOMETRY") {
    return new GeomFieldView(format_label(field.field), field.type, editable, callback, view_blob_callback);
  } else if (field.type == "ENUM" && !full_type.empty()) {
    return new SelectorFieldView(format_label(field.field), parse_enum_definition(full_type), editable, callback);
  } else if (field.type == "SET" && !full_type.empty()) {
    return new SetFieldView(format_label(field.field), parse_enum_definition(full_type), editable, callback);
  } else
    return new StringFieldView(format_label(field.field), field.display_size, editable, callback);
  return NULL;
}

//----------------------------------------------------------------------------------------------------------------------

void ResultFormView::navigate(mforms::ToolBarItem *item) {
  std::string name = item->getInternalName();
  Recordset::Ref rset(_rset.lock());
  if (rset) {
    ssize_t row = rset->edited_field_row();
    if (row < 0) // Useless. RowID is size_t and can never be 0.
      return;

    if (name == "delete") {
      rset->delete_node(row);
    } else if (name == "back") {
      row--;
      if (row < 0)
        row = 0;
      rset->set_edited_field(row, rset->edited_field_column());
      if (rset->update_edited_field)
        rset->update_edited_field();
    } else if (name == "first") {
      row = 0;
      rset->set_edited_field(row, rset->edited_field_column());
      if (rset->update_edited_field)
        rset->update_edited_field();
    } else if (name == "next") {
      row++;
      if ((size_t)row >= rset->count())
        row = rset->count() - 1;
      rset->set_edited_field(row, rset->edited_field_column());
      if (rset->update_edited_field)
        rset->update_edited_field();
    } else if (name == "last") {
      row = rset->count() - 1;
      rset->set_edited_field(row, rset->edited_field_column());
      if (rset->update_edited_field)
        rset->update_edited_field();
    }
    display_record();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ResultFormView::update_value(int column, const std::string &value) {
  Recordset::Ref rset(_rset.lock());
  if (rset) {
    RowId row = rset->edited_field_row();
    if (rset->count() > row && (int)row >= 0)
      rset->set_field(row, column, value);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ResultFormView::open_field_editor(int column, const std::string &type) {
  Recordset::Ref rset(_rset.lock());
  if (rset) {
    RowId row = rset->edited_field_row();
    if (row < rset->count() && (int)row >= 0)
      rset->open_field_data_editor(row, column, type);
  }
}

//----------------- ResultFormView -------------------------------------------------------------------------------------

ResultFormView::ResultFormView(bool editable)
  : mforms::AppView(false, "Result Form View", "ResultFormView", false),
    _spanel(mforms::ScrollPanelDrawBackground),
    _tbar(mforms::SecondaryToolBar),
    _editable(editable) {
  mforms::ToolBarItem *item;
  mforms::App *app = mforms::App::get();

  item = mforms::manage(new mforms::ToolBarItem(mforms::TitleItem));
  item->set_text("Form Editor");
  _tbar.add_item(item);
  _tbar.add_separator_item();

  item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
  item->set_text("Navigate:");
  _tbar.add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("First");
  item->setInternalName("first");
  item->set_tooltip("Go to the first row in the recordset.");
  item->signal_activated()->connect(std::bind(&ResultFormView::navigate, this, item));
  item->set_icon(app->get_resource_path("record_first.png"));
  _tbar.add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Back");
  item->setInternalName("back");
  item->set_tooltip("Go back one row in the recordset.");
  item->signal_activated()->connect(std::bind(&ResultFormView::navigate, this, item));
  item->set_icon(app->get_resource_path("record_back.png"));
  _tbar.add_item(item);

  _label_item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
  _label_item->set_name("Location");
  _label_item->setInternalName("location");
  _tbar.add_item(_label_item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Next");
  item->setInternalName("next");
  item->set_tooltip("Go next one row in the recordset.");
  item->signal_activated()->connect(std::bind(&ResultFormView::navigate, this, item));
  item->set_icon(app->get_resource_path("record_next.png"));
  _tbar.add_item(item);

  item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
  item->set_name("Last");
  item->setInternalName("last");
  item->set_tooltip("Go to the last row in the recordset.");
  item->signal_activated()->connect(std::bind(&ResultFormView::navigate, this, item));
  item->set_icon(app->get_resource_path("record_last.png"));
  _tbar.add_item(item);

  if (editable) {
    _tbar.add_separator_item();

    item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    item->set_text("Edit:");
    _tbar.add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_name("Delete");
    item->setInternalName("delete");
    item->set_tooltip("Delete current row from the recordset.");
    item->signal_activated()->connect(std::bind(&ResultFormView::navigate, this, item));
    item->set_icon(app->get_resource_path("record_del.png"));
    _tbar.add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_name("Last");
    item->setInternalName("last");
    item->set_tooltip("Add a new row to the recordset.");
    item->signal_activated()->connect(std::bind(&ResultFormView::navigate, this, item));
    item->set_icon(app->get_resource_path("record_add.png"));
    _tbar.add_item(item);
  }

  {
    _tbar.add_separator_item("geom_separator");

    _geom_type_item = item = mforms::manage(new mforms::ToolBarItem(mforms::SelectorItem), false);
    std::vector<std::string> options;
    options.push_back("View Geometry as WKT");
    options.push_back("View Geometry as GeoJSON");
    options.push_back("View Geometry as GML");
    options.push_back("View Geometry as KML");
    item->set_selector_items(options);
    item->signal_activated()->connect(std::bind(&ResultFormView::geom_type_changed, this));
    _tbar.add_item(item);
  }

  add(&_tbar, false, true);
  _spanel.set_back_color("#ffffff");

  add(&_spanel, true, true);
  _spanel.add(&_table);
  _table.set_column_count(2);
  _table.set_padding(12, 12, 12, 12);
  _table.set_row_spacing(8);
  _table.set_column_spacing(8);
}

//----------------------------------------------------------------------------------------------------------------------

ResultFormView::~ResultFormView() {
  if (_geom_type_item)
    _geom_type_item->release();
  _refresh_ui_connection.disconnect();
  for (std::vector<FieldView *>::const_iterator i = _fields.begin(); i != _fields.end(); ++i)
    delete *i;
}

//----------------------------------------------------------------------------------------------------------------------

void ResultFormView::geom_type_changed() {
  std::string type = _geom_type_item->get_text();
  for (std::vector<FieldView *>::const_iterator i = _fields.begin(); i != _fields.end(); ++i) {
    GeomFieldView *geom = dynamic_cast<GeomFieldView *>(*i);
    if (geom) {
      geom->set_view_type(type);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

int ResultFormView::display_record(RowId row_id) {
  Recordset::Ref rset(_rset.lock());
  if (rset)
    rset->set_edited_field(row_id, 0);
  return display_record();
}

//----------------------------------------------------------------------------------------------------------------------

int ResultFormView::display_record() {
  Recordset::Ref rset(_rset.lock());
  if (rset) {
    unsigned int c = 0;

    for (std::vector<FieldView *>::const_iterator i = _fields.begin(); i != _fields.end(); ++i, ++c) {
      std::string value;
      rset->get_raw_field(rset->edited_field_row(), c, value);
      (*i)->set_value(value, rset->is_field_null(rset->edited_field_row(), c));
    }

    _label_item->set_text(base::strfmt("%zi / %zi", rset->edited_field_row() + 1, rset->count()));
    _tbar.find_item("first")->set_enabled(rset->edited_field_row() > 0);
    _tbar.find_item("back")->set_enabled(rset->edited_field_row() > 0);

    _tbar.find_item("next")->set_enabled(rset->edited_field_row() < rset->count() - 1);
    _tbar.find_item("last")->set_enabled(rset->edited_field_row() < rset->count() - 1);
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

std::string ResultFormView::get_full_column_type(SqlEditorForm *editor, const std::string &schema,
                                                 const std::string &table, const std::string &column) {
  // we only support 5.5+ for this feature
  if (bec::is_supported_mysql_version_at_least(editor->rdbms_version(), 5, 5)) {
    std::string q = base::sqlstring(
                      "SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE table_schema = ? and table_name = ? "
                      "and column_name = ?",
                      0)
                    << schema << table << column;
    try {
      // XXX handle case where column is an alias, in that case we have to parse the query and extract the original
      // column name by hand
      sql::Dbc_connection_handler::Ref conn;
      base::RecMutexLock lock(editor->ensure_valid_aux_connection(conn));

      std::unique_ptr<sql::Statement> stmt(conn->ref->createStatement());
      std::unique_ptr<sql::ResultSet> result(stmt->executeQuery(q));
      if (result.get() && result->first())
        return result->getString(1);
    } catch (std::exception &e) {
      logException(("Exception getting column information: " + q).c_str(), e);
    }
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

void ResultFormView::init_for_resultset(Recordset::Ptr rset_ptr, SqlEditorForm *editor) {
  Recordset::Ref rset(rset_ptr.lock());
  _rset = rset_ptr;
  if (rset) {
    _refresh_ui_connection.disconnect();
    rset->refresh_ui_signal.connect([this]() { display_record(); });

    if (rset->edited_field_row() == (RowId)-1 && rset->count() > 0) {
      rset->set_edited_field(0, 0);
      if (rset->update_edited_field)
        rset->update_edited_field();
    }

    Recordset_cdbc_storage::Ref storage(std::dynamic_pointer_cast<Recordset_cdbc_storage>(rset->data_storage()));

    _table.suspend_layout();

    std::vector<Recordset_cdbc_storage::FieldInfo> &field_info(storage->field_info());
    _table.set_row_count((int)field_info.size());

    _tbar.remove_item(_geom_type_item);

    int i = 0;
    bool seen_geometry = false;
    for (std::vector<Recordset_cdbc_storage::FieldInfo>::const_iterator iter = field_info.begin();
         iter != field_info.end(); ++iter, ++i) {
      std::string full_type;

      if ((iter->type == "ENUM" || iter->type == "SET") && !iter->table.empty()) {
        full_type = get_full_column_type(editor, iter->schema, iter->table, iter->field);
      }

      FieldView *fview = FieldView::create(*iter, full_type, _editable,
                                           std::bind(&ResultFormView::update_value, this, i, std::placeholders::_1),
                                           std::bind(&ResultFormView::open_field_editor, this, i, iter->type));
      if (fview) {
        _table.add(fview->label(), 0, 1, i, i + 1, mforms::HFillFlag);
        _table.add(fview->value(), 1, 2, i, i + 1, mforms::HFillFlag | (fview->expands() ? mforms::HExpandFlag : 0));
        _fields.push_back(fview);

        if (iter->type == "GEOMETRY")
          seen_geometry = true;
      }
    }
    if (seen_geometry) {
      const std::vector<mforms::ToolBarItem *> &items(_tbar.get_items());
      int i = 0;
      for (std::vector<mforms::ToolBarItem *>::const_iterator iter = items.begin(); iter != items.end(); ++iter) {
        if ((*iter)->getInternalName() == "geom_separator")
          break;
        i++;
      }
      _tbar.insert_item(i + 1, _geom_type_item);
    }

    _table.resume_layout();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ResultFormView::updateColors() {
  _spanel.set_back_color(base::Color::getSystemColor(base::TextBackgroundColor).to_html());
}

//----------------------------------------------------------------------------------------------------------------------
