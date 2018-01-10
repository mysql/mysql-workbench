/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_progressbar.h"

using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Controls;

public
ref class _ProgressBar : ProgressBar {
public:
  void SetProgressAsync(ProgressBar ^ sender, int value) {
    ProgressBar ^ progressbar = (ProgressBar ^)sender;
    if (progressbar != nullptr)
      sender->Value = value;
  }
};

//----------------- ProgressBarWrapper -------------------------------------------------------------

ProgressBarWrapper::ProgressBarWrapper(mforms::ProgressBar *pbar) : ViewWrapper(pbar) {
}

//--------------------------------------------------------------------------------------------------

bool ProgressBarWrapper::create(mforms::ProgressBar *backend) {
  ProgressBarWrapper *wrapper = new ProgressBarWrapper(backend);
  _ProgressBar ^ progressbar = ProgressBarWrapper::Create<_ProgressBar>(backend, wrapper);
  progressbar->Maximum = 1000;
  progressbar->Minimum = 0;
  progressbar->Size = System::Drawing::Size(100, 20);
  return true;
}

//--------------------------------------------------------------------------------------------------

delegate void RunProgressDelegate(ProgressBar ^ sender, int value);

void ProgressBarWrapper::set_value(mforms::ProgressBar *backend, float pct) {
  _ProgressBar ^ progressbar = ProgressBarWrapper::GetManagedObject<_ProgressBar>(backend);
  int value = (int)(pct * 1000);
  if (value < progressbar->Minimum)
    value = progressbar->Minimum;
  else if (value > progressbar->Maximum)
    value = progressbar->Maximum;

  array<Object ^> ^ parameters = gcnew array<Object ^>(2);
  parameters[0] = progressbar;
  parameters[1] = value;
  if (progressbar->InvokeRequired) {
    progressbar->BeginInvoke(gcnew RunProgressDelegate(progressbar, &_ProgressBar::SetProgressAsync), parameters);
    return;
  }
  progressbar->Value = value;
}

//--------------------------------------------------------------------------------------------------

void ProgressBarWrapper::set_indeterminate(mforms::ProgressBar *backend, bool flag) {
  ProgressBar ^ progressbar = ProgressBarWrapper::GetManagedObject<ProgressBar>(backend);
  if (flag) {
    progressbar->Style = ProgressBarStyle::Marquee;
    progressbar->MarqueeAnimationSpeed = 100;
  } else {
    progressbar->Style = ProgressBarStyle::Continuous;
    progressbar->MarqueeAnimationSpeed = 0;
    progressbar->Value = 0;
  }
}

//--------------------------------------------------------------------------------------------------

void ProgressBarWrapper::set_started(mforms::ProgressBar *backend, bool flag) {
  ProgressBar ^ progressbar = ProgressBarWrapper::GetManagedObject<ProgressBar>(backend);
  progressbar->Value = 0;
}

//--------------------------------------------------------------------------------------------------

void ProgressBarWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_progressbar_impl.create = &ProgressBarWrapper::create;
  f->_progressbar_impl.set_value = &ProgressBarWrapper::set_value;
  f->_progressbar_impl.set_indeterminate = &ProgressBarWrapper::set_indeterminate;
  f->_progressbar_impl.set_started = &ProgressBarWrapper::set_started;
}

//--------------------------------------------------------------------------------------------------
