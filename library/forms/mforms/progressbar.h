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

#pragma once

#include <mforms/base.h>
#include <mforms/view.h>

namespace mforms {
  class ProgressBar;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct ProgressBarImplPtrs {
    bool (*create)(ProgressBar *self);
    void (*set_value)(ProgressBar *self, float pct);
    void (*set_indeterminate)(ProgressBar *self, bool flag);
    void (*set_started)(ProgressBar *self, bool flag);
  };
#endif
#endif

  /** A progress bar to show completion state of a task. */
  class MFORMS_EXPORT ProgressBar : public View {
  public:
    ProgressBar();

    /** Sets whether the progressbar knows how much actual progress was made. */
    void set_indeterminate(bool flag);

    /** Starts animating the progressbar to indicate the task is in progress. */
    void start();
    /** Stops animating the progressbar. */
    void stop();
    /** Sets the progress value (0.0 to 1.0) */
    void set_value(float pct);

  protected:
    ProgressBarImplPtrs *_progressbar_impl;
  };
};
