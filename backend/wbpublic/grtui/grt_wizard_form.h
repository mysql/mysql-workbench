/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include <vector>
#include <set>

#include "grt.h"
#include "grt/common.h"

#include "wbpublic_public_interface.h"
#include "base/string_utilities.h"

#include "mforms/wizard.h"
#include "mforms/box.h"
#include "mforms/filechooser.h"

namespace mforms {
  class TextEntry;
};

namespace grtui {

  class WizardPage;

  class WBPUBLICBACKEND_PUBLIC_FUNC WizardForm : public mforms::Wizard {
  public:
    WizardForm();
    virtual ~WizardForm();
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
    virtual bool run_modal();
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

    void add_page(WizardPage *page);

    void update_buttons();
    void update_heading();

    void set_problem(const std::string &text);
    void clear_problem();

    virtual void reset();

    void switch_to_page(WizardPage *page, bool advancing);
    WizardPage *get_active_page() {
      return _active_page;
    }
    int get_active_page_number();

    WizardPage *get_page_with_id(const std::string &id);

    grt::DictRef values() {
      return _values;
    }

    // util stuff for storing state
    void set_wizard_option(const std::string &key, const std::string &value);
    std::string string_wizard_option(const std::string &key, const std::string &default_value = "");

    void set_wizard_option(const std::string &key, int value);
    int int_wizard_option(const std::string &key, int default_value = 0);

  private:
    grt::DictRef _values;

    std::string _problem;

    WizardPage *_active_page;
    std::vector<WizardPage *> _pages;
    std::list<WizardPage *> _turned_pages;

    bool _cancelled;

  protected:
    virtual WizardPage *get_next_page(WizardPage *current);
    void refresh_step_list();

    void extra_clicked();

  public:
    void go_to_next();
    void go_to_back();
    void finish();
    virtual bool cancel();
  };

  /** A page of a wizard.
    */
  class WBPUBLICBACKEND_PUBLIC_FUNC WizardPage : public ::mforms::Box {
  public:
    WizardPage(WizardForm *form, const std::string &pageid);

    std::string get_id() const {
      return _id;
    }

    std::string get_title() const {
      return _title;
    }
    std::string get_short_title() const {
      return _short_title;
    }

    void set_title(const std::string &title);
    void set_short_title(const std::string &title);

    void validate();

    boost::signals2::signal<void(bool)> *signal_enter() {
      return &_signal_enter;
    }
    boost::signals2::signal<void(bool)> *signal_leave() {
      return &_signal_leave;
    }

  public:
  protected:
    friend class WizardForm;

    WizardForm *wizard() {
      return _form;
    }

    grt::DictRef values() {
      return _form->values();
    }

    //! Subclasses must override this to implement validation.
    //! If there is a validation error, it must call _form->set_problem()
    virtual void do_validate() {
    }

    virtual int load() {
      return -1;
    } // delme XXX

    virtual bool pre_load();
    virtual void enter(bool advancing);
    virtual bool advance();
    virtual void leave(bool advancing);

    virtual bool allow_next() {
      return true;
    }
    virtual bool allow_back() {
      return true;
    }
    virtual bool allow_cancel() {
      return true;
    }
    virtual bool skip_page() {
      return false;
    } // Return true if the page should not be displayed (due to some condition).

    //! return true if this is the last page and pressing next should close wizard
    virtual bool next_closes_wizard() {
      return false;
    }

    //! overrider may return "" for default caption
    virtual std::string next_button_caption() {
      return "";
    }

    virtual std::string extra_button_caption() {
      return "";
    }

    std::string finish_button_caption() const {
#ifdef __APPLE__
      return _("Close");
#elif defined(_MSC_VER)
      return _("Finish");
#else
      return _("_Close");
#endif
    }

    virtual void extra_clicked() {
    }

  protected:
    WizardForm *_form;
    std::string _id;
    boost::signals2::signal<void(bool)> _signal_enter;
    boost::signals2::signal<void(bool)> _signal_leave;
    std::string _title;
    std::string _short_title;

    std::string execute_caption() const {
#ifdef __APPLE__
      return _("Execute");
#elif defined(_MSC_VER)
      return _("_Execute >");
#else
      return _("_Execute");
#endif
    }

    std::string finish_caption() const {
#ifdef __APPLE__
      return _("Finish");
#else
      return _("_Finish");
#endif
    }

    virtual std::string close_caption() const {
#ifdef __APPLE__
      return _("Close");
#else
      return _("_Close");
#endif
    }

  private:
    void filename_changed(mforms::TextEntry *entry);
    void browse_file_callback(mforms::TextEntry *entry, mforms::FileChooserType type, const std::string &extensions);
  };
};
