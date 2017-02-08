
#pragma once

#include "grt/grt_manager.h"
#include "grts/structs.db.mysql.h"
#include "grt/grt_string_list_model.h"
#include "diff_tree.h"
#include "db_mysql_sql_script_sync.h"

class DbMySQLDiffAlterException : public std::logic_error {
public:
  DbMySQLDiffAlterException(const std::string &message) : std::logic_error(message) {
  }
};

class DbMySQLDiffAlter : public SynchronizeDifferencesPageBEInterface {
  grt::StringListRef _alter_list;
  grt::ListRef<GrtNamedObject> _alter_object_list;
  db_mysql_CatalogRef _left_catalog, _right_catalog;
  std::shared_ptr<DiffChange> _alter_change;
  db_mysql_CatalogRef _left_cat_copy;
  grt::DictRef _db_options;

public:
  void set_db_options(grt::DictRef db_options) {
    _db_options = db_options;
  };
  grt::DictRef get_db_options() const {
    return _db_options.is_valid() ? _db_options : grt::DictRef();
  };
  virtual db_mysql_CatalogRef get_model_catalog() {
    return db_mysql_CatalogRef::cast_from(grt::GRT::get()->get("/wb/doc/physicalModels/0/catalog"));
  }
  virtual void get_compared_catalogs(db_CatalogRef &left, db_CatalogRef &right) {
    left = _left_catalog;
    right = _right_catalog;
  }

  DbMySQLDiffAlter();
  virtual ~DbMySQLDiffAlter();

  std::string generate_alter();

  virtual std::shared_ptr<DiffTreeBE> init_diff_tree(const std::vector<std::string> &schemata,
                                                     const grt::ValueRef &ext_cat, const grt::ValueRef &cat2,
                                                     grt::StringListRef SchemaSkipList, grt::DictRef options);

  virtual std::string get_col_name(const size_t col_id);

  virtual std::string get_sql_for_object(GrtNamedObjectRef obj) {
    std::string result;
    for (size_t i = 0; i < _alter_list.count(); ++i)
      if (_alter_object_list.get(i) == obj) {
        result.append(_alter_list.get(i)).append("\n");
      }
    return result;
  }

  void restore_overriden_names();
};
