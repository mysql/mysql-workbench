/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _WIZARDFINISHEDPAGE_H_
#define _WIZARDFINISHEDPAGE_H_

#include "grt_wizard_form.h"

#include "mforms/label.h"

namespace grtui {

  class WBPUBLICBACKEND_PUBLIC_FUNC WizardFinishedPage : public WizardPage {
  public:
    WizardFinishedPage(WizardForm *form, const std::string &top_title);

    void set_heading(const std::string &title);
    void set_summary(const std::string &text);

  private:
    mforms::Label _label1;
    mforms::Label _label2;
    std::string _top_title;

    virtual bool next_closes_wizard() {
      return true;
    }

    virtual bool allow_next() {
      return true;
    }

    virtual bool allow_cancel() {
      return false;
    }

    virtual std::string next_button_caption();
  };
};

#endif /* _WIZARDFINISHEDPAGE_H_ */
