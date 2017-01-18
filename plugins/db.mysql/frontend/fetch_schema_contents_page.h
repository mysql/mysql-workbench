#pragma once

#include "grtui/wizard_progress_page.h"

class FetchSchemaContentsProgressPage : public WizardProgressPage {
public:
  FetchSchemaContentsProgressPage(WizardForm *form, const char *name = "fetchSchema")
    : WizardProgressPage(form, name, true) {
    set_title(_("Retrieve and Reverse Engineer Schema Objects"));
    set_short_title(_("Retrieve Objects"));

    add_async_task(_("Retrieve Objects from Selected Schemata"),
                   std::bind(&FetchSchemaContentsProgressPage::perform_fetch, this),
                   _("Retrieving object lists from selected schemata..."));

    add_task(_("Check Results"), std::bind(&FetchSchemaContentsProgressPage::perform_check, this),
             _("Checking Retrieved data..."));

    end_adding_tasks(_("Retrieval Completed Successfully"));

    set_status_text("");
  }

  bool perform_fetch() {
    execute_grt_task(std::bind(&FetchSchemaContentsProgressPage::do_fetch, this), false);
    return true;
  }

  bool perform_check() {
    _finished = true;

    return true;
  }

  grt::ValueRef do_fetch() {
    grt::StringListRef selection(grt::StringListRef::cast_from(values().get("selectedSchemata")));
    std::vector<std::string> names;

    for (grt::StringListRef::const_iterator iter = selection.begin(); iter != selection.end(); ++iter)
      names.push_back(*iter);

    // tell the backend about the selection
    _dbplugin->schemata_selection(names, true);

    _dbplugin->load_db_objects(Db_plugin::dbotTable);
    _dbplugin->load_db_objects(Db_plugin::dbotView);
    if (!values().get_int("SkipRoutines"))
      _dbplugin->load_db_objects(Db_plugin::dbotRoutine);
    if (!values().get_int("SkipTriggers"))
      _dbplugin->load_db_objects(Db_plugin::dbotTrigger);

    return grt::ValueRef();
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      _finished = false;
      reset_tasks();
    }
    WizardProgressPage::enter(advancing);
  }

  virtual bool allow_next() {
    return _finished;
  }

  void set_db_plugin(Db_plugin *dbplugin) {
    _dbplugin = dbplugin;
  }

private:
  Db_plugin *_dbplugin;
  bool _finished;
};
