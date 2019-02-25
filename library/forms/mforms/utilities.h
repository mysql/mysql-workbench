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

#pragma once

/**
 * Implementation some miscellaneous stuff needed in mforms.
 */
#include <cairo/cairo.h>
#include <functional>
#include "base/geometry.h"
#include "mforms/base.h"

namespace mforms {
  // Constants for special folders on a system.
  enum MFORMS_EXPORT FolderType {
    Documents,       //!<< The path to the user's documents folder.
    Desktop,         //!<< The physical path to the user's desktop.
    ApplicationData, //!<< Path to folder to store application specifc data for a user.

    // Platform specific folders.
    WinProgramFiles,    //!<< Windows only, 64 bit applications.
    WinProgramFilesX86, //!<< Windows only, 32 bit applications.

    ApplicationSettings //!<< Full path to App specific folder inside ApplicationData where config files and others are
                        //!kept
  };

  enum MFORMS_EXPORT PasswordStoreScheme { SessionStorePasswordScheme = 1, PersistentStorePasswordScheme = 2 };

  /**
  * Code which abstracts special keys for each platform, to be used in the key event.
  */
  enum MFORMS_EXPORT KeyCode {
    KeyNone,
    KeyChar,         //!< No special char. The key event has the entered character(s) in the text field.
    KeyModifierOnly, //!< A combination of Shift/Control/Command/Alt only, without another key.
    KeyEnter,        //!< The numpad <enter> key.
    KeyReturn,       //!< The main keyboard <return> key.
    KeyHome,
    KeyEnd,
    KeyPrevious,
    KeyNext,
    KeyUp,
    KeyDown,
    KeyTab,
    KeyMenu,
    KeyF1,
    KeyF2,
    KeyF3,
    KeyF4,
    KeyF5,
    KeyF6,
    KeyF7,
    KeyF8,
    KeyF9,
    KeyF10,
    KeyF11,
    KeyF12,
    KeyUnkown, //!< Any other key, not yet mapped.
  };

  /**
   * Flags which describe which modifier key was pressed during a event.
   */
  enum MFORMS_EXPORT ModifierKey {
    ModifierNoModifier = 0,
    ModifierControl = 1 << 0,
    ModifierShift = 1 << 1,
    ModifierCommand = 1 << 2, // Command on Mac, Windows key on Windows.
    ModifierAlt = 1 << 3,
  };

#ifndef SWIG
  inline ModifierKey operator|(ModifierKey a, ModifierKey b) {
    return (ModifierKey)((int)a | (int)b);
  }
#endif

  enum DialogResult { ResultOk = 1, ResultCancel = 0, ResultOther = -1, ResultUnknown = -2 };

  // Describes the type of message, confirmation etc. we want to show to the user.
  enum DialogType {
    DialogMessage,
    DialogError,
    DialogWarning,
    DialogQuery,
    DialogSuccess, // Like DialogMessage but with a special icon to signal a successful operation.
  };

  class Box;
  class Button;

  typedef int TimeoutHandle;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT UtilitiesImplPtrs {
    void (*beep)();
    int (*show_message)(const std::string &title, const std::string &text, const std::string &ok,
                        const std::string &cancel, const std::string &other);
    int (*show_error)(const std::string &title, const std::string &text, const std::string &ok,
                      const std::string &cancel, const std::string &other);
    int (*show_warning)(const std::string &title, const std::string &text, const std::string &ok,
                        const std::string &cancel, const std::string &other);
    int (*show_message_with_checkbox)(
      const std::string &title, const std::string &text, const std::string &ok, const std::string &cancel,
      const std::string &other,
      const std::string &checkbox_text, // empty text = default "Don't show this message again" text
      bool &remember_checked);

    void (*show_wait_message)(const std::string &title, const std::string &text);
    bool (*hide_wait_message)();
    bool (*run_cancelable_wait_message)(const std::string &title, const std::string &text,
                                        const std::function<void()> &start_task,
                                        const std::function<bool()> &cancel_task);
    void (*stop_cancelable_wait_message)();

    void (*set_clipboard_text)(const std::string &text);
    std::string (*get_clipboard_text)();
    std::string (*get_special_folder)(mforms::FolderType type);

    void (*open_url)(const std::string &url);
    void (*reveal_file)(const std::string &url);
    bool (*move_to_trash)(const std::string &path);

    TimeoutHandle (*add_timeout)(float delay, const std::function<bool()> &callback);
    void (*cancel_timeout)(TimeoutHandle handle);

    void (*store_password)(const std::string &service, const std::string &account, const std::string &password);
    bool (*find_password)(const std::string &service, const std::string &account, std::string &password);
    void (*forget_password)(const std::string &service, const std::string &account);

    void *(*perform_from_main_thread)(const std::function<void *()> &slot, bool wait_completion);
    void (*set_thread_name)(const std::string &name);

    double (*get_text_width)(const std::string &text, const std::string &font);
  };
#endif
#endif

  /** Various Utility functions */
  class MFORMS_EXPORT Utilities {
#ifdef SWIG
    %ignore show_message(const std::string &title, const std::string &text, const std::string &ok,
                          const std::string &cancel);
    %ignore show_message(const std::string &title, const std::string &text, const std::string &ok);
    %ignore show_warning(const std::string &title, const std::string &text, const std::string &ok,
                          const std::string &cancel);
    %ignore show_warning(const std::string &title, const std::string &text, const std::string &ok);
    %ignore show_error(const std::string &title, const std::string &text, const std::string &ok,
                        const std::string &cancel);
    %ignore show_error(const std::string &title, const std::string &text, const std::string &ok);
#endif
  public:
    /** Plays the system's default error sound. */
    static void beep(); // TODO: Mac, Linux

    /** Show a message dialog. Return value is from the DialogResult enum.
     * In Python, all arguments are mandatory. */
    static int show_message(const std::string &title, const std::string &text, const std::string &ok,
                            const std::string &cancel = "", const std::string &other = "");

    /** Show an error dialog. Return value is from the DialogResult enum.
     * In Python, all arguments are mandatory. */
    static int show_error(const std::string &title, const std::string &text, const std::string &ok,
                          const std::string &cancel = "", const std::string &other = "");

    /** Show a warning dialog. Return value is from the DialogResult enum.
     * In Python, all arguments are mandatory. */
    static int show_warning(const std::string &title, const std::string &text, const std::string &ok,
                            const std::string &cancel = "", const std::string &other = "");

    /** Show a message dialog and save the answer, if the checkbox is enabled.
     * In Python, all arguments are mandatory. */
    static int show_message_and_remember(const std::string &title, const std::string &text, const std::string &ok,
                                         const std::string &cancel, const std::string &other,
                                         const std::string &answer_id, const std::string &checkbox_text);
    static void forget_message_answers();
    static void set_message_answers_storage_path(const std::string &path);

    static void show_wait_message(const std::string &title, const std::string &text);
    static bool hide_wait_message();

    static bool run_cancelable_task(const std::string &title, const std::string &text,
                                    const std::function<void *()> &task, const std::function<bool()> &cancel_task,
                                    void *&task_result);

    /** Asks the user to enter a string, which is returned to the caller.
     * @param title - the title of the input dialog
     * @param description - description of the value to enter.
     * @param default_value - an initial value for the edit control
     * @param ret_value - the string the user entered, can be empty or the default string too
     *
     * @return true if user presses ok or false if its canceled.
     */
    static bool request_input(const std::string &title, const std::string &description,
                              const std::string &default_value, std::string &ret_value /*out*/);

    /** Prompts the user for a password and whether it should be stored.
     * @param title - the title of the password dialog
     * @param service - the service the password refers to (ie sudo@hostname, Mysql@hostname etc)
     * @param username - the username the password corresponds to, if empty the user will be able to enter it
     * @param ret_password - the password the user typed
     * @param ret_store - true if the user clicks in "Store Password" checkbox
     *
     * @return true if user presses ok or false if its canceled.
     * In Python, ret_password and ret_store are returned as a tuple.
     */
    static bool ask_for_password_check_store(const std::string &title, const std::string &service,
                                             std::string &username /*in/out*/, std::string &ret_password /*out*/,
                                             bool &ret_store /*out*/);

    /** Prompts the user for a password.
     * @param title - the title of the password dialog
     * @param service - the service the password refers to (ie sudo@hostname, Mysql@hostname etc)
     * @param username - the username the password corresponds to
     * @param ret_password - the password the user typed
     *
     * @return true if user presses ok or false if its canceled.
     * If you need the username to be editable by the user, use credentials_for_service()
     * In Python, ret_password is returned by the function.
     */
    static bool ask_for_password(const std::string &title, const std::string &service, const std::string &username,
                                 std::string &ret_password /*out*/);

    /** Tries to find a previously stored password and prompts the user if not found.
     * @param title - the title of the password dialog
     * @param service - the service the password refers to (ie sudo@hostname, Mysql@hostname etc)
     * @param username - the username the password corresponds to, if empty the user will be able to enter it
     * @param reset_password - whether the password should be reset without looking for a stored one
     * @param ret_password - the password the user typed
     *
     * @return true if user presses ok or false if its canceled.
     * In Python, ret_password and ret_store are returned as a tuple.
     */
    static bool find_or_ask_for_password(const std::string &title, const std::string &service,
                                         const std::string &username, bool reset_password,
                                         std::string &ret_password /*out*/) {
      std::string tmp(username);
      return credentials_for_service(title, service, tmp, reset_password, ret_password);
    }

#ifndef SWIG
    /**
     * Function similar to ask_for_password, but it also allows to enter a user name if none is given on call.
     */
    static bool credentials_for_service(const std::string &title, const std::string &service,
                                        std::string &username /*in/out*/, bool reset_password,
                                        std::string &ret_password /*out*/);
#endif

    /** Store the password for the given service and account.
     * @li In Windows, an encrypted Vault file is used.
     * @li In Mac, the secure KeyChain is used.
     * @li In Linux, the gnome-keychain is used, unless it's not available (such as in KDE).
     * In that case, passwords will be forgotten when the application exits.
     */
    static void store_password(const std::string &service, const std::string &account, const std::string &password);

    /** Locates the password for the given service and account.
     * @return true if password was found else false.
     */
    static bool find_password(const std::string &service, const std::string &account, std::string &ret_password);

    /** Locates the password for the given service and account in the in-memory cache only.
     * @return true if password was found else false.
     */
    static bool find_cached_password(const std::string &service, const std::string &account, std::string &ret_password);

    /** Clears the stored password for the given service and account from in-memory cache only
     */
    static void forget_cached_password(const std::string &service, const std::string &account);

    /** Clears the stored password for the given service and account */
    static void forget_password(const std::string &service, const std::string &account);

    /** Sets the given text to the system clipboard */
    static void set_clipboard_text(const std::string &text);
    /** Gets the text stored in the system clipboard */
    static std::string get_clipboard_text();

    /** Gets the path of the requested special folder: documents, desktop etc. */
    static std::string get_special_folder(FolderType type);

    /** Opens the given URL in the default system browser. */
    static void open_url(const std::string &url);

    /** Moves the given file or folder to the trash. The file might be permanently
     deleted instead of being moved to trash, under some circumstances. */
    static bool move_to_trash(const std::string &path);

    /** Shows the given file path in the OS Finder/Explorer */
    static void reveal_file(const std::string &path);

#ifndef SWIG
    /** Sets up a callback to be called after a given interval.

     Interval is in seconds, with fractional values allowed.
     The callback must return true if it wants to be triggered again
     */
    static TimeoutHandle add_timeout(float interval, const std::function<bool()> &callback) WB_UNUSED_RETURN_VALUE;
#endif
    static void cancel_timeout(TimeoutHandle handle);

    /** Convenience function to add an OK and Cancel buttons in a box.

     This function will reorder buttons according to the standard order in the platform
     (ie OK Cancel in Windows and Cancel OK elsewhere).
     */
    static void add_end_ok_cancel_buttons(mforms::Box *box, mforms::Button *ok, mforms::Button *cancel);

#ifndef SWIG
    // Don't wrap this from mforms, because the typeinfo isnt getting shared across
    // modules... uncomment this if that's solved
    static cairo_surface_t *load_icon(const std::string &name, bool allow_hidpi = false);
    static bool is_hidpi_icon(cairo_surface_t *s);
    static bool icon_needs_reload(cairo_surface_t *s);

    static void paint_icon(cairo_t *cr, cairo_surface_t *icon, double x, double y, float alpha = 1.0);
    static base::Size getImageSize(cairo_surface_t *icon);

    static std::string shorten_string(cairo_t *cr, const std::string &text, double width);

    static double get_text_width(const std::string &text, const std::string &font);
#endif

#ifndef SWIG
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _MSC_VER
  public:
    static void enter_modal_loop();
    static void leave_modal_loop();
    static bool in_modal_loop();
#endif
#endif

    static void *perform_from_main_thread(const std::function<void *()> &slot, bool wait_completion = true);
#endif // !SWIG

    static bool in_main_thread();
    static void set_thread_name(const std::string &name);

    // Should be called at the end of python thread, when there was some query involved.
    static void driver_shutdown();

#ifndef SWIG
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    static void add_driver_shutdown_callback(const std::function<void()> &slot);
#endif
#endif

  private:
    static std::function<void()> _driver_shutdown_cb;

    static void save_message_answers();
  };
};
