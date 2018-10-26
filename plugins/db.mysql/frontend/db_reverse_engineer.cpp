/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtui/grt_wizard_plugin.h"
#include "grtui/wizard_progress_page.h"
#include "grtui/wizard_schema_filter_page.h"
#include "grtui/wizard_object_filter_page.h"
#include "grtui/wizard_finished_page.h"
#include "grtui/grtdb_connect_panel.h"
#include "db_rev_eng_be.h"
#include "base/string_utilities.h"

using namespace grtui;
using namespace mforms;
using namespace base;

namespace DBImport {
#include "grtui/connection_page.h"
#include "fetch_schema_names_page.h"
#include "schema_selection_page.h"
#include "fetch_schema_contents_page.h"

  class WbPluginDbImport;

  class ObjectSelectionPage : public WizardObjectFilterPage {
  public:
    ObjectSelectionPage(WbPluginDbImport *form);
    void setup_filters();

  protected:
    virtual void enter(bool advancing) {
      if (advancing)
        setup_filters();

      WizardObjectFilterPage::enter(advancing);
    }

    virtual bool advance();

    virtual std::string next_button_caption() {
      return execute_caption();
    }

  private:
    std::map<Db_plugin::Db_object_type, DBObjectFilterFrame *> _filters;
    mforms::Box _box;
    mforms::Label _empty_label;
    mforms::CheckBox _autoplace_check;
  };

  //--------------------------------------------------------------------------------

  /* not used
  class ReviewScriptPage : public WizardPage
  {
  public:
    ReviewScriptPage(WbPluginDbImport *form)
      : WizardPage(form, "review"), _text(true)
    {
      set_title(_("Review DDL Script to be Parsed and Imported"));
      set_short_title(_("Review DDL Script"));

      add(&_text, true, true);
      _text.set_read_only(true);
    }


    virtual void enter(bool advancing)
    {
      if (advancing)
      {
        std::string text;

        ((WbPluginDbImport*)_form)->db_plugin()->dump_ddl(text);

        _text.set_value(text);
      }
    }

    virtual std::string next_button_caption()
    {
      return execute_caption();
    }

  private:
    mforms::TextBox _text;
  };
    */

  //--------------------------------------------------------------------------------

  class DBImportProgressPage : public WizardProgressPage {
    TaskRow *_place_task;

  public:
    DBImportProgressPage(WbPluginDbImport *form);

    virtual void enter(bool advancing) {
      bool place = values().get_int("import.place_figures", 0) != 0;

      _place_task->set_enabled(place);

      WizardProgressPage::enter(advancing);
    }

    bool perform_import();
    bool perform_place();

    virtual bool allow_back() {
      return false;
    }
  };

  //--------------------------------------------------------------------------------

  class FinishPage : public WizardFinishedPage {
  public:
    FinishPage(WbPluginDbImport *form);
    virtual void enter(bool advancing);

    virtual bool next_closes_wizard() {
      return true;
    }

  private:
    struct Summary {
      int tables;
      int views;
      int routines;

      Summary() : tables(0), views(0), routines(0) {
      }
    };

    std::string create_summary(const grt::ListRef<GrtObject> &objects) {
      std::map<std::string, Summary> schema_summary;

      std::string summary = _("Summary of Reverse Engineered Objects:\n\n");

      for (grt::ListRef<GrtObject>::const_iterator iter = objects.begin(); iter != objects.end(); ++iter) {
        std::string owner_name = (*iter)->owner()->name();

        if ((*iter).is_instance<db_Schema>()) {
          if (schema_summary.find((*iter)->id()) == schema_summary.end()) {
            Summary s;
            schema_summary[(*iter)->name()] = s;
          }
        } else if ((*iter).is_instance<db_Table>())
          schema_summary[owner_name].tables++;
        else if ((*iter).is_instance<db_View>())
          schema_summary[owner_name].views++;
        else if ((*iter).is_instance<db_Routine>())
          schema_summary[owner_name].routines++;
      }

      for (std::map<std::string, Summary>::const_iterator iter = schema_summary.begin(); iter != schema_summary.end();
           ++iter) {
        if (iter->second.tables == 0 && iter->second.views == 0 && iter->second.routines == 0)
          summary.append(strfmt(_(" - empty schema '%s'\n"), iter->first.c_str()));
        else {
          summary.append(" - ");
          std::string sep = "";
          if (iter->second.tables > 0) {
            summary.append(strfmt(_("%i tables"), iter->second.tables));
            sep = ", ";
          }
          if (iter->second.views > 0) {
            summary.append(strfmt(_("%s%i views"), sep.c_str(), iter->second.views));
            sep = ", ";
          }
          if (iter->second.routines > 0) {
            summary.append(strfmt(_("%s%i routines"), sep.c_str(), iter->second.routines));
            sep = ", ";
          }
          summary.append(strfmt(_(" from schema '%s'\n"), iter->first.c_str()));
        }
      }

      return summary;
    }
  };

  class WbPluginDbImport : public WizardPlugin {
    ConnectionPage *_connection_page;
    FetchSchemaNamesProgressPage *_fetch_progress_page;
    SchemaSelectionPage *_schema_selection_page;
    FetchSchemaContentsProgressPage *_fetch_schemas_progress_page;
    ObjectSelectionPage *_object_selection_page;
    //  ReviewScriptPage *_review_script_page;
    DBImportProgressPage *_import_progress_page;
    FinishPage *_finish_page;

    Db_rev_eng _db_rev_eng;

    std::vector<std::string> load_schemas() {
      std::vector<std::string> schema_names;

      db_plugin()->load_schemata(schema_names);
      return schema_names;
    }

  public:
    WbPluginDbImport(grt::Module *module);

    Db_plugin *db_plugin() {
      return &_db_rev_eng;
    }
    Sql_import *sql_import() {
      return &_db_rev_eng;
    }
  };

  //--------------------------------------------------------------------------------

  WbPluginDbImport::WbPluginDbImport(grt::Module *module) : WizardPlugin(module) {
    set_name("DB Import Wizard");
    _db_rev_eng.grtm();

    _connection_page = new ConnectionPage(this);
    _connection_page->set_db_connection(db_plugin()->db_conn());

    _fetch_progress_page = new FetchSchemaNamesProgressPage(this);
    _fetch_progress_page->set_db_connection(db_plugin()->db_conn());
    _fetch_progress_page->set_load_schemas_slot(std::bind(&WbPluginDbImport::load_schemas, this));

    _schema_selection_page = new SchemaSelectionPage(this);
    _schema_selection_page->set_db_plugin(db_plugin());

    _fetch_schemas_progress_page = new FetchSchemaContentsProgressPage(this);
    _fetch_schemas_progress_page->set_db_plugin(db_plugin());
    _object_selection_page = new ObjectSelectionPage(this);
    //  _review_script_page= new ReviewScriptPage(this);
    _import_progress_page = new DBImportProgressPage(this);
    _finish_page = new FinishPage(this);

    add_page(mforms::manage(_connection_page));
    add_page(mforms::manage(_fetch_progress_page));
    add_page(mforms::manage(_schema_selection_page));
    add_page(mforms::manage(_fetch_schemas_progress_page));
    add_page(mforms::manage(_object_selection_page));
    //  add_page(mforms::manage(_review_script_page));
    add_page(mforms::manage(_import_progress_page));
    add_page(mforms::manage(_finish_page));

    set_title("Reverse Engineer Database");
    set_size(900, 700);
  }

  //--------------------------------------------------------------------------------

  ObjectSelectionPage::ObjectSelectionPage(WbPluginDbImport *form)
    : WizardObjectFilterPage(form, "objectFilter"), _box(false) {
    set_title(_("Select Objects to Reverse Engineer"));
    set_short_title(_("Select Objects"));

    _box.set_padding(12);
    add_end(&_box, false);

    _empty_label.set_text(_("The selected schemas contain no objects."));
    _box.add(&_empty_label, false);

    _autoplace_check.set_text(_("Place imported objects on a diagram"));
    _autoplace_check.set_active(true);
    _box.add(&_autoplace_check, false);
  }

  void ObjectSelectionPage::setup_filters() {
    Db_plugin *plugin = ((WbPluginDbImport *)_form)->db_plugin();
    bool empty = true;

    reset();
    _filters.clear();

    if (plugin->db_objects_selection_model(Db_plugin::dbotTable)->total_items_count() > 0) {
      _filters[Db_plugin::dbotTable] =
        add_filter(plugin->db_objects_struct_name_by_type(Db_plugin::dbotTable), _("Import %s Objects"),
                   plugin->db_objects_selection_model(Db_plugin::dbotTable),
                   plugin->db_objects_exclusion_model(Db_plugin::dbotTable),
                   plugin->db_objects_enabled_flag(Db_plugin::dbotTable));
      empty = false;
    }
    if (plugin->db_objects_selection_model(Db_plugin::dbotView)->total_items_count() > 0) {
      _filters[Db_plugin::dbotView] = add_filter(
        plugin->db_objects_struct_name_by_type(Db_plugin::dbotView), _("Import %s Objects"),
        plugin->db_objects_selection_model(Db_plugin::dbotView),
        plugin->db_objects_exclusion_model(Db_plugin::dbotView), plugin->db_objects_enabled_flag(Db_plugin::dbotView));
      empty = false;
    }
    if (plugin->db_objects_selection_model(Db_plugin::dbotRoutine)->total_items_count() > 0) {
      _filters[Db_plugin::dbotRoutine] =
        add_filter(plugin->db_objects_struct_name_by_type(Db_plugin::dbotRoutine), _("Import %s Objects"),
                   plugin->db_objects_selection_model(Db_plugin::dbotRoutine),
                   plugin->db_objects_exclusion_model(Db_plugin::dbotRoutine),
                   plugin->db_objects_enabled_flag(Db_plugin::dbotRoutine));
      empty = false;
    }
    if (plugin->db_objects_selection_model(Db_plugin::dbotTrigger)->total_items_count() > 0) {
      _filters[Db_plugin::dbotTrigger] =
        add_filter(plugin->db_objects_struct_name_by_type(Db_plugin::dbotTrigger), _("Import %s Objects"),
                   plugin->db_objects_selection_model(Db_plugin::dbotTrigger),
                   plugin->db_objects_exclusion_model(Db_plugin::dbotTrigger),
                   plugin->db_objects_enabled_flag(Db_plugin::dbotTrigger));
      empty = false;
    }

    _empty_label.show(empty);
  }

  bool ObjectSelectionPage::advance() {
    Db_plugin *plugin = ((WbPluginDbImport *)_form)->db_plugin();
      

    GrtVersionRef version = GrtVersionRef::cast_from(bec::getModelOption(workbench_physical_ModelRef::cast_from(plugin->db_catalog()->owner()), "CatalogVersion"));
    version->owner(plugin->model_catalog());
    plugin->model_catalog()->version(version);
      
    std::list<std::string> errors;
    std::string text;

    if (!plugin->validate_db_objects_selection(&errors)) {
      for (std::list<std::string>::const_iterator iter = errors.begin(); iter != errors.end(); ++iter)
        text.append(*iter + "\n");
    }

    if (_autoplace_check.get_active()) {
      size_t total_placable_object_count =
        plugin->db_objects_selection_model(Db_plugin::dbotTable)->active_items_count() +
        plugin->db_objects_selection_model(Db_plugin::dbotView)->active_items_count() +
        plugin->db_objects_selection_model(Db_plugin::dbotRoutine)->active_items_count();
      if (total_placable_object_count > 250) {
        mforms::Utilities::show_warning(_("Resource Warning"), _("Too many objects are selected for auto placement."
                                                                 "\nSelect fewer elements to create the EER diagram."),
                                        "OK");
        _autoplace_check.set_active(false);
        return false;
      }
    }

    if (!text.empty()) {
      mforms::Utilities::show_error("Error in Object Selection", text, "OK");

      return false;
    }

    for (std::map<Db_plugin::Db_object_type, DBObjectFilterFrame *>::const_iterator iter = _filters.begin();
         iter != _filters.end(); ++iter) {
      plugin->db_objects_activated(iter->first, iter->second->get_active());
    }

    values().gset("import.place_figures", _autoplace_check.get_active());

    return true;
  }

  DBImportProgressPage::DBImportProgressPage(WbPluginDbImport *form)
    : WizardProgressPage(form, "importProgress", true) {
    set_title(_("Reverse Engineering Progress"));
    set_short_title(_("Reverse Engineer"));

    add_async_task(_("Reverse Engineer Selected Objects"), std::bind(&DBImportProgressPage::perform_import, this),
                   _("Reverse engineering DDL from selected objects..."));

    _place_task = add_async_task(_("Place Objects on Diagram"), std::bind(&DBImportProgressPage::perform_place, this),
                                 _("Placing objects..."));

    end_adding_tasks(_("Operation Completed Successfully"));
  }

  bool DBImportProgressPage::perform_import() {
    execute_grt_task(((WbPluginDbImport *)_form)->sql_import()->get_task_slot(), false);
    return true;
  }

  bool DBImportProgressPage::perform_place() {
    execute_grt_task(((WbPluginDbImport *)_form)->sql_import()->get_autoplace_task_slot(), false);

    return true;
  }

  FinishPage::FinishPage(WbPluginDbImport *form) : WizardFinishedPage(form, "Reverse Engineering Finished") {
    set_title(_("Reverse Engineering Results"));
    set_short_title(_("Results"));
  }

  void FinishPage::enter(bool advancing) {
    if (advancing)
      set_summary(create_summary(((WbPluginDbImport *)_form)->sql_import()->get_created_objects()));
  }
}; // namespace DBImport

grtui::WizardPlugin *createDbImportWizard(grt::Module *module, db_CatalogRef catalog) {
  return new DBImport::WbPluginDbImport(module);
}

void deleteDbImportWizard(grtui::WizardPlugin *plugin) {
  delete plugin;
}
