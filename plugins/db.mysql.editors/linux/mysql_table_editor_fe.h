/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef __WB_MYSQL_TABLE_EDITOR_H__
#define __WB_MYSQL_TABLE_EDITOR_H__

#include <gtkmm/notebook.h>
#include "linux_utilities/plugin_editor_base.h"
#include "../backend/mysql_table_editor.h"

#include <gtkmm/notebook.h>

class DbMySQLTableEditorColumnPage;
class DbMySQLTableEditorIndexPage;
class DbMySQLTableEditorFKPage;
class DbMySQLTableEditorTriggerPage;
class DbMySQLTableEditorPartPage;
class DbMySQLTableEditorOptPage;
class DbMySQLEditorPrivPage;

//==============================================================================
//
//==============================================================================
class DbMySQLTableEditor : public PluginEditorBase {
  friend class DbMySQLTableEditorColumnPage;

  MySQLTableEditorBE *_be;
  DbMySQLTableEditorColumnPage *_columns_page;
  DbMySQLTableEditorIndexPage *_indexes_page;
  DbMySQLTableEditorFKPage *_fks_page;
  DbMySQLTableEditorTriggerPage *_triggers_page;
  DbMySQLTableEditorPartPage *_part_page;
  DbMySQLTableEditorOptPage *_opts_page;
  mforms::View *_inserts_panel;
  DbMySQLEditorPrivPage *_privs_page;
  Gtk::Widget *_main_page_widget;

  void create_table_page();

  void refresh_table_page();
  void partial_refresh(const int what);

  void set_table_collation(Gtk::ComboBoxText *combo);
  void set_table_engine(Gtk::ComboBoxText *combo);

  virtual bec::BaseEditor *get_be();

  bool event_from_table_name_entry(GdkEvent *);

  void page_changed(Gtk::Widget *page, guint page_num);

  void set_table_name(const std::string &);

  // TESTING
  void refresh_indices();
  //\TESTING
  void set_table_option_by_name(const std::string &name, const std::string &value);
  void set_comment(const std::string &cmt);

  void toggle_header_part();

protected:
  virtual void decorate_object_editor();

public:
  DbMySQLTableEditor(grt::Module *m, const grt::BaseListRef &args);

  virtual ~DbMySQLTableEditor();
  virtual void do_refresh_form_data(); // That's called from PluginEditorBase::refresh_form_data
                                       // which is passed to the backend refresh slot
  virtual bool can_close();
  virtual bool switch_edited_object(const grt::BaseListRef &args);
};

#endif
