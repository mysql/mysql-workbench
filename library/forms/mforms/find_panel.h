/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "mforms/view.h"

namespace mforms {
  class CodeEditor;

  enum FindPanelAction {
    FindNext,       // Select next occurrence after current selection or after current caret position
                    // (if there is no selection).
    FindPrevious,   // Same as FindNext, but backwards.
    FindAndReplace, // Replace next occurrence after current selection or after current caret position.
    ReplaceAll,     // Replace all occurrences of the search text.
  };

  class FindPanel;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct FindPanelImplPtrs {
    bool (*create)(FindPanel *fp);
    size_t (*perform_action)(FindPanel *fp, FindPanelAction action);
    void (*focus)(FindPanel *fp);
    void (*enable_replace)(FindPanel *fp, bool);
  };
#endif
#endif

  // Implementation is in platform specific code
  class MFORMS_EXPORT FindPanel : public View {
  protected:
    FindPanelImplPtrs *_find_impl;
    CodeEditor *_editor;

  public:
    FindPanel(CodeEditor *editor);

    CodeEditor *get_editor() {
      return _editor;
    }

    /** Perform the action and return the number of items affected */
    size_t perform_action(FindPanelAction action);

    virtual void focus();
    void enable_replace(bool flag);
  };
};
