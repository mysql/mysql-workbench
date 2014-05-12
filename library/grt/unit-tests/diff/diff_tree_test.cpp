/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "grt_test_utility.h"
#include "testgrt.h"
#include "diff/grtdiff.h"
#include "grtpp.h"
#include "diff/diffchange.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"
#include "grtdb/diff_dbobjectmatch.h"
#include "wb_helpers.h"
#include "synthetic_mysql_model.h"
#include "module_db_mysql.h"
#include "backend/diff_tree.h"

using namespace grt;

static grt::DictRef get_traits(grt::GRT* grt, bool case_sensitive = false)
{
  grt::DictRef traits(grt);
    traits.set("CaseSensitive", grt::IntegerRef(case_sensitive));
  traits.set("maxTableCommentLength", grt::IntegerRef(60));
  traits.set("maxIndexCommentLength", grt::IntegerRef(0));
  traits.set("maxColumnCommentLength", grt::IntegerRef(255));
  return traits;
}

BEGIN_TEST_DATA_CLASS(sync_diff)
public:
  WBTester tester;
END_TEST_DATA_CLASS

TEST_MODULE(sync_diff, "sync diff");

/**
 * there was a sync problem when model has difference in schema collation and table collation,
 * wb was seeing only table difference, there was SR connect to bug #16492371, but the bug itself was not related
 */
TEST_FUNCTION(1)
{
  ValueRef source_val(tester.grt->unserialize("data/diff/sync-catalogs-collations/source_catalog.xml"));
  ValueRef target_val(tester.grt->unserialize("data/diff/sync-catalogs-collations/target_catalog.xml"));

  db_mysql_CatalogRef mod_cat = db_mysql_CatalogRef::cast_from(source_val);
  db_mysql_CatalogRef org_cat = db_mysql_CatalogRef::cast_from(target_val);

  grt::NormalizedComparer normalizer(tester.grt, get_traits(tester.grt, true));
  grt::DbObjectMatchAlterOmf omf;
  omf.dontdiff_mask = 3;
  normalizer.init_omf(&omf);
  boost::shared_ptr<DiffChange> diff_change= diff_make(org_cat, mod_cat, &omf);

  DbMySQLImpl *diffsql_module= tester.grt->get_native_module<DbMySQLImpl>();

  grt::StringListRef alter_map(tester.grt);
  grt::ListRef<GrtNamedObject> alter_object_list(tester.grt);
  grt::DictRef options(tester.grt);
  options.set("UseFilteredLists", grt::IntegerRef(0));
  options.set("OutputContainer", alter_map);
  options.set("OutputObjectContainer", alter_object_list);
  options.set("CaseSensitive", grt::IntegerRef(omf.case_sensitive));

  diffsql_module->generateSQL(org_cat, options, diff_change);
  boost::shared_ptr<DiffTreeBE> diff_tree(new ::DiffTreeBE(std::vector<std::string>(), mod_cat, org_cat, diff_change));

  bool foundSchemaDiff = false;
  bool foundTableDiff = false;
  for (std::size_t c = diff_tree->count(), i = 0; i < c; i++)
  {
    bec::NodeId schema((int)i);
    for (int j = 0; j  < diff_tree->count_children(schema); j++)
    {
      bec::NodeId object(diff_tree->get_child(schema, j));
      std::string name;

      diff_tree->get_field(schema, DiffTreeBE::ModelObjectName, name);
      if (name == "chartest" && (diff_tree->get_apply_direction(schema) == DiffNode::ApplyToDb))
      {
        foundSchemaDiff = true;
      }
      diff_tree->get_field(object, DiffTreeBE::ModelObjectName, name);
      if (name == "chartable" && (diff_tree->get_apply_direction(object) == DiffNode::ApplyToDb))
      {
        foundTableDiff = true;
      }

    }

  }

  ensure("Schema and table difference", foundTableDiff && foundSchemaDiff);

}

// Regression test for bug #17454626
TEST_FUNCTION(2)
{
  ValueRef source_val(tester.grt->unserialize("data/diff/sync-catalogs-rowformat/source_catalog.xml"));
  ValueRef target_val(tester.grt->unserialize("data/diff/sync-catalogs-rowformat/target_catalog.xml"));

  db_mysql_CatalogRef source_cat = db_mysql_CatalogRef::cast_from(source_val);
  db_mysql_CatalogRef target_cat = db_mysql_CatalogRef::cast_from(target_val);

  DbMySQLImpl *diffsql_module= tester.grt->get_native_module<DbMySQLImpl>();

  grt::DictRef options(tester.grt);
  options.set("CaseSensitive", grt::IntegerRef(true));

  std::string script = diffsql_module->makeAlterScript(target_cat, source_cat, options);
  
  std::string expected_sql = "data/diff/sync-catalogs-rowformat/good.sql";
  std::ifstream ref(expected_sql.c_str());
  std::stringstream ss(script);

  std::string line, refline;

  tut::ensure(expected_sql, ref.is_open());

  while (ref.good() && ss.good())
  {
    getline(ref, refline);
    getline(ss, line);
    tut::ensure_equals("SQL compare failed", line, refline);
  }
}

END_TESTS
