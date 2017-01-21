/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SNIPPET_POPOVER_H_
#define _SNIPPET_POPOVER_H_

#include "mforms/popover.h"
#include "mforms/drawbox.h"

/**
 * Implementation of a "fly out" control providing extended display for snippet entries that allows
 * to edit them.
 */

namespace mforms {
  class CodeEditor;
  class Label;
  class TextEntry;
  class Button;
  class Box;
}

namespace wb {

  class Separator : public mforms::DrawBox {
  public:
    virtual base::Size getLayoutSize(base::Size proposedSize) override;
    virtual void repaint(cairo_t* cr, int x, int y, int w, int h) override;
  };

  class SnippetPopover : public mforms::Popover {
  private:
    mforms::Box* _content;
    mforms::Box* _header;
    mforms::CodeEditor* _editor;
    mforms::Label* _heading_label;     // Non-editable heading text.
    mforms::TextEntry* _heading_entry; // Editable heading text.
    mforms::Button* _edit_button;
    mforms::Button* _close_button;
    mforms::Button* _revert_button;

    std::string _original_text;
    std::string _original_heading;

    boost::signals2::signal<void()> _closed;

  protected:
    void revert_clicked();
    void edit_clicked();
    void close_clicked();
    void text_changed(int start_line, int lines_changed);

  public:
    SnippetPopover();
    ~SnippetPopover();

    void set_heading(const std::string& text);
    void set_text(const std::string& text);
    void set_read_only(bool flag);

    bool has_changed();
    std::string get_text();
    std::string get_heading();

    boost::signals2::signal<void()>* signal_closed() {
      return &_closed;
    }
  };

} // namespace wb

#endif // _SNIPPET_POPOVER_H_
