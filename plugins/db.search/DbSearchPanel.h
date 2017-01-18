/*
* Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DB_SEARCH_PANEL_H_
#define _DB_SEARCH_PANEL_H_

#include <mforms/mforms.h>
#include "grt/grt_manager.h"
#include "grtui/db_conn_be.h"

class DBSearch;

enum SearchMode { Contains, ExactMatch, Like, Regexp };

enum SearchDataType { numeric_type = 1, datetime_type = 1 << 1, text_type = 1 << 2, search_all_types = -1 };

class DBSearchPanel : public mforms::Box {
protected:
  mforms::Box _progress_box;
  mforms::Label _progress_label;
  mforms::Button _pause_button;
  mforms::ProgressBar _progress_bar;
  mforms::Label _matches_label;
  mforms::TreeView _results_tree;
  mforms::ContextMenu _context_menu;
  std::shared_ptr<DBSearch> _searcher;
  bec::GRTManager::Timer* _update_timer;
  std::map<std::string, std::list<std::string> > _key_columns;

  void prepare_menu();
  void activate_menu_item(const std::string& action);

  void load_model(mforms::TreeNodeRef tnode);

public:
  DBSearchPanel();
  ~DBSearchPanel();
  void search(sql::ConnectionWrapper connection, const std::string& search_keyword,
              const grt::StringListRef& filter_list, const SearchMode search_mode, const int limit_total,
              const int limt_per_table, const bool invert, const int search_data_type, const std::string cast_to,
              std::function<void(grt::ValueRef)> finished_callback, std::function<void()> failed_callback);
  void toggle_pause();
  bool stop_search_if_working();
  bool update();
  bool _search_finished;
};

#endif //#ifndef _DB_SEARCH_PANEL_H_
