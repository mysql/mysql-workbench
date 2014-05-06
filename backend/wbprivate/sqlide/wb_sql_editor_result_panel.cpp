/*
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "wb_sql_editor_form.h"
#include "wb_sql_editor_result_panel.h"
#include "objimpl/db.query/db_query_Resultset.h"
#include "sqlide/recordset_cdbc_storage.h"
#include "grtdb/db_helpers.h"
#include "grtui/inserts_export_form.h"

#include "base/sqlstring.h"
#include "grt/parse_utils.h"
#include "base/log.h"
#include "base/boost_smart_ptr_helpers.h"

#include "mforms/utilities.h"
#include "mforms/treenodeview.h"
#include "mforms/textbox.h"
#include "mforms/label.h"
#include "mforms/box.h"
#include "mforms/table.h"
#include "mforms/tabview.h"
#include "mforms/tabswitcher.h"
#include "mforms/toolbar.h"
#include "mforms/scrollpanel.h"
#include "mforms/menubar.h"

#include "mforms/button.h"
#include "mforms/selector.h"
#include "mforms/textentry.h"

#include <algorithm>

using namespace base;

DEFAULT_LOG_DOMAIN("SqlResult")

class FieldView
{
  mforms::Label _label;

protected:

  boost::function<void (const std::string &s)> _change_callback;

public:
  FieldView(const std::string &name, const boost::function<void (const std::string &s)> &change_callback)
  : _label(name), _change_callback(change_callback)
  {
    _label.set_text_align(mforms::TopRight);
  }

  virtual ~FieldView() {}

  static FieldView *create(const Recordset_cdbc_storage::FieldInfo &field, const std::string &full_type, bool editable,
                           const boost::function<void (const std::string &s)> &callback,
                           const boost::function<void ()> &view_blob_callback);

  mforms::Label *label() { return &_label; }
  virtual mforms::View *value() = 0;
  virtual bool expands() { return false; }

  virtual void set_value(const std::string &value, bool is_null) = 0;

};


class StringFieldView : public FieldView
{
  mforms::TextEntry *_entry;
  bool _expands;

  void changed()
  {
    _change_callback(_entry->get_string_value());
  }

public:
  StringFieldView(const std::string &name, int max_length, bool editable, const boost::function<void (const std::string &s)> &change_callback)
  : FieldView(name, change_callback), _expands(false)
  {
    _entry = new mforms::TextEntry();
    _entry->set_enabled(editable);
    _entry->signal_changed()->connect(boost::bind(&StringFieldView::changed, this));
    if (max_length > 64)
      _expands = true;
    else
      _entry->set_size(std::max(max_length * 10, 60), -1);
  }

  virtual bool expands()
  {
    return _expands;
  }

  virtual ~StringFieldView()
  {
    _entry->release();
  }

  virtual mforms::View *value() { return _entry; }

  virtual void set_value(const std::string &value, bool is_null)
  {
    _entry->set_value(value);
  }
};


class SelectorFieldView : public FieldView
{
  mforms::Selector *_selector;

  void changed()
  {
    _change_callback(_selector->get_string_value());
  }

public:
  SelectorFieldView(const std::string &name, const std::list<std::string> &items,
                    bool editable, const boost::function<void (const std::string &s)> &change_callback)
  : FieldView(name, change_callback)
  {
    _selector = new mforms::Selector();
    _selector->add_items(items);
    _selector->set_enabled(editable);
    _selector->signal_changed()->connect(boost::bind(&SelectorFieldView::changed, this));
  }

  virtual ~SelectorFieldView()
  {
    _selector->release();
  }

  virtual mforms::View *value() { return _selector; }

  virtual void set_value(const std::string &value, bool is_null)
  {
    _selector->set_value(value);
  }
};


class SetFieldView : public FieldView
{
  mforms::TreeNodeView _tree;

  void changed()
  {
    std::string value;

    for (int c = _tree.count(), i = 0; i < c; i++)
    {
      mforms::TreeNodeRef node = _tree.node_at_row(i);
      if (node->get_bool(0))
      {
        if (!value.empty())
          value.append(",");
        value.append(node->get_string(1));
      }
    }
    _change_callback(value);
  }

public:
  SetFieldView(const std::string &name, const std::list<std::string> &items,
               bool editable, const boost::function<void (const std::string &s)> &change_callback)
  : FieldView(name, change_callback), _tree(mforms::TreeFlatList|mforms::TreeNoHeader)
  {
    _tree.add_column(mforms::CheckColumnType, "", 30, true);
    _tree.add_column(mforms::StringColumnType, "", 200, false);
    _tree.end_columns();

    for (std::list<std::string>::const_iterator i = items.begin(); i != items.end(); ++i)
    {
      mforms::TreeNodeRef node = _tree.add_node();
      node->set_string(1, *i);
    }

    int height = (int)items.size() * 20;
    _tree.set_size(250, height > 100 ? 100 : height);

    _tree.set_enabled(editable);
    _tree.signal_changed()->connect(boost::bind(&SetFieldView::changed, this));
  }

  virtual mforms::View *value() { return &_tree; }

  virtual void set_value(const std::string &value, bool is_null)
  {
    std::vector<std::string> l(base::split_token_list(value, ','));

    for (int c = _tree.count(), i = 0; i < c; i++)
    {
      mforms::TreeNodeRef node = _tree.node_at_row(i);
      if (std::find(l.begin(), l.end(), node->get_string(1)) != l.end())
        node->set_bool(0, true);
      else
        node->set_bool(0, false);
    }
  }
};


class TextFieldView : public FieldView
{
  mforms::TextBox *_tbox;

  void changed()
  {
    _change_callback(_tbox->get_string_value());
  }

public:
  TextFieldView(const std::string &name, bool editable, const boost::function<void (const std::string &s)> &change_callback)
  : FieldView(name, change_callback)
  {
    _tbox = new mforms::TextBox(mforms::BothScrollBars);
    _tbox->set_enabled(editable);
    _tbox->signal_changed()->connect(boost::bind(&TextFieldView::changed, this));
    _tbox->set_size(-1, 60);
  }

  virtual bool expands()
  {
    return true;
  }

  virtual ~TextFieldView()
  {
    _tbox->release();
  }

  virtual mforms::View *value() { return _tbox; }

  virtual void set_value(const std::string &value, bool is_null)
  {
    _tbox->set_value(value);
  }
};


class BlobFieldView : public FieldView
{
  mforms::Box _box;
  mforms::Label _blob;

  void changed()
  {
  }

public:
  BlobFieldView(const std::string &name, bool editable, const boost::function<void (const std::string &s)> &change_callback,
                const boost::function<void ()> &view_callback)
  : FieldView(name, change_callback), _box(true), _blob("BLOB")
  {
    _box.set_spacing(8);
    _box.add(&_blob, false, true);
    mforms::Button *b = mforms::manage(new mforms::Button());
    b->enable_internal_padding(false);
    b->signal_clicked()->connect(view_callback);
    b->set_text("View...");
    _box.add(b, false, true);
  }

  virtual mforms::View *value() { return &_box; }

  virtual void set_value(const std::string &value, bool is_null)
  {
    _blob.set_text(is_null ? "NULL" : "BLOB");
  }
};


static std::list<std::string> parse_enum_definition(const std::string &full_type)
{
  std::list<std::string> l;
  std::string::size_type b, e;

  b = full_type.find('(');
  e = full_type.rfind(')');
  if (b != std::string::npos && e != std::string::npos && e > b)
  {
    bec::tokenize_string_list(full_type.substr(b+1, e-b-1), '\'', true, l);
    for (std::list<std::string>::iterator i = l.begin(); i != l.end(); ++i)
    {
      // strip quotes
      *i = i->substr(1, i->size()-2);
    }
  }
  return l;
}


inline std::string format_label(const std::string &label)
{
  std::string flabel = label + ":";

  if (g_ascii_isalpha(flabel[0]))
    flabel = g_ascii_toupper(flabel[0]) + flabel.substr(1);

  return flabel;
}


FieldView *FieldView::create(const Recordset_cdbc_storage::FieldInfo &field, const std::string &full_type, bool editable,
                             const boost::function<void (const std::string &s)> &callback,
                             const boost::function<void ()> &view_blob_callback)
{
  if (field.type == "VARCHAR")
  {
    if (field.display_size > 40)
    {
      TextFieldView *text = new TextFieldView(format_label(field.field), editable, callback);
      if (field.display_size > 1000)
        text->value()->set_size(-1, 200);
      return text;
    }
    else
      return new StringFieldView(format_label(field.field), field.display_size, editable, callback);
  }
  else if (field.type == "TEXT")
  {
    return new TextFieldView(format_label(field.field), editable, callback);
  }
  else if (field.type == "BLOB")
  {
    return new BlobFieldView(format_label(field.field), editable, callback, view_blob_callback);
  }
  else if (field.type == "ENUM" && !full_type.empty())
  {
    return new SelectorFieldView(format_label(field.field), parse_enum_definition(full_type), editable, callback);
  }
  else if (field.type == "SET" && !full_type.empty())
  {
    return new SetFieldView(format_label(field.field), parse_enum_definition(full_type), editable, callback);
  }
  else
    return new StringFieldView(format_label(field.field), field.display_size, editable, callback);
  return NULL;
}


class ResultFormView : public mforms::Box
{
  Recordset::Ptr _rset;

  mforms::ScrollPanel _spanel;
  mforms::Table _table;
  std::vector<FieldView*> _fields;
  mforms::ToolBar _tbar;
  mforms::ToolBarItem *_label_item;

  bool _editable;
  boost::signals2::connection _refresh_ui_connection;

  void navigate(mforms::ToolBarItem *item)
  {
    std::string name = item->get_name();
    Recordset::Ref rset(_rset.lock());
    if (rset)
    {
      ssize_t row = rset->edited_field_row();
      if (row < 0)
        return;

      if (name == "delete")
      {
        rset->delete_node(row);
      }
      else if (name == "back")
      {
        row--;
        if (row < 0)
          row = 0;
        rset->set_edited_field(row, rset->edited_field_column());
        if (rset->update_edited_field)
          rset->update_edited_field();
      }
      else if (name == "first")
      {
        row = 0;
        rset->set_edited_field(row, rset->edited_field_column());
        if (rset->update_edited_field)
          rset->update_edited_field();
      }
      else if (name == "next")
      {
        row++;
        if (row >= (ssize_t)rset->count())
          row = rset->count()-1;
        rset->set_edited_field(row, rset->edited_field_column());
        if (rset->update_edited_field)
          rset->update_edited_field();
      }
      else if (name == "last")
      {
        row = rset->count()-1;
        rset->set_edited_field(row, rset->edited_field_column());
        if (rset->update_edited_field)
          rset->update_edited_field();
      }
      display_record();
    }
  }


  void update_value(int column, const std::string &value)
  {
    Recordset::Ref rset(_rset.lock());
    if (rset)
    {
      size_t row = rset->edited_field_row();
      if (rset->count() > row)
        rset->set_field(row, column, value);
    }
  }


  void open_field_editor(int column)
  {
    Recordset::Ref rset(_rset.lock());
    if (rset)
    {
      size_t row = rset->edited_field_row();
      if (row < rset->count())
        rset->open_field_data_editor(row, column);
    }
  }

public:
  ResultFormView(bool editable)
  : mforms::Box(false), _spanel(mforms::ScrollPanelDrawBackground), _tbar(mforms::SecondaryToolBar),
  _editable(editable)
  {
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
    item->set_name("first");
    item->set_tooltip("Go to the first row in the recordset.");
    item->signal_activated()->connect(boost::bind(&ResultFormView::navigate, this, item));
    item->set_icon(app->get_resource_path("record_first.png"));
    _tbar.add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_name("back");
    item->set_tooltip("Go back one row in the recordset.");
    item->signal_activated()->connect(boost::bind(&ResultFormView::navigate, this, item));
    item->set_icon(app->get_resource_path("record_back.png"));
    _tbar.add_item(item);

    _label_item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
    _label_item->set_name("location");
    _tbar.add_item(_label_item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_name("next");
    item->set_tooltip("Go next one row in the recordset.");
    item->signal_activated()->connect(boost::bind(&ResultFormView::navigate, this, item));
    item->set_icon(app->get_resource_path("record_next.png"));
    _tbar.add_item(item);

    item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
    item->set_name("last");
    item->set_tooltip("Go to the last row in the recordset.");
    item->signal_activated()->connect(boost::bind(&ResultFormView::navigate, this, item));
    item->set_icon(app->get_resource_path("record_last.png"));
    _tbar.add_item(item);

    if (editable)
    {
      _tbar.add_separator_item();

      item = mforms::manage(new mforms::ToolBarItem(mforms::LabelItem));
      item->set_text("Edit:");
      _tbar.add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      item->set_name("delete");
      item->set_tooltip("Delete current row from the recordset.");
      item->signal_activated()->connect(boost::bind(&ResultFormView::navigate, this, item));
      item->set_icon(app->get_resource_path("record_del.png"));
      _tbar.add_item(item);

      item = mforms::manage(new mforms::ToolBarItem(mforms::ActionItem));
      item->set_name("last");
      item->set_tooltip("Add a new row to the recordset.");
      item->signal_activated()->connect(boost::bind(&ResultFormView::navigate, this, item));
      item->set_icon(app->get_resource_path("record_add.png"));
      _tbar.add_item(item);
    }

    add(&_tbar, false, true);
    _spanel.set_back_color(mforms::App::get()->get_system_color(mforms::SystemColorContainer).to_html());

    add(&_spanel, true, true);
    _spanel.add(&_table);
    _table.set_column_count(2);
    _table.set_padding(12, 12, 12, 12);
    _table.set_row_spacing(8);
    _table.set_column_spacing(8);
  }

  mforms::ToolBar *get_toolbar() { return &_tbar; }

  virtual ~ResultFormView()
  {
    _refresh_ui_connection.disconnect();
    for (std::vector<FieldView*>::const_iterator i = _fields.begin(); i != _fields.end(); ++i)
      delete *i;
  }

  int display_record()
  {
    Recordset::Ref rset(_rset.lock());
    if (rset)
    {
      int c = 0;

      for (std::vector<FieldView*>::const_iterator i = _fields.begin(); i != _fields.end(); ++i, ++c)
      {
        std::string value;
        rset->get_field_repr_no_truncate(rset->edited_field_row(), c, value);
        (*i)->set_value(value, rset->is_field_null(rset->edited_field_row(), c));
      }

      _label_item->set_text(base::strfmt("%zi / %zi", rset->edited_field_row()+1, rset->count()));
      _tbar.find_item("first")->set_enabled(rset->edited_field_row() > 0);
      _tbar.find_item("back")->set_enabled(rset->edited_field_row() > 0);

      _tbar.find_item("next")->set_enabled(rset->edited_field_row() < rset->count()-1);
      _tbar.find_item("last")->set_enabled(rset->edited_field_row() < rset->count()-1);
    }
    return 0;
  }

  std::string get_full_column_type(SqlEditorForm *editor, const std::string &schema, const std::string &table, const std::string &column)
  {
    // we only support 5.5+ for this feature
    if (bec::is_supported_mysql_version_at_least(editor->rdbms_version(), 5, 5))
    {
      std::string q = base::sqlstring("SELECT COLUMN_TYPE FROM INFORMATION_SCHEMA.COLUMNS WHERE table_schema = ? and table_name = ? and column_name = ?", 0) << schema << table << column;
      try
      {
        // XXX handle case where column is an alias, in that case we have to parse the query and extract the original column name by hand
        sql::Dbc_connection_handler::Ref conn;
        base::RecMutexLock lock(editor->ensure_valid_aux_connection(conn));

        std::auto_ptr<sql::Statement> stmt(conn->ref->createStatement());
        std::auto_ptr<sql::ResultSet> result(stmt->executeQuery(q));
        if (result.get() && result->first())
          return result->getString(1);
      }
      catch (std::exception &e)
      {
        log_exception(("Exception getting column information: "+q).c_str(), e);
      }
    }
    return "";
  }

  void init_for_resultset(Recordset::Ptr rset_ptr, SqlEditorForm *editor)
  {
    Recordset::Ref rset(rset_ptr.lock());
    _rset = rset_ptr;
    if (rset)
    {
      _refresh_ui_connection.disconnect();
      rset->refresh_ui_signal.connect(boost::bind(&ResultFormView::display_record, this));

      size_t cols = rset->get_column_count();
      _table.set_row_count((int)cols);

      if (rset->count() > 0)
      {
        rset->set_edited_field(0, 0);
        if (rset->update_edited_field)
          rset->update_edited_field();
      }

      Recordset_cdbc_storage::Ref storage(boost::dynamic_pointer_cast<Recordset_cdbc_storage>(rset->data_storage()));

      std::vector<Recordset_cdbc_storage::FieldInfo> &field_info(storage->field_info());

      int i = 0;
      for (std::vector<Recordset_cdbc_storage::FieldInfo>::const_iterator iter = field_info.begin();
           iter != field_info.end(); ++iter, ++i)
      {
        std::string full_type;

        if ((iter->type == "ENUM" || iter->type == "SET") && !iter->table.empty())
        {
          full_type = get_full_column_type(editor, iter->schema, iter->table, iter->field);
        }

        FieldView *fview = FieldView::create(*iter, full_type, _editable,
                                             boost::bind(&ResultFormView::update_value, this, i, _1),
                                             boost::bind(&ResultFormView::open_field_editor, this, i));
        if (fview)
        {
          _table.add(fview->label(), 0, 1, i, i+1, mforms::HFillFlag);
          _table.add(fview->value(), 1, 2, i, i+1, mforms::HFillFlag | (fview->expands() ? mforms::HExpandFlag : 0));
          _fields.push_back(fview);
        }
      }
    }
  }
};





SqlEditorResult::SqlEditorResult(SqlEditorForm *owner, Recordset::Ref rset)
: mforms::Box(true), _owner(owner), _rset(rset), _column_info_tab(-1), _query_stats_tab(-1), _switcher(NULL)
{
  _result_grid = NULL;
  _column_info_menu = NULL;
  _column_info_created = false;
  _query_stats_created = false;
  _form_view_created = false;

  _tabview = mforms::manage(new mforms::TabView(mforms::TabViewTabless));
  add(_tabview, true, true);

  _switcher = mforms::manage(new mforms::TabSwitcher(mforms::VerticalIconSwitcher));
  _switcher->attach_to_tabview(_tabview);
  _switcher->set_collapsed(_owner->grt_manager()->get_app_option_int("Recordset:SwitcherCollapsed", 0) != 0);

  add(_switcher, false, true);

  
  rset->get_toolbar()->find_item("record_export")->signal_activated()->connect(boost::bind(&SqlEditorResult::show_export_recordset, this));
  if (rset->get_toolbar()->find_item("record_import"))
    rset->get_toolbar()->find_item("record_import")->signal_activated()->connect(boost::bind(&SqlEditorResult::show_import_recordset, this));
  
  _switcher->signal_changed()->connect(boost::bind(&SqlEditorResult::switch_tab, this));
  _switcher->signal_collapse_changed()->connect(boost::bind(&SqlEditorResult::switcher_collapsed, this));
}


SqlEditorResult::~SqlEditorResult()
{
  delete _column_info_menu;
}

SqlEditorResult::Ref SqlEditorResult::create(SqlEditorForm *owner, Recordset::Ref rset)
{
  return Ref(new SqlEditorResult(owner, rset));
}


Recordset::Ref SqlEditorResult::recordset() const
{
  return _rset.lock();
}


std::string SqlEditorResult::caption() const
{
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs, "")
  {
    return rs->caption();
  }
}


void SqlEditorResult::switch_tab()
{
  int tab = _tabview->get_active_tab();
  if (tab == _column_info_tab && !_column_info_created)
  {
    _column_info_created = true;
    create_column_info_panel();
  }
  else if (tab == _query_stats_tab && !_query_stats_created)
  {
    _query_stats_created = true;
    create_query_stats_panel();
  }
  else if (tab == _form_result_tab)
  {
    if (!_form_view_created)
    {
      _form_view_created = true;
      _form_result_view->init_for_resultset(_rset, _owner);
    }
    _form_result_view->display_record();
  }
  else if (tab == _result_grid_tab)
  {

  }
}


void SqlEditorResult::add_switch_toggle_toolbar_item(mforms::ToolBar *tbar)
{
  _collapse_toggled_sig.disconnect();
  mforms::App *app = mforms::App::get();
  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::ExpanderItem)));
  mforms::ToolBarItem *item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("sidetoggle");
  item->set_icon(app->get_resource_path("output_type-toggle-on.png"));
  item->set_alt_icon(app->get_resource_path("output_type-toggle-off.png"));
  item->signal_activated()->connect(boost::bind(&SqlEditorResult::toggle_switcher_collapsed, this));
  item->set_checked(!_switcher->get_collapsed());
  tbar->add_item(item);
  _collapse_toggled_sig = _collapse_toggled.connect(boost::bind(&mforms::ToolBarItem::set_checked, item, _1));
}


void SqlEditorResult::switcher_collapsed()
{
  bool state = _switcher->get_collapsed();
  for (std::list<mforms::ToolBar*>::const_iterator it = _toolbars.begin(); it != _toolbars.end(); ++it)
  {
    (*it)->find_item("sidetoggle")->set_checked(state);
  }
  relayout();
  
  bec::GRTManager *grtm = _owner->grt_manager();
  grtm->set_app_option("Recordset:SwitcherCollapsed", grt::IntegerRef(state?1:0));
}


void SqlEditorResult::show_export_recordset()
{
  bec::GRTManager *grtm = _owner->grt_manager();
  try
  {
    RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR (Recordset, _rset, rs)
    {
      grt::ValueRef option(grtm->get_app_option("Recordset:LastExportPath"));
      std::string path = option.is_valid() ? grt::StringRef::cast_from(option) : "";
      option = grtm->get_app_option("Recordset:LastExportExtension");
      std::string extension = option.is_valid() ? grt::StringRef::cast_from(option) : "";
      InsertsExportForm exporter(0/*mforms::Form::main_form()*/, rs_ref, extension);
      exporter.set_title(_("Export Resultset"));
      if (!path.empty())
        exporter.set_path(path);
      path = exporter.run();
      if (path.empty())
        grtm->replace_status_text(_("Export resultset canceled"));
      else
      {
        grtm->replace_status_text(strfmt(_("Exported resultset to %s"), path.c_str()));
        grtm->set_app_option("Recordset:LastExportPath", grt::StringRef(path));
        extension = base::extension(path);
        if (!extension.empty() && extension[0] == '.')
          extension = extension.substr(1);
        if (!extension.empty())
          grtm->set_app_option("Recordset:LastExportExtension", grt::StringRef(extension));
      }
    }
  }
  catch (const std::exception &exc)
  {
    mforms::Utilities::show_error("Error exporting recordset", exc.what(), "OK");
  }
}


void SqlEditorResult::show_import_recordset()
{
  bec::GRTManager *grtm = _owner->grt_manager();
  try
  {
    RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR (Recordset, _rset, rs)
    RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Sql_editor, dynamic_cast<SqlEditorForm::RecordsetData*>(rs->client_data())->editor, editor)
    {
      grt::BaseListRef args(grtm->get_grt());

      db_query_ResultsetRef rset;
      db_query_QueryEditorRef qeditor(db_query_QueryEditorRef::cast_from(editor->grtobj()));
      for (size_t c = qeditor->resultsets().count(), i = 0; i < c; i++)
      {
        if (dynamic_cast<WBRecordsetResultset*>(qeditor->resultsets()[i]->get_data())->recordset.get() == rs)
        {
          rset = qeditor->resultsets()[i];
          break;
        }
      }

      if (rset.is_valid())
      {
        args.ginsert(rset);
        grt::Module *module = grtm->get_grt()->get_module("SQLIDEUtils");
        if (module)
          module->call_function("importRecordsetDataFromFile", args);
      }
    }
  }
  catch (const std::exception &exc)
  {
    mforms::Utilities::show_error("Error importing recordset", exc.what(), "OK");
  }
}


void SqlEditorResult::toggle_switcher_collapsed()
{
  bool flag = !_switcher->get_collapsed();
  _switcher->set_collapsed(flag);
  _collapse_toggled(flag);
}


void SqlEditorResult::dock_result_grid(mforms::View *view)
{
  mforms::App *app = mforms::App::get();
  _result_grid = view;
  view->set_name("result-grid-wrapper");

  {
    mforms::Box *box = mforms::manage(new mforms::Box(false));
    box->set_name("resultset-host");
    mforms::ToolBar *tbar = _rset.lock()->get_toolbar();
    tbar->set_name("resultset-toolbar");
    _toolbars.push_back(tbar);

    add_switch_toggle_toolbar_item(tbar);
    
    box->add(tbar, false, true);
    box->add(view, true, true);
    
    _result_grid_tab = _tabview->add_page(box, "");
    _switcher->add_item("Result\nGrid", "", app->get_resource_path("output_type-resultset.png"), "");
  }
  {
    bool editable = false;
    if (Recordset::Ref rset = _rset.lock())
      editable = !rset->is_readonly();
    _form_result_view = mforms::manage(new ResultFormView(editable));
    add_switch_toggle_toolbar_item(_form_result_view->get_toolbar());
    _form_result_tab = _tabview->add_page(_form_result_view, "");
    _switcher->add_item("Form\nEditor", "", app->get_resource_path("output_type-formeditor.png"), "");
  }
  {
    _column_info_box = mforms::manage(new mforms::Box(false));
    _column_info_tab = _tabview->add_page(_column_info_box, "");
    _column_info_box->set_back_color("#ffffff");
    _switcher->add_item("Field\nTypes", "", app->get_resource_path("output_type-fieldtypes.png"), "");
  }
  {
    _query_stats_box = mforms::manage(new mforms::Box(false));
    _query_stats_tab = _tabview->add_page(_query_stats_box, "");
    _query_stats_box->set_back_color("#ffffff");
    _switcher->add_item("Query\nStats", "", app->get_resource_path("output_type-querystats.png"), "");
  }
}


static std::string format_ps_time(boost::int64_t t)
{
  int hours, mins;
  double secs;

  secs = t / 1000000000000.0;
  mins = int(secs / 60) % 60;
  hours = mins / 3600;

  return base::strfmt("%i:%02i:%02.8f", hours, mins, secs);
}

static mforms::Label *bold_label(const std::string text)
{
  mforms::Label *l = mforms::manage(new mforms::Label(text));
  l->set_style(mforms::BoldStyle);
  return l;
}


void SqlEditorResult::copy_column_info(mforms::TreeNodeView *tree)
{
  std::list<mforms::TreeNodeRef> nodes(tree->get_selection());
  std::string text;

  for (std::list<mforms::TreeNodeRef>::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
  {
    text.append(base::strfmt("%i", (*node)->get_int(0)));
    for (int i= 1; i < tree->get_column_count(); i++)
    {
      if (i >= 1 && i <= 4)
        text.append(",").append((*node)->get_string(i));
      else
        text.append(",").append(base::strfmt("%i", (*node)->get_int(i)));
    }
    text.append("\n");
  }
  mforms::Utilities::set_clipboard_text(text);
}

void SqlEditorResult::copy_column_info_name(mforms::TreeNodeView *tree)
{
  std::list<mforms::TreeNodeRef> nodes(tree->get_selection());
  std::string text;

  for (std::list<mforms::TreeNodeRef>::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
  {
    text.append((*node)->get_string(1)).append("\n");
  }
  mforms::Utilities::set_clipboard_text(text);
}


void SqlEditorResult::create_column_info_panel()
{
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs)
  {
    Recordset_cdbc_storage::Ref storage(boost::dynamic_pointer_cast<Recordset_cdbc_storage>(rs->data_storage()));

    mforms::Box *box = _column_info_box;
    mforms::ToolBar *tbar = mforms::manage(new mforms::ToolBar(mforms::SecondaryToolBar));
    _toolbars.push_back(tbar);
    mforms::ToolBarItem *item;
    item = mforms::manage(new mforms::ToolBarItem(mforms::TitleItem));
    item->set_text("Field Types");
    tbar->add_item(item);

    add_switch_toggle_toolbar_item(tbar);

    box->add(tbar, false, true);
    
    if (_owner->collect_field_info())
    {
      mforms::TreeNodeView *tree = mforms::manage(new mforms::TreeNodeView(mforms::TreeFlatList|mforms::TreeAltRowColors|mforms::TreeShowRowLines|mforms::TreeShowColumnLines|mforms::TreeNoBorder));
      tree->add_column(mforms::IntegerColumnType, "#", 50);
      tree->add_column(mforms::StringColumnType, "Field", 130);
      tree->add_column(mforms::StringColumnType, "Schema", 130);
      tree->add_column(mforms::StringColumnType, "Table", 130);
      tree->add_column(mforms::StringColumnType, "Type", 200);
      tree->add_column(mforms::IntegerColumnType, "Display Size", 80);
      tree->add_column(mforms::IntegerColumnType, "Precision", 80);
      tree->add_column(mforms::IntegerColumnType, "Scale", 80);
      tree->end_columns();

      tree->set_selection_mode(mforms::TreeSelectMultiple);

      _column_info_menu = new mforms::ContextMenu();
      _column_info_menu->add_item_with_title("Copy", boost::bind(&SqlEditorResult::copy_column_info, this, tree));
      _column_info_menu->add_item_with_title("Copy Name", boost::bind(&SqlEditorResult::copy_column_info_name, this, tree));
      tree->set_context_menu(_column_info_menu);

      int i = 0;
      std::vector<Recordset_cdbc_storage::FieldInfo> &field_info(storage->field_info());
      for (std::vector<Recordset_cdbc_storage::FieldInfo>::const_iterator iter = field_info.begin();
           iter != field_info.end(); ++iter)
      {
        mforms::TreeNodeRef node = tree->add_node();
        node->set_int(0, ++i);
        node->set_string(1, iter->field);
        node->set_string(2, iter->schema);
        node->set_string(3, iter->table);
        node->set_string(4, iter->type);
        node->set_int(5, iter->display_size);
        node->set_int(6, iter->precision);
        node->set_int(7, iter->scale);
      }
      box->add(tree, true, true);
    }
    else
    {
      mforms::Label *label = mforms::manage(new mforms::Label("To get field type information for query results, enable Query -> Collect Resultset Field Metadata"));
      label->set_style(mforms::BigBoldStyle);
      box->add(label, true, true);
    }
  }
}


void SqlEditorResult::create_query_stats_panel()
{
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs)
  {
    SqlEditorForm::RecordsetData* rsdata = dynamic_cast<SqlEditorForm::RecordsetData*>(rs->client_data());
    std::string info;
    
    mforms::ScrollPanel *spanel = mforms::manage(new mforms::ScrollPanel());
    mforms::Table *table = mforms::manage(new mforms::Table());
    table->set_row_count(2);
    table->set_column_count(2);
    spanel->set_back_color("#ffffff");

    mforms::ToolBar *tbar = mforms::manage(new mforms::ToolBar(mforms::SecondaryToolBar));
    _toolbars.push_back(tbar);
    mforms::ToolBarItem *item;
    item = mforms::manage(new mforms::ToolBarItem(mforms::TitleItem));
    item->set_text("Query Statistics");
    tbar->add_item(item);

    add_switch_toggle_toolbar_item(tbar);

    _query_stats_box->add(tbar, false, true);
    
    mforms::Box *box = mforms::manage(new mforms::Box(false));
    box->set_padding(8);
    // show basic stats
    box->add(bold_label("Timing (as measured at client side):"), false, true);
    info.clear();
    info = strfmt("Execution time: %s\n", format_ps_time(boost::int64_t(rsdata->duration * 1000000000000.0)).c_str());
    box->add(mforms::manage(new mforms::Label(info)), false, true);
    
    // if we're in a server with PS, show some extra PS goodies
    std::map<std::string, boost::int64_t> &ps_stats(rsdata->ps_stat_info);

    if (ps_stats.empty())
    {
      if (!rsdata->ps_stat_error.empty())
        box->add(bold_label(rsdata->ps_stat_error), false, true);
      else
        box->add(bold_label("For more details, enable \"statement instrumentation\" for the Performance Schema and \"Query -> Collect Performance Schema Stats\"."), false, true);
        
      table->add(box, 0, 1, 1, 2, mforms::HFillFlag|mforms::HExpandFlag);
    }
    else
    {
      box->add(bold_label("Timing (as measured by the server):"), false, true);
      info.clear();
      info.append(strfmt("Execution time: %s\n", format_ps_time(ps_stats["TIMER_WAIT"]).c_str()));
      info.append(strfmt("Table lock wait time: %s\n", format_ps_time(ps_stats["LOCK_TIME"]).c_str()));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Errors:"), false, true);
      info.clear();
      info.append(strfmt("Had Errors: %s\n", ps_stats["ERRORS"] ? "YES" : "NO"));
      info.append(strfmt("Warnings: %" PRId64 "\n", ps_stats["WARNINGS"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Rows Processed:"), false, true);
      info.clear();
      info.append(strfmt("Rows affected: %" PRId64 "\n", ps_stats["ROWS_AFFECTED"])),
      info.append(strfmt("Rows sent to client: %" PRId64 "\n", ps_stats["ROWS_SENT"]));
      info.append(strfmt("Rows examined: %" PRId64 "\n", ps_stats["ROWS_EXAMINED"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Temporary Tables:"), false, true);
      info.clear();
      info.append(strfmt("Temporary disk tables created: %" PRId64 "\n", ps_stats["CREATED_TMP_DISK_TABLES"]));
      info.append(strfmt("Temporary tables created: %" PRId64 "\n", ps_stats["CREATED_TMP_TABLES"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);
      
      table->add(box, 0, 1, 1, 2, mforms::HFillFlag|mforms::HExpandFlag);

      box = mforms::manage(new mforms::Box(false));
      box->set_padding(8);

      box->add(bold_label("Joins per Type:"), false, true);
      info.clear();
      info.append(strfmt("Full table scans (Select_scan): %" PRId64 "\n", ps_stats["SELECT_SCAN"]));
      info.append(strfmt("Joins using table scans (Select_full_join): %" PRId64 "\n", ps_stats["SELECT_FULL_JOIN"]));
      info.append(strfmt("Joins using range search (Select_full_range_join): %" PRId64 "\n", ps_stats["SELECT_FULL_RANGE_JOIN"]));
      info.append(strfmt("Joins with range checks (Select_range_check): %" PRId64 "\n", ps_stats["SELECT_RANGE_CHECK"]));
      info.append(strfmt("Joins using range (Select_range): %" PRId64 "\n", ps_stats["SELECT_RANGE"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Sorting:"), false, true);
      info.clear();
      info.append(strfmt("Sorted rows (Sort_rows): %" PRId64 "\n", ps_stats["SORT_ROWS"]));
      info.append(strfmt("Sort merge passes (Sort_merge_passes): %" PRId64 "\n", ps_stats["SORT_MERGE_PASSES"]));
      info.append(strfmt("Sorts with ranges (Sort_range): %" PRId64 "\n", ps_stats["SORT_RANGE"]));
      info.append(strfmt("Sorts with table scans (Sort_scan): %" PRId64 "\n", ps_stats["SORT_SCAN"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Index Usage:"), false, true);
      info.clear();
      if (ps_stats["NO_INDEX_USED"])
        info.append("No Index used");
      else
        info.append("At least one Index was used");
      if (ps_stats["NO_GOOD_INDEX_USED"])
        info.append("No good index used");        
      box->add(mforms::manage(new mforms::Label(info)), false, true);
      
      table->add(box, 1, 2, 1, 2, mforms::HFillFlag|mforms::HExpandFlag);
    }
    spanel->add(table);
    _query_stats_box->add(spanel, true, true);
  }
}
