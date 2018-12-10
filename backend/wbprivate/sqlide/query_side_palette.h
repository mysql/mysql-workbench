/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

namespace mforms {
  class Box;
  class ToolBar;
  class HyperText;
  class ScrollPanel;
  class ToolBarItem;
}

namespace help {
  class HelpContext;
}

#include "base/notifications.h"
#include "mforms/tabview.h"

class SnippetListView;
class GrtThreadedTask;

class MYSQLWBBACKEND_PUBLIC_FUNC QuerySidePalette : public mforms::TabView, base::Observer {
public:
  QuerySidePalette(const SqlEditorForm::Ref &owner);
  ~QuerySidePalette();
  void cancel_timer();
  void close_popover();

  void refresh_snippets();
  void edit_last_snippet();

private:
  SqlEditorForm::Ptr _owner;

  help::HelpContext *_helpContext;
  mforms::ToolBar *_help_toolbar;
  mforms::HyperText *_help_text;
  bec::GRTManager::Timer *_help_timer;

  mforms::Box *_contentBorder;
  mforms::ScrollPanel *_snippet_box;
  mforms::ToolBar *_snippet_toolbar;
  SnippetListView *_snippet_list;

  bool _automatic_help; // Automatically show help after moving the caret.
  bool _switching_help; // Recursion stopper when switching to new topic and setting drop down.
  bool _pending_snippets_refresh;
  std::string _currentHelpTopic;

  // Topic history.
  mforms::ToolBarItem *_back_item;
  mforms::ToolBarItem *_forward_item;
  mforms::ToolBarItem *_quick_jump_item;
  mforms::ToolBarItem *_manual_help_item;
  std::vector<std::string> _topic_history;
  int _current_topic_index;
  std::map<std::string, std::pair<std::string, std::string> >
    _topic_cache; // Plain text and html text under a specific topic.

  void handle_notification(const std::string &name, void *sender, base::NotificationInfo &info);
  void updateColors();

  void show_help_text_for_topic(const std::string &topic);
  void show_help_hint_or_update();

  bool find_context_help(MySQLEditor *editor);
  grt::StringRef get_help_topic_threaded(const std::string &query, std::pair<ssize_t, ssize_t> caret);
  void update_help_history(const std::string &topic);

  void click_link(const std::string &link);
  mforms::ToolBar *prepare_snippet_toolbar();
  mforms::ToolBar *prepare_help_toolbar();
  void snippet_toolbar_item_activated(mforms::ToolBarItem *item);
  void help_toolbar_item_activated(mforms::ToolBarItem *item);

  void snippet_selection_changed();
};

