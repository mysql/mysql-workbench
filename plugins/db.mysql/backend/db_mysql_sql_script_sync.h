
#pragma once

#include "db_mysql_public_interface.h"
#include "grts/structs.db.mysql.h"
#include "grt/grt_string_list_model.h"
#include "diff_tree.h"
#include "db_mysql_validation_page.h"
#include "grtdb/diff_dbobjectmatch.h"

class SynchronizeDifferencesPageBEInterface {
protected:
  std::shared_ptr<DiffTreeBE> _diff_tree;
  StringRef _sync_profile_name;

public:
  SynchronizeDifferencesPageBEInterface(){};
  virtual ~SynchronizeDifferencesPageBEInterface(){};
  StringRef get_sync_profile_name() {
    return _sync_profile_name;
  };
  void set_sync_profile_name(StringRef sync_profile_name) {
    _sync_profile_name = sync_profile_name;
  };
  ValueRef get_model_object(const bec::NodeId &node) const {
    return _diff_tree->get_node_with_id(node)->get_model_part().get_object();
  };
  ValueRef get_db_object(const bec::NodeId &node) const {
    return _diff_tree->get_node_with_id(node)->get_db_part().get_object();
  };
  void set_next_apply_direction(bec::NodeId nodeid) {
    _diff_tree->set_next_apply_direction(nodeid);
  }
  void set_apply_direction(bec::NodeId nodeid, DiffNode::ApplicationDirection dir, bool recursive) {
    _diff_tree->set_apply_direction(nodeid, dir, recursive);
  }
  DiffNode::ApplicationDirection get_apply_direction(bec::NodeId nodeid) {
    return _diff_tree->get_apply_direction(nodeid);
  }
  virtual db_mysql_CatalogRef get_model_catalog() = 0;
  virtual void get_compared_catalogs(db_CatalogRef &left, db_CatalogRef &right) = 0;
  virtual std::string get_col_name(const size_t col_id) = 0;
  virtual std::string get_sql_for_object(GrtNamedObjectRef obj) = 0;
  virtual std::shared_ptr<DiffTreeBE> init_diff_tree(const std::vector<std::string> &schemata,
                                                     const grt::ValueRef &ext_cat, const grt::ValueRef &cat2,
                                                     grt::StringListRef SchemaSkipList, grt::DictRef options) = 0;
};

struct WBPLUGINDBMYSQLBE_PUBLIC_FUNC DbMySQLScriptSyncException : public std::logic_error {
  DbMySQLScriptSyncException(const std::string &message) : std::logic_error(message) {
  }
};

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC DbMySQLScriptSync : public DbMySQLValidationPage,
                                                        public SynchronizeDifferencesPageBEInterface {
  // db_mysql_CatalogRef _catalog;
  db_mysql_CatalogRef _org_cat;
  db_mysql_CatalogRef _mod_cat_copy;
  grt::StringListRef _alter_list;
  grt::ListRef<GrtNamedObject> _alter_object_list;
  grt::DictRef _options;
  grt::DictRef _db_options;

  // options
  std::string _input_filename1, _input_filename2;
  std::string _output_filename;
  std::vector<std::string> schemata_list; // all schemata present on server (unfiltered)

  std::shared_ptr<DiffChange> _alter_change;

  void sync_finished(grt::ValueRef res);
  grt::ValueRef sync_task(grt::StringRef);
  db_mysql_CatalogRef get_cat_from_file_or_tree(std::string filename, std::string &error_msg);

protected:
  virtual db_mysql_CatalogRef get_model_catalog();
  virtual void get_compared_catalogs(db_CatalogRef &left, db_CatalogRef &right);

public:
  DbMySQLScriptSync();
  virtual ~DbMySQLScriptSync();

  void start_sync();

  void set_option(const std::string &name, const std::string &value);

  std::shared_ptr<DiffTreeBE> init_diff_tree(const std::vector<std::string> &schemata, const ValueRef &left,
                                             const ValueRef &right, StringListRef SchemaSkipList = StringListRef(),
                                             grt::DictRef options = grt::DictRef());

  std::string get_sql_for_object(GrtNamedObjectRef obj);

  void set_options(grt::DictRef options) {
    _options = options;
  }
  grt::DictRef get_options() const {
    return _options.is_valid() ? _options : grt::DictRef(true);
  }

  void set_db_options(grt::DictRef db_options) {
    _db_options = db_options;
  };
  grt::DictRef get_db_options() const {
    return _db_options.is_valid() ? _db_options : grt::DictRef(true);
  }

  grt::StringRef generate_alter(db_mysql_CatalogRef org_cat, db_mysql_CatalogRef org_cat_copy,
                                db_mysql_CatalogRef mod_cat_copy);

  std::string generate_diff_tree_script();
  std::string generate_diff_tree_report();

  void apply_changes_to_model();

  void save_sync_profile();
  void restore_sync_profile(db_CatalogRef catalog);
  std::string get_col_name(const size_t col_id);

  void restore_overriden_names();
};
