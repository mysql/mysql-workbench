/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <mforms/form.h>
#include <vector>

namespace mforms {

  class Wizard;
  class View;
  class Form;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT WizardImplPtrs {
    bool (*create)(Wizard *self, Form *owner);
    void (*set_title)(Wizard *self, const std::string &title);
    void (*run_modal)(Wizard *self);
    void (*close)(Wizard *self);

    void (*set_content)(Wizard *self, View *view);
    void (*set_heading)(Wizard *self, const std::string &);
    void (*set_step_list)(Wizard *self, const std::vector<std::string> &);
    void (*set_allow_cancel)(Wizard *self, bool flag);
    void (*set_allow_back)(Wizard *self, bool flag);
    void (*set_allow_next)(Wizard *self, bool flag);
    void (*set_show_extra)(Wizard *self, bool flag);

    void (*set_extra_caption)(Wizard *self, const std::string &);
    void (*set_next_caption)(Wizard *self, const std::string &);
  };
#endif
#endif

  class MFORMS_EXPORT Wizard : public Form {
    friend class ControlFactory;

    WizardImplPtrs *_wizard_impl;
    View *_content;

    Wizard();

  private:
    // hide methods from base-class that are not supported
    Wizard(Form *owner, FormFlag flag) {
    }
    virtual bool run_modal(Button *accept, Button *cancel) {
      return false;
    }
    virtual void show_modal(Button *accept, Button *cancel) {
    }
    virtual void end_modal(bool result) {
    }

  public:
    Wizard(Form *owner);
    virtual ~Wizard();

    virtual void set_title(const std::string &title);
    virtual void run();
    virtual void close();

    //!< set_content should place view as a wizard's current page.
    //!< Note: When view is 0, then content should be removed
    virtual void set_content(View *view);
    void set_heading(const std::string &text);
    //!< 1st char of each step title must be . for executed tasks, * for current and - for not executed
    void set_step_list(const std::vector<std::string> &steps);
    void set_allow_cancel(bool flag);
    void set_allow_back(bool flag);
    void set_allow_next(bool flag);
    void set_show_extra(bool flag);
    void set_extra_caption(const std::string &caption);
    void set_next_caption(const std::string &caption = "");

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
    void set_cancel_handler(const std::function<bool()> &slot);
    boost::signals2::signal<void()> *signal_next_clicked() {
      return &_next_signal;
    }
    boost::signals2::signal<void()> *signal_back_clicked() {
      return &_back_signal;
    }
    boost::signals2::signal<void()> *signal_extra_clicked() {
      return &_extra_signal;
    }

  public:
    void back_clicked() {
      _back_signal();
    }

    void next_clicked() {
      _next_signal();
    }

    void extra_clicked() {
      _extra_signal();
    }

    std::function<bool()> _cancel_slot;
#endif
#endif
  protected:
    boost::signals2::signal<void()> _back_signal;
    boost::signals2::signal<void()> _next_signal;
    boost::signals2::signal<void()> _extra_signal;
  };
};
