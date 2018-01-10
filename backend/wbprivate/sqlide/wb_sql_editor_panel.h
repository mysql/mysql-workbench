/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

namespace mforms {
  class TabView;
  class TabSwitcher;
  class ToolBar;
  class ToolBarItem;
  class ContextMenu;
  class TreeView;
  class RecordGrid;
};

class SqlEditorForm;
class MySQLEditor;

class SqlEditorResult;

class MYSQLWBBACKEND_PUBLIC_FUNC SqlEditorPanel : public mforms::AppView {
  friend class SqlEditorResult;

  SqlEditorForm *_form;
  std::shared_ptr<MySQLEditor> _editor;

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

  std::string _autosave_file_suffix;

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
  void lower_tab_reordered(mforms::View *, int, int);

  void result_removed();

  void apply_clicked();
  void revert_clicked();

  void resultset_edited();
  void splitter_resized();

  void tab_menu_will_show();
  void rename_tab_clicked();
  void pin_tab_clicked();
  void close_tab_clicked();
  void close_other_tabs_clicked();

  bool is_pinned(int tab);
  void tab_pinned(int tab, bool flag);

  void limit_rows(mforms::ToolBarItem *);

public:
  typedef std::shared_ptr<SqlEditorPanel> Ref;
  SqlEditorPanel(SqlEditorForm *owner, bool is_scratch, bool start_collapsed);
  ~SqlEditorPanel();

  std::shared_ptr<MySQLEditor> editor_be() {
    return _editor;
  }
  db_query_QueryEditorRef grtobj();

  mforms::ToolBar *get_toolbar();
  virtual void set_title(const std::string &title);

  void update_limit_rows();

  SqlEditorForm *owner() {
    return _form;
  }

  bool is_scratch() {
    return _is_scratch;
  }

public:
  struct AutoSaveInfo {
    std::string orig_encoding;
    std::string type;
    std::string title;
    std::string filename;
    size_t first_visible_line;
    size_t caret_pos;
    bool word_wrap;
    bool show_special;

    AutoSaveInfo() : first_visible_line(0), caret_pos(0), word_wrap(false), show_special(false) {
    }
    AutoSaveInfo(const std::string &info_file);

    static AutoSaveInfo old_scratch(const std::string &scratch_file);
    static AutoSaveInfo old_autosave(const std::string &autosave_file);
  };

  enum LoadResult { Cancelled, Loaded, RunInstead };

  LoadResult load_from(const std::string &file, const std::string &encoding = "", bool keep_dirty = false);
  bool load_autosave(const AutoSaveInfo &info, const std::string &text_file);

  virtual bool can_close();
  virtual void close();

  bool save();
  bool save_as(const std::string &file);
  void revert_to_saved();

  void auto_save(const std::string &directory);
  void delete_auto_save(const std::string &directory);
  std::string autosave_file_suffix();

  void set_filename(const std::string &f);
  std::string filename() const {
    return _filename;
  }

  bool is_dirty() const;
  void check_external_file_changes();

  std::pair<const char *, std::size_t> text_data() const;

  void list_members();
  void jump_to_placeholder();

public:
  void query_started(bool retain_old_recordsets);
  void query_finished();
  void query_failed(const std::string &message);

  // recordset management
  SqlEditorResult *active_result_panel();

  SqlEditorResult *result_panel(int i);

  size_t result_panel_count();
  size_t resultset_count();

  SqlEditorResult *add_panel_for_recordset(Recordset::Ref rset);
  void add_panel_for_recordset_from_main(Recordset::Ref rset);

  std::list<SqlEditorResult *> dirty_result_panels();
};

#endif /* defined(__MySQLWorkbench__wb_sql_editor_panel__) */
