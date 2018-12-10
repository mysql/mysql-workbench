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

#include "base/log.h"

#include "wizard_progress_page.h"
#include "grt/common.h"
#include "grt/grt_manager.h"

DEFAULT_LOG_DOMAIN("wizard")

using namespace grtui;

void WizardProgressPage::TaskRow::set_state(WizardProgressPage::TaskState state) {
  std::string file;
  switch (state) {
    case WizardProgressPage::StateNormal:
#ifdef __APPLE__
      file = "task_unchecked_mac.png";
#else
      file = "task_unchecked.png";
#endif
      break;
    case WizardProgressPage::StateBusy:
#ifdef __APPLE__
      file = "task_executing_mac.png";
#else
      file = "task_executing.png";
#endif
      break;
    case WizardProgressPage::StateDone:
#ifdef __APPLE__
      file = "task_checked_mac.png";
#else
      file = "task_checked.png";
#endif
      break;
    case WizardProgressPage::StateError:
#ifdef __APPLE__
      file = "task_error_mac.png";
#else
      file = "task_error.png";
#endif
      break;
    case WizardProgressPage::StateWarning:
#ifdef __APPLE__
      file = "task_warning_mac.png";
#else
      file = "task_warning.png";
#endif
      break;
    case WizardProgressPage::StateDisabled:
#ifdef __APPLE__
      file = "task_disabled_mac.png";
#else
      file = "task_disabled.png";
#endif
      break;
  }

  std::string path = bec::IconManager::get_instance()->get_icon_path(file);
  if (path.empty())
    logWarning("Could not find icon %s", file.c_str());
  icon.set_image(path);
}

void WizardProgressPage::TaskRow::set_enabled(bool flag) {
  set_state(flag ? StateNormal : StateDisabled);
  label.set_enabled(flag);

  enabled = flag;
}

WizardProgressPage::WizardProgressPage(WizardForm *form, const std::string &id, bool has_progressbar)
  : WizardPage(form, id), _log_panel(mforms::TitledBoxPanel), _log_text(mforms::VerticalScrollBar), _done(false) {
  _progress_bar = 0;
  _progress_bar_box = NULL;
  _progress_label = 0;

  _busy = false;
  _current_task = 0;

  _heading.set_text(
    _("The following tasks will now be executed. Please monitor the execution.\n"
      "Press Show Logs to see the execution logs."));
  _heading.set_wrap_text(true);

  set_spacing(8);

  add(&_heading, false, true);

  //  _log_text.set_padding(12);
  _log_text.set_read_only(true);

  _log_panel.set_title(_("Message Log"));
  _log_panel.add(&_log_text);
  _log_panel.set_padding(10);

  add(&_task_table, false, true);
  _task_table.set_padding(10);
  _task_table.set_column_spacing(2);
  _task_table.set_row_spacing(8);
  _task_table.set_column_count(2);

  _status_text.set_wrap_text(true);
  add(&_status_text, false, true);

  if (has_progressbar) {
    _progress_bar_box = mforms::manage(new mforms::Box(true));
    _progress_bar_box->set_spacing(8);

    _progress_bar = manage(new mforms::ProgressBar());
    _progress_label = manage(new mforms::Label());

    _progress_label->set_text("");
    add(_progress_label, false, true);
    _progress_bar_box->add(_progress_bar, true, true);

    add(_progress_bar_box, false, true);

    _progress_bar_box->show(false);
  }

  add(&_log_panel, true, true);
  _log_panel.show(false);
}

void WizardProgressPage::set_heading(const std::string &text) {
  _heading.set_text(text);
}

WizardProgressPage::~WizardProgressPage() {
  clear_tasks();
  _task_list.clear();
}

void WizardProgressPage::clear_tasks() {
  for (std::vector<TaskRow *>::iterator iter = _tasks.begin(); iter != _tasks.end(); ++iter) {
    _task_table.remove(&(*iter)->icon);
    _task_table.remove(&(*iter)->label);
    delete *iter;
  }
  _tasks.clear();
}

WizardProgressPage::TaskRow *WizardProgressPage::add_async_task(const std::string &caption,
                                                                const std::function<bool()> &execute,
                                                                const std::string &status_text) {
  return add_task(true, caption, execute, status_text);
}

WizardProgressPage::TaskRow *WizardProgressPage::add_task(const std::string &caption,
                                                          const std::function<bool()> &execute,
                                                          const std::string &status_text) {
  return add_task(false, caption, execute, status_text);
}

WizardProgressPage::TaskRow *WizardProgressPage::add_task(bool async, const std::string &caption,
                                                          const std::function<bool()> &execute,
                                                          const std::string &status_text) {
  TaskRow *row = new TaskRow;

  row->label.set_text(caption);

  _task_table.set_row_count((int)_tasks.size() + 1);
  _task_table.add(&row->icon, 0, 1, (int)_tasks.size(), (int)_tasks.size() + 1, 0);
  _task_table.add(&row->label, 1, 2, (int)_tasks.size(), (int)_tasks.size() + 1, mforms::HFillFlag);

  row->execute = execute;
  row->status_text = status_text;

  row->async = async;
  row->async_running = false;
  row->async_failed = false;

  row->set_state(WizardProgressPage::StateNormal);
  _tasks.push_back(row);

  return row;
}

void WizardProgressPage::end_adding_tasks(const std::string &finish_message) {
  _finish_message = finish_message;

  _status_text.set_text("");
  _log_panel.show(false);
}

void WizardProgressPage::reset_tasks() {
  for (std::vector<TaskRow *>::iterator task = _tasks.begin(); task != _tasks.end(); ++task) {
    (*task)->async_running = false;
    (*task)->async_failed = false;
    (*task)->set_state(WizardProgressPage::StateNormal);
  }
}

void WizardProgressPage::start_tasks() {
  _got_warning_messages = false;
  _got_error_messages = false;
  _current_task = 0;
  _busy = true;

  _form->update_buttons();

  if (_progress_bar_box) {
    _progress_bar_box->show(true);
    _progress_bar->start();
  }
  perform_tasks();
}

WizardProgressPage::TaskRow *WizardProgressPage::current_task() {
  if (_current_task < (int)_tasks.size())
    return _tasks[_current_task];
  return 0;
}

void WizardProgressPage::perform_tasks() {
  bool failed = false;

  if (!bec::GRTManager::get()->in_main_thread())
    throw std::logic_error("Method must be called from main thread");

  while (_current_task < (int)_tasks.size()) {
    TaskRow *task = _tasks[_current_task];

    _form->flush_events();
    bec::GRTManager::get()->perform_idle_tasks();

    // check if we're being called because an async task finished
    if (task->async_running) {
      task->async_running = false;
      if (task->async_failed) {
        failed = true;

        break;
      } else {
        task->set_state(StateDone);
        // task.ProcessTaskMsg((int)Msg_type.MT_info, ""); // delimit task log messages with empty line
        _current_task++;
        continue;
      }
    }

    set_status_text(task->status_text);

    if (task->enabled) {
      try {
        task->set_state(StateBusy);

        _form->flush_events();

        bool flag = task->execute();

        if (task->async && flag) {
          task->async_running = true;
          // if the task is asynchronous, return here to allow other stuff to be
          // executed. When the task is finished, process_task_finish() (or _task_fail())
          // will get called which will in turn, call this method again
          return;
        }

        task->set_state(StateDone);

        _current_task++;
      } catch (std::exception &exc) {
        failed = true;

        set_status_text(std::string("Error: ").append(exc.what()), true);
        break;
      }
    } else
      _current_task++;
  }

  if (!failed) {
    if (_got_error_messages)
      set_status_text(_("Operation has completed with errors. Please see logs for details."), true);
    else if (_got_warning_messages)
      set_status_text(_("Operation has completed with warnings. Please see logs for details."), true);
    else
      set_status_text(_finish_message);
  } else {
    while (_current_task < (int)_tasks.size()) {
      TaskRow *task = _tasks[_current_task++];
      task->set_state(StateError);
    }
    if (!_log_panel.is_shown())
      extra_clicked();
  }

  if (_progress_bar_box) {
    _progress_bar->stop();
    _progress_bar_box->show(false);
  }
  _done = true;
  _busy = false;
  tasks_finished(!failed);

  validate();
}

void WizardProgressPage::set_status_text(const std::string &text, bool is_error) {
  if (!bec::GRTManager::get()->in_main_thread()) {
    bec::GRTManager::get()->run_once_when_idle(this,
                                               std::bind(&WizardProgressPage::set_status_text, this, text, is_error));
    return;
  }

  if (is_error)
    _status_text.set_color("#ff0000");
  else
    _status_text.set_color(base::Color::getSystemColor(base::TextColor).to_html());
  _status_text.set_text(text);
}

void WizardProgressPage::update_progress(float pct, const std::string &caption) {
  if (!bec::GRTManager::get()->in_main_thread()) {
    bec::GRTManager::get()->run_once_when_idle(this,
                                               std::bind(&WizardProgressPage::update_progress, this, pct, caption));
    return;
  }

  if (_progress_label)
    _progress_label->set_text(caption);

  if (_progress_bar)
    _progress_bar->set_value(pct);
}

bool WizardProgressPage::allow_cancel() {
  return !_busy;
}

bool WizardProgressPage::allow_back() {
  return !_busy;
}

bool WizardProgressPage::allow_next() {
  return !_busy && _done;
}

void WizardProgressPage::enter(bool advancing) {
  WizardPage::enter(advancing);

  if (advancing) {
    _done = false;

    start_tasks();
  }
}

void WizardProgressPage::add_log_text(const std::string &text) {
  _log_text.append_text(text + "\n", true);
}

std::string WizardProgressPage::extra_button_caption() {
#ifdef _MSC_VER
  return _log_panel.is_shown() ? _("&Hide Logs") : _("&Show Logs");
#else
  return _log_panel.is_shown() ? _("Hide Logs") : _("Show Logs");
#endif
}

void WizardProgressPage::extra_clicked() {
  _log_panel.show(!_log_panel.is_shown());

  _form->update_buttons();
  relayout();
}

//--------------------------------------------------------------------------------------------------

void WizardProgressPage::execute_grt_task(const std::function<grt::ValueRef()> &slot, bool sync) {
  bec::GRTTask::Ref task = bec::GRTTask::create_task("wizard task", bec::GRTManager::get()->get_dispatcher(), slot);

  // We hold an extra ptr for the task so it's not released too early
  _task_list.insert(std::make_pair(task.get(), task));

  // We need to pass task to the signals, so we can remove it from the _task_list and allow shared_ptr to release the
  // task
  scoped_connect(task->signal_message(),
                 std::bind(&WizardProgressPage::process_grt_task_message, this, std::placeholders::_1));
  scoped_connect(task->signal_failed(),
                 std::bind(&WizardProgressPage::process_grt_task_fail, this, std::placeholders::_1, task.get()));
  scoped_connect(task->signal_finished(),
                 std::bind(&WizardProgressPage::process_grt_task_finish, this, std::placeholders::_1, task.get()));

  if (sync)
    bec::GRTManager::get()->get_dispatcher()->add_task_and_wait(task);
  else
    bec::GRTManager::get()->get_dispatcher()->add_task(task);
}

//--------------------------------------------------------------------------------------------------

void WizardProgressPage::process_grt_task_message(const grt::Message &msg) {
  std::string msgTypeStr;
  switch (msg.type) {
    case grt::ErrorMsg: {
      _got_error_messages = true;
      _tasks[_current_task]->async_errors++;

      msgTypeStr = "ERROR: ";
    } break;
    case grt::WarningMsg: {
      _got_warning_messages = true;

      msgTypeStr = "WARNING: ";
    } break;
    case grt::InfoMsg:
      msgTypeStr = "";
      break;

    case grt::ProgressMsg:
      update_progress(msg.progress, msg.text);
      return;

    case grt::OutputMsg:
      _log_text.append_text(msg.text, true);
      return;

    case grt::ControlMsg:
      return;
    case grt::NoErrorMsg:
    case grt::VerboseMsg:
      return;
  }

  add_log_text(msgTypeStr + msg.text);
}

//----------------------------------------------------------------------------------------------------------------------

void WizardProgressPage::process_grt_task_fail(const std::exception &error, bec::GRTTask *task) {
  _tasks[_current_task]->async_failed = true;
  if (_tasks[_current_task]->process_fail) {
    // if process_fail returns true, the error was recovered
    if (_tasks[_current_task]->process_fail())
      _tasks[_current_task]->async_failed = false;
    else
      set_status_text(std::string("Error: ").append(error.what()), true);
  } else {
    add_log_text(std::string("Operation failed: ").append(error.what()));
    set_status_text(std::string("Error: ").append(error.what()), true);
  }

  // continue with task execution
  std::map<bec::GRTTask *, bec::GRTTask::Ref>::iterator it = _task_list.find(task);
  if (it != _task_list.end())
    _task_list.erase(it);

  perform_tasks();
}

//----------------------------------------------------------------------------------------------------------------------

void WizardProgressPage::process_grt_task_finish(const grt::ValueRef &result, bec::GRTTask *task) {
  bec::GRTManager::get()->perform_idle_tasks();

  if (_got_error_messages || _got_warning_messages) {
    if (!_log_panel.is_shown())
      extra_clicked();
  }

  if (_tasks[_current_task]->process_finish)
    _tasks[_current_task]->process_finish(result);

  std::map<bec::GRTTask *, bec::GRTTask::Ref>::iterator it = _task_list.find(task);
  if (it != _task_list.end())
    _task_list.erase(it);
  // continue with task execution
  perform_tasks();
}

//--------------------------------------------------------------------------------------------------
