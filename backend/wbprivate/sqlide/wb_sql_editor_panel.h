/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __MySQLWorkbench__wb_sql_editor_panel__
#define __MySQLWorkbench__wb_sql_editor_panel__

#include "workbench/wb_backend_public_interface.h"

#include "mforms/appview.h"
#include "mforms/box.h"
#include "mforms/splitter.h"
#include "mforms/tabview.h"
#include "mforms/tabview_dock.h"
#include "mforms/button.h"
#include "mforms/label.h"
#include "mforms/imagebox.h"
#include "mforms/dockingpoint.h"
#include "mforms/menubar.h"

#include <boost/signals2.hpp>

namespace mforms
{
  class TabView;
  class TabSwitcher;
  class ToolBar;
  class ToolBarItem;
  class ContextMenu;
  class TreeNodeView;
  class RecordGrid;
};

class SqlEditorForm;
class MySQLEditor;

class SqlEditorResult;

class MYSQLWBBACKEND_PUBLIC_FUNC SqlEditorPanel : public mforms::AppView
{
  SqlEditorForm *_form;
  boost::shared_ptr<MySQLEditor> _editor;

  mforms::Box _editor_box;

  mforms::Splitter _splitter;
  mforms::TabView _lower_tabview;
  mforms::TabViewDockingPoint _lower_dock_delegate;
  mforms::DockingPoint _lower_dock;
  mforms::ContextMenu _lower_tab_menu;

  mforms::Box _tab_action_box;
  mforms::Button _tab_action_apply;
  mforms::Button _tab_action_revert;
  mforms::ImageBox _tab_action_icon;
  mforms::Label _tab_action_info;

  std::string _title;
  std::string _filename;
  std::string _orig_encoding;
  std::string _caption;

  std::string _autosave_file_path;

  time_t _file_timestamp;
  int _rs_sequence;

  bool _busy;
  bool _was_empty;
  bool _is_scratch;

  mforms::ToolBar *setup_editor_toolbar();
  void update_title();

  void dock_result_panel(SqlEditorResult *result);
  void show_find_panel(mforms::CodeEditor *editor, bool show);

  void dispose_recordset(Recordset::Ptr rs_ptr);
  bool on_close_by_user();
  void on_recordset_context_menu_show(Recordset::Ptr rs_ptr);

  void lower_tab_switched();
  bool lower_tab_closing(int tab);
  void lower_tab_closed(mforms::View *page, int tab);
  void lower_tab_reordered(mforms::View*, int, int);

  void result_removed();

  void apply_clicked();
  void revert_clicked();

  void resultset_edited();
  void splitter_resized();

  void tab_menu_will_show();
  void close_tab_clicked();
  void close_other_tabs_clicked();

public:
  typedef boost::shared_ptr<SqlEditorPanel> Ref;
  SqlEditorPanel(SqlEditorForm *owner, bool is_scratch, bool start_collapsed);
  ~SqlEditorPanel();

  boost::shared_ptr<MySQLEditor> editor_be() { return _editor; }
  db_query_QueryEditorRef grtobj();

  mforms::ToolBar *get_toolbar();
  virtual void set_title(const std::string &title);

  SqlEditorForm *owner() { return _form; }

  bool is_scratch() { return _is_scratch; }
public:
  bool load_from(const std::string &file, const std::string &encoding = "", bool keep_dirty=false);
  bool load_autosave(const std::string &file, const std::string &real_filename, const std::string &encoding = "");

  virtual bool can_close();
  virtual void close();

  bool save();
  bool save_as(const std::string &file);
  void revert_to_saved();

  void auto_save(const std::string &directory, int order);
  void delete_auto_save();
  void rename_auto_save(int to_order);
  int autosave_index();

  void set_filename(const std::string &f);
  std::string filename() const { return _filename; }

  bool is_dirty() const;
  void check_external_file_changes();

  std::pair<const char*, size_t> text_data() const;

  void list_members();
  void jump_to_placeholder();

public:
  void query_started(bool retain_old_recordsets);
  void query_finished();
  void query_failed(const std::string &message);

  // recordset management
  SqlEditorResult *active_result_panel();

  SqlEditorResult *result_panel(int i);

  SqlEditorResult* add_panel_for_recordset(Recordset::Ref rset);
  void add_panel_for_recordset_from_main(Recordset::Ref rset);

  std::list<SqlEditorResult*> dirty_result_panels();
};

#endif /* defined(__MySQLWorkbench__wb_sql_editor_panel__) */
