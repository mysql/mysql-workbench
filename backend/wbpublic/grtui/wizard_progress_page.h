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

#include "grt_wizard_form.h"

#include "mforms/imagebox.h"
#include "mforms/label.h"
#include "mforms/table.h"
#include "mforms/panel.h"
#include "mforms/textbox.h"
#include "mforms/progressbar.h"
#include "grt/grt_dispatcher.h"

namespace grtui {

  class WBPUBLICBACKEND_PUBLIC_FUNC WizardProgressPage : public WizardPage {
  public:
    WizardProgressPage(WizardForm *form, const std::string &id, bool has_progressbar);
    virtual ~WizardProgressPage();

    virtual ::mforms::View *get_advanced_panel() {
      return &_log_panel;
    }

    void set_heading(const std::string &text);

  protected:
    enum TaskState { StateNormal, StateBusy, StateDone, StateWarning, StateError, StateDisabled };

    struct WBPUBLICBACKEND_PUBLIC_FUNC TaskRow {
      mforms::ImageBox icon;
      mforms::Label label;
      std::function<bool()> execute; //! return value indicates whether an asynchronous function was actually executed
      std::function<bool()> process_fail; //! return value indicates whether it can continue executing ok
      std::function<void(grt::ValueRef)> process_finish;
      std::string status_text;
      bool enabled;
      bool async;
      bool async_running;
      bool async_failed;
      int async_errors;

      TaskRow() : enabled(true), async(false), async_running(false), async_failed(false), async_errors(0) {
      }

      void set_state(TaskState state);
      void set_enabled(bool flag);
    };

    mforms::Label _heading;

    std::vector<TaskRow *> _tasks;
    std::map<bec::GRTTask *, bec::GRTTask::Ref> _task_list;

    std::string _finish_message;

    mforms::Label _status_text;

    mforms::Table _task_table;

    mforms::Box *_progress_bar_box;
    mforms::ProgressBar *_progress_bar;
    mforms::Label *_progress_label;

    mforms::Panel _log_panel;
    mforms::TextBox _log_text;

    int _current_task;
    bool _busy;
    bool _done;
    bool _got_warning_messages;
    bool _got_error_messages;

    TaskRow *add_async_task(const std::string &caption, const std::function<bool()> &execute,
                            const std::string &status_text);

    TaskRow *add_task(const std::string &caption, const std::function<bool()> &execute, const std::string &status_text);

    TaskRow *add_disabled_task(const std::string &caption);

    TaskRow *current_task();

    void end_adding_tasks(const std::string &finish_message);

    void clear_tasks();
    void reset_tasks();

    void start_tasks();

    void set_status_text(const std::string &text, bool is_error = false);

    void update_progress(float pct, const std::string &caption);

    void add_log_text(const std::string &text);

    virtual void extra_clicked();

  private:
    TaskRow *add_task(bool async, const std::string &caption, const std::function<bool()> &execute,
                      const std::string &status_text);

  public:
    void execute_grt_task(const std::function<grt::ValueRef()> &slot, bool sync);

    void process_grt_task_message(const grt::Message &msg);
    void process_grt_task_fail(const std::exception &error, bec::GRTTask *task);
    void process_grt_task_finish(const grt::ValueRef &result, bec::GRTTask *task);

  protected:
    void perform_tasks();

    virtual bool allow_cancel();
    virtual bool allow_next();
    virtual bool allow_back();

    virtual void tasks_finished(bool success) {
    }

    virtual std::string extra_button_caption();

    virtual void enter(bool advancing);
  };
};
