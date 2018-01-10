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

#pragma once

#include <mforms/base.h>
#include <mforms/view.h>

#ifdef __APPLE__
#define TOP_FORM_PADDING 20
#else
#define TOP_FORM_PADDING 12
#endif

namespace mforms {

  enum FormFlag {
    FormNone = 0,
    FormSingleFrame = 1 << 0,
    FormDialogFrame = 1 << 1,
    FormResizable = 1 << 2,
    FormMinimizable = 1 << 3,
    FormHideOnClose = 1 << 4,
    FormStayOnTop = 1 << 5,
    FormToolWindow = 1 << 6, // Not combinable with any of the frame styles above. Also min and max
                             // are not available.
    FormNormal = (FormResizable | FormMinimizable)
  };

  class Form;
  class Button;
  class MenuBar;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT FormImplPtrs {
    bool (*create)(Form *self, Form *owner, FormFlag flag);
    void (*set_title)(Form *self, const std::string &title);
    void (*set_menubar)(Form *self, MenuBar *menu);
    void (*show_modal)(Form *self, Button *accept, Button *cancel);
    bool (*run_modal)(Form *self, Button *accept, Button *cancel);
    void (*end_modal)(Form *self, bool result);
    void (*set_content)(Form *self, View *view);
    void (*close)(Form *self);

    void (*center)(Form *self);

    void (*flush_events)(Form *self);
  };
#endif
#endif

  /** A standalone, top-level window.
   */
  class MFORMS_EXPORT Form : public View {
    FormImplPtrs *_form_impl;
    View *_content;
    MenuBar *_menu;
    bool _fixed_size;
    bool _release_on_close;
    bool _active;

    std::function<bool()> _can_close_slot;
    boost::signals2::signal<void()> _closed_signal;
    boost::signals2::signal<void()> _activated_signal;
    boost::signals2::signal<void()> _deactivated_signal;

  protected:
    Form();

  public:
    /** Constructor.

     @param owner - the owner of this window. Pass 0/None for no owner.
     @param flag - flags specifying some features of the window:
       FormNone
       FormSingleFrame
       FormDialogFrame
       FormResizable
       FormMinimizable
       FormHideOnClose
       FormStayOnTop
     */
    Form(Form *owner, FormFlag flag = (FormFlag)(FormResizable | FormMinimizable));
    virtual ~Form();

    /** Used to specify the main form to the Form() constructor. The returned object cannot be used for anything else */
    static Form *main_form();

    static Form *active_form();

    /** Sets the content view of the window.

     Usually a layouting container, such as a Box or a Table.
     */
    virtual void set_content(View *view);

    View *get_content() {
      return _content;
    }

    /** Sets the title of the window.
     */
    virtual void set_title(const std::string &title);

    /** Sets the menubar for the window
        In Linux and Windows, this will add to the toplevel box of the window,
        which is expected to have the correct layout.
        In MacOS, it will make the menu the active menu, whenever the window becomes key.
    */
    virtual void set_menubar(mforms::MenuBar *menu);

    mforms::MenuBar *get_menubar() {
      return _menu;
    }

    /** Deprecated */
    virtual void show_modal(Button *accept, Button *cancel);

    /** Shows the window as a modal window.

     @param accept - a button to be used as the default accept button (ie when user presses Return key) or None
     @param cancel - button to be used as cancel button (ie when user presses Escape key) or None
     */
    virtual bool run_modal(Button *accept, Button *cancel);

    /** Ends a modal session started with run_modal()

     This function is automatically called if accept and cancel buttons are given to run_modal.

     @param result - the result to be returned by run_modal
     */
    virtual void end_modal(bool result);

    /** Closes the window. */
    virtual void close();

    /** Centers the window in the screen. */
    virtual void center();

    virtual void flush_events();

    /** Sets whether the window must be automatically released when closed. */
    void set_release_on_close(bool flag);

#ifndef SWIG

    /** Function called when window is about to close.
        return false to prevent window closing.

     In Python use on_close()
     */
    void set_on_close(const std::function<bool()> &slot) {
      _can_close_slot = slot;
    }

    /** Signal sent when the user clicks the close button in the window.

     In Python use add_closed_callback()
     */
    boost::signals2::signal<void()> *signal_closed() {
      return &_closed_signal;
    }

    /** Signal sent when the window becomes the active window.
     */
    boost::signals2::signal<void()> *signal_activated() {
      return &_activated_signal;
    }
    boost::signals2::signal<void()> *signal_deactivated() {
      return &_deactivated_signal;
    }

    void activated();
    void deactivated();
#endif

    bool is_active();
    bool can_close();

    void was_closed() {
      _closed_signal();
      if (_release_on_close)
        release();
    }
  };
};
