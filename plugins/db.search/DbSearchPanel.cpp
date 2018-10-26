/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "DbSearchPanel.h"
#include <sstream>
#include "grtui/grt_wizard_form.h"
#include "grtui/connection_page.h"
#include "grt/grt_string_list_model.h"
#include "base/sqlstring.h"
#include "grt/grt_manager.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("db.search");

grt::ValueRef call_search(std::function<void()> search, std::function<void()> fail_cb) {
  try {
    search();
  } catch (...) {
    fail_cb();
    throw;
  }
  return grt::ValueRef();
};

bool is_string_type(const std::string& type) {
  // The string types are CHAR, VARCHAR, BINARY, VARBINARY, BLOB, TEXT, ENUM, and SET
  static const std::set<std::string> chartypes = {"char", "varchar", "binary", "varbinary",
                                                  "blob", "text",    "enum",   "set"};
  std::string searchtype = type.substr(0, type.find("("));
  return chartypes.find(searchtype) != chartypes.end();
};

bool is_numeric_type(const std::string& type) {
  /*
  MySQL supports all standard SQL numeric data types. These types include the exact numeric data types
  (INTEGER, SMALLINT, DECIMAL, and NUMERIC), as well as the approximate numeric data types
  (FLOAT, REAL, and DOUBLE PRECISION). The keyword INT is a synonym for INTEGER, and the keywords
  DEC and FIXED are synonyms for DECIMAL. MySQL treats DOUBLE as a synonym for DOUBLE PRECISION
  (a nonstandard extension). MySQL also treats REAL as a synonym for DOUBLE PRECISION (a nonstandard variation),
  unless the REAL_AS_FLOAT SQL mode is enabled.
  */
  static const std::set<std::string> chartypes = {"integer", "smallint",         "decimal", "numeric", "float",
                                                  "real",    "double precision", "int",     "dec",     "fixed",
                                                  "double",  "double precision", "real"};
  std::string searchtype = type.substr(0, type.find("("));
  return chartypes.find(searchtype) != chartypes.end();
};

bool is_datetime_type(const std::string& type) {
  // The date and time types for representing temporal values are DATE, TIME, DATETIME, TIMESTAMP, and YEAR.
  static const std::set<std::string> chartypes = {"date", "time", "datetime", "timestamp", "year"};
  std::string searchtype = type.substr(0, type.find("("));
  return chartypes.find(searchtype) != chartypes.end();
};

class DBSearch {
public:
  typedef std::vector<std::vector<std::pair<std::string, std::string> > > column_data_t;
  struct SearchResultEntry {
    std::string schema;
    std::string table;
    std::list<std::string> keys;
    std::string query;
    column_data_t data;
  };

private:
  sql::ConnectionWrapper _db_conn;
  grt::StringListRef _filter_list;
  std::string _search_keyword;
  std::string _state;
  float _progress;
  SearchMode _search_mode;
  int _limit_total;
  int _limt_per_table;
  int _limit_counter;
  std::vector<SearchResultEntry> _search_result;
  volatile bool _working;
  volatile bool _stop;
  volatile bool _starting;
  volatile bool _paused;
  bool _invert;
  int _searched_tables;
  int _matched_rows;
  std::string _cast_to;
  int _search_data_type;
  base::Mutex _search_result_mutex;
  base::Mutex _pause_mutex;

protected:
  typedef std::function<void(const std::string&, const std::string&, const std::list<std::string>&,
                             const std::list<std::string>&, const std::string&, const bool match_PK)>
    select_func_t;
  void run(select_func_t select_func);
  void select_data(const std::string& schema_name, const std::string& table_name,
                   const std::list<std::string>& pk_columns, const std::list<std::string>& select_columns,
                   const std::string& limit_clause, const bool match_PK);
  void count_data(const std::string& schema_name, const std::string& table_name,
                  const std::list<std::string>& pk_columns, const std::list<std::string>& select_columns,
                  const std::string& limit_clause, const bool match_PK);

public:
  /*
      DBSearch():_working(false), _stop(false)
      {
          _pause_mutex = g_mutex_new();
          _search_result_mutex = g_mutex_new();
      };
    */
  DBSearch(sql::ConnectionWrapper connection, const std::string& search_keyword, const grt::StringListRef& filter_list,
           const SearchMode search_mode, const int limit_total, const int limt_per_table, const bool invert,
           const int search_data_type, const std::string cast_to)
    : _db_conn(connection),
      _filter_list(filter_list),
      _search_keyword(search_keyword),
      _state("Starting"),
      _progress(0),
      _search_mode(search_mode),
      _limit_total(limit_total),
      _limt_per_table(limt_per_table),
      _limit_counter(0),
      _working(false),
      _stop(false),
      _starting(false),
      _paused(false),
      _invert(invert),
      _searched_tables(0),
      _matched_rows(0),
      _cast_to(cast_to),
      _search_data_type(search_data_type) {
  }

  ~DBSearch() {
    stop();
  };

  std::string get_keyword() {
    return _search_keyword;
  }

  void prepare() {
    _starting = true;
  }
  bool is_starting() const {
    return _starting;
  }
  void toggle_pause() {
    _paused = !_paused;
    if (_paused)
      _pause_mutex.lock();
    else
      _pause_mutex.unlock();
  }
  void wait_if_paused() {
    if (is_paused()) {
      base::MutexLock lock(_pause_mutex); // Wait for unlock
    };
  };
  bool is_paused() const {
    return _paused;
  }
  float get_progress() const {
    return _progress;
  }
  std::string get_state() const {
    return _state;
  }
  const std::vector<SearchResultEntry>& search_results() const {
    return _search_result;
  }
  base::Mutex& get_search_result_mutex() {
    return _search_result_mutex;
  };
  int searched_table_count() {
    return _searched_tables;
  }
  int matched_rows() {
    return _matched_rows;
  }
  bool is_working() const {
    return _working;
  }
  void stop();
  std::string build_where(const std::string& col, const std::string& data) const;
  std::string build_select_query(const std::string& schema, const std::string& table,
                                 const std::list<std::string>& columns, const std::string& limit,
                                 const bool match_PK) const;
  std::string build_count_query(const std::string& schema, const std::string& table,
                                const std::list<std::string>& columns, const std::string& limit,
                                const bool match_PK) const;
  void search();
  void count();
};

void DBSearch::stop() {
  if (is_paused())
    toggle_pause();
  if (!_working)
    return;
  _stop = true;
  while (_working)
    ;
  _state = "Cancelled";
}

std::string DBSearch::build_where(const std::string& col, const std::string& data) const {
  static const std::vector<std::string> select_modes = {"LIKE", "=", "LIKE", "REGEXP"};
  static const std::vector<std::string> inverted_select_modes = {"LIKE", "<>", "NOT LIKE", "NOT REGEXP"};

  std::string where_condition;
  if (_cast_to.empty())
    where_condition.append(base::sqlstring("!", base::QuoteOnlyIfNeeded) << col);
  else {
    std::string tmpl("CAST(! AS ");
    tmpl += _cast_to;
    tmpl += ") ";
    where_condition.append(base::sqlstring(tmpl.c_str(), base::QuoteOnlyIfNeeded) << col);
  }

  where_condition.append(" ");
  where_condition.append(_invert ? inverted_select_modes[_search_mode].c_str() : select_modes[_search_mode].c_str());
  if (_search_mode == Contains)
    where_condition.append(std::string(base::sqlstring(" ? ", 0) << "%" + data + "%"));
  else
    where_condition.append(std::string(base::sqlstring(" ? ", 0) << data));
  return where_condition;
}

std::string DBSearch::build_count_query(const std::string& schema, const std::string& table,
                                        const std::list<std::string>& columns, const std::string& limit,
                                        const bool match_PK) const {
  if (columns.empty())
    return std::string();
  std::string result("SELECT COUNT(*) ");
  std::string or_clause;
  std::string where_condition;
  for (std::list<std::string>::const_iterator It = columns.begin(); It != columns.end(); ++It) {
    std::string col_where = build_where(*It, _search_keyword);
    where_condition.append(or_clause).append(col_where);
    or_clause = "OR ";
  }

  result.append(base::sqlstring(" FROM !.! WHERE ", 0) << schema << table);
  result.append(where_condition).append(limit);
  return result;
}

std::string DBSearch::build_select_query(const std::string& schema, const std::string& table,
                                         const std::list<std::string>& columns, const std::string& limit,
                                         const bool match_PK) const {
  if (columns.empty())
    return std::string();

  std::string result("SELECT ");
  bool pk_col = true;
  std::string or_clause;
  std::string where_condition;
  for (std::list<std::string>::const_iterator It = columns.begin(); It != columns.end(); ++It) {
    if (pk_col) // Add data for PK column
    {
      if (It->empty()) // No PK indicator
      {
        result.append("'N/A' ");
        pk_col = false;
      } else
        result.append(base::sqlstring("! ", base::QuoteOnlyIfNeeded) << *It);
      pk_col = false;
      continue;
    }
    std::string col_where = build_where(*It, _search_keyword);
    result.append(", IF(").append(col_where);
    result.append(base::sqlstring(", !, '') AS ! ", base::QuoteOnlyIfNeeded) << *It << *It);

    where_condition.append(or_clause).append(col_where);
    or_clause = "OR ";
  }
  if (where_condition.empty()) {
    return std::string();
  }
  result.append(base::sqlstring("FROM !.! WHERE ", base::QuoteOnlyIfNeeded) << schema << table);
  result.append(where_condition).append(limit);
  return result;
}

void DBSearch::count_data(const std::string& schema_name, const std::string& table_name,
                          const std::list<std::string>& pk_columns, const std::list<std::string>& select_columns,
                          const std::string& limit_clause, const bool match_PK) {
  std::string query = build_count_query(schema_name, table_name, select_columns, limit_clause, match_PK);
  if (query.empty())
    return;

  std::unique_ptr<sql::Statement> stmt(_db_conn->createStatement());
  std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));
  if (_limit_counter > 0)
    _limit_counter -= (int)rs->rowsCount();
  SearchResultEntry result;
  result.schema = schema_name;
  result.table = table_name;
  result.keys = pk_columns;
  result.query = query;
  while (rs->next()) {
    std::vector<std::pair<std::string, std::string> > data;
    data.reserve(select_columns.size());
    data.push_back(std::pair<std::string, std::string>("COUNT", rs->getString(1)));
    _matched_rows += rs->getInt(1);
    result.data.push_back(data);
  }
  base::MutexLock lock(_search_result_mutex);
  _search_result.push_back(result);
};

void DBSearch::select_data(const std::string& schema_name, const std::string& table_name,
                           const std::list<std::string>& pk_columns, const std::list<std::string>& select_columns,
                           const std::string& limit_clause, const bool match_PK) {
  std::string query = build_select_query(schema_name, table_name, select_columns, limit_clause, match_PK);
  if (query.empty())
    return;
  std::unique_ptr<sql::Statement> stmt(_db_conn->createStatement());
  std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));
  if (_limit_counter > 0)
    _limit_counter -= (int)rs->rowsCount();
  SearchResultEntry result;
  result.schema = schema_name;
  result.table = table_name;
  result.query = query;
  result.keys = pk_columns;
  while (rs->next()) {
    size_t col_idx = 1;
    std::vector<std::pair<std::string, std::string> > data;
    data.reserve(select_columns.size());
    for (std::list<std::string>::const_iterator It = select_columns.begin(); It != select_columns.end(); ++It)
      data.push_back(std::pair<std::string, std::string>(*It, rs->getString((int)col_idx++)));
    if (!data.empty())
      result.data.push_back(data);
  }
  _matched_rows += (int)result.data.size();
  if (!result.data.empty()) {
    base::MutexLock lock(_search_result_mutex);
    _search_result.push_back(result);
  }
};

void DBSearch::search() {
  run(std::bind(&DBSearch::select_data, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
};

void DBSearch::count() {
  run(std::bind(&DBSearch::count_data, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
};

void DBSearch::run(select_func_t select_func) {
  struct working_state_guard {
    volatile bool& _state;
    working_state_guard(volatile bool& state) : _state(state) {
    }
    ~working_state_guard() {
      _state = false;
    }
  };
  working_state_guard w(_working);
  if (is_paused())
    toggle_pause();
  _starting = false;
  _working = true;
  _stop = false;
  _limit_counter = _limit_total ? _limit_total : -1;
  _state = "Fetch schema list";
  _searched_tables = 0;
  _matched_rows = 0;
  std::map<std::string, std::vector<std::string> > schemas;
  std::map<std::string, std::vector<std::string> > schemas_tables;
  {
    std::unique_ptr<sql::Statement> stmt(_db_conn->createStatement());
    for (size_t count = _filter_list.count(), i = 0; i < count; i++) {
      wait_if_paused();
      if (_stop) {
        _working = false;
        return;
      }
      std::string schema_pattern = _filter_list.get(i);
      size_t dotpos = schema_pattern.find('.');
      std::string table_column;
      if (dotpos != std::string::npos)
        table_column = schema_pattern.substr(dotpos + 1);
      schema_pattern = schema_pattern.substr(0, dotpos);
      if (schema_pattern.empty() || schema_pattern.find('%') != std::string::npos) {
        schema_pattern = "%";
        std::unique_ptr<sql::ResultSet> rs(
          stmt->executeQuery(std::string(base::sqlstring("SHOW DATABASES LIKE ?", 0) << schema_pattern)));
        while (rs->next()) {
          std::string schema = rs->getString(1);
          schemas[schema].push_back(table_column);
        }
      } else
        schemas[schema_pattern].push_back(table_column);
    }
  }
  {
    std::unique_ptr<sql::Statement> stmt(_db_conn->createStatement());
    for (std::map<std::string, std::vector<std::string> >::const_iterator It = schemas.begin(); It != schemas.end();
         ++It) {
      std::string schema_name = It->first;
      _state = std::string("Populate tables in ") + schema_name;
      std::vector<std::string> tables = It->second;
      for (std::vector<std::string>::const_iterator It_tables = tables.begin(); It_tables != tables.end();
           ++It_tables) {
        wait_if_paused();
        if (_stop) {
          _working = false;
          return;
        }
        std::string table_pattern = *It_tables;
        size_t dotpos = table_pattern.find('.');
        std::string column_pattern;
        if (dotpos != std::string::npos)
          column_pattern = table_pattern.substr(dotpos + 1);
        else
          column_pattern = '%';
        table_pattern = table_pattern.substr(0, dotpos);
        if (table_pattern.empty())
          table_pattern = "%";

        std::string query;
        if (table_pattern == "%") {
          query.append(base::sqlstring("SHOW FULL TABLES FROM ! WHERE Table_type = 'BASE TABLE'", 0) << schema_name);
        } else {
          query.append(base::sqlstring("SHOW TABLES FROM ! LIKE ?", 0) << schema_name << table_pattern);
        }
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));
        while (rs->next()) {
          std::string table = rs->getString(1);
          schemas_tables[schema_name + '.' + table].push_back(column_pattern);
        }
      }
    }
  }
  int cntr = 0;
  for (std::map<std::string, std::vector<std::string> >::const_iterator It = schemas_tables.begin();
       It != schemas_tables.end(); ++It) {
    // Pick columns
    std::string table_name = It->first;
    size_t dotpos = table_name.find('.');
    std::string schema_name = table_name.substr(0, dotpos);
    _state = std::string("SELECT data from ") + schema_name + "." + table_name;
    table_name = table_name.substr(++dotpos);
    std::vector<std::string> columns = It->second;
    std::string like_clause;
    static const std::string like_pattern = "Field LIKE ? OR ";
    for (std::vector<std::string>::const_iterator It_cols = columns.begin(); It_cols != columns.end(); ++It_cols)
      like_clause.append(std::string(base::sqlstring(like_pattern.c_str(), base::UseAnsiQuotes) << *It_cols));
    like_clause.append("FALSE");

    wait_if_paused();
    if (_stop) {
      _working = false;
      return;
    }

    std::list<std::string> pk_columns;
    bool match_PK = false;
    std::list<std::string> select_columns;
    try {
      std::unique_ptr<sql::Statement> stmt(_db_conn->createStatement());
      std::unique_ptr<sql::ResultSet> rs(
        stmt->executeQuery(std::string(base::sqlstring("SHOW COLUMNS FROM !.! WHERE ", base::QuoteOnlyIfNeeded)
                                       << schema_name << table_name)
                             .append(like_clause)));
      while (rs->next()) {
        std::string column = rs->getString(1);
        std::string column_type = rs->getString(2);
        if ((_search_data_type == search_all_types) ||
            ((_search_data_type & numeric_type) && is_numeric_type(column_type)) ||
            ((_search_data_type & datetime_type) && is_datetime_type(column_type)) ||
            ((_search_data_type & text_type) && is_string_type(column_type))) {
          if (rs->getString(4) == "PRI") {
            select_columns.push_front(column);
            pk_columns.push_back(column);
            match_PK = true; // PK should be searched, not just displayed
          }
          select_columns.push_back(column);
        } else {
          if (rs->getString(4) == "PRI") {
            select_columns.push_front(column);
            pk_columns.push_back(column);
          }
        }
      }
    } catch (std::exception& exc) {
      logWarning("Could not get columns list from %s.%s: %s\n", schema_name.c_str(), table_name.c_str(), exc.what());
    }
    // Add PK col if there is at least one column matching pattern and it it wasn't added during col patterns search
    if (pk_columns.empty() && !select_columns.empty()) {
      try {
        std::unique_ptr<sql::Statement> stmt(_db_conn->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(
          std::string(base::sqlstring("SHOW COLUMNS FROM !.! WHERE `Key` = 'PRI'", base::QuoteOnlyIfNeeded)
                      << schema_name << table_name)));
        while (rs->next()) {
          select_columns.push_back(rs->getString(1));
          pk_columns.push_back(rs->getString(1));
        }
        // set PK col to be the first, or push empty string to indicate that there is no PK at all
        if (pk_columns.empty())
          select_columns.push_front("");
      } catch (std::exception& exc) {
        logWarning("Could not get columns list from %s.%s: %s\n", schema_name.c_str(), table_name.c_str(), exc.what());
      }
    }

    { // Build select from columns fetched on previous step and use it to collect data
      wait_if_paused();
      if (_stop) {
        _working = false;
        return;
      }
      ++cntr;
      std::string limit_clause("");
      if (_limit_counter > 0) {
        size_t limit = std::min(_limit_counter, _limt_per_table);
        std::stringstream sout;
        sout << "LIMIT " << limit;
        limit_clause = sout.str();
      } else if (_limt_per_table) {
        std::stringstream sout;
        sout << "LIMIT " << _limt_per_table;
        limit_clause = sout.str();
      }

      _progress = (cntr * 1.f) / (schemas_tables.size());
      select_func(schema_name, table_name, pk_columns, select_columns, limit_clause, match_PK);
      _searched_tables++;

      if ((_limit_total > 0) && (_limit_counter <= 0))
        break;
    }
  }
  if (_searched_tables == 0)
    _state = "No tables were searched";
  else
    _state = base::strfmt("Search completed in %i tables", _searched_tables);
  _progress = 1;
  _working = false;
}

DBSearchPanel::DBSearchPanel()
  : Box(false),
    _progress_box(true),
    _results_tree(mforms::TreeAltRowColors),
    _update_timer(NULL),
    _search_finished(true) {
  set_spacing(8);

  _pause_button.set_text("Pause");
  scoped_connect(_pause_button.signal_clicked(), std::bind(&DBSearchPanel::toggle_pause, this));

  _progress_box.set_spacing(4);

  _progress_label.set_text(_("Searching in server..."));
  add(&_progress_label, false, true);
  _progress_box.add(&_progress_bar, true, true);
  _progress_box.add(&_pause_button, false, true);
  add(&_progress_box, false, true);

  _results_tree.set_selection_mode(mforms::TreeSelectMultiple);
  _results_tree.add_column(mforms::StringColumnType, "Schema", 100);
  _results_tree.add_column(mforms::StringColumnType, "Table", 100);
  _results_tree.add_column(mforms::StringColumnType, "Key", 80);
  _results_tree.add_column(mforms::StringColumnType, "Column", 100);
  _results_tree.add_column(mforms::StringColumnType, "Data", 800);
  _results_tree.end_columns();
  add(&_results_tree, true, true);

  _results_tree.set_context_menu(&_context_menu);
  _context_menu.signal_will_show()->connect(std::bind(&DBSearchPanel::prepare_menu, this));

  _matches_label.set_text(_("Matches:"));
  add(&_matches_label, false, true);
}

DBSearchPanel::~DBSearchPanel() {
  stop_search_if_working();
  if (_update_timer)
    bec::GRTManager::get()->cancel_timer(_update_timer);
}

void DBSearchPanel::activate_menu_item(const std::string& action) {
  std::list<mforms::TreeNodeRef> selection(_results_tree.get_selection());
  if (selection.empty())
    return;

  if (action == "copy_query") {
    std::string res;
    std::set<std::string> added;
    for (std::list<mforms::TreeNodeRef>::const_iterator n = selection.begin(); n != selection.end(); ++n) {
      if ((*n)->get_string(0).empty()) // match node
      {
        mforms::TreeNodeRef parent((*n)->get_parent());
        if (added.find(parent->get_tag()) == added.end()) {
          added.insert(parent->get_tag());
          res.append(parent->get_tag()).append(";\n");
        }
      } else {
        if (added.find((*n)->get_tag()) == added.end()) {
          added.insert((*n)->get_tag());
          res.append((*n)->get_tag()).append(";\n");
        }
      }
    }
    mforms::Utilities::set_clipboard_text(res);
  } else if (action == "copy_query_for_selected") {
    std::map<std::string, std::string> key_column;
    std::map<std::string, std::string> pks;

    for (std::list<mforms::TreeNodeRef>::const_iterator n = selection.begin(); n != selection.end(); ++n) {
      if ((*n)->get_string(0).empty()) // match node
      {
        mforms::TreeNodeRef parent((*n)->get_parent());
        std::string s = base::sqlstring("!.!", base::QuoteOnlyIfNeeded) << parent->get_string(0)
                                                                        << parent->get_string(1);
        if (pks.find(s) == pks.end()) {
          pks[s] = (*n)->get_string(2);
          key_column[s] = base::join(_key_columns[parent->get_tag()], ", ");
        } else
          pks[s].append(",").append((*n)->get_string(2));
      }
    }
    std::string res;
    for (std::map<std::string, std::string>::const_iterator it = pks.begin(); it != pks.end(); ++it) {
      if (key_column.find(it->first) != key_column.end()) {
        std::string q = base::sqlstring(("SELECT * FROM " + it->first + " WHERE ! IN (" + it->second + ");").c_str(),
                                        base::QuoteOnlyIfNeeded)
                        << key_column[it->first];
        res.append(q).append("\n");
      }
    }
    mforms::Utilities::set_clipboard_text(res);
  } else if (action == "copy_pks") {
    std::string pks;
    for (std::list<mforms::TreeNodeRef>::const_iterator n = selection.begin(); n != selection.end(); ++n) {
      if ((*n)->get_string(0).empty()) {
        if (!pks.empty())
          pks.append(",");
        pks.append((*n)->get_string(2));
      }
    }
    mforms::Utilities::set_clipboard_text(pks);
  } else if (action == "copy_query_for_selected_table") {
    std::string pks;

    for (size_t c = selection.front()->count(), i = 0; i < c; i++) {
      mforms::TreeNodeRef child = selection.front()->get_child((int)i);
      if (!pks.empty())
        pks.append(",");
      pks.append(child->get_string(2));
    }

    mforms::TreeNodeRef tref = selection.front();

    std::list<std::string> kcols;
    if (_key_columns.find(tref->get_tag()) != _key_columns.end())
      kcols = _key_columns[tref->get_tag()];

    if (kcols.empty())
      return;

    std::string q = base::sqlstring(("SELECT * FROM !.! WHERE ! IN (" + pks + ");").c_str(), base::QuoteOnlyIfNeeded)
                    << tref->get_string(0) << tref->get_string(1) << kcols.front();
    mforms::Utilities::set_clipboard_text(q);
  } else if (action == "copy_pks_table") {
    std::string pks;
    for (size_t c = selection.front()->count(), i = 0; i < c; i++) {
      mforms::TreeNodeRef child = selection.front()->get_child((int)i);
      if (!pks.empty())
        pks.append(",");
      pks.append(child->get_string(2));
    }

    mforms::Utilities::set_clipboard_text(pks);
  }
}

void DBSearchPanel::prepare_menu() {
  _context_menu.remove_all();

  bool searcher_is_working = !_search_finished || (_searcher && _searcher->is_working());

  int table_selected = 0;
  int column_selected = 0;

  std::list<mforms::TreeNodeRef> selection(_results_tree.get_selection());
  for (std::list<mforms::TreeNodeRef>::const_iterator n = selection.begin(); n != selection.end(); ++n) {
    if (!(*n)->get_tag().empty())
      table_selected++;
    else
      column_selected++;
  }

  mforms::MenuItem* item;
  if (column_selected) {
    //        item = _context_menu.add_item_with_title(_("Query Matching Rows"),
    //        std::bind(&DBSearchPanel::activate_menu_item, this, "show_matches"), "show_matches");
    //        item->set_enabled(table_selected == 0);
    item = _context_menu.add_item_with_title(
      _("Copy Query"), std::bind(&DBSearchPanel::activate_menu_item, this, "copy_query"), "Copy Query", "copy_query");
    if (searcher_is_working)
      item->set_enabled(false);

    item = _context_menu.add_item_with_title(
      _("Copy Query for Matches"), std::bind(&DBSearchPanel::activate_menu_item, this, "copy_query_for_selected"),
      "Copy Query for Matches", "copy_query_for_selected");
    if (searcher_is_working)
      item->set_enabled(false);
    else
      item->set_enabled(table_selected == 0);

    item = _context_menu.add_item_with_title(
      _("Copy Keys"), std::bind(&DBSearchPanel::activate_menu_item, this, "copy_pks"), "Copy Keys", "copy_pks");
    if (searcher_is_working)
      item->set_enabled(false);
    else
      item->set_enabled(table_selected == 0);
  } else {
    //        item = _context_menu.add_item_with_title(_("Query Matching Rows"),
    //        std::bind(&DBSearchPanel::activate_menu_item, this, "view_matches"), "show_matches");
    //        item->set_enabled(table_selected > 0);
    item = _context_menu.add_item_with_title(
      _("Copy Query"), std::bind(&DBSearchPanel::activate_menu_item, this, "copy_query"), "Copy Query", "copy_query");
    if (searcher_is_working)
      item->set_enabled(false);
    else
      item->set_enabled(table_selected > 0);

    item = _context_menu.add_item_with_title(
      _("Copy Query for Matches"), std::bind(&DBSearchPanel::activate_menu_item, this, "copy_query_for_selected_table"),
      "Copy Query for Matches", "copy_query_for_selected_table");
    if (searcher_is_working)
      item->set_enabled(false);
    else
      item->set_enabled(table_selected == 1);

    item = _context_menu.add_item_with_title(
      _("Copy Keys"), std::bind(&DBSearchPanel::activate_menu_item, this, "copy_pks_table"), "Copy Keys", "copy_pks_table");
    if (searcher_is_working)
      item->set_enabled(false);
    else
      item->set_enabled(table_selected == 1);
  }
}

void DBSearchPanel::load_model(mforms::TreeNodeRef tnode) {
  _key_columns.clear();
  for (size_t c = _searcher->search_results().size(), i = tnode->count(); i < c; i++) {
    const DBSearch::column_data_t& rows = _searcher->search_results()[i].data;
    mforms::TreeNodeRef table_node = tnode->add_child();
    table_node->set_string(0, _searcher->search_results()[i].schema);
    table_node->set_string(1, _searcher->search_results()[i].table);
    table_node->set_string(4, base::strfmt("%i rows matched", (int)rows.size()).c_str());
    table_node->set_tag(_searcher->search_results()[i].query);
    _key_columns.insert(std::make_pair(table_node->get_tag(), _searcher->search_results()[i].keys));

    for (DBSearch::column_data_t::const_iterator It_rows = rows.begin(); It_rows != rows.end(); ++It_rows) {
      std::string cols;
      std::string data;
      mforms::TreeNodeRef data_node = table_node->add_child();
      std::vector<std::pair<std::string, std::string> >::const_iterator It_cols = It_rows->begin();
      data_node->set_string(2, (It_cols++)->second);
      for (; It_cols != It_rows->end(); ++It_cols) {
        if (It_cols->second.empty())
          continue;
        if (!cols.empty())
          cols.append(", ");
        cols.append(It_cols->first);
        if (!data.empty())
          data.append(", ");
        data.append(It_cols->second);
      }
      data_node->set_string(3, cols);
      data_node->set_string(4, data);
    }
  }
};

void DBSearchPanel::search(sql::ConnectionWrapper connection, const std::string& search_keyword,
                           const grt::StringListRef& filter_list, const SearchMode search_mode, const int limit_total,
                           const int limt_per_table, const bool invert, const int search_data_type,
                           const std::string cast_to, std::function<void(grt::ValueRef)> finished_callback,
                           std::function<void()> failed_callback) {
  if (_searcher)
    return;
  _progress_label.show(true);
  _progress_box.show(true);

  _results_tree.clear();

  stop_search_if_working();
  _search_finished = false;
  if (_update_timer)
    bec::GRTManager::get()->cancel_timer(_update_timer);
  _searcher = std::shared_ptr<DBSearch>(new DBSearch(connection, search_keyword, filter_list, search_mode, limit_total,
                                                     limt_per_table, invert, search_data_type, cast_to));
  load_model(_results_tree.root_node());
  std::function<void()> fsearch = (std::bind(&DBSearch::search, _searcher.get()));
  // fsearch = (std::bind(&DBSearch::count, _searcher.get()));//COUNT test
  _searcher->prepare();
  bec::GRTManager::get()->execute_grt_task("Search", std::bind(call_search, fsearch, failed_callback),
                                           finished_callback);
  while (_searcher->is_starting())
    ;
  _update_timer = bec::GRTManager::get()->run_every(std::bind(&DBSearchPanel::update, this), 1);
}

bool DBSearchPanel::update() {
  bool is_working = false;
  if (_searcher) {
    base::MutexLock search_lock(_searcher->get_search_result_mutex());
    is_working = _searcher->is_working();
    if (_searcher->is_paused()) {
      _progress_label.set_text("Paused");
      //            _progress_bar.stop();
    } else {
      //            _progress_bar.start();
      _progress_bar.set_value(_searcher->get_progress());
      _progress_label.set_text(_searcher->get_state());
      std::string matches = base::strfmt("%i rows matched in %i searched tables", _searcher->matched_rows(),
                                         _searcher->searched_table_count());
      _matches_label.set_text(matches);
      load_model(_results_tree.root_node());
    }
  }
  if (!is_working) {
    _searcher.reset();
    _progress_label.show(false);
    _progress_box.show(false);
  }
  return is_working;
}

void DBSearchPanel::toggle_pause() {
  if (_searcher) {
    _searcher->toggle_pause();
    _pause_button.set_text(_searcher->is_paused() ? "Resume" : "Pause");
    _search_finished = _searcher->is_paused();
  }
}

bool DBSearchPanel::stop_search_if_working() {
  if (_searcher && _searcher->is_working()) {
    _searcher->stop();
    return true;
  }
  return false;
}
