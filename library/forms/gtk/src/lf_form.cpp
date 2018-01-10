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

#include "../lf_mforms.h"
#include "../lf_form.h"
#include "../lf_menubar.h"
#include "../lf_wizard.h"

static GThread *_main_thread = 0;

namespace mforms {
  namespace gtk {

    bool FormImpl::create(::mforms::Form *self, ::mforms::Form *owner, mforms::FormFlag flag) {
      return new FormImpl(self, owner, flag) != 0;
    }

    void FormImpl::set_title(::mforms::Form *self, const std::string &title) {
      FormImpl *form = self->get_data<FormImpl>();
      if (form) {
        form->set_title(title);
      }
    }

    void FormImpl::set_title(const std::string &title) {
      _window->set_title(title);
    }

    void FormImpl::accept_clicked(bool *status, const bool is_run) {
      *status = true;
      if (is_run)
        _loop.quit();
      else
        _window->hide();

      accept_c.disconnect();
      cancel_c.disconnect();
    }

    void FormImpl::cancel_clicked(bool *status, const bool is_run) {
      *status = false;
      if (is_run)
        _loop.quit();
      else
        _window->hide();

      accept_c.disconnect();
      cancel_c.disconnect();
    }

    bool FormImpl::on_widget_delete_event(GdkEventAny *event, Button *cancel) {
      mforms::Form *form = dynamic_cast<mforms::Form *>(owner); // ObjectImpl::owner
      if (form) {
        form->end_modal(false);
        _window->hide();
        form->was_closed();
      }

      return false;
    }

    void FormImpl::end_modal(::mforms::Form *self, bool result) {
      FormImpl *form = self->get_data<FormImpl>();
      if (form) {
        form->_result = result;
        if (form->_in_modal_loop > 0) {
          form->_loop.quit();
          form->_in_modal_loop--;
        }
      }
    }

    void FormImpl::show_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel) {
      FormImpl *form = self->get_data<FormImpl>();
      if (form) {
        form->_window->signal_delete_event().connect(
          sigc::bind(sigc::mem_fun(form, &FormImpl::on_widget_delete_event), cancel));
        if (get_mainwindow())
          form->_window->set_transient_for(*get_mainwindow());
        form->_window->set_modal(false);

        bool status = false;

        if (accept)
          form->accept_c = accept->signal_clicked()->connect(
            sigc::bind(sigc::mem_fun(form, &FormImpl::accept_clicked), &status, false));
        if (cancel)
          form->cancel_c = cancel->signal_clicked()->connect(
            sigc::bind(sigc::mem_fun(form, &FormImpl::cancel_clicked), &status, false));

        form->_window->signal_key_release_event().connect(
          sigc::bind(sigc::mem_fun(form, &FormImpl::on_key_release), &status, false, accept, cancel));

        form->_window->show();
      }
    }

    bool FormImpl::on_key_release(GdkEventKey *event, bool *status, const bool is_run, ::mforms::Button *accept,
                                  ::mforms::Button *cancel) {
      if (event->keyval == GDK_KEY_Escape) {
        *status = false;
        cancel_clicked(status, is_run);
        if (cancel)
          cancel->callback();
      }
      return false;
    }

    bool FormImpl::run_modal(::mforms::Form *self, ::mforms::Button *accept, ::mforms::Button *cancel) {
      if (g_thread_self() != _main_thread)
        g_warning("mforms::Form::run_modal() called in non-main thread, which is invalid");

      FormImpl *form = self->get_data<FormImpl>();
      if (form) {
        form->_window->signal_delete_event().connect(
          sigc::bind(sigc::mem_fun(form, &FormImpl::on_widget_delete_event), cancel));
        if (get_mainwindow())
          form->_window->set_transient_for(*get_mainwindow());
        form->_window->set_modal(true);
        form->_window->show();

        form->_result = false;
        if (accept)
          form->accept_c = accept->signal_clicked()->connect(
            sigc::bind(sigc::mem_fun(form, &FormImpl::accept_clicked), &form->_result, true));
        if (cancel)
          form->cancel_c = cancel->signal_clicked()->connect(
            sigc::bind(sigc::mem_fun(form, &FormImpl::cancel_clicked), &form->_result, true));

        form->_window->signal_key_release_event().connect(
          sigc::bind(sigc::mem_fun(form, &FormImpl::on_key_release), &form->_result, true, accept, cancel));

        form->_in_modal_loop++;
        form->_loop.run();

        form->_window->set_modal(false); // to be sure it wont Quit the app on close()

        form->_window->hide();

        form->accept_c.disconnect();
        form->cancel_c.disconnect();

        return form->_result;
      }
      return false;
    }

    void FormImpl::close(::mforms::Form *self) {
      FormImpl *form = self->get_data<FormImpl>();
      if (form) {
        form->_window->hide();
        if (form->_in_modal_loop > 0) {
          form->_loop.quit();
          form->_in_modal_loop--;
        }
      }
    }

    void FormImpl::set_content(::mforms::Form *self, ::mforms::View *child) {
      FormImpl *form = self->get_data<FormImpl>();
      if (form) {
        form->_window->add(*child->get_data<ViewImpl>()->get_outer());
        child->show();
      }
    }

    void FormImpl::flush_events(::mforms::Form *self) {
      while (Gtk::Main::events_pending())
        Gtk::Main::iteration();
    }

    void FormImpl::center(Form *self) {
      FormImpl *form = self->get_data<FormImpl>();
      form->_window->set_position(Gtk::WIN_POS_CENTER);
    }

    void FormImpl::set_menubar(mforms::Form *self, mforms::MenuBar *menu) {
      FormImpl *form = self->get_data<FormImpl>();
      Gtk::MenuBar *mbar = widget_for_menubar(menu);
      if (form && mbar) {
        Gtk::Box *box = dynamic_cast<Gtk::Box *>(self->get_content()->get_data<ViewImpl>()->get_inner());
        if (!box)
          throw std::logic_error("set_menubar called on a window without a Box as toplevel content");
        box->pack_start(*mbar, false, true);
        box->reorder_child(*mbar, 0);

        on_add_menubar_to_window(menu, form->_window);
      }
    }

    FormImpl::FormImpl(::mforms::Form *form, ::mforms::Form *owner, mforms::FormFlag form_flag)
      : ViewImpl(form), _in_modal_loop(0), _result(false) {
      _window = new Gtk::Window(Gtk::WINDOW_TOPLEVEL);

      if (owner) {
        if (dynamic_cast<mforms::Wizard *>(owner)) {
          WizardImpl *impl = owner->get_data<WizardImpl>();
          if (impl) {
            Gtk::Window *w = impl->get_window();
            if (w)
              _window->set_transient_for(*w);
          }
        } else {
          FormImpl *impl = owner->get_data<FormImpl>();
          if (impl) {
            Gtk::Window *w = impl->get_window();
            if (w)
              _window->set_transient_for(*w);
          }
        }
      }

      _window->set_position(Gtk::WIN_POS_CENTER);

      Gdk::WMDecoration flags = Gdk::DECOR_ALL; // this will make other flags be interpreted inverted

      if (form_flag & mforms::FormResizable)
        flags |= Gdk::DECOR_RESIZEH;
      if (form_flag & mforms::FormMinimizable)
        flags |= Gdk::DECOR_MINIMIZE;

      _window->set_events(Gdk::FOCUS_CHANGE_MASK);
      _window->signal_realize().connect(sigc::bind(sigc::mem_fun(this, &FormImpl::realized), form, flags));

      _window->signal_focus_in_event().connect(
        sigc::bind< ::mforms::Form *>(sigc::mem_fun(this, &FormImpl::on_focus_event), form));
      _window->signal_focus_out_event().connect(
        sigc::bind< ::mforms::Form *>(sigc::mem_fun(this, &FormImpl::on_focus_event), form));
      _window->signal_delete_event().connect(sigc::mem_fun(this, &FormImpl::can_delete_widget), false);
    }

    bool FormImpl::can_delete_widget(GdkEventAny *event) {
      mforms::Form *form = dynamic_cast<mforms::Form *>(owner);
      if (form)
        return !form->can_close();

      return false;
    }

    bool FormImpl::on_focus_event(GdkEventFocus *ev, ::mforms::Form *form) {
      if (ev->in)
        form->activated();
      else
        form->deactivated();
      return false;
    }

    void FormImpl::realized(mforms::Form *owner, Gdk::WMDecoration flags) {
      owner->relayout();
      _window->get_window()->set_decorations(flags);
    }

    void FormImpl::set_name(const std::string &name) {
      _window->set_role(name);
    }

    void FormImpl::show(bool show) {
      if (show) {
        _window->signal_delete_event().connect(
          sigc::bind(sigc::mem_fun(this, &FormImpl::on_widget_delete_event), nullptr));
        _window->show();
      } else
        _window->hide();
    }

    void FormImpl::init_main_form(Gtk::Window *main) {
      mforms::Form *the_main_form = mforms::Form::main_form();
      if (the_main_form) {
        static FormImpl *form = new FormImpl(the_main_form, (mforms::Form *)0, mforms::FormNone);
        form->_window = main;
      }
    }

    void FormImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      _main_thread = g_thread_self();

      f->_form_impl.create = &FormImpl::create;
      f->_form_impl.close = &FormImpl::close;
      f->_form_impl.set_title = &FormImpl::set_title;
      f->_form_impl.show_modal = &FormImpl::show_modal;
      f->_form_impl.run_modal = &FormImpl::run_modal;
      f->_form_impl.end_modal = &FormImpl::end_modal;
      f->_form_impl.set_content = &FormImpl::set_content;
      f->_form_impl.flush_events = &FormImpl::flush_events;
      f->_form_impl.center = &FormImpl::center;
      f->_form_impl.set_menubar = &FormImpl::set_menubar;
    }
  };
};
