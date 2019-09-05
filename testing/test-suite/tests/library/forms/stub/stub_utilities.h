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

#ifndef _STUB_UTILITIES_H_
#define _STUB_UTILITIES_H_

#include "stub_mforms.h"

namespace mforms {
  namespace stub {

    class UtilitiesWrapper {
      static std::function<void(std::string)> open_url_slot;
      static std::function<mforms::DialogResult(void)> message_callback;
      static std::map<const std::string, std::string> &passwords();

      static int show_message(const std::string &title, const std::string &text, const std::string &ok,
                              const std::string &cancel, const std::string &other);
      static int show_error(const std::string &title, const std::string &text, const std::string &ok,
                            const std::string &cancel, const std::string &other);
      static int show_warning(const std::string &title, const std::string &text, const std::string &ok,
                              const std::string &cancel, const std::string &other);
      static int show_message_with_checkbox(
        const std::string &title, const std::string &text, const std::string &ok, const std::string &cancel,
        const std::string &other,
        const std::string &checkbox_text, // empty text = default "Don't show this message again" text
        bool &remember_checked);

      static void show_wait_message(const std::string &title, const std::string &text);
      static bool hide_wait_message();
      static bool run_cancelable_wait_message(const std::string &title, const std::string &text,
                                              const std::function<void()> &start_task,
                                              const std::function<bool()> &cancel_task);
      static void stop_cancelable_wait_message();

      static void set_clipboard_text(const std::string &text);
      static std::string get_clipboard_text();

      static std::string get_special_folder(mforms::FolderType type);

      static void open_url(const std::string &url);
      static mforms::TimeoutHandle add_timeout(float interval, const std::function<bool()> &slot);
      static void cancel_timeout(mforms::TimeoutHandle);

      static void store_password(const std::string &service, const std::string &account, const std::string &password);
      static bool find_password(const std::string &service, const std::string &account, std::string &password);
      static void forget_password(const std::string &service, const std::string &account);
      static void *perform_from_main_thread(const std::function<void *()> &slot, bool wait);
      static void beep();
      static void revealFile(const std::string &url);
      static bool moveToTrash(const std::string &path);
      static void setThreadName(const std::string &name);
      static double getTextWidth(const std::string &text, const std::string &font);

    public:
      static void init();
      static void set_message_callback(std::function<mforms::DialogResult(void)> callback);
    };
  };
};

#endif
