/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "wb_sql_editor_panel.h"
#include "wb_sql_editor_result_panel.h"
#include "spatial_data_view.h"
#include "result_form_view.h"
#include "objimpl/db.query/db_query_Resultset.h"
#include "sqlide/recordset_cdbc_storage.h"
#include "grtdb/db_helpers.h"
#include "grtui/inserts_export_form.h"
#include "objimpl/wrapper/mforms_ObjectReference_impl.h"
#include "objimpl/db.query/db_query_Resultset.h"
#include "objimpl/db.query/db_query_EditableResultset.h"

#include "sqlide/column_width_cache.h"

#include "base/sqlstring.h"
#include "grt/parse_utils.h"
#include "grt/spatial_handler.h"
#include "base/log.h"
#include "base/string_utilities.h"
#include "base/boost_smart_ptr_helpers.h"

#include "mforms/utilities.h"
#include "mforms/treeview.h"
#include "mforms/label.h"
#include "mforms/box.h"
#include "mforms/table.h"
#include "mforms/tabview.h"
#include "mforms/tabswitcher.h"
#include "mforms/toolbar.h"
#include "mforms/scrollpanel.h"
#include "mforms/menubar.h"
#include "mforms/gridview.h"
#include "mforms/imagebox.h"

#include "mforms/button.h"
#include "mforms/selector.h"
#include "mforms/textentry.h"

#include <algorithm>

using namespace base;

DEFAULT_LOG_DOMAIN("SqlResult")

//----------------------------------------------------------------------------------------------------------------------

class SqlEditorResult::DockingDelegate : public mforms::TabViewDockingPoint {
  mforms::TabSwitcher *_switcher;

public:
  DockingDelegate(mforms::TabView *tabview, mforms::TabSwitcher *switcher, const std::string &name)
    : mforms::TabViewDockingPoint(tabview, name), _switcher(switcher) {
  }

  virtual void undock_view(AppView *view) {
    for (int i = 0; i < view_count(); i++)
      if (view_at_index(i) == view) {
        _switcher->remove_item(i);
        break;
      }

    mforms::TabViewDockingPoint::undock_view(view);
  }

  virtual void dock_view(AppView *view, const std::string &icon, int arg2) {
    mforms::TabViewDockingPoint::dock_view(view, icon, arg2);
    _switcher->add_item(view->get_title(), "", icon, "");
  }
};

//----------------------------------------------------------------------------------------------------------------------

SqlEditorResult::SqlEditorResult(SqlEditorPanel *owner)
  : mforms::AppView(true, "Query Result", "QueryResult", false),
    _owner(owner),
    _tabview(mforms::TabViewTabless),
    _switcher(mforms::VerticalIconSwitcher),
    _tabdock_delegate(new DockingDelegate(&_tabview, &_switcher, std::string("SqlResultPanel"))),
    _tabdock(_tabdock_delegate, true),
    _pinned(false) {

  _result_grid = NULL;
  _grid_header_menu = NULL;
  _column_info_menu = NULL;
  _column_info_created = false;
  _query_stats_created = false;
  _form_view_created = false;

  _column_info_box = nullptr;
  _query_stats_box = nullptr;
  _execution_plan_placeholder = nullptr;
  _query_stats_panel = nullptr;
  _form_result_view = nullptr;

  _spatial_view_initialized = false;
  _spatial_result_view = NULL;

  add(&_tabview, true, true);

  _switcher.set_name("Resultset View Switcher");
  _switcher.attach_to_tabview(&_tabview);
  _switcher.set_collapsed(bec::GRTManager::get()->get_app_option_int("Recordset:SwitcherCollapsed", 0) != 0);

  add(&_switcher, false, true);
  _switcher.signal_changed()->connect(std::bind(&SqlEditorResult::switch_tab, this));
  _switcher.signal_collapse_changed()->connect(std::bind(&SqlEditorResult::switcher_collapsed, this));

  // put a placeholder for the resultset, which will be replaced when a resultset is actually available
  _resultset_placeholder = mforms::manage(new mforms::AppView(false, "Result Grid Placeholder", "ResultGridPlaceholder", false));
  _resultset_placeholder->set_title("Result\nGrid");
  _resultset_placeholder->set_identifier("result_grid");
  _tabdock.dock_view(_resultset_placeholder, "output_type-resultset.png");
  _tabdock.set_name("Resultset Views");

  {
    db_query_QueryEditorRef editor(owner->grtobj());
    _grtobj = db_query_ResultPanelRef(grt::Initialized);
    _grtobj->dockingPoint(mforms_to_grt(&_tabdock));
  }

  set_on_close(std::bind(&SqlEditorResult::can_close, this));

  NotificationCenter::get()->add_observer(this, "GNColorsChanged");
  updateColors();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info) {
  if (name == "GNColorsChanged") {
    updateColors();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::updateColors() {
  std::string background = base::Color::getSystemColor(TextBackgroundColor).to_html();
  if (_resultset_placeholder != nullptr)
    _resultset_placeholder->set_back_color(background);
  if (_column_info_box != nullptr)
    _column_info_box->set_back_color(background);
  if (_query_stats_box != nullptr)
    _query_stats_box->set_back_color(background);
  if (_execution_plan_placeholder != nullptr)
    _execution_plan_placeholder->set_back_color(background);
  if (_query_stats_panel != nullptr)
    _query_stats_panel->set_back_color(background);
  if (_form_result_view != nullptr)
    _form_result_view->updateColors();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::reset_sorting() {
  Recordset::Ref rset(recordset());
  if (rset)
    rset->sort_by(0, 0, false);
  if (_result_grid) {
    for (int i = 0; i < _result_grid->get_column_count(); i++)
      _result_grid->set_column_header_indicator(i, mforms::NoIndicator);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::copy_column_name() {
  int column = _result_grid->get_clicked_header_column();
  Recordset::Ref rset(recordset());
  if (rset)
    mforms::Utilities::set_clipboard_text(rset->get_column_caption(column));
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::copy_all_column_names() {
  Recordset::Ref rset(recordset());
  if (rset) {
    std::string text;
    size_t visibleColumnCount = rset->get_column_count();
    Recordset::Column_names::const_iterator end = rset->column_names()->end();
    for (Recordset::Column_names::const_iterator col = rset->column_names()->begin();
         col != end && visibleColumnCount > 0; ++col, --visibleColumnCount)
      text.append(", ").append(*col);
    if (!text.empty())
      text = text.substr(2);
    mforms::Utilities::set_clipboard_text(text);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::open_field_editor(int row, int column) {
  Recordset::Ref rset(recordset());
  if (rset) {
    Recordset_cdbc_storage::Ref storage(std::dynamic_pointer_cast<Recordset_cdbc_storage>(rset->data_storage()));
    if (storage) {
      rset->open_field_data_editor(row, column, storage->field_info()[column].type);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::update_selection_for_menu_extra(mforms::ContextMenu *menu, const std::vector<int> &rows,
                                                      int column) {
  mforms::MenuItem *item = menu->find_item("edit_cell");
  if (item) {
    if (item->signal_clicked()->empty() && !rows.empty())
      item->signal_clicked()->connect(std::bind(&SqlEditorResult::open_field_editor, this, rows[0], column));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::set_recordset(Recordset::Ref rset) {
  if (_resultset_placeholder) {
    _tabdock_delegate->undock_view(_resultset_placeholder);
    _resultset_placeholder = NULL;
  }

  _rset = rset;
  if (!rset->is_readonly())
    _grtobj->resultset(grtwrap_editablerecordset(grtobj(), rset));
  else
    _grtobj->resultset(grtwrap_recordset(grtobj(), rset));

  rset->update_selection_for_menu_extra =
    std::bind(&SqlEditorResult::update_selection_for_menu_extra, this, std::placeholders::_1, std::placeholders::_2,
              std::placeholders::_3);

  rset->get_toolbar()
    ->find_item("record_export")
    ->signal_activated()
    ->connect(std::bind(&SqlEditorResult::show_export_recordset, this));
  if (rset->get_toolbar()->find_item("record_import"))
    rset->get_toolbar()
      ->find_item("record_import")
      ->signal_activated()
      ->connect(std::bind(&SqlEditorResult::show_import_recordset, this));

  // reset the column header indicators
  rset->get_toolbar()
    ->find_item("record_sort_reset")
    ->signal_activated()
    ->connect(std::bind(&SqlEditorResult::reset_sorting, this));

  _grid_header_menu = new mforms::ContextMenu();
  _grid_header_menu->add_item_with_title("Copy Field Name", std::bind(&SqlEditorResult::copy_column_name, this), "Copy Field Name", "");
  _grid_header_menu->add_item_with_title("Copy All Field Names",
                                         std::bind(&SqlEditorResult::copy_all_column_names, this), "Copy All Field Names", "");
  _grid_header_menu->add_separator();
  _grid_header_menu->add_item_with_title("Reset Sorting", std::bind(&SqlEditorResult::reset_sorting, this), "Reset Sorting", "");
  _grid_header_menu->add_item_with_title("Reset Column Widths", std::bind(&SqlEditorResult::reset_column_widths, this), "Reset Column Widths", "");

  
  std::string fontDescription = bec::GRTManager::get()->get_app_option_string("workbench.general.Resultset:Font");
  std::string font;
  float size = 0;
  bool bold, italic;
  base::parse_font_description(fontDescription, font, size, bold, italic);
  rset->setFont(font, size);
  mforms::GridView *grid = mforms::manage(mforms::GridView::create(rset));
  {
    if (!font.empty())
      grid->set_font(fontDescription);
    grid->set_header_menu(_grid_header_menu);
  }
  dock_result_grid(grid);

  Recordset_cdbc_storage::Ref storage(std::dynamic_pointer_cast<Recordset_cdbc_storage>(rset->data_storage()));
  rset->caption(strfmt("%s %i", (storage->table_name().empty() ? _("Result") : storage->table_name().c_str()),
                       ++_owner->_rs_sequence));

  bec::UIForm::scoped_connect(rset->get_context_menu()->signal_will_show(),
                              std::bind(&SqlEditorPanel::on_recordset_context_menu_show, _owner, Recordset::Ptr(rset)));

  restore_grid_column_widths();
  bec::UIForm::scoped_connect(_result_grid->signal_column_resized(),
                              std::bind(&SqlEditorResult::on_recordset_column_resized, this, std::placeholders::_1));

  bec::UIForm::scoped_connect(_result_grid->signal_columns_resized(),
                              std::bind(&SqlEditorResult::onRecordsetColumnsResized, this, std::placeholders::_1));

  rset->data_edited_signal.connect(std::bind(&SqlEditorPanel::resultset_edited, _owner));
  rset->data_edited_signal.connect(std::bind(&mforms::View::set_needs_repaint, grid));
}

//----------------------------------------------------------------------------------------------------------------------

SqlEditorResult::~SqlEditorResult() {
  base::NotificationCenter::get()->remove_observer(this);

  delete _column_info_menu;
  delete _grid_header_menu;
}

//----------------------------------------------------------------------------------------------------------------------

Recordset::Ref SqlEditorResult::recordset() const {
  return _rset.lock();
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorResult::has_pending_changes() {
  Recordset::Ref rset(recordset());
  if (rset)
    return rset->has_pending_changes();
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::apply_changes() {
  Recordset::Ref rset(recordset());
  if (rset)
    rset->apply_changes();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::discard_changes() {
  Recordset::Ref rset(recordset());
  if (rset)
    rset->rollback();
}

//----------------------------------------------------------------------------------------------------------------------

std::string SqlEditorResult::caption() const {
  RETVAL_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs, "") {
    return rs->caption();
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<SpatialDataView::SpatialDataSource> SqlEditorResult::get_spatial_columns() {
  std::vector<SpatialDataView::SpatialDataSource> spatial_columns;
  int i = 0;
  Recordset_cdbc_storage::Ref storage(std::dynamic_pointer_cast<Recordset_cdbc_storage>(_rset.lock()->data_storage()));
  std::vector<Recordset_cdbc_storage::FieldInfo> &field_info(storage->field_info());
  for (std::vector<Recordset_cdbc_storage::FieldInfo>::const_iterator iter = field_info.begin();
       iter != field_info.end(); ++iter, ++i) {
    if (iter->type == "GEOMETRY") {
      SpatialDataView::SpatialDataSource field;
      field.source = get_title();
      field.resultset = _rset;
      field.column = iter->field;
      field.type = iter->type;
      field.column_index = i;
      spatial_columns.push_back(field);
    }
  }
  return spatial_columns;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::set_title(const std::string &title) {
  grtobj()->name(title);
  mforms::AppView::set_title(title);
}

//----------------------------------------------------------------------------------------------------------------------

bool SqlEditorResult::can_close() {
  if (Recordset::Ref rs = recordset())
    if (!rs->can_close(true))
      return false;

  if (!_tabdock.close_all_views())
    return false;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::close() { // called by DockingPoint::close_view()
  if (Recordset::Ref rs = recordset())
    rs->close();
  _tabdock.close_all_views();

  mforms::AppView::close();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::switch_tab() {
  mforms::AppView *tab = _tabdock.selected_view();

  if (tab) {
    if (tab->identifier() == "column_info" && !_column_info_created) {
      _column_info_created = true;
      create_column_info_panel();
    } else if (tab->identifier() == "query_stats" && !_query_stats_created) {
      _query_stats_created = true;
      create_query_stats_panel();
    } else if (tab->identifier() == "form_result") {
      if (!_form_view_created) {
        _form_view_created = true;
        _form_result_view->init_for_resultset(_rset, _owner->owner());
      }
      _form_result_view->display_record();
    } else if (tab->identifier() == "result_grid") {
      if (_resultset_placeholder) {
        _owner->owner()->exec_editor_sql(_owner, true, true, true, false, this);
        if (!_rset.expired())
          set_title(_rset.lock()->caption());
      }
    } else if (tab->identifier() == "execution_plan") {
      if (_execution_plan_placeholder) {
        _tabdock_delegate->undock_view(_execution_plan_placeholder);
        _execution_plan_placeholder = NULL;

        // if the explain tab is just a placeholder, execute visual explain, which will replace the tab when docking
        grt::BaseListRef args(true);
        args.ginsert(_owner->grtobj());
        args.ginsert(_grtobj);
        try {
          // run the visual explain plugin, so it will fill the result panel
          grt::GRT::get()->call_module_function("SQLIDEQueryAnalysis", "visualExplain", args);
        } catch (std::exception &exc) {
          logError("Error executing visual explain: %s\n", exc.what());
          mforms::Utilities::show_error(
            "Execution Plan", "An internal error occurred while building the execution plan, please file a bug report.",
            "OK");
        }
      }
    } else if (tab->identifier() == "spatial_result_view") {
      if (!_spatial_view_initialized) {
        _spatial_view_initialized = true;
        _spatial_result_view->refresh_layers();
      }
      _spatial_result_view->activate();
    }
  }

  updateColors();
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::add_switch_toggle_toolbar_item(mforms::ToolBar *tbar) {
  _collapse_toggled_sig.disconnect();
  mforms::App *app = mforms::App::get();
  tbar->add_item(mforms::manage(new mforms::ToolBarItem(mforms::ExpanderItem)));
  mforms::ToolBarItem *item = mforms::manage(new mforms::ToolBarItem(mforms::ToggleItem));
  item->set_name("Side Toggle");
  item->setInternalName("sidetoggle");
  item->set_icon(app->get_resource_path("output_type-toggle-on.png"));
  item->set_alt_icon(app->get_resource_path("output_type-toggle-off.png"));
  item->signal_activated()->connect(std::bind(&SqlEditorResult::toggle_switcher_collapsed, this));
  item->set_checked(!_switcher.get_collapsed());
  tbar->add_item(item);
  _collapse_toggled_sig =
    _collapse_toggled.connect(std::bind(&mforms::ToolBarItem::set_checked, item, std::placeholders::_1));
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::switcher_collapsed() {
  bool state = _switcher.get_collapsed();
  for (std::list<mforms::ToolBar *>::const_iterator it = _toolbars.begin(); it != _toolbars.end(); ++it) {
    (*it)->find_item("sidetoggle")->set_checked(state);
  }
  relayout();

  bec::GRTManager::get()->set_app_option("Recordset:SwitcherCollapsed", grt::IntegerRef(state ? 1 : 0));
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::show_export_recordset() {
  try {
    RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs) {
      grt::ValueRef option(bec::GRTManager::get()->get_app_option("Recordset:LastExportPath"));
      std::string path = option.is_valid() ? grt::StringRef::cast_from(option) : "";
      option = bec::GRTManager::get()->get_app_option("Recordset:LastExportExtension");
      std::string extension = option.is_valid() ? grt::StringRef::cast_from(option) : "";
      InsertsExportForm exporter(0 /*mforms::Form::main_form()*/, rs_ref, extension);
      exporter.set_title(_("Export Resultset"));
      if (!path.empty())
        exporter.set_path(path);
      path = exporter.run();
      if (path.empty())
        bec::GRTManager::get()->replace_status_text(_("Export resultset canceled"));
      else {
        bec::GRTManager::get()->replace_status_text(strfmt(_("Exported resultset to %s"), path.c_str()));
        bec::GRTManager::get()->set_app_option("Recordset:LastExportPath", grt::StringRef(path));
        extension = base::extension(path);
        if (!extension.empty() && extension[0] == '.')
          extension = extension.substr(1);
        if (!extension.empty())
          bec::GRTManager::get()->set_app_option("Recordset:LastExportExtension", grt::StringRef(extension));
      }
    }
  } catch (const std::exception &exc) {
    mforms::Utilities::show_error("Error exporting recordset", exc.what(), "OK");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::show_import_recordset() {
  try {
    RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs) {
      grt::BaseListRef args(true);
      grt::Module *mod = grt::GRT::get()->get_module("SQLIDEUtils");
      if (mod) {
        args.ginsert(_owner->owner()->grtobj());
        mod->call_function("launchPowerImport", args);
      }
      else
        logFatal("Unable to launch import wizard\n");
    }
  } catch (const std::exception &exc) {
    mforms::Utilities::show_error("Error importing recordset", exc.what(), "OK");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::toggle_switcher_collapsed() {
  bool flag = !_switcher.get_collapsed();
  _switcher.set_collapsed(flag);
  _collapse_toggled(flag);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::on_recordset_column_resized(int column) {
  if (column >= 0) {
    std::string column_id = _column_width_storage_ids[column];
    int width = _result_grid->get_column_width(column);
    _owner->owner()->column_width_cache()->save_column_width(column_id, width);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::onRecordsetColumnsResized(const std::vector<int> cols) {
  std::vector<int>::const_iterator it;
  std::map<std::string, int> widths;
  for (it = cols.begin(); it != cols.end(); ++it) {
    if (*it >= 0) {
      std::string column_id = _column_width_storage_ids[*it];
      int width = _result_grid->get_column_width(*it);
      widths.insert(std::make_pair(column_id, width));
    }
  }
  if (!widths.empty()) {
    bec::GRTManager::get()->get_dispatcher()->execute_async_function("store column widths", [this, widths]() {
      _owner->owner()->column_width_cache()->save_columns_width(widths);
      return grt::ValueRef();
    });
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::reset_column_widths() {
  ColumnWidthCache *cache = _owner->owner()->column_width_cache();

  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs) {
    Recordset_cdbc_storage::Ref storage(std::dynamic_pointer_cast<Recordset_cdbc_storage>(rs->data_storage()));
    std::vector<Recordset_cdbc_storage::FieldInfo> &field_info(storage->field_info());

    for (int c = (int)field_info.size(), i = 0; i < c; i++) {
      std::string column_storage_id;

      column_storage_id = field_info[i].field + "::" + field_info[i].schema + "::" + field_info[i].table;

      cache->delete_column_width(column_storage_id);
    }
  }

  restore_grid_column_widths();
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<float> SqlEditorResult::get_autofit_column_widths(Recordset *rs) {
  std::vector<float> widths(rs->get_column_count());
  std::string font = bec::GRTManager::get()->get_app_option_string("workbench.general.Resultset:Font");

  for (size_t c = rs->get_column_count(), j = 0; j < c; j++) {
    widths[j] = (float)mforms::Utilities::get_text_width(rs->get_column_caption(j), font);
  }

  // look in 1st 10 rows for the max width of the columns
  for (size_t i = 0; i < 10; i++) {
    for (size_t c = rs->get_column_count(), j = 0; j < c; j++) {
      std::string value;
      rs->get_field(i, j, value);
      widths[j] = std::max(widths[j], (float)mforms::Utilities::get_text_width(value, font));
    }
  }
  return widths;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::restore_grid_column_widths() {
  ColumnWidthCache *cache = _owner->owner()->column_width_cache();

  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs) {
    Recordset_cdbc_storage::Ref storage(std::dynamic_pointer_cast<Recordset_cdbc_storage>(rs->data_storage()));
    std::vector<Recordset_cdbc_storage::FieldInfo> &field_info(storage->field_info());
    std::vector<float> autofit_widths;

    for (int c = (int)field_info.size(), i = 0; i < c; i++) {
      std::string column_storage_id;

      column_storage_id = field_info[i].field + "::" + field_info[i].schema + "::" + field_info[i].table;
      _column_width_storage_ids.push_back(column_storage_id);

      // check if we have a remembered column width
      int width = cache->get_column_width(column_storage_id);
      if (width > 0) {
        _result_grid->set_column_width(i, width);
      } else {
        // if not, we set a default width based on the width of the 1st 50 rows
        if (autofit_widths.empty())
          autofit_widths = get_autofit_column_widths(rs);

        int width = int(autofit_widths[i] + 10);
        if (width < 40)
          width = 40;
        else if (width > 250)
          width = 250;
        _result_grid->set_column_width(i, width);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::dock_result_grid(mforms::GridView *view) {
  _result_grid = view;
  view->set_name("Result Grid Wrapper");
  view->setInternalName("result-grid-wrapper");

  mforms::AppView *grid_view;
  {
    mforms::AppView *box = grid_view = mforms::manage(new mforms::AppView(false, "Result Grid View", "ResultGridView", false));
    box->set_name("Resultset Host");
    box->setInternalName("resultset-host");
    mforms::ToolBar *tbar = _rset.lock()->get_toolbar();
    tbar->set_name("Resultset Toolbar");
    tbar->setInternalName("resultset-toolbar");
    _toolbars.push_back(tbar);

    add_switch_toggle_toolbar_item(tbar);

    box->add(tbar, false, true);
    box->add(view, true, true);

    box->set_title("Result\nGrid");
    box->set_identifier("result_grid");
    _tabdock.dock_view(box, "output_type-resultset.png");
  }
  {
    bool editable = false;
    if (Recordset::Ref rset = _rset.lock())
      editable = !rset->is_readonly();
    _form_result_view = mforms::manage(new ResultFormView(editable));
    add_switch_toggle_toolbar_item(_form_result_view->get_toolbar());
    _form_result_view->set_title("Form\nEditor");
    _form_result_view->set_identifier("form_result");
    _tabdock.dock_view(_form_result_view, "output_type-formeditor.png");
  }
  {
    _column_info_box = mforms::manage(new mforms::AppView(false, "Result Field Types", "ResultFieldTypes", false));
    _column_info_box->set_title("Field\nTypes");
    _column_info_box->set_identifier("column_info");
    _tabdock.dock_view(_column_info_box, "output_type-fieldtypes.png");
  }
  {
    _query_stats_box = mforms::manage(new mforms::AppView(false, "Result Query Stats", "ResultQueryStats", false));
    _query_stats_box->set_title("Query\nStats");
    _query_stats_box->set_identifier("query_stats");
    _tabdock.dock_view(_query_stats_box, "output_type-querystats.png");
  }

  create_spatial_view_panel_if_needed();

  // reorder the explain tab to last
  bool has_explain_tab = false;
  for (int i = 0; i < _tabdock_delegate->view_count(); i++) {
    mforms::AppView *view = _tabdock_delegate->view_at_index(i);
    if (!view)
      continue;

    if (view->identifier() == "execution_plan") {
      has_explain_tab = true;
      view->retain();
      _tabdock_delegate->undock_view(view);
      _tabdock.dock_view(view, "output_type-executionplan.png");
      view->release();
      break;
    }
  }
  if (!has_explain_tab) {
    _execution_plan_placeholder = mforms::manage(new mforms::AppView(false, "Execution Plan", "ExecutionPlan", false));
    _execution_plan_placeholder->set_title("Execution\nPlan");
    _execution_plan_placeholder->set_identifier("execution_plan");
    _tabdock.dock_view(_execution_plan_placeholder, "output_type-executionplan.png");
  }
  _switcher.set_selected(0);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::create_spatial_view_panel_if_needed() {
  if (Recordset::Ref rset = _rset.lock()) {
    Recordset_cdbc_storage::Ref storage(std::dynamic_pointer_cast<Recordset_cdbc_storage>(rset->data_storage()));
    bool has_geometry = false;
    std::vector<Recordset_cdbc_storage::FieldInfo> &field_info(storage->field_info());
    for (std::vector<Recordset_cdbc_storage::FieldInfo>::const_iterator iter = field_info.begin();
         iter != field_info.end(); ++iter) {
      if (iter->type == "GEOMETRY") {
        has_geometry = true;
        break;
      }
    }
    if (has_geometry) {
      if (!spatial::Projection::get_instance().check_libproj_availability()) {
        mforms::Utilities::show_message_and_remember("Unable to initialize Spatial Viewer",
                                                     "Spatial support requires the PROJ.4 library (libproj). If you "
                                                     "already have it installed, please set the PROJSO environment "
                                                     "variable to its location before starting Workbench.",
                                                     "Ok", "", "", "SqlEditorResult.libprojcheck", "");
        return;
      }
      _spatial_result_view = mforms::manage(new SpatialDataView(this));
      add_switch_toggle_toolbar_item(_spatial_result_view->get_toolbar());
      mforms::AppView *box = mforms::manage(new mforms::AppView(false, "Spatial View", "SpatialView", false));
      box->set_title("Spatial\nView");
      box->set_identifier("spatial_result_view");
      box->set_name("Spatial Host");
      box->setInternalName("spatial-host");
      box->add(_spatial_result_view, true, true);
      _tabdock.dock_view(box, "output_type-spacialview.png");
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::string format_ps_time(std::int64_t t) {
  int hours, mins;
  double secs;

  secs = t / 1000000000000.0;
  mins = int(secs / 60) % 60;
  hours = mins / 3600;

  return base::strfmt("%i:%02i:%02.8f", hours, mins, secs);
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::Label *bold_label(const std::string text) {
  mforms::Label *l = mforms::manage(new mforms::Label(text));
  l->set_style(mforms::BoldStyle);
  return l;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::copy_column_info(mforms::TreeView *tree) {
  std::list<mforms::TreeNodeRef> nodes(tree->get_selection());
  std::string text;

  for (std::list<mforms::TreeNodeRef>::const_iterator node = nodes.begin(); node != nodes.end(); ++node) {
    text.append(base::strfmt("%i", (*node)->get_int(0)));
    for (int i = 1; i < tree->get_column_count(); i++) {
      if (i >= 1 && i <= 5)
        text.append(",").append((*node)->get_string(i));
      else
        text.append(",").append(base::strfmt("%i", (*node)->get_int(i)));
    }
    text.append("\n");
  }
  mforms::Utilities::set_clipboard_text(text);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::copy_column_info_name(mforms::TreeView *tree) {
  std::list<mforms::TreeNodeRef> nodes(tree->get_selection());
  std::string text;

  for (std::list<mforms::TreeNodeRef>::const_iterator node = nodes.begin(); node != nodes.end(); ++node) {
    text.append((*node)->get_string(1)).append("\n");
  }
  mforms::Utilities::set_clipboard_text(text);
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::create_column_info_panel() {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs) {
    Recordset_cdbc_storage::Ref storage(std::dynamic_pointer_cast<Recordset_cdbc_storage>(rs->data_storage()));

    mforms::Box *box = _column_info_box;
    mforms::ToolBar *tbar = mforms::manage(new mforms::ToolBar(mforms::SecondaryToolBar));
    _toolbars.push_back(tbar);
    mforms::ToolBarItem *item;
    item = mforms::manage(new mforms::ToolBarItem(mforms::TitleItem));
    item->set_text("Field Types");
    tbar->add_item(item);

    add_switch_toggle_toolbar_item(tbar);

    box->add(tbar, false, true);

    if (_owner->owner()->collect_field_info()) {
      mforms::TreeView *tree =
        mforms::manage(new mforms::TreeView(mforms::TreeFlatList | mforms::TreeAltRowColors | mforms::TreeShowRowLines |
                                            mforms::TreeShowColumnLines | mforms::TreeNoBorder));
      tree->add_column(mforms::IntegerColumnType, "#", 50);
      tree->add_column(mforms::StringColumnType, "Field", 130);
      tree->add_column(mforms::StringColumnType, "Schema", 130);
      tree->add_column(mforms::StringColumnType, "Table", 130);
      tree->add_column(mforms::StringColumnType, "Type", 150);
      tree->add_column(mforms::StringColumnType, "Character Set", 100);
      tree->add_column(mforms::IntegerColumnType, "Display Size", 80);
      tree->add_column(mforms::IntegerColumnType, "Precision", 80);
      tree->add_column(mforms::IntegerColumnType, "Scale", 80);
      tree->end_columns();

      tree->set_selection_mode(mforms::TreeSelectMultiple);

      _column_info_menu = new mforms::ContextMenu();
      _column_info_menu->add_item_with_title("Copy", std::bind(&SqlEditorResult::copy_column_info, this, tree), "Copy", "");
      _column_info_menu->add_item_with_title("Copy Name",
                                             std::bind(&SqlEditorResult::copy_column_info_name, this, tree), "Copy Name", "");
      tree->set_context_menu(_column_info_menu);

      int i = 0;
      std::vector<Recordset_cdbc_storage::FieldInfo> &field_info(storage->field_info());
      for (std::vector<Recordset_cdbc_storage::FieldInfo>::const_iterator iter = field_info.begin();
           iter != field_info.end(); ++iter) {
        mforms::TreeNodeRef node = tree->add_node();
        node->set_int(0, ++i);
        node->set_string(1, iter->field);
        node->set_string(2, iter->schema);
        node->set_string(3, iter->table);
        node->set_string(4, iter->type);
        node->set_string(5, iter->charset);
        node->set_int(6, iter->display_size);
        node->set_int(7, iter->precision);
        node->set_int(8, iter->scale);
      }
      box->add(tree, true, true);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static struct ColorDefinitions {
  double r, g, b;
} colors[] = {
  {0.17, 0.34, 0.89}, {0.89, 0.34, 0.17}, {0.34, 0.89, 0.17},

  {1.00, 0.37, 0.37},

  {0.17, 0.89, 0.89}, {0.89, 0.89, 0.17}, {0.89, 0.17, 0.89},

  {0.37, 0.64, 0.64}, {0.64, 0.37, 0.64}, {0.64, 0.64, 0.37},
};

static std::string render_stages(std::vector<SqlEditorForm::PSStage> &stages) {
  std::string path = mforms::Utilities::get_special_folder(mforms::ApplicationData) + "/stages.png";

  int ncolors = sizeof(colors) / sizeof(ColorDefinitions);
  double total = 0;

  for (size_t i = 0; i < stages.size(); i++) {
    total += stages[i].wait_time;
  }

  int rows_of_text = (int)stages.size() / 3 + 1;
  cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 800, 30 + 20 + rows_of_text * 25);
  cairo_t *cr = cairo_create(surf);

  cairo_set_font_size(cr, 12);
  cairo_set_line_width(cr, 1);

  double x = 0.0;
  for (size_t i = 0; i < stages.size(); i++) {
    cairo_set_source_rgb(cr, colors[i % ncolors].r, colors[i % ncolors].g, colors[i % ncolors].b);
    cairo_rectangle(cr, x, 0, x + stages[i].wait_time * 800 / total, 30);
    cairo_fill(cr);

    {
      double capx = (i % 3) * 800.0 / 3 + 1;
      double capy = 50 + (i / 3) * 25.0;

      cairo_text_extents_t ext;
      cairo_text_extents(cr, stages[i].name.c_str(), &ext);

      cairo_save(cr);
      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_move_to(cr, floor(capx) + 30, capy + 25 + ext.y_bearing);
      if (base::hasPrefix(stages[i].name, "stage/sql/"))
        cairo_show_text(
          cr,
          base::strfmt("%s - %.4fms", stages[i].name.c_str() + sizeof("stage/sql/") - 1, stages[i].wait_time).c_str());
      else
        cairo_show_text(cr, base::strfmt("%s - %.4fms", stages[i].name.c_str(), stages[i].wait_time).c_str());
      cairo_rectangle(cr, floor(capx), capy, 20, 20);
      cairo_stroke_preserve(cr);
      cairo_restore(cr);
      cairo_fill(cr);
    }
    x += stages[i].wait_time * 800 / total;
  }

  cairo_rectangle(cr, 0, 0, 800, 30);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_stroke(cr);

  cairo_set_line_width(cr, 3);
  cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
  cairo_move_to(cr, 2, 28);
  cairo_line_to(cr, 2, 2);
  cairo_line_to(cr, 798, 2);
  cairo_stroke(cr);

  cairo_surface_write_to_png(surf, path.c_str());
  cairo_destroy(cr);
  cairo_surface_destroy(surf);

  return path;
}

//----------------------------------------------------------------------------------------------------------------------

static std::string render_waits(std::vector<SqlEditorForm::PSWait> &waits) {
  std::string path = mforms::Utilities::get_special_folder(mforms::ApplicationData) + "/waits.png";

  int ncolors = sizeof(colors) / sizeof(ColorDefinitions);
  double total = 0;

  for (size_t i = 0; i < waits.size(); i++) {
    total += waits[i].wait_time;
  }

  int rows_of_text = (int)waits.size() / 2 + 1;
  cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 800, 30 + 20 + rows_of_text * 25);
  cairo_t *cr = cairo_create(surf);

  cairo_set_font_size(cr, 12);
  cairo_set_line_width(cr, 1);

  double x = 0.0;
  for (size_t i = 0; i < waits.size(); i++) {
    cairo_set_source_rgb(cr, colors[i % ncolors].r, colors[i % ncolors].g, colors[i % ncolors].b);
    cairo_rectangle(cr, x, 0, x + waits[i].wait_time * 800 / total, 30);
    cairo_fill(cr);

    {
      double capx = (i % 2) * 800.0 / 2 + 1;
      double capy = 50 + (i / 2) * 25.0;

      cairo_text_extents_t ext;
      cairo_text_extents(cr, waits[i].name.c_str(), &ext);

      cairo_save(cr);
      cairo_set_source_rgb(cr, 0, 0, 0);
      cairo_move_to(cr, floor(capx) + 30, capy + 25 + ext.y_bearing);
      cairo_show_text(cr, base::strfmt("%s - %.4fms", waits[i].name.c_str(), waits[i].wait_time).c_str());

      cairo_rectangle(cr, floor(capx), capy, 20, 20);
      cairo_stroke_preserve(cr);
      cairo_restore(cr);
      cairo_fill(cr);
    }
    x += waits[i].wait_time * 800 / total;
  }

  cairo_rectangle(cr, 0, 0, 800, 30);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_stroke(cr);

  cairo_set_line_width(cr, 3);
  cairo_set_source_rgba(cr, 0, 0, 0, 0.3);
  cairo_move_to(cr, 2, 28);
  cairo_line_to(cr, 2, 2);
  cairo_line_to(cr, 798, 2);
  cairo_stroke(cr);

  cairo_surface_write_to_png(surf, path.c_str());
  cairo_destroy(cr);
  cairo_surface_destroy(surf);

  return path;
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::create_query_stats_panel() {
  RETURN_IF_FAIL_TO_RETAIN_WEAK_PTR(Recordset, _rset, rs) {
    SqlEditorForm::RecordsetData *rsdata = dynamic_cast<SqlEditorForm::RecordsetData *>(rs->client_data());
    std::string info;

    _query_stats_panel = mforms::manage(new mforms::ScrollPanel());
    mforms::Table *table = mforms::manage(new mforms::Table());
    table->set_padding(20);
    table->set_row_count(6);
    table->set_column_count(2);
    table->set_row_spacing(4);

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
    info = strfmt("Execution time: %s\n", format_ps_time(std::int64_t(rsdata->duration * 1000000000000.0)).c_str());
    box->add(mforms::manage(new mforms::Label(info)), false, true);

    // if we're in a server with PS, show some extra PS goodies
    // we need to convert this to long long it cause int64_t is not the same (long long) on the all platforms.
    std::map<std::string, long long int> ps_stats;
    for (auto &it : rsdata->ps_stat_info)
      ps_stats[it.first] = (long long int)it.second;

    if (ps_stats.size() <= 1) //  "EVENT_ID" is always present
    {
      if (!rsdata->ps_stat_error.empty())
        box->add(bold_label(rsdata->ps_stat_error), false, true);
      else
        box->add(bold_label("For more details, enable \"statement instrumentation\" for the Performance Schema and "
                            "\"Query -> Collect Performance Schema Stats\"."),
                 false, true);

      table->add(box, 0, 1, 1, 2, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
    } else {
      box->add(bold_label("Timing (as measured by the server):"), false, true);
      info.clear();
      info.append(strfmt("Execution time: %s\n", format_ps_time(ps_stats["TIMER_WAIT"]).c_str()));
      info.append(strfmt("Table lock wait time: %s\n", format_ps_time(ps_stats["LOCK_TIME"]).c_str()));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Errors:"), false, true);
      info.clear();
      info.append(strfmt("Had Errors: %s\n", ps_stats["ERRORS"] ? "YES" : "NO"));
      info.append(strfmt("Warnings: %lli\n", ps_stats["WARNINGS"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Rows Processed:"), false, true);
      info.clear();
      info.append(strfmt("Rows affected: %lli\n", ps_stats["ROWS_AFFECTED"]));
      info.append(strfmt("Rows sent to client: %lli\n", ps_stats["ROWS_SENT"]));
      info.append(strfmt("Rows examined: %lli\n", ps_stats["ROWS_EXAMINED"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Temporary Tables:"), false, true);
      info.clear();
      info.append(strfmt("Temporary disk tables created: %lli\n", ps_stats["CREATED_TMP_DISK_TABLES"]));
      info.append(strfmt("Temporary tables created: %lli\n", ps_stats["CREATED_TMP_TABLES"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      table->add(box, 0, 1, 1, 2, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);

      box = mforms::manage(new mforms::Box(false));
      box->set_padding(8);

      box->add(bold_label("Joins per Type:"), false, true);
      info.clear();
      info.append(strfmt("Full table scans (Select_scan): %lli\n", ps_stats["SELECT_SCAN"]));
      info.append(strfmt("Joins using table scans (Select_full_join): %lli\n", ps_stats["SELECT_FULL_JOIN"]));
      info.append(
        strfmt("Joins using range search (Select_full_range_join): %lli\n", ps_stats["SELECT_FULL_RANGE_JOIN"]));
      info.append(strfmt("Joins with range checks (Select_range_check): %lli\n", ps_stats["SELECT_RANGE_CHECK"]));
      info.append(strfmt("Joins using range (Select_range): %lli\n", ps_stats["SELECT_RANGE"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Sorting:"), false, true);
      info.clear();
      info.append(strfmt("Sorted rows (Sort_rows): %lli\n", ps_stats["SORT_ROWS"]));
      info.append(strfmt("Sort merge passes (Sort_merge_passes): %lli\n", ps_stats["SORT_MERGE_PASSES"]));
      info.append(strfmt("Sorts with ranges (Sort_range): %lli\n", ps_stats["SORT_RANGE"]));
      info.append(strfmt("Sorts with table scans (Sort_scan): %lli\n", ps_stats["SORT_SCAN"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Index Usage:"), false, true);
      info.clear();
      if (ps_stats["NO_INDEX_USED"])
        info.append("No Index used");
      else
        info.append("At least one Index was used");
      if (ps_stats["NO_GOOD_INDEX_USED"])
        info.append("No good index used");
      info.append("\n");
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      box->add(bold_label("Other Info:"), false, true);
      info.clear();
      info.append(strfmt("Event Id: %lli\n", ps_stats["EVENT_ID"]));
      info.append(strfmt("Thread Id: %lli\n", ps_stats["THREAD_ID"]));
      box->add(mforms::manage(new mforms::Label(info)), false, true);

      table->add(box, 1, 2, 1, 2, mforms::HFillFlag | mforms::HExpandFlag | mforms::VFillFlag);
    }

    std::vector<SqlEditorForm::PSStage> stages(rsdata->ps_stage_info);
    if (!stages.empty()) {
      mforms::Label *l = mforms::manage(new mforms::Label("Time Spent per Execution Stage (aggregated)"));
      l->set_text_align(mforms::MiddleCenter);
      l->set_style(mforms::BoldStyle);
      table->add(l, 0, 2, 2, 3, mforms::HFillFlag);

      std::string file = render_stages(stages);
      mforms::ImageBox *image = mforms::manage(new mforms::ImageBox());
      image->set_image(file);
      table->add(image, 0, 2, 3, 4, mforms::FillAndExpand);
    }

    std::vector<SqlEditorForm::PSWait> waits(rsdata->ps_wait_info);
    if (!waits.empty()) {
      mforms::Label *l = mforms::manage(new mforms::Label("Time Spent Waiting (aggregated)"));
      l->set_text_align(mforms::MiddleCenter);
      l->set_style(mforms::BoldStyle);
      table->add(l, 0, 2, 4, 5, mforms::HFillFlag);

      std::string file = render_waits(waits);
      mforms::ImageBox *image = mforms::manage(new mforms::ImageBox());
      image->set_image(file);
      table->add(image, 0, 2, 5, 6, mforms::HFillFlag | mforms::VFillFlag);
    }

    _query_stats_panel->add(table);

    _query_stats_box->add(_query_stats_panel, true, true);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void SqlEditorResult::view_record_in_form(int row_id) {
  if (_form_result_view) {
    _tabview.set_active_tab(1);
    switch_tab();
    _form_result_view->display_record(row_id);
  }
}

//----------------------------------------------------------------------------------------------------------------------
