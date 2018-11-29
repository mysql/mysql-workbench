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



#if 1

#include "grtui/grt_wizard_form.h"

#include "grt/grt_manager.h"

#include "grt/common.h"

using namespace grtui;
using namespace mforms;

class Page1 : public WizardPage {
  Panel _left_panel;
  Box _left_vbox;
  RadioButton _left_model;
  RadioButton _left_file;
  RadioButton _left_db;
  Box _left_filebox;
  Label _left_filebox_l;
  TextEntry _left_filename;
  Button _left_filebox_b;

  Panel _right_panel;
  Box _right_vbox;
  RadioButton _right_model;
  RadioButton _right_file;
  RadioButton _right_db;
  Box _right_filebox;
  Label _right_filebox_l;
  TextEntry _right_filename;
  Button _right_filebox_b;

  Box _top_level;

public:
  Page1(WizardForm *form)
    : WizardPage(form, "pg1"),
      _left_panel(true),
      _left_vbox(false),
      _left_filebox(true),
      _right_panel(true),
      _right_vbox(false),
      _right_filebox(true),
      _top_level(true) {
    set_title("Catalog synchronization");
    set_subtitle("Select your catalog objects for synchronization");

    add(&_top_level, false, true);

    _left_panel.set_title("Source for the Left Catalog");
    _left_panel.add(&_left_vbox);

    _left_model.set_text("Model Schemata");
    _left_db.set_text("Live Database Server");
    _left_file.set_text("SQL Script File");
    _left_model.signal_toggled().connect(sigc::mem_fun(this, &Page1::left_changed));
    _left_db.signal_toggled().connect(sigc::mem_fun(this, &Page1::left_changed));
    _left_file.signal_toggled().connect(sigc::mem_fun(this, &Page1::left_changed));

    _left_vbox.set_spacing(8);
    _left_vbox.set_padding(8);

    _left_filebox.set_spacing(4);
    _left_filebox_l.set_text("File Name:");
    _left_filebox_b.set_text("Browse...");
    _left_filebox_b.set_name("Browse");
    _left_filebox_b.set_size(100, -1);
    _left_filename.set_size(30, -1);

    _left_filebox.add(&_left_filebox_l, false, true);
    _left_filebox.add(&_left_filename, true, true);
    _left_filebox.add(&_left_filebox_b, false, false);

    enable_file_browsing(&_left_filename, &_left_filebox_b, mforms::OpenFile);

    _left_vbox.add(&_left_model, false, true);
    _left_vbox.add(&_left_db, false, true);
    _left_vbox.add(&_left_file, false, true);
    _left_vbox.add(&_left_filebox, false, true);

    _right_panel.set_title("Source for the Right Catalog");
    _right_panel.add(&_right_vbox);

    _right_model.set_text("Model Schemata");
    _right_db.set_text("Live Database Server");
    _right_file.set_text("SQL Script File");
    _right_model.signal_toggled().connect(sigc::mem_fun(this, &Page1::right_changed));
    _right_db.signal_toggled().connect(sigc::mem_fun(this, &Page1::right_changed));
    _right_file.signal_toggled().connect(sigc::mem_fun(this, &Page1::right_changed));

    _right_filebox.set_spacing(4);
    _right_filebox_l.set_text("File Name:");
    _right_filebox_b.set_text("Browse...");
    _right_filebox_b.set_name("Browse");
    _right_filebox_b.set_size(100, -1);
    _right_filename.set_size(30, -1);
    _right_filebox.add(&_right_filebox_l, false, true);
    _right_filebox.add(&_right_filename, true, true);
    _right_filebox.add(&_right_filebox_b, false, false);

    enable_file_browsing(&_right_filename, &_right_filebox_b, mforms::OpenFile);

    _right_vbox.set_spacing(8);
    _right_vbox.set_padding(8);

    _right_vbox.add(&_right_model, false, true);
    _right_vbox.add(&_right_db, false, true);
    _right_vbox.add(&_right_file, false, true);
    _right_vbox.add(&_right_filebox, false, true);

    _right_file.set_active(true);

    _top_level.set_homogeneous(true);
    _top_level.add(&_left_panel, false, true);
    _top_level.add(&_right_panel, false, true);

    _top_level.set_padding(12);
    _top_level.set_spacing(12);
  }

  virtual void enter(grt::DictRef values) {
    validate();
  }

  void left_changed() {
    if (_left_model.get_active()) {
      _right_model.set_enabled(false);
      _left_filebox_b.set_enabled(false);
      _left_filename.set_enabled(false);
    } else if (_left_db.get_active()) {
      _right_model.set_enabled(true);
      _left_filebox_b.set_enabled(false);
      _left_filename.set_enabled(false);
    } else {
      _right_model.set_enabled(true);
      _left_filebox_b.set_enabled(true);
      _left_filename.set_enabled(true);
    }
    validate();
  }

  void right_changed() {
    if (_right_model.get_active()) {
      _left_model.set_enabled(false);
      _right_filebox_b.set_enabled(false);
      _right_filename.set_enabled(false);
    } else if (_right_db.get_active()) {
      _left_model.set_enabled(true);
      _right_filebox_b.set_enabled(false);
      _right_filename.set_enabled(false);
    } else {
      _left_model.set_enabled(true);
      _right_filebox_b.set_enabled(true);
      _right_filename.set_enabled(true);
    }
    validate();
  }

  virtual void do_validate() {
    bool ok = true;

    if (_left_file.get_active()) {
      if (!g_file_test(_left_filename.get_string_value().c_str(), G_FILE_TEST_IS_REGULAR))
        ok = false;
    }
    if (!ok) {
      if (_form)
        _form->set_problem("Select Left File");
      return;
    }

    if (_right_file.get_active()) {
      if (!g_file_test(_right_filename.get_string_value().c_str(), G_FILE_TEST_IS_REGULAR))
        ok = false;
    }
    if (!ok) {
      if (_form)
        _form->set_problem("Select Right File");
      return;
    }
    if (_form)
      _form->set_problem("");
  }

  virtual std::string get_title() {
    return "Wizard Test";
  }

  virtual std::string get_subtitle() {
    return "Wizard page description.";
  }
};

#include "../grtdb_connect_panel.h"

class Page2 : public WizardPage {
protected:
  DbConnectPanel _dbconnect;

public:
  Page2(WizardForm *form, grt::GRT *grt) : WizardPage(form, "dbconnect") {
    set_title("Pick a connection");
    set_subtitle("Select an existing connection or create a new one");

    db_mgmt_RdbmsRef rdbms(
      db_mgmt_RdbmsRef::cast_from(grt::GRT::get()->unserialize("../../modules/db.mysql/res/mysql_rdbms_info.xml")));
    db_mgmt_ManagementRef mgr(grt);

    mgr->rdbms().insert(rdbms);

    _dbconnect.init(mgr);

    add(&_dbconnect, true, true);
  }

  virtual std::string get_title() {
    return "Wizard Test";
  }

  virtual std::string get_subtitle() {
    return "Connection parameters.";
  }
};

class ImportInputPage : public WizardPage {
public:
  ImportInputPage(WizardForm *form)
    : WizardPage(form, "options"), _frame(true), _contents(true), _options(true), _options_box(false) {
    set_name("Import Page");
    _contents.set_name("Contents");
    _options.set_name("Options");
    _caption.set_name("Caption");
    _browse_button.set_name("Browse");
    _autoplace_check.set_name("Auto Place");

    set_title("SQL Import Options");
    set_subtitle("Set options for SQL script to be imported.");

    add(&_frame, false, true);

    _frame.set_title(_("Input File"));
    _frame.add(&_contents);

    _contents.set_padding(12);
    _contents.set_spacing(8);
    _contents.add(&_caption, false, false);
    _contents.add(&_filename, true, true);
    _filename.set_size(50, -1);
    _contents.add(&_browse_button, false, false);

    enable_file_browsing(&_filename, &_browse_button, mforms::OpenFile);

    _caption.set_text(_("Input SQL Script File:"));
    _browse_button.set_text(_("Browse..."));
    _browse_button.set_name("Browse");
    _browse_button.set_size(100, -1);

    _options.set_title(_("Options"));

    _options.add(&_options_box);
    _options_box.set_padding(12);
    _options_box.set_spacing(8);
    _options_box.add(&_autoplace_check, false, true);
    _autoplace_check.set_text(_("Place Imported Objects on a New Diagram"));
    _autoplace_check.set_size(-1, -1);

    add(&_options, false, true);
  }

protected:
  Panel _frame;
  Box _contents;
  Label _caption;
  TextEntry _filename;
  Button _browse_button;

  Panel _options;
  Box _options_box;
  CheckBox _autoplace_check;
};

#ifdef _MSC_VER
GRTUI_EXPORT void wiztest();

void wiztest() {
#if 0
  Form window;
  Box vbox(false);

  Box hbox(true);

  window.set_size(400,400);
  
  window.set_content(&hbox);
  
  //vbox.add(&hbox, false, false);

  Button b1;
  Button b2;
  Button b3;
  
  b1.set_text("Button1");
  b2.set_text("Button2");
  b3.set_text("Button3");
  
  hbox.add(&b1, true, true);
  hbox.add(&b2, true, false);
  hbox.add(&b3, false, false);

  hbox.set_spacing(8);
  hbox.set_padding(12);
  
  window.show();

#else
  bec::GRTManager grtm;

  grtm.set_datadir("../../");
  grtm.get_grt()->scan_metaclasses_in("../../res/grt");
  grtm.get_grt()->end_loading_metaclasses();

  WizardForm* wizard = new WizardForm(&grtm);

  Page2* page2 = new Page2(wizard, grtm.get_grt());
  wizard->add_page(page2);

  Page1* page1 = new Page1(wizard);
  wizard->add_page(page1);

  ImportInputPage* import1 = new ImportInputPage(wizard);
  wizard->add_page(import1);

  grt::DictRef values(true);

  wizard->run_modal(values);
#endif
}
#else
#include "gtk/lf_mforms.h"
#include <gtkmm.h>
#include "grt/grt_manager.h"

void wiztest()
// int main(int argc, char **argv)
{
  bec::GRTManager grtm;
  WizardForm wizard(&grtm);

  ImportInputPage import1(&wizard);

  wizard.add_page(&import1);

#if 0
  Page1 page1(&wizard);
  Page2 page2(&wizard);

  wizard.add_page(&page2);
  wizard.add_page(&page1);
#endif
  grt::DictRef values(true);

  wizard.run_modal(values);

  g_message("wizard exited");
  // main.run();
  // return 0;
}
#endif

#endif
