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

#include "testgrt.h"

#include "grt_test_utility.h"

#include "grtpp.h"
#include "diffchange.h"
#include "grts/structs.db.mgmt.h"
#include "util_functions.h"
#include "grtdb/diff_dbobjectmatch.h"

using namespace std;
using namespace tut;

BEGIN_TEST_DATA_CLASS(grt_diff)
public:
GRT grt;
db_mgmt_RdbmsRef rdbms;
END_TEST_DATA_CLASS

BEGIN_TEST_DATA_CLASS(grt_diff_suite)
END_TEST_DATA_CLASS

BEGIN_TEST_DATA_CLASS(grt_diff_omf_suite)
END_TEST_DATA_CLASS

struct DiffSubSuiteParam {
  GRT* grt;
  std::string source_file;
  std::string target_file;
  bool logging;
};

typedef std::map<std::string, std::vector<DiffSubSuiteParam> > DiffTestsParams;
DiffTestsParams tests_params;

void test_files(GRT& grt, std::string source_file, std::string target_file, bool logging = false, grt::Omf* omf = NULL);

void test_suites(GRT& grt, const char* rootpath, bool run_single_test, const char* single_suite,
                 const char* single_test, grt::Omf* omf = NULL);

TEST_MODULE(grt_diff, "GRT: diff");

TEST_FUNCTION(1) {
  grt.scan_metaclasses_in("../../res/grt/");
  grt.scan_metaclasses_in("data");
  grt.end_loading_metaclasses();

  rdbms = db_mgmt_RdbmsRef::cast_from(grt.unserialize("data/res/mysql_rdbms_info.xml"));
  ensure("db_mgmt_RdbmsRef initialization", rdbms.is_valid());

  grt.set_root(rdbms);
}

TEST_FUNCTION(2) {
  const bool run_single_test = false;
  const char* single_suite = "list-reorder-simple-type";
  const char* single_test = "s04_order_end_beg.xml";

  std::string rootpath = "data/diff";
  grt::default_omf omf;
  test_suites(rootpath.c_str(), run_single_test, single_suite, single_test, &omf);
}

TEST_FUNCTION(3) {
  const bool run_single_test = false;
  const char* single_suite = "";
  const char* single_test = "";

  std::string rootpath = "data/diff-omf";
  grt::default_omf omf;
  test_suites(rootpath.c_str(), run_single_test, single_suite, single_test, &omf);
}

END_TESTS

#if 1

void test_suites(GRT& grt, const char* rootpath, bool run_single_test, const char* single_suite,
                 const char* single_test, grt::Omf* omf) {
  GError* err = NULL;
  GDir* root = g_dir_open(rootpath, 0, &err);
  if (err) {
    std::cout << ((err->message) ? (const char*)err->message : (const char*)"unknown error") << std::endl;
    g_error_free(err);
    err = NULL;
  }

  const char* suite_name = single_suite;

  while ((suite_name = g_dir_read_name(root))) {
    if (run_single_test)
      suite_name = single_suite;

    // Filter out unwanted folders (e.g. from version control or disabled test cases).
    // TODO: x_integration and x_sakira-db must be fixed yet (xml recreated from model). They are disabled currently in
    // file system.
    if (strlen(suite_name) < 1 || suite_name[0] == '.')
      continue;

    GDir* test_suite = g_dir_open((rootpath + std::string("/") + suite_name).c_str(), 0, &err);
    if (err) {
      g_error_free(err);
      err = NULL;
    } else {
      std::string test_group_name = std::string("grt_diff::") + suite_name;
      // TODO LEAK!
      // DiffTestsParams::iterator iter= tests_params.insert(std::make_pair(test_group_name,
      // std::vector<DiffSubSuiteParam>())).first;
      // Test_group<testsuite> *group= new Test_group<testsuite>(&test_params, iter->first.c_str(),
      // iter->first.c_str());

      std::string initial_object;
      const char* test_name = single_test;

      while ((test_name = g_dir_read_name(test_suite))) {
        // Let's assume that initial_object goes first - TODO!!
        if (initial_object.empty()) {
          initial_object = test_name;
          if (!run_single_test)
            continue;
        }
        DiffSubSuiteParam p;
        p.grt = &grt;
        p.source_file = rootpath + std::string("/") + suite_name + "/" + initial_object;
        p.target_file = rootpath + std::string("/") + suite_name + "/" + (run_single_test ? single_test : test_name);
        p.logging = run_single_test;

        test_files(p.source_file, p.target_file, p.logging, omf);

        if (run_single_test)
          break;
      };
    }
    if (test_suite)
      g_dir_close(test_suite);

    if (run_single_test)
      break;
  }
  if (root)
    g_dir_close(root);
}

void test_files(GRT& grt, std::string source_file, std::string target_file, bool logging, grt::Omf* omf) {
  ValueRef v(source_file.size() > 0 ? grt.unserialize(source_file) : ValueRef());
  ValueRef target(grt.unserialize(target_file));
  ValueRef target1(grt.unserialize(target_file));

  assure(v.is_valid());
  assure(target.is_valid());
  if (logging) {
    // std::ofstream target_log(target_file + ".old.log");
    // std::cout << myx_grt_value_to_xml(grt.grt(), v) << std::endl;
    // target_log << myx_grt_value_to_xml(grt.grt(), target) << std::endl;
  }

  test_time_point t1;
  grt::NormalizedComparer normalizer(&grt);
  normalizer.init_omf(omf);
  std::shared_ptr<DiffChange> change = diff_make(v, target, omf);
  test_time_point t2;
  if (logging) {
    double time_rate = test_time_point(t2 - t1).ticks_ / 1000.;
    std::cout << "Xmldiff: " << time_rate << " [sec]" << std::endl;
    std::cout << std::endl;
    if (change)
      change->dump_log(0);
  }

  ValueRef source_bkup = copy_value(v, true);
  ValueRef source_to_change = copy_value(v, true);
  test_time_point t3;
  //  if (change)
  //    diff_apply(source_to_change, *change);
  test_time_point t4;

  if (logging) {
    double time_rate = (t4 - t3).ticks_ / 1000.;
    std::cout << "Xmlapply: " << time_rate << " [sec]" << std::endl;

    grt.serialize(source_to_change, target_file + ".log");
  }

  grt::NormalizedComparer normalizer2(&grt);
  normalizer2.init_omf(omf);

  // QQQ Likely will fail due to diff_apply removal
  std::shared_ptr<DiffChange> zero_change = diff_make(source_to_change, target1, omf);

  ensure("unexpected change", zero_change == NULL);

  // QQQ
  // Must be fixed as the diff code uses pointer invariance for grt::Object values to determine changed entries.
  assure_grt_values_equal(source_bkup, v);
}

#endif
