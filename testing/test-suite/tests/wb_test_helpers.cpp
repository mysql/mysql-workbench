/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"
#include "helpers.h"

#include "grtdb/db_helpers.h"
#include "mdc_canvas_view_image.h"

#include "stub_mforms.h"
#include "stub_utilities.h"

#include "model/wb_overview_physical.h"
#include "grtsqlparser/mysql_parser_services.h"
#include "db_rev_eng_be.h"

#include "structs.test.h"
#include "sqlide/wb_sql_editor_form.h"
#include "sqlide/wb_sql_editor_help.h"
#include "wb_test_helpers.h"

#include "grtdb/db_helpers.h"
#include "wb_connection_helpers.h"

#include "wb_version.h"
#include "wb_test_helpers.h"
#include "casmine.h"


#undef min
#undef max
#undef bool

using namespace wb;
using namespace parsers;

extern void register_all_metaclasses();

base::Logger testLogger(".", getenv("WB_LOG_STDERR") != 0);

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::reinitGRT() {
  grt::GRT::get()->reinitialiseForTests();
  bec::GRTManager::get()->cleanUpAndReinitialize();
}

//----------------------------------------------------------------------------------------------------------------------

WorkbenchTester::WorkbenchTester(bool initPython, const base::Size &apage_size, const WBFrontendCallbacks &callbacks)
: wboptions(new wb::WBOptions("test")), lastView(nullptr), _pageSize(apage_size), _guiLock(false) {
  // Reset any previously set callback, as this is a singleton and might conflict with other tests.
  mforms::stub::UtilitiesWrapper::set_message_callback(std::function<mforms::DialogResult(void)>());
  _wbcallbacks = callbacks;
  if (!_wbcallbacks.show_status_text) {
    _wbcallbacks.show_status_text = [&](const std::string &text) {
      if (std::get<bool>(casmine::CasmineContext::get()->settings["verbose"])) {
        std::cout << text << std::endl;
      }
    };
  }

  if (!_wbcallbacks.create_diagram) {
    _wbcallbacks.create_diagram = [&](const model_DiagramRef &view) -> mdc::ImageCanvasView* {
      auto nview = new mdc::ImageCanvasView(800, 600);
      nview->initialize();
      nview->set_page_size(_pageSize);
      lastView = nview;
      return nview;
    };
  }

  if (!_wbcallbacks.destroy_view) {
    _wbcallbacks.destroy_view = [&](mdc::CanvasView *view) {
      std::ignore = view;
      lastView = nullptr;
    };
  }

  if (!_wbcallbacks.switched_view)
    _wbcallbacks.switched_view = std::bind([](){});
  if (!_wbcallbacks.refresh_gui)
    _wbcallbacks.refresh_gui = std::bind([](){});

  if (!_wbcallbacks.perform_command) {
    _wbcallbacks.perform_command = [&](const std::string &cmd) { std::ignore = cmd; };
  }

  if (!_wbcallbacks.show_file_dialog) {
    _wbcallbacks.show_file_dialog = [&](std::string, std::string, std::string) {
      if (fileDialogInput.empty())
        throw std::logic_error("show_file_dialog() was called, but there's no paths to feed it");

      std::string file = fileDialogInput.front();
      fileDialogInput.pop_front();

      return file;
    };
  }

  if (!_wbcallbacks.lock_gui) {
    _wbcallbacks.lock_gui = [&](bool lock) {
      _guiLock = lock;
    };
  }

  wboptions->init_python = initPython;

#ifdef _MSC_VER

  std::string release = APP_RELEASE_TYPE;
  std::string configuration;
  if (release.empty())
    configuration = "Debug";
  else if (release == "SE")
    configuration = "Release";
  else
    configuration = "Release_OSS";

  wboptions->basedir = "../../bin/x64/" + configuration;

  // This code was added to setup the python environment
  TCHAR szPath[MAX_PATH];
  std::string python_path;
  if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
    std::string full_path = base::wstring_to_string(szPath);
    size_t path_end = full_path.find_last_of('\\');
    if (path_end != std::string::npos) {
      wboptions->plugin_search_path = full_path.substr(0, path_end);
    }
  }

  wboptions->module_search_path = wboptions->basedir;

#else

  wboptions->basedir = casmine::getEnvVar("MWB_DATA_DIR");
  auto testModulesDir = casmine::getEnvVar("TEST_MODULES_DIR");
  if (testModulesDir.empty()) {
    std::cerr << "TEST_MODULES_DIR environment variable is not set" << std::endl;
    wboptions->module_search_path = casmine::getEnvVar("MWB_MODULE_DIR");
  } else {
    wboptions->module_search_path = casmine::getEnvVar("TEST_MODULES_DIR");
  }

  wboptions->library_search_path = casmine::getEnvVar("MWB_LIBRARY_DIR");
  wboptions->struct_search_path = wboptions->basedir + "/grt";
#endif
  wboptions->user_data_dir = casmine::CasmineContext::get()->tmpDataDir();

  mforms::stub::init(wboptions);

  base::Color::prepareForTesting();

  wbui = wb::WBContextUI::get();
  wbui->reinit();
  wb = wbui->get_wb();

  wboptions->testing = true;
  wbui->init(&_wbcallbacks, wboptions);

  // we have to wait for it as it's triggered in the c-tor
  help::DbSqlEditorContextHelp::get()->waitForLoading();

  {
    db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(
      grt::GRT::get()->unserialize(bec::GRTManager::get()->get_basedir() + "/modules/data/mysql_rdbms_info.xml"));
    workbench_WorkbenchRef::cast_from(grt::GRT::get()->get("/wb"))->rdbmsMgmt()->rdbms().insert(rdbms);
  }

  mforms::stub::check();

  wbui->init_finish(wboptions);
}

//----------------------------------------------------------------------------------------------------------------------

WorkbenchTester::~WorkbenchTester() {
  wb->close_document_finish();
  delete wboptions;
  wbui->finalize();
  wbui->cleanUp();
  wb = nullptr;

  WorkbenchTester::reinitGRT();
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::initializeRuntime() {
  std::string prefix;
#ifdef __APPLE__
  prefix = "/../Resources"; // Bundle path.
#endif
  std::string rdbmsInfoPath = wboptions->basedir + prefix + "/modules/data/mysql_rdbms_info.xml";
  std::string typeGroupsPath = wboptions->basedir + prefix + "/data/db_datatype_groups.xml";

  workbench_WorkbenchRef workbench = wb->get_root();
  workbench_DocumentRef doc(grt::Initialized);
  doc->owner(workbench);
  workbench->doc(doc);
  grt::GRT::get()->set("/wb", workbench);

  db_mgmt_ManagementRef mgmt(grt::Initialized);
  workbench->rdbmsMgmt(mgmt);
  mgmt->owner(workbench);

  // Load datatype groups so that it can be found during load of types.
  grt::ListRef<db_DatatypeGroup> grouplist =
    grt::ListRef<db_DatatypeGroup>::cast_from(grt::GRT::get()->unserialize(typeGroupsPath));
  grt::replace_contents(mgmt->datatypeGroups(), grouplist);

  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->unserialize(rdbmsInfoPath));
  if (!rdbms.is_valid())
    throw std::runtime_error("db_mgmt_Rdbms initialization");

  std::string targetVersion = bec::GRTManager::get()->get_app_option_string("DefaultTargetMySQLVersion");
  if (targetVersion.empty())
    targetVersion = "8.0.11";
  rdbms->version(bec::parse_version(targetVersion));
  grt::GRT::get()->set("/rdbms", rdbms);

  mgmt->rdbms().insert(rdbms);
  rdbms->owner(mgmt);
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::executeScript(sql::Statement *stmt, const std::string& script) {
  std::vector<StatementRange> statementRanges;
  MySQLParserServices::get()->determineStatementRanges(script.c_str(), script.size(), ";", statementRanges);
  for (auto &range : statementRanges) {
    std::string sql(script, range.start, range.length);
    stmt->execute(sql);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::createNewDocument() {
  wb->new_document();

  // Focus the Physical model in the overview by default.
  PhysicalOverviewBE *physical_overview = wbui->get_physical_overview();
  if (physical_overview)
    physical_overview->focus_node(1);
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::activateOverview() {
  wbui->set_active_form(wbui->get_physical_overview());
}

//----------------------------------------------------------------------------------------------------------------------

workbench_physical_ModelRef WorkbenchTester::getPmodel() {
  return wb->get_document()->physicalModels()[0];
}

//----------------------------------------------------------------------------------------------------------------------

db_mgmt_RdbmsRef WorkbenchTester::getRdbms() {
  return db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->get("/rdbms"));
}

//----------------------------------------------------------------------------------------------------------------------

workbench_physical_DiagramRef WorkbenchTester::getPview() {
  return getPmodel()->diagrams().get(0);
}

//----------------------------------------------------------------------------------------------------------------------

db_CatalogRef WorkbenchTester::getCatalog() {
  return wb->get_document()->physicalModels().get(0)->catalog();
}

//----------------------------------------------------------------------------------------------------------------------

db_SchemaRef WorkbenchTester::getSchema() {
  return wb->get_document()->physicalModels().get(0)->catalog()->schemata().get(0);
}

//----------------------------------------------------------------------------------------------------------------------

db_mysql_CatalogRef createEmptyCatalog() {
  db_mgmt_RdbmsRef rdbms = db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->get("/rdbms"));

  db_mysql_CatalogRef cat(grt::Initialized);
  cat->version(rdbms->version());
  cat->name("default");
  cat->oldName("default");
  grt::replace_contents(cat->simpleDatatypes(), rdbms->simpleDatatypes());
  grt::replace_contents(cat->characterSets(), rdbms->characterSets());

  return cat;
}

//----------------------------------------------------------------------------------------------------------------------

static mforms::DialogResult messageOtherCallback() {
  return mforms::ResultOther; // Indicates to ignore any pending changes.
}

//----------------------------------------------------------------------------------------------------------------------

bool WorkbenchTester::closeDocument() {
  if (!wb->cancel_idle_tasks()) {
    // Idle tasks are currently being executed. Wait a moment and then try again.
    g_usleep((int)(100000));
    wb->cancel_idle_tasks();
  }
  mforms::stub::UtilitiesWrapper::set_message_callback(messageOtherCallback);
  return wb->close_document();
}

//----------------------------------------------------------------------------------------------------------------------

bool WorkbenchTester::renewDocument() {
  if (!closeDocument())
    return false;

  wb->close_document_finish();
  createNewDocument();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::addFileForFileDialog(const std::string &path) {
  fileDialogInput.push_back(path);
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::addView() {
  wb->get_model_context()->add_new_diagram(wb->get_document()->physicalModels()[0]);

  syncView();

  wbui->set_active_form((bec::UIForm*)wb->get_model_context()->get_diagram_form(lastView));
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::syncView() {
  int i = 0;
  g_usleep((int)(10000));
  i = 1;
  while (lastView == 0 || bec::GRTManager::get()->get_dispatcher()->get_busy()) {
    i++;
    g_usleep((int)(10000));
    wb->flush_idle_tasks(false);
  }
}

//----------------------------------------------------------------------------------------------------------------------

db_mysql_TableRef WorkbenchTester::addTableFigure(const std::string &name, int x, int y) {
  db_SchemaRef schema(getSchema());

  db_mysql_TableRef table(grt::Initialized);
  table->name(name);
  table->owner(schema);
  schema->tables().insert(table);

  wb::ModelDiagramForm *vform = dynamic_cast<wb::ModelDiagramForm *>(wbui->get_active_main_form());

  if (vform == nullptr)
    throw std::logic_error("WorkbenchTester: adding table figure failed.");

  wb->get_component<WBComponentPhysical>()->place_db_object(vform, vform->get_view()->window_to_canvas(x, y), table);

  syncView();

  return table;
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::interactivePlaceDbObjects(int x, int y, std::list<db_DatabaseObjectRef> &objects) {
  ModelDiagramForm *form = 0;
  form = wb->get_model_context()->get_diagram_form(lastView);
  if (!form)
    form = wb->get_model_context()->get_diagram_form_for_diagram_id(
      wb->get_document()->physicalModels().get(0)->diagrams().get(0).id());
  wb->get_component<WBComponentPhysical>()->interactive_place_db_objects(form, x, y, objects);
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::openAllDiagrams() {
  workbench_DocumentRef doc = wb->get_document();

  for (int i = 0; i < (int)doc->physicalModels()[0]->diagrams().count(); i++) {
    bec::NodeId node(0);
    node.append(i + 1); // There's the "Add Diagram" node at index 0.
    wbui->get_physical_overview()->activate_node(node);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::flushUntil(float timeout) {
  time_t start = time(NULL);
  while (time(NULL) - start < timeout) {
    g_usleep((int)(100000 * timeout));
    if (wb == nullptr)
      return;
    
    wb->flush_idle_tasks(false);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::flushUntil(float timeout, std::function<bool()> condition) {
  time_t start = time(NULL);
  while (time(NULL) - start < timeout && !condition()) {
    g_usleep((int)(100000 * timeout));
    if (wb == nullptr)
      return;
    wb->flush_idle_tasks(false);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::flushWhile(float timeout, std::function<bool()> condition) {
  time_t start = time(NULL);
  while (time(NULL) - start < timeout && condition()) {
    g_usleep((int)(100000 * timeout));
    if (wb == nullptr)
      return;
    wb->flush_idle_tasks(false);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::flushUntil(float timeout, std::function<size_t()> condition, size_t value) {
  time_t start = time(NULL);
  while ((time(NULL) - start < timeout) && (condition() != value)) {
    g_usleep((int)(100000 * timeout));
    if (wb == nullptr)
      return;
    wb->flush_idle_tasks(false);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void WorkbenchTester::exportPNG(const std::string &path) {
  lastView->export_png(path);
}

//----------------------------------------------------------------------------------------------------------------------

db_mysql_CatalogRef WorkbenchTester::reverseEngineerSchemas(const std::list<std::string> &schema_names) {
  db_mgmt_ConnectionRef properties(grt::Initialized);
  setupConnectionEnvironment(properties, getRdbms()->drivers()[0]);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();
  auto connection = dm->getConnection(properties);

  wb->new_document();
  Db_rev_eng db_rev_eng;
  db_rev_eng.grtm();

  db_rev_eng.db_conn()->set_connection_and_update(properties);

  db_rev_eng.model_catalog()->schemata().clear();
  {
    std::vector<std::string> schemata;
    db_rev_eng.load_schemata(schemata);
    std::sort(schemata.begin(), schemata.end());
    std::vector<std::string> names(schema_names.begin(), schema_names.end());
    std::sort(names.begin(), names.end());
    std::vector<std::string> selection(names.size());
    selection.erase(
                    std::set_intersection(schemata.begin(), schemata.end(), names.begin(), names.end(), selection.begin()),
                    selection.end());
    db_rev_eng.schemata_selection(selection, false);
  }

  db_rev_eng.load_db_objects(Db_rev_eng::dbotTable);
  db_rev_eng.load_db_objects(Db_rev_eng::dbotView);
  db_rev_eng.load_db_objects(Db_rev_eng::dbotRoutine);
  db_rev_eng.load_db_objects(Db_rev_eng::dbotTrigger);

  if (!db_rev_eng.model_catalog()->owner().is_valid())
    throw std::runtime_error("catalog invalid");

  bec::GRTTask::Ref task =
    bec::GRTTask::create_task("reveng task", bec::GRTManager::get()->get_dispatcher(), db_rev_eng.get_task_slot());
  bec::GRTManager::get()->get_dispatcher()->add_task_and_wait(task);
  db_mysql_CatalogRef cat = db_mysql_CatalogRef::cast_from(db_rev_eng.model_catalog());
  if ((cat->schemata().get(0).is_valid()) && (cat->schemata().get(0)->name() == "mydb"))
    cat->schemata().remove(0);

  // Remove the default roles created by new_document() so that it won't interfere with diff results.
  while (cat->roles().count() > 0)
    cat->roles().remove(0);

  return cat;
};

//----------------------------------------------------------------------------------------------------------------------
