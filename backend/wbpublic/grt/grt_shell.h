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

#include "grt.h"
#include "grtpp_shell.h"

#include "grt_dispatcher.h"
#include "wbpublic_public_interface.h"

#define ShellBE_VERSION 4

namespace bec {
  class GRTManager;

  class WBPUBLICBACKEND_PUBLIC_FUNC ShellBE {
  public:
    ShellBE(const GRTDispatcher::Ref dispatcher);
    ~ShellBE();

    bool setup(const std::string &lang);

    void set_save_directory(const std::string &path);
    void start();

    void process_line_async(const std::string &line);

    void run_script_file(const std::string &path);
    bool run_script(const std::string &script, const std::string &language);

    bool previous_history_line(const std::string &current_line, std::string &line);
    bool next_history_line(std::string &line);
    void reset_history_position();

    std::vector<std::string> get_grt_tree_bookmarks();
    void add_grt_tree_bookmark(const std::string &path);
    void delete_grt_tree_bookmark(const std::string &path);

    void write_line(const std::string &line);
    void write(const std::string &text);
    void writef(const char *fmt, ...);

    void set_output_handler(const std::function<void(const std::string &)> &slot);
    void set_ready_handler(const std::function<void(const std::string &)> &slot);

    void flush_shell_output();

    void set_saves_history(int line_count);

    std::vector<std::string> complete_line(const std::string &line, std::string &nprefix);

    grt::ValueRef get_shell_variable(const std::string &varname);

    void clear_history();
    void save_history_line(const std::string &line);

    std::string get_snippet_data();
    void set_snippet_data(const std::string &data);

    void store_state();
    void restore_state();

    void handle_msg(const grt::Message &msgs);

  protected:
    grt::Shell *_shell;
    GRTDispatcher::Ref _dispatcher;
    std::vector<std::string> _grt_tree_bookmarks;

    std::string _savedata_dir;

    std::string _current_statement;

    std::list<std::string> _history; // most recent first
    std::list<std::string>::iterator _history_ptr;

    std::function<void(const std::string &)> _ready_slot;

    std::function<void(const std::string &)> _output_slot;

    base::Mutex _text_queue_mutex;

    std::list<std::string> _text_queue;

    int _save_history_size;
    int _skip_history;

  private:
    void shell_finished_cb(grt::ShellCommand result, const std::string &prompt, const std::string &line);
  };
};
