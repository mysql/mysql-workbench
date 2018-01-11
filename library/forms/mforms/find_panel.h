/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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
