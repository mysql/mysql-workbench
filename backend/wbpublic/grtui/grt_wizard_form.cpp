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

#include "base/string_utilities.h"
#include <algorithm>

#include <grt/grt_manager.h>

#include "grt_wizard_form.h"

#include "mforms/fs_object_selector.h"
#include "mforms/textentry.h"
#include "mforms/form.h"
#include "mforms/utilities.h"

using namespace grtui;
using namespace mforms;

WizardForm::WizardForm() : Wizard(Form::main_form()), _cancelled(false) {
  set_name("Wizard");
  setInternalName("wizard");
  _active_page = 0;

  scoped_connect(signal_next_clicked(), std::bind(&WizardForm::go_to_next, this));
  scoped_connect(signal_back_clicked(), std::bind(&WizardForm::go_to_back, this));
  scoped_connect(signal_extra_clicked(), std::bind(&WizardForm::extra_clicked, this));
  set_cancel_handler(std::bind(&WizardForm::cancel, this));

  _values = grt::DictRef(true);
}

WizardForm::~WizardForm() {
  for (std::vector<WizardPage *>::iterator iter = _pages.begin(); iter != _pages.end(); ++iter)
    (*iter)->release();
}

void WizardForm::add_page(WizardPage *page) {
  _pages.push_back(page);
}

WizardPage *WizardForm::get_page_with_id(const std::string &id) {
  for (std::vector<WizardPage *>::const_iterator iter = _pages.begin(); iter != _pages.end(); ++iter) {
    if ((*iter)->get_id() == id)
      return *iter;
  }
  return 0;
}

void WizardForm::reset() {
  if (_active_page) {
    set_content(0);
    _turned_pages.clear();

    _active_page = 0;
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Runs the wizard modally and returns true if it was successfully finished, false if it has been cancelled.
 * Presetting values or reading results is done via WizardForm::values().
 */
bool WizardForm::run_modal() {
  refresh_step_list();

  reset();

  switch_to_page(_pages.front(), true);

  run();

  return !_cancelled;
}

//--------------------------------------------------------------------------------------------------

void WizardForm::refresh_step_list() {
  int i = 0;
  std::vector<std::string> steps;

  for (std::vector<WizardPage *>::const_iterator iter = _pages.begin(); iter != _pages.end(); ++iter, ++i) {
    std::string title;

    if (*iter == _active_page)
      title = "*";
    else if (std::find(_turned_pages.begin(), _turned_pages.end(), *iter) != _turned_pages.end())
      title = ".";
    else
      title = "-";

    title.append((*iter)->get_short_title());
    steps.push_back(title);
  }

  set_step_list(steps);
}

void WizardForm::switch_to_page(WizardPage *page, bool advancing) {
  if (_active_page) {
    if (page != _active_page)
      _active_page->leave(advancing);
  }

  if (!page) {
    // leave() method from the current page must be called before get_next_page()
    // since skip_page() can depend on the actions performed by the previous page's leave()
    page = get_next_page(_active_page);
    if (!page) {
      // finish(); don't need to call finish as leave() was already called early
      close();
      return;
    }
  }

  if (_active_page != page) {
    if (advancing) {
      try {
        if (!page->pre_load())
          return;
      } catch (std::exception &exc) {
        Utilities::show_error(_("Error"), exc.what(), _("OK"));

        update_buttons();
        return;
      }
    }

    set_content(page);

    _active_page = page;

    // Set new section text and let it resize to minimal size (which then gets laid out to our rules).
    update_heading();

    _active_page->enter(advancing);
  } else {
    // Set new section text and let it resize to minimal size (which then gets laid out to our rules).
    set_heading(_active_page->get_title());
  }
  update_buttons();

  refresh_step_list();
}

int WizardForm::get_active_page_number() {
  return (int)(std::find(_pages.begin(), _pages.end(), _active_page) - _pages.begin());
}

void WizardForm::set_problem(const std::string &problem) {
  _problem = problem;

  update_buttons();

  // show problem when next button is clicked
  // Utilities::show_error(_("Error"), problem, _("OK"));
}

void WizardForm::clear_problem() {
  _problem.clear();
  update_buttons();
}

void WizardForm::update_buttons() {
  if (_active_page) {
    std::string caption = _active_page->next_button_caption();
    if (caption.empty()) {
      if (_active_page->next_closes_wizard())
        caption = _active_page->close_caption();
      else
        caption = "";
    }
    set_next_caption(caption);

    caption = _active_page->extra_button_caption();
    if (caption.empty())
      set_show_extra(false);
    else {
      set_show_extra(true);
      set_extra_caption(caption);
    }

    set_allow_next(/*_problem.empty() &&*/ _active_page->allow_next());
    set_allow_back(!_turned_pages.empty() && _active_page->allow_back());
    set_allow_cancel(_active_page->allow_cancel());
  }
}

//--------------------------------------------------------------------------------------------------

void WizardForm::update_heading() {
  if (_active_page != NULL)
    set_heading(_active_page->get_title());
}

//--------------------------------------------------------------------------------------------------

WizardPage *WizardForm::get_next_page(WizardPage *current) {
  bool found_current = false;
  for (std::vector<WizardPage *>::const_iterator iter = _pages.begin(); iter != _pages.end(); ++iter) {
    if (*iter == current)
      found_current = true;
    else if (found_current && !(*iter)->skip_page()) {
      return *iter;
    }
  }
  return 0;
}

void WizardForm::finish() {
  if (_active_page)
    _active_page->leave(true);

  close();
}

void WizardForm::extra_clicked() {
  if (_active_page)
    _active_page->extra_clicked();
}

void WizardForm::go_to_next() {
  if (!_problem.empty()) {
    Utilities::show_error(_("Cannot Advance"), _problem, _("OK"));
    return;
  }

  if (_active_page) {
    bool advance_ok = false;

    set_allow_next(false);
    set_allow_back(false);
    set_allow_cancel(false);

    // catch uncaught exceptions
    try {
      advance_ok = _active_page->advance();
    } catch (std::exception &exc) {
      Utilities::show_error(_("Error"), exc.what(), _("OK"));
    }

    if (!advance_ok) {
      update_buttons();
      return;
    }

    _turned_pages.push_back(_active_page);

    if (_active_page->next_closes_wizard()) {
      finish();
      return;
    }

    // passing 0 signals that it should switch to the "next" page, whatever it is
    switch_to_page(0, true);
  }
}

void WizardForm::go_to_back() {
  if (!_turned_pages.empty()) {
    WizardPage *back_page = _turned_pages.back();
    _turned_pages.pop_back();

    clear_problem();

    switch_to_page(back_page, false);
  }
}

bool WizardForm::cancel() {
  _cancelled = true;
  return true;
}

/** Set a option value in the application options dict.

 The key will be prepended by a module specific prefix to avoid name collisions.
 */
void WizardForm::set_wizard_option(const std::string &key, const std::string &value) {
}

std::string WizardForm::string_wizard_option(const std::string &key, const std::string &default_value) {
  return default_value;
}

void WizardForm::set_wizard_option(const std::string &key, int value) {
}

int WizardForm::int_wizard_option(const std::string &key, int default_value) {
  return default_value;
}

//--------------------------------------------------------------------------------------------------

WizardPage::WizardPage(WizardForm *form, const std::string &id) : Box(false), _form(form), _id(id) {
  set_padding(14);
  set_spacing(12);
}

//--------------------------------------------------------------------------------------------------

void WizardPage::set_title(const std::string &title) {
  _title = title;
  if (_form != NULL)
    _form->update_heading();
}

//--------------------------------------------------------------------------------------------------

void WizardPage::set_short_title(const std::string &title) {
  _short_title = title;
}

//--------------------------------------------------------------------------------------------------

/** Validate the current page and update navigation buttons accordingly.
 *
 * This method will call the do_validate() method which must be overridden by subclasses.
 * The subclass may either call set_problem() to indicate a problem that prevents
 * the wizard to continue or provide that allows_next() returns false. clear_problem()
 * must be called if the problem raised by set_problem() is cleared.
 */
void WizardPage::validate() {
  if (_form->get_active_page() == this) {
    do_validate();
  }
  _form->update_buttons();
}

/** Called before the previous page exits. (override)
 *
 * Pages may override this method when something must be executed before the previous
 * page leaves. This gives a chance to cancel leaving the previous page and denying entry to the
 * new page by returning false in case there was invalid input given by the user in a previous page.
 */
bool WizardPage::pre_load() {
  return true;
}

/** Called when the Next/Continue button is called. (override)
 *
 * Pages may override this if they want something to be executed when the user presses Next.
 * If true is returned the page will be switched to the next one, otherwise the current page
 * will remain.
 */
bool WizardPage::advance() {
  return true;
}

/** Called the page is shown on screen.
 *
 * Pages may override this to perform some action when a page is entered and shown on screen.
 * The advancing argument will be true if the page was hit by pressing Next. If it's false, it
 * was entered by pressing Back.
 */
void WizardPage::enter(bool advancing) {
  mforms::FsObjectSelector::clear_stored_filenames();

  _signal_enter(advancing);
}

/** Called the page is taken away from screen.
 *
 * Pages may override this to perform some action when a page is existed and taken away from screen.
 * The advancing argument will be true if the page was hit by pressing Next. If it's false, it
 * was entered by pressing Back.
 */
void WizardPage::leave(bool advancing) {
  _signal_leave(advancing);
}
