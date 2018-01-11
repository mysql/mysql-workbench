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

#include "data_source_selector_page.h"
#include "grtui/grt_wizard_form.h"

class MultiSourceSelectPage : public WizardPage {
private:
  DataSourceSelector _left;
  DataSourceSelector _right;
  DataSourceSelector _result;
  bool _has_result;

  void left_changed() {
    if (_left.model_radio->get_active())
      _right.model_radio->set_enabled(false);
    else
      _right.model_radio->set_enabled(true);

    _left.file_selector.set_enabled(_left.file_radio->get_active());
  }

  void right_changed() {
    if (_right.model_radio->get_active())
      _left.model_radio->set_enabled(false);
    else
      _left.model_radio->set_enabled(true);

    _right.file_selector.set_enabled(_right.file_radio->get_active());

    if (_has_result) {
      _result.server_radio->set_enabled(_right.server_radio->get_active());
      if (!_right.server_radio->get_active())
        _result.file_radio->set_active(true);
    }
  }

  inline DataSourceSelector::SourceType source_for_name(std::string s, const std::string &default_name) {
    if (s.empty())
      s = default_name;

    if (s == "model")
      return DataSourceSelector::ModelSource;
    else if (s == "server")
      return DataSourceSelector::ServerSource;
    else
      return DataSourceSelector::FileSource;
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      _left.set_source(
        source_for_name(bec::GRTManager::get()->get_app_option_string("db.mysql.synchronizeAny:left_source"), "model"));
      _right.set_source(source_for_name(
        bec::GRTManager::get()->get_app_option_string("db.mysql.synchronizeAny:right_source"), "server"));
      if (_has_result)
        _result.set_source(
          source_for_name(bec::GRTManager::get()->get_app_option_string("db.mysql.synchronizeAny:result"), "server"));

      _left.file_selector.set_filename(
        bec::GRTManager::get()->get_app_option_string("db.mysql.synchronizeAny:left_source_file"));
      _right.file_selector.set_filename(
        bec::GRTManager::get()->get_app_option_string("db.mysql.synchronizeAny:right_source_file"));
      if (_has_result)
        _result.file_selector.set_filename(
          bec::GRTManager::get()->get_app_option_string("db.mysql.synchronizeAny:result_file"));
    }
  }

  virtual bool advance() {
    const char *sources[] = {"model", "server", "file"};

    // Remember defaults
    bec::GRTManager::get()->set_app_option("db.mysql.synchronizeAny:left_source",
                                           grt::StringRef(sources[get_left_source()]));
    bec::GRTManager::get()->set_app_option("db.mysql.synchronizeAny:right_source",
                                           grt::StringRef(sources[get_right_source()]));
    if (_has_result)
      bec::GRTManager::get()->set_app_option("db.mysql.synchronizeAny:result", grt::StringRef(sources[get_result()]));
    bec::GRTManager::get()->set_app_option("db.mysql.synchronizeAny:left_source_file",
                                           grt::StringRef(_left.file_selector.get_filename()));
    bec::GRTManager::get()->set_app_option("db.mysql.synchronizeAny:right_source_file",
                                           grt::StringRef(_right.file_selector.get_filename()));
    if (_has_result)
      bec::GRTManager::get()->set_app_option("db.mysql.synchronizeAny:result_file",
                                             grt::StringRef(_result.file_selector.get_filename()));

    values().gset("left_source", sources[get_left_source()]);
    values().gset("right_source", sources[get_right_source()]);
    if (_has_result) {
      values().gset("result", get_result());
      values().gset("result_path", _result.file_selector.get_filename());
    }
    values().gset("left_source_file", _left.file_selector.get_filename());
    values().gset("right_source_file", _right.file_selector.get_filename());

    bool ret_val = true;
    std::string msg;
    if (_has_result) {
      _result.file_selector.get_filename();
      if ((get_result() == DataSourceSelector::FileSource) &&
          (!_result.file_selector.check_and_confirm_file_overwrite() || _result.file_selector.get_filename().empty())) {
        ret_val = false;
        if (_result.file_selector.get_filename().empty()) {
          msg += "You didn't specify the result file, please select one.\n";
        } else {
          msg += "Result File: ";
          msg += _result.file_selector.get_filename();
          msg += " cannot be found, please check the path.\n";
        }
      }
    }

    if (get_left_source() == DataSourceSelector::FileSource &&
        !g_file_test(_left.file_selector.get_filename().c_str(), G_FILE_TEST_EXISTS)) {
      ret_val = false;
      if (_left.file_selector.get_filename().empty()) {
        msg += "You didn't specify the source file, please select one.\n";
      } else {
        msg += "Source File: ";
        msg += _left.file_selector.get_filename();
        msg += " cannot be found, please check the path.\n";
      }
    }

    if (get_right_source() == DataSourceSelector::FileSource &&
        !g_file_test(_right.file_selector.get_filename().c_str(), G_FILE_TEST_EXISTS)) {
      ret_val = false;
      if (_right.file_selector.get_filename().empty()) {
        msg += "You didn't specify the destination file, please select one.";
      } else {
        msg += "Dest File: ";
        msg += _right.file_selector.get_filename();
        msg += " cannot be found, please check the path.";
      }
    }
    if (!ret_val) {
      mforms::Utilities::show_error(_("File not found"), msg, _("OK"));
    }

    return ret_val;
  }

public:
  MultiSourceSelectPage(WizardForm *form, bool include_result)
    : WizardPage(form, "source"), _result(true), _has_result(include_result) {
    set_title(_("Select Databases for Updates"));
    set_short_title(_("Select Sources"));

    mforms::Label info_label;
    info_label.set_wrap_text(true);
    info_label.set_style(mforms::SmallStyle);
    info_label.set_text(
      _("Select the source and destination databases to be compared. The script needed to alter the source schema to "
        "match destination will be executed in the destination server or written to the output script file, as "
        "selected."));

    add(&info_label, false, true);

    add(&_left.panel, false, true);
    add(&_right.panel, false, true);
    if (include_result)
      add(&_result.panel, false, true);

    _left.panel.set_title(_("Source – Database To Take Updates From"));

    _left.set_change_slot(std::bind(&MultiSourceSelectPage::left_changed, this));
    _right.set_change_slot(std::bind(&MultiSourceSelectPage::right_changed, this));

    _left.model_radio->set_active(true);
    _right.model_radio->set_enabled(false);
    _right.server_radio->set_active(true);

    _left.file_source_selected();
    _right.file_source_selected();
    _right.panel.set_title(_("Destination – Database To Receive Updates"));

    if (include_result) {
      _result.panel.set_title(_("Send Updates To:"));
      _result.model_radio->show(false);
      _result.server_radio->set_text("Destination Database Server");
      _result.file_radio->set_text("ALTER Script File:");
      // TODO: add Source Model
      _result.server_radio->set_active(true);
    }
  }

  DataSourceSelector::SourceType get_left_source() {
    return _left.get_source();
  }
  DataSourceSelector::SourceType get_right_source() {
    return _right.get_source();
  }
  DataSourceSelector::SourceType get_result() {
    return _result.get_source();
  }
};
