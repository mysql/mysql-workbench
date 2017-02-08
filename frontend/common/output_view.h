/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _OUTPUT_VIEW_H_
#define _OUTPUT_VIEW_H_

#include "grt/grt_message_list.h"

#include "mforms/treeview.h"
#include "mforms/splitter.h"
#include "mforms/menubar.h"
#include "mforms/appview.h"
#include "mforms/textbox.h"

namespace wb {
  class WBContext;
}

class OutputView : public mforms::AppView {
public:
  OutputView(wb::WBContext* context);
  virtual ~OutputView();

  bec::MessageListBE* get_be() {
    return _messages;
  }
  void setup_ui();

private:
  wb::WBContext* _wb;
  bec::MessageListStorage* _storage;
  bec::MessageListBE* _messages;

  mforms::Splitter _splitter;
  mforms::TreeView _message_list;
  mforms::TextBox _output_text;
  mforms::ContextMenu _context_menu;
  bool _can_track_changes;

  void handle_command(const std::string& command);

  void refresh();
  void row_added();
  bool will_close();
  void splitter_moved();
};

#endif // _OUTPUT_VIEW_H_
