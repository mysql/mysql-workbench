/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

/**
 * Utilities is a support class to trigger message boxes and the like in the front end.
 */

#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/log.h"
#include "base/threading.h"

#include "mforms/mforms.h"
#include "mforms/password_cache.h"

#include "mdc_image.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE);

using namespace mforms;
using namespace base;

GThread *_mforms_main_thread = NULL;

static std::map<std::string, int> remembered_message_answers;
static std::string remembered_message_answer_file;

std::function<void()> mforms::Utilities::_driver_shutdown_cb;

//--------------------------------------------------------------------------------------------------

void Utilities::beep() {
  ControlFactory::get_instance()->_utilities_impl.beep();
}

//--------------------------------------------------------------------------------------------------

static void *_show_dialog(const DialogType type, const std::string &title, const std::string &text,
                          const std::string &ok, const std::string &cancel, const std::string &other) {
  int *ret = new int;
  switch (type) {
    case DialogMessage:
      *ret = ControlFactory::get_instance()->_utilities_impl.show_message(title, text, ok, cancel, other);
      break;
    case DialogWarning:
      *ret = ControlFactory::get_instance()->_utilities_impl.show_warning(title, text, ok, cancel, other);
      break;
    case DialogError:
      *ret = ControlFactory::get_instance()->_utilities_impl.show_error(title, text, ok, cancel, other);
      break;
    default:
      *ret = mforms::ResultUnknown;
  }

  return (void *)ret;
}

static int void_to_int(void *val) {
  int *ret = (int *)val;
  int ret_val = *ret;
  delete ret;
  return ret_val;
}

int Utilities::show_message(const std::string &title, const std::string &text, const std::string &ok,
                            const std::string &cancel, const std::string &other) {
  if (Utilities::in_main_thread())
    return void_to_int(_show_dialog(DialogMessage, title, text, ok, cancel, other));
  else
    return void_to_int(
      Utilities::perform_from_main_thread(std::bind(&_show_dialog, DialogMessage, title, text, ok, cancel, other)));
}

//--------------------------------------------------------------------------------------------------

int Utilities::show_error(const std::string &title, const std::string &text, const std::string &ok,
                          const std::string &cancel, const std::string &other) {
  if (Utilities::in_main_thread())
    return void_to_int(_show_dialog(DialogError, title, text, ok, cancel, other));
  else
    return void_to_int(
      Utilities::perform_from_main_thread(std::bind(&_show_dialog, DialogError, title, text, ok, cancel, other)));
}

//--------------------------------------------------------------------------------------------------

int Utilities::show_warning(const std::string &title, const std::string &text, const std::string &ok,
                            const std::string &cancel, const std::string &other) {
  if (Utilities::in_main_thread())
    return void_to_int(_show_dialog(DialogWarning, title, text, ok, cancel, other));
  else
    return void_to_int(
      Utilities::perform_from_main_thread(std::bind(&_show_dialog, DialogWarning, title, text, ok, cancel, other)));
}

//--------------------------------------------------------------------------------------------------

int Utilities::show_message_and_remember(const std::string &title, const std::string &text, const std::string &ok,
                                         const std::string &cancel, const std::string &other,
                                         const std::string &answer_id, const std::string &checkbox_text) {
  if (remembered_message_answers.find(answer_id) != remembered_message_answers.end())
    return remembered_message_answers[answer_id];

  if (!ControlFactory::get_instance()->_utilities_impl.show_message_with_checkbox)
    return show_message(title, text, ok, cancel, other);

  bool remember = false;
  int rc = ControlFactory::get_instance()->_utilities_impl.show_message_with_checkbox(title, text, ok, cancel, other,
                                                                                      checkbox_text, remember);
  if (remember) {
    remembered_message_answers[answer_id] = rc;
    save_message_answers();
  }
  return rc;
}

void Utilities::set_message_answers_storage_path(const std::string &path) {
  remembered_message_answer_file = path;

  FILE *f = base_fopen(remembered_message_answer_file.c_str(), "r");
  if (f) {
    char line[1024];

    while (fgets(line, sizeof(line), f)) {
      char *ptr = strrchr(line, '=');
      if (ptr) {
        *ptr = 0;
        remembered_message_answers[line] = base::atoi<int>(ptr + 1, 0);
      }
    }
    fclose(f);
  }
}

void Utilities::save_message_answers() {
  if (!remembered_message_answer_file.empty()) {
    FILE *f = base_fopen(remembered_message_answer_file.c_str(), "w+");

    for (std::map<std::string, int>::const_iterator iter = remembered_message_answers.begin();
         iter != remembered_message_answers.end(); ++iter)
      fprintf(f, "%s=%i\n", iter->first.c_str(), iter->second);
    fclose(f);
  }
}

void Utilities::forget_message_answers() {
  remembered_message_answers.clear();
  save_message_answers();
}

//--------------------------------------------------------------------------------------------------

void Utilities::show_wait_message(const std::string &title, const std::string &text) {
  // The wait message is a special window, so there's no need to hide the splash screen.
  ControlFactory::get_instance()->_utilities_impl.show_wait_message(title, text);
}

//--------------------------------------------------------------------------------------------------

bool Utilities::hide_wait_message() {
  return ControlFactory::get_instance()->_utilities_impl.hide_wait_message();
}

//--------------------------------------------------------------------------------------------------

class CancellableTaskData {
public:
  std::function<void *()> task;
  bool finished;
  std::shared_ptr<void *> result_ptr;

  int ref_count;

  base::Semaphore semaphore;
  CancellableTaskData() : finished(false), ref_count(1), semaphore(0) {
  }
};

// To ensure the shared thread data is not freed too early regardless what finishes first
// (the thread or run_cancelable_task()) we store this data here in a private structure, ref counted.
static base::Mutex thread_data_mutex;
static std::map<void *, CancellableTaskData *> thread_data;

static void *cancellable_task_thread(void *) {
  CancellableTaskData *data = NULL;

  {
    base::MutexLock lock(thread_data_mutex);
    data = thread_data[g_thread_self()];
    if (data != NULL)
      data->ref_count++; // Increment ref count while the data is still protected.
  }

  if (data != NULL) // Should never be NULL but...
  {
    void *ptr = NULL;
    try {
      ptr = data->task();
    } catch (std::exception &exc) {
      logError("Cancellable task threw uncaught exception: %s", exc.what());
    }

    data->semaphore.wait(); // Wait for the main thread to signal it is ready.
    *data->result_ptr = ptr;
    data->finished = true;
    ControlFactory::get_instance()->_utilities_impl.stop_cancelable_wait_message();

    {
      base::MutexLock lock(thread_data_mutex);
      data->ref_count--;
      if (data->ref_count == 0) {
        thread_data.erase(g_thread_self());
        delete data;
      }
    }
  }

  return NULL;
}

bool Utilities::run_cancelable_task(const std::string &title, const std::string &text,
                                    const std::function<void *()> &task, const std::function<bool()> &cancel_task,
                                    void *&task_result) {
  std::shared_ptr<void *> result(new void *((void *)-1));

  CancellableTaskData *data = NULL;
  GThread *thread = NULL;

  // Start a thread that will run the task. Store thread data so it can be accessed by the thread
  // safely.
  {
    base::MutexLock lock(thread_data_mutex);
    data = new CancellableTaskData(); // Ref count is 1.

    GError *error = NULL;
    thread = base::create_thread((void *(*)(void *))cancellable_task_thread, NULL, &error);
    if (thread == NULL) {
      std::string msg("Error creating thread: ");
      msg.append(error->message);
      g_error_free(error);

      delete data;
      throw std::runtime_error(msg);
    }

    // result_ptr is used by the task thread to pass back the result from the task callback.
    // We cannot directly pass a pointer to task_result, because if the task is canceled,
    // this function will return and the caller may free the task_result object before the
    // canceled task actually finishes executing, which would invalidate any ptr to task_result.
    data->result_ptr = result;

    // The thread will wait on the mutex as we still lock it, so it's safe to work on the data.
    thread_data[thread] = data;
    data->task = task;
  }

  // Callback for the frontend to signal the worker thread that it's ready.
  std::function<void()> signal_ready = std::bind(&base::Semaphore::post, &data->semaphore);

  bool function_result = false;

retry:
  if (ControlFactory::get_instance()->_utilities_impl.run_cancelable_wait_message(title, text, signal_ready,
                                                                                  cancel_task)) {
    // Sometimes, in the mac, there's a race and/or bug that causes the event loop
    // from the wait panel dialog to be exited when a nested modal dialog is closed.
    // Clicking OK for the nested dialog, exits the wait panel, which would cause it
    // to return prematurely. Just execute the modal panel again if that's the case.
    // Additional note: sometimes the wait panel must be closed on purpose, so this is necessary anyway.
    if (!data->finished)
      goto retry;

    // Task completed. We can directly access the data here without lock as we still have
    // the increased ref count here.
    task_result = *result;
    function_result = true;
  } else {
    // Task canceled by user.
    logDebug2("run_cancelable_wait_message returned false\n");
  }

  {
    base::MutexLock lock(thread_data_mutex);
    data->ref_count--;
    if (data->ref_count == 0) {
      thread_data.erase(thread);
      delete data;
    }
  }

  return function_result;
}

//--------------------------------------------------------------------------------------------------

void Utilities::set_clipboard_text(const std::string &text) {
  ControlFactory::get_instance()->_utilities_impl.set_clipboard_text(text);
}

//--------------------------------------------------------------------------------------------------

std::string Utilities::get_clipboard_text() {
  return ControlFactory::get_instance()->_utilities_impl.get_clipboard_text();
}

//--------------------------------------------------------------------------------------------------

std::string Utilities::get_special_folder(FolderType type) {
  return ControlFactory::get_instance()->_utilities_impl.get_special_folder(type);
}

//--------------------------------------------------------------------------------------------------

void Utilities::open_url(const std::string &url) {
  return ControlFactory::get_instance()->_utilities_impl.open_url(url);
}

//--------------------------------------------------------------------------------------------------

bool Utilities::move_to_trash(const std::string &path) {
  if (ControlFactory::get_instance()->_utilities_impl.move_to_trash)
    return ControlFactory::get_instance()->_utilities_impl.move_to_trash(path);
  else {
    if (g_file_test(path.c_str(), G_FILE_TEST_IS_DIR)) {
      if (base_rmdir_recursively(path.c_str()) < 0)
        return false;
    } else {
      if (!base::remove(path))
        return false;
    }
    return true;
  }
}

//--------------------------------------------------------------------------------------------------
void Utilities::reveal_file(const std::string &path) {
  ControlFactory::get_instance()->_utilities_impl.reveal_file(path);
}

//--------------------------------------------------------------------------------------------------

TimeoutHandle Utilities::add_timeout(float interval, const std::function<bool()> &callback) {
  return ControlFactory::get_instance()->_utilities_impl.add_timeout(interval, callback);
}

//--------------------------------------------------------------------------------------------------

void Utilities::cancel_timeout(TimeoutHandle handle) {
  ControlFactory::get_instance()->_utilities_impl.cancel_timeout(handle);
}

//--------------------------------------------------------------------------------------------------

void Utilities::add_end_ok_cancel_buttons(mforms::Box *box, mforms::Button *ok, mforms::Button *cancel) {
#ifdef __APPLE__
  box->add_end(ok, false, true);
  box->add_end(cancel, false, true);
#else
  box->add_end(cancel, false, true);
  box->add_end(ok, false, true);
#endif
}

//--------------------------------------------------------------------------------------------------

static void on_request_action(mforms::TextEntryAction action, mforms::Button *btn) {
  if (action == mforms::EntryActivate)
    btn->signal_clicked()->operator()();
}

//--------------------------------------------------------------------------------------------------

static void *_request_input_main(const std::string &title, const std::string &description,
                                 const std::string &default_value, std::string *ret_value) {
  // In order to avoid trouble with window z-ordering we explicitly ask to hide any wait window
  // that could get in the way. Same for the splash screen.
  Utilities::hide_wait_message();

  mforms::Form input_form(NULL, (FormFlag)(FormDialogFrame | FormStayOnTop));
  mforms::Table content;
  mforms::ImageBox icon;
  mforms::Label description_label("");
  mforms::TextEntry edit;
  mforms::Box button_box(true);
  mforms::Button ok_button;
  mforms::Button cancel_button;

  input_form.set_title(title.empty() ? _("Enter a value") : title);

  content.set_padding(12);
  content.set_row_count(2);
  content.set_row_spacing(10);
  content.set_column_count(3);
  content.set_column_spacing(4);

  icon.set_image("message_edit.png");
  content.add(&icon, 0, 1, 0, 2, HFillFlag | VFillFlag);

  description_label.set_text(description);
  description_label.set_style(BoldStyle);

  edit.set_size(150, -1);
  edit.set_value(default_value);
  edit.signal_action()->connect(std::bind(&on_request_action, std::placeholders::_1, &ok_button));

  content.add(&description_label, 1, 2, 0, 1, HFillFlag | VFillFlag);
  content.add(&edit, 2, 3, 0, 1, HFillFlag | VFillFlag);

  button_box.set_spacing(8);
  ok_button.set_text(_("OK"));
  ok_button.set_size(75, -1);
  cancel_button.set_text(_("Cancel"));
  cancel_button.set_size(75, -1);
  Utilities::add_end_ok_cancel_buttons(&button_box, &ok_button, &cancel_button);
  content.add(&button_box, 1, 3, 1, 2, HFillFlag | VFillFlag);

  input_form.set_content(&content);
  input_form.center();
  edit.focus();
  bool result = input_form.run_modal(&ok_button, &cancel_button);
  if (result)
    *ret_value = edit.get_string_value();

  return (void *)result;
}

//--------------------------------------------------------------------------------------------------

static bool _request_input(const std::string &title, const std::string &description, const std::string &default_value,
                           std::string &ret_value) {
  if (Utilities::in_main_thread())
    return _request_input_main(title, description, default_value, &ret_value) != nullptr;
  else
    return Utilities::perform_from_main_thread(
             std::bind(&_request_input_main, title, description, default_value, &ret_value)) != nullptr;
}

//--------------------------------------------------------------------------------------------------

bool Utilities::request_input(const std::string &title, const std::string &description,
                              const std::string &default_value, std::string &ret_value /*out*/) {
  return _request_input(title, description, default_value, ret_value);
}

//--------------------------------------------------------------------------------------------------

void Utilities::store_password(const std::string &service, const std::string &account, const std::string &password) {
  // in-memory cache
  PasswordCache::get()->add_password(service, account, password.c_str());

  // OS storage
  logDebug("Storing password for '%s'@'%s'\n", account.c_str(), service.c_str());
  ControlFactory::get_instance()->_utilities_impl.store_password(service, account, password);
}

//--------------------------------------------------------------------------------------------------

bool Utilities::find_password(const std::string &service, const std::string &account, std::string &password) {
  const bool ret = ControlFactory::get_instance()->_utilities_impl.find_password(service, account, password);
  logDebug("Looking up password for '%s'@'%s' has %s\n", account.c_str(), service.c_str(),
           ret ? "succeeded" : "failed");

  if (ret)
    PasswordCache::get()->add_password(service, account, password.c_str());

  return ret;
}

//--------------------------------------------------------------------------------------------------

bool Utilities::find_cached_password(const std::string &service, const std::string &account, std::string &password) {
  return PasswordCache::get()->get_password(service, account, password);
}

//--------------------------------------------------------------------------------------------------

void Utilities::forget_cached_password(const std::string &service, const std::string &account) {
  logDebug2("Forgetting cached password for '%s'@'%s'\n", account.c_str(), service.c_str());
  PasswordCache::get()->remove_password(service, account);
}

//--------------------------------------------------------------------------------------------------

void Utilities::forget_password(const std::string &service, const std::string &account) {
  Utilities::forget_cached_password(service, account);

  logDebug("Forgetting password for '%s'@'%s'\n", account.c_str(), service.c_str());
  ControlFactory::get_instance()->_utilities_impl.forget_password(service, account);
}

//-------------------------------------------------------------------------------

void *Utilities::perform_from_main_thread(const std::function<void *()> &slot, bool wait_done) {
  return ControlFactory::get_instance()->_utilities_impl.perform_from_main_thread(slot, wait_done);
}

//--------------------------------------------------------------------------------------------------

static void *_ask_for_password_main(const std::string &title, const std::string &service,
                                    std::string *username /*in/out*/, bool prompt_storage,
                                    std::string *ret_password /*out*/, bool *ret_store /*out*/) {
  logDebug("Creating and showing password dialog\n");

  Utilities::hide_wait_message();

  mforms::Form password_form(NULL, (FormFlag)(FormDialogFrame | FormStayOnTop));
  mforms::Table content;
  mforms::ImageBox icon;
  mforms::Label description("");
  mforms::Label service_description_label("");
  mforms::Label service_label("");
  mforms::Label user_description_label("");
  mforms::Label pw_description_label("");
  mforms::TextEntry password_edit(PasswordEntry);
  mforms::CheckBox save_password_box;
  mforms::Box button_box(true);
  mforms::Button ok_button;
  mforms::Button cancel_button;

  // Since we cannot simply change the function's signature (I only say: python) we have to use
  // a different way to pass an additional value in. The description used for the login details
  // request is passed in the title parameter as well, separated by the pipe symbol.
  std::vector<std::string> title_parts = base::split(title, "|", 2);
  std::string caption;
  if (title_parts.size() == 0 || title_parts[0].empty())
    caption = _("MySQL Workbench Authentication");
  else
    caption = title_parts[0];

  password_form.set_title(caption);
  password_form.set_name(caption);

  content.set_padding(12);
  content.set_row_count(6);
  content.set_row_spacing(prompt_storage ? 8 : 7);
  content.set_column_count(3);
  content.set_column_spacing(4);

  icon.set_image("message_wb_lock.png");
  content.add(&icon, 0, 1, 0, 6, HFillFlag | VFillFlag);

  if (title_parts.size() < 2 || title_parts[1].empty())
    description.set_text(_("Please enter password for the following service:"));
  else
    description.set_text(title_parts[1]);
  description.set_wrap_text(true);
  description.set_style(BigBoldStyle);
  description.set_size(300, -1);
  content.add(&description, 1, 3, 0, 1, HFillFlag | HExpandFlag | VFillFlag);

  service_description_label.set_text(_("Service:"));
  service_description_label.set_text_align(MiddleRight);
  service_description_label.set_style(BoldStyle);
  service_label.set_text(service);
  content.add(&service_description_label, 1, 2, 1, 2, HFillFlag | VFillFlag);
  content.add(&service_label, 2, 3, 1, 2, HFillFlag | VFillFlag);

  user_description_label.set_text(_("User:"));
  user_description_label.set_text_align(MiddleRight);
  user_description_label.set_style(BoldStyle);

  // Create an edit box for the user name if the given one is not set, otherwise just display the name.
  mforms::TextEntry *user_edit = NULL;
  if (username->empty()) {
    user_edit = mforms::manage(new mforms::TextEntry());
    user_edit->set_value(_("<user name>"));
    content.add(&user_description_label, 1, 2, 2, 3, HFillFlag | VFillFlag);
    content.add(user_edit, 2, 3, 2, 3, HFillFlag | VFillFlag);
  } else {
    mforms::Label *user_label = mforms::manage(new mforms::Label(*username));
    content.add(&user_description_label, 1, 2, 2, 3, HFillFlag | VFillFlag);
    content.add(user_label, 2, 3, 2, 3, HFillFlag | VFillFlag);
  }

  pw_description_label.set_text(_("Password:"));
  pw_description_label.set_text_align(MiddleRight);
  pw_description_label.set_style(BoldStyle);
  password_edit.set_name("Password");
  content.add(&pw_description_label, 1, 2, 3, 4, HFillFlag | VFillFlag);
  content.add(&password_edit, 2, 3, 3, 4, HFillFlag | HExpandFlag);

  if (prompt_storage) {
#ifdef _MSC_VER
    save_password_box.set_text(_("Save password in vault"));
#else
    save_password_box.set_text(_("Save password in keychain"));
#endif
    content.add(&save_password_box, 2, 3, 4, 5, HExpandFlag | HFillFlag);
  }

  button_box.set_spacing(8);
  button_box.set_name("Button Bar");
  ok_button.set_text(_("OK"));
  //  ok_button.set_size(75, -1);
  cancel_button.set_text(_("Cancel"));
  //  cancel_button.set_size(75, -1);
  Utilities::add_end_ok_cancel_buttons(&button_box, &ok_button, &cancel_button);
  if (prompt_storage)
    content.add(&button_box, 1, 3, 5, 6, HFillFlag | VFillFlag);
  else
    content.add(&button_box, 1, 3, 4, 5, HFillFlag | VFillFlag);

  password_form.set_content(&content);
  password_form.center();

  password_edit.focus();
  password_edit.signal_action()->connect(std::bind(&on_request_action, std::placeholders::_1, &ok_button));

  bool result = password_form.run_modal(&ok_button, &cancel_button);
  if (result) {
    *ret_password = password_edit.get_string_value();
    *ret_store = save_password_box.get_active();

    if (user_edit != NULL)
      *username = user_edit->get_string_value();

    // always store in cache
    PasswordCache::get()->add_password(service, *username, ret_password->c_str());
  }

  return (void *)result;
}

static bool _ask_for_password(const std::string &title, const std::string &service, std::string &username /*in/out*/,
                              bool prompt_storage, std::string &ret_password /*out*/, bool &ret_store /*out*/) {
  if (Utilities::in_main_thread())
    return _ask_for_password_main(title, service, &username, prompt_storage, &ret_password, &ret_store) != NULL;
  else
    return Utilities::perform_from_main_thread(std::bind(&_ask_for_password_main, title, service, &username,
                                                         prompt_storage, &ret_password, &ret_store)) != NULL;
}

//--------------------------------------------------------------------------------------------------

bool Utilities::ask_for_password(const std::string &title, const std::string &service, const std::string &username,
                                 std::string &ret_password /*out*/) {
  std::string ret_username = username;
  bool dummy = false;
  return _ask_for_password(title, service, ret_username, false, ret_password, dummy);
}

//--------------------------------------------------------------------------------------------------

/**
 * Shows a dialog to request the password for the given service and user name.
 *
 * @param title Optional title describing the reason for the password request (eg. "Connect to MySQL Server")
 * @param service A description for what the password is required (e.g. "MySQL Server 5.1 on Thunder").
 * @param username [in/out] The name of the user for which to request the password.
 * @param password [out] Contains the password on return.
 *
 * @return True if the user pressed OK, otherwise false.
 */
bool Utilities::ask_for_password_check_store(const std::string &title, const std::string &service,
                                             std::string &username, std::string &password, bool &store) {
  return _ask_for_password(title, service, username, true, password, store);
}

//--------------------------------------------------------------------------------------------------

/**
 * Shows a dialog to request the password for the given service and user name.
 * If requested by the user the password will be saved using either system facilities like macOS Keychain or
 * Gnome Keyring, or an encrypted file.
 *
 * @param title Optional title describing the reason for the password request (eg. "Connect to MySQL Server")
 * @param service A description for what the password is required (e.g. "MySQL Server 5.1 on Thunder").
 * @param username The name of the user for which to request the password. If empty on enter then the user name can
 *        also be edited.
 * @param reset_password Delete stored password and ask for a new one.
 * @param password [out] Contains the password on return.
 *
 * @return True if the user pressed OK, otherwise false.
 */
bool Utilities::credentials_for_service(const std::string &title, const std::string &service,
                                        std::string &username /*in/out*/, bool reset_password,
                                        std::string &password /*out*/) {
  if (!reset_password && find_password(service, username, password))
    return true;

  if (reset_password)
    forget_password(service, username);

  bool should_store_password_out = false;
  if (ask_for_password_check_store(title, service, username, password, should_store_password_out)) {
    if (should_store_password_out) {
      try {
        store_password(service, username, password);
      } catch (std::exception &exc) {
        logWarning("Could not store password vault: %s\n", exc.what());
        show_warning(title.empty() ? _("Error Storing Password") : title,
                     std::string("There was an error storing the password:\n") + exc.what(), "OK");
      }
    }
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

#ifdef _MSC_VER

static int modal_loops = 0;

void Utilities::enter_modal_loop() {
  modal_loops++;
}

void Utilities::leave_modal_loop() {
  modal_loops--;
}

bool Utilities::in_modal_loop() {
  return modal_loops > 0;
}

#endif

//--------------------------------------------------------------------------------------------------

static cairo_user_data_key_t hidpi_icon_key;

/**
 * Helper function to simplify icon loading. Returns NULL if the icon could not be found or
 * something wrong happened while loading.
 */
cairo_surface_t *Utilities::load_icon(const std::string &name, bool allow_hidpi) {
  if (name.empty())
    return NULL;

#ifdef __APPLE__
  allow_hidpi = true; // For OSX we always want hires images.
#endif

  if (allow_hidpi && mforms::App::get()->backing_scale_factor() > 1.0) {
    std::string hidpi_name = base::strip_extension(name) + "@2x" + base::extension(name);
    std::string icon_path = App::get()->get_resource_path(hidpi_name);
    cairo_surface_t *tmp = mdc::surface_from_png_image(icon_path);
    if (tmp) {
      // Mark the surface as being a hi-res variant of a standard icon.
      cairo_surface_set_user_data(tmp, &hidpi_icon_key, (void *)1, NULL);
      return tmp;
    }
  }

  std::string icon_path = App::get()->get_resource_path(name);
  return mdc::surface_from_png_image(icon_path);
}

//--------------------------------------------------------------------------------------------------

bool Utilities::is_hidpi_icon(cairo_surface_t *s) {
  return cairo_surface_get_user_data(s, &hidpi_icon_key) == (void *)1;
}

//--------------------------------------------------------------------------------------------------

bool Utilities::icon_needs_reload(cairo_surface_t *s) {
  float scale = s && mforms::Utilities::is_hidpi_icon(s) ? 2.0f : 1.0f;
  return mforms::App::get()->backing_scale_factor() != scale;
}

//--------------------------------------------------------------------------------------------------

void Utilities::paint_icon(cairo_t *cr, cairo_surface_t *image, double x, double y, float alpha) {
  if (cr == nullptr || image == nullptr)
    return;

  float backing_scale_factor = mforms::App::get()->backing_scale_factor();

  if (backing_scale_factor > 1 && mforms::Utilities::is_hidpi_icon(image)) {
    cairo_save(cr);
    cairo_scale(cr, 1 / backing_scale_factor, 1 / backing_scale_factor);
    cairo_set_source_surface(cr, image, x * backing_scale_factor, y * backing_scale_factor);
    if (alpha == 1.0)
      cairo_paint(cr);
    else
      cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
  } else if (backing_scale_factor == 1 && mforms::Utilities::is_hidpi_icon(image)) {
    // Special case where the icon is for hidpi but the screen is not.
    // This happens when the icon was cached while the window was
    // on a hidpi screen but is then dragged to a low dpi screen.
    // Ideally this would trigger a reload of the icon.
    cairo_save(cr);
    cairo_scale(cr, 0.5, 0.5);
    cairo_set_source_surface(cr, image, x * 2, y * 2);
    if (alpha == 1.0)
      cairo_paint(cr);
    else
      cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
    logDebug3("Icon is for hidpi screen but the screen is not.\n");
  } else {
    cairo_set_source_surface(cr, image, x, y);
    if (alpha == 1.0)
      cairo_paint(cr);
    else
      cairo_paint_with_alpha(cr, alpha);
  }
}

//--------------------------------------------------------------------------------------------------

base::Size Utilities::getImageSize(cairo_surface_t *icon) {
  base::Size result(cairo_image_surface_get_width(icon), cairo_image_surface_get_height(icon));
  if (mforms::Utilities::is_hidpi_icon(icon)) {
    result.width /= 2;
    result.height /= 2;
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Shortens the string so that it fits into the given width. If there is already enough room then
 * the input is simply returned. Otherwise letters are removed (via binary search) and ellipses
 * are added so that the entire result fits into that width.
 */
std::string Utilities::shorten_string(cairo_t *cr, const std::string &text, double width) {
  int ellipsis_width = 0;
  size_t length;
  size_t l, h, n, w;
  cairo_text_extents_t extents;

  // If the text fits already, return the input.
  cairo_text_extents(cr, text.c_str(), &extents);
  if (extents.width <= width)
    return text;

  length = g_utf8_strlen(text.data(), (gssize)text.size());
  if (length == 0 || width <= 0)
    return "";
  else {
    cairo_text_extents(cr, "...", &extents);
    ellipsis_width = (int)ceil(extents.width);
  }

  const gchar *head = text.c_str();
  if (width <= ellipsis_width)
    return "";
  else {
    // Do a binary search for the optimal string length which fits into the given width.
    l = 0;
    h = length - 1;
    while (l < h) {
      n = (l + h) / 2;

      // Skip to the nth position, which needs the following loop as we don't have direct
      // access to a char in an utf-8 buffer (one of the limitations of that transformation format).
      const gchar *tail = head;
      for (size_t i = 0; i < n; i++)
        tail = g_utf8_next_char(tail);
      gchar *part = g_strndup(head, (gsize)(tail - head));
      cairo_text_extents(cr, part, &extents);
      g_free(part);
      w = (int)ceil(extents.width) + ellipsis_width;
      if (w <= width)
        l = n + 1;
      else
        h = n;
    }
    const gchar *begin = g_utf8_offset_to_pointer(text.data(), 0);
    const gchar *end = g_utf8_offset_to_pointer(begin, (glong)(l - 1));
    std::string temp = std::string(text.data(), end - begin) + "...";
    return temp;
  }

  return "";
}

//--------------------------------------------------------------------------------------------------

double Utilities::get_text_width(const std::string &text, const std::string &font) {
  return ControlFactory::get_instance()->_utilities_impl.get_text_width(text, font);
}

//--------------------------------------------------------------------------------------------------

bool Utilities::in_main_thread() {
  return g_thread_self() == _mforms_main_thread;
}

void Utilities::set_thread_name(const std::string &name) {
  if (ControlFactory::get_instance()->_utilities_impl.set_thread_name)
    ControlFactory::get_instance()->_utilities_impl.set_thread_name(name);
}

void Utilities::driver_shutdown() {
  if (Utilities::_driver_shutdown_cb)
    Utilities::_driver_shutdown_cb();
}

void Utilities::add_driver_shutdown_callback(const std::function<void()> &slot) {
  Utilities::_driver_shutdown_cb = slot;
}
