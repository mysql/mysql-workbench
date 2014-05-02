/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _CATALOG_VALIDATION_PAGE_H_
#define _CATALOG_VALIDATION_PAGE_H_

#include "grtui/wizard_progress_page.h"
#include "grt/grt_manager.h"
#include "grt/grt_message_list.h"
#include "interfaces/wbvalidation.h"
#include "grti/wbvalidation.h"

namespace grtui {

class CatalogValidationPage : public WizardProgressPage
{
public:
  static bool has_modules(grt::GRT *grt)
  {
    return !grt->get_implementing_modules<WbValidationInterfaceWrapper>().empty();
  }

  CatalogValidationPage(WizardForm *form, bool optional= true)
    : WizardProgressPage(form, "validate", true)
  {
    set_title(_("Catalog Validation"));
    set_short_title(_("Catalog Validation"));
    
    grt::GRT *grt= form->grtm()->get_grt();

    // get list of available validation modules
    std::vector<WbValidationInterfaceWrapper*> validation_modules;
    validation_modules= grt->get_implementing_modules<WbValidationInterfaceWrapper>();

    _target_catalog= db_CatalogRef::cast_from(grt->get("/wb/doc/physicalModels/0/catalog"));

    // add a task for each validation module
    for (std::vector<WbValidationInterfaceWrapper*>::iterator module= validation_modules.begin();
         module != validation_modules.end(); ++module)
    {
      std::string caption= (*module)->getValidationDescription(_target_catalog);

      if (!caption.empty())
        add_async_task(caption,
                       boost::bind(&CatalogValidationPage::validation_step, this, *module, caption),
                       _("Performing catalog validations..."));
    }

    end_adding_tasks(_("Validation Finished Successfully"));
    
    set_status_text("");

    if (optional)
    {
      _run_box= mforms::manage(new mforms::Box(true));

      _run_button= mforms::manage(new mforms::Button());
#ifdef _WIN32
      _run_button->set_text(_("Run &Validations"));
#else
      _run_button->set_text(_("_Run Validations"));
#endif
      scoped_connect(_run_button->signal_clicked(),boost::bind(&CatalogValidationPage::run_validations, this));

      _run_box->add_end(_run_button, false, false);
      _run_button->set_size(160, -1);

      add_end(_run_box, false, false);
    }
    else
    {
      _run_button= 0;
      _run_box= 0;
    }
  }

  virtual ~CatalogValidationPage()
  {
  }


  grt::ValueRef execute_validation_module(WbValidationInterfaceWrapper *module)
  {
    return grt::IntegerRef(module->validate("All", _target_catalog));
  }


  bool validation_step(WbValidationInterfaceWrapper *module, const std::string &caption)
  {
    add_log_text("Starting "+caption);

    execute_grt_task(boost::bind(&CatalogValidationPage::execute_validation_module, this, module), false);
    
    return true;
  }


  virtual void enter(bool advancing)
  {
    if (advancing && !_run_button)
    {
      run_validations();
    }
  }
  
  
  virtual void tasks_finished(bool success)
  {
    if (success)
      _form->clear_problem();
    else
      _form->set_problem(_("Validation Errors"));
  }
  

  void run_validations()
  {    
    start_tasks();
  }

protected:
  bec::GRTManager *_manager;
  mforms::Box *_run_box;
  mforms::Button *_run_button;

  db_CatalogRef _target_catalog;
};

};

#endif /* _CATALOG_VALIDATION_PAGE_H_ */
