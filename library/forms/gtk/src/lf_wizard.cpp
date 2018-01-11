/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_wizard.h"
#include "text_list_columns_model.h"
#include "gtk_helpers.h"
#include "../lf_view.h"
#include "base/string_utilities.h"

#define DEFAULT_WIDTH 700
#define DEFAULT_HEIGHT 500

namespace mforms {
  namespace gtk {

    static std::string icon_path;

    static void setup_padded_button(Gtk::Button *button, Gtk::Label *label, Gtk::Image *image) {
      if (image) {
        Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
        button->add(*hbox);
        hbox->pack_start(*image, false, true);
        hbox->pack_start(*label, true, true);
        button->set_margin_left(6);
        button->set_margin_right(8);
      } else if (label) {
        button->add(*label);
        label->set_margin_left(6);
        label->set_margin_right(8);
      }

      button->show_all();
    }

    //------------------------------------------------------------------------------
    WizardImpl::WizardImpl(::mforms::Wizard *wiz, ::mforms::Form *owner)
      : FormImpl(wiz, owner, mforms::FormDialogFrame),
        _top_table(3, 2),
        _button_box(Gtk::ORIENTATION_HORIZONTAL),
        _cancel_btn(),
        _back_btn() {
      get_window()->add(_top_table);

      setup_padded_button(&_fwd_btn, &_fwd_label,
                          Gtk::manage(new Gtk::Image(Gtk::Stock::GO_FORWARD, Gtk::ICON_SIZE_BUTTON)));
      setup_padded_button(&_back_btn, Gtk::manage(new Gtk::Label(_("_Back"), true)),
                          Gtk::manage(new Gtk::Image(Gtk::Stock::GO_BACK, Gtk::ICON_SIZE_BUTTON)));
      setup_padded_button(&_cancel_btn, Gtk::manage(new Gtk::Label(_("_Cancel"), true)),
                          Gtk::manage(new Gtk::Image(Gtk::Stock::CANCEL, Gtk::ICON_SIZE_BUTTON)));
      setup_padded_button(&_extra_btn, &_extra_label, 0);
      _extra_label.set_use_markup(true);

      _heading.set_halign(Gtk::ALIGN_START);
      _heading.set_valign(Gtk::ALIGN_CENTER);
      Gtk::Box *holder = Gtk::manage(new Gtk::Box());
      holder->pack_start(_heading, true, true);
      holder->set_border_width(12);

      _top_table.attach(*holder, 1, 2, 0, 1, Gtk::FILL, Gtk::FILL);
      _top_table.attach(*Gtk::manage(new Gtk::HSeparator()), 0, 2, 2, 3, Gtk::FILL, Gtk::FILL);
      _top_table.attach(_button_box, 0, 2, 3, 4, Gtk::FILL, Gtk::FILL);

      _button_box.set_border_width(12);
      _button_box.set_spacing(12);
      _top_table.set_border_width(0);
      _top_table.set_row_spacings(0);
      _top_table.set_col_spacings(0);
      //_content.set_border_width(12);

      _button_box.pack_start(_extra_btn, false, true);

      _button_box.pack_end(_fwd_btn, false, true);
      _button_box.pack_end(_back_btn, false, true);
      _button_box.pack_end(_cancel_btn, false, true);

      _content.set_shadow_type(Gtk::SHADOW_NONE);

      get_window()->signal_delete_event().connect(sigc::bind(sigc::ptr_fun(&WizardImpl::delete_event), wiz));

      _cancel_btn.signal_clicked().connect(sigc::bind(sigc::ptr_fun(&WizardImpl::cancel), wiz));
      _fwd_btn.signal_clicked().connect(sigc::mem_fun(wiz, &::mforms::Wizard::next_clicked));
      _back_btn.signal_clicked().connect(sigc::mem_fun(wiz, &::mforms::Wizard::back_clicked));
      _extra_btn.signal_clicked().connect(sigc::mem_fun(wiz, &::mforms::Wizard::extra_clicked));

      _extra_label.set_use_underline(true);
      _fwd_label.set_use_underline(true);

      _step_table.set_border_width(12);
      _step_table.set_row_spacings(8);

      _step_background.add(_step_table);

      Gdk::RGBA c("#ffffff");
      _step_background.override_background_color(c, Gtk::STATE_FLAG_NORMAL);

      _top_table.attach(_step_background, 0, 1, 0, 2, Gtk::FILL, Gtk::FILL);
      _top_table.attach(_content, 1, 2, 1, 2, Gtk::EXPAND | Gtk::FILL, Gtk::EXPAND | Gtk::FILL);

      get_window()->set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
      get_window()->set_position(Gtk::WIN_POS_CENTER);
      _top_table.show_all();

      _fwd_btn.set_use_underline(true);
      _fwd_label.set_use_underline(true);
      _extra_btn.hide();

      get_window()->set_size_request(-1, 650);

      if (owner) {
        FormImpl *form = owner->get_data<FormImpl>();
        if (form && form->get_window())
          get_window()->set_transient_for(*form->get_window());
      }
    }

    //------------------------------------------------------------------------------
    void WizardImpl::cancel(::mforms::Wizard *wiz) {
      WizardImpl *wiz_impl = wiz->get_data<WizardImpl>();

      if (wiz->_cancel_slot()) {
        wiz_impl->get_window()->hide();
        wiz_impl->_loop.quit();
      }
    }

    //------------------------------------------------------------------------------
    bool WizardImpl::delete_event(GdkEventAny *ev, ::mforms::Wizard *wiz) {
      WizardImpl *wiz_impl = wiz->get_data<WizardImpl>();

      wiz->_cancel_slot();
      wiz_impl->get_window()->hide();
      wiz_impl->_loop.quit();
      return true;
    }

    //------------------------------------------------------------------------------
    bool WizardImpl::create(::mforms::Wizard *self, ::mforms::Form *owner) {
      return new WizardImpl(self, owner);
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_title(::mforms::Wizard *self, const std::string &title) {
      WizardImpl *wiz = self->get_data<WizardImpl>();
      if (wiz) {
        wiz->FormImpl::set_title(title);
      }
    }
    //------------------------------------------------------------------------------
    void WizardImpl::run_modal(::mforms::Wizard *self) {
      WizardImpl *wiz = self->get_data<WizardImpl>();
      if (wiz) {
        Gtk::Window *_wnd = wiz->get_window();
        if (_wnd) {
          _wnd->set_modal(true);
          _wnd->show();
          if (get_mainwindow() != nullptr)
            _wnd->set_transient_for(*get_mainwindow());

          wiz->_loop.run();
          _wnd->set_modal(false);
        }
      }
    }

    //------------------------------------------------------------------------------
    void WizardImpl::close(::mforms::Wizard *self) {
      WizardImpl *wiz = self->get_data<WizardImpl>();
      if (wiz) {
        Gtk::Window *_wnd = wiz->get_window();
        if (_wnd)
          _wnd->hide();
      }
      wiz->_loop.quit();
    }

    //------------------------------------------------------------------------------
    void WizardImpl::flush_events(::mforms::Wizard *self) {
      while (Gtk::Main::events_pending())
        Gtk::Main::iteration();
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_content(::mforms::Wizard *self, View *view) {
      WizardImpl *wiz = self->get_data<WizardImpl>();

      wiz->_content.remove();
      if (view) {
        wiz->_content.add(*view->get_data<ViewImpl>()->get_outer());
        view->show();
      }
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_heading(::mforms::Wizard *self, const std::string &heading) {
      WizardImpl *wiz = self->get_data<WizardImpl>();

      wiz->_heading.set_markup("<b>" + heading + "</b>");
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_step_list(::mforms::Wizard *self, const std::vector<std::string> &steps) {
      WizardImpl *wiz = self->get_data<WizardImpl>();

      wiz->refresh_step_list(steps);
    }

    //------------------------------------------------------------------------------
    void WizardImpl::refresh_step_list(const std::vector<std::string> &steps) {
      int row = 0;

      for (std::vector<std::string>::const_iterator iter = steps.begin(); iter != steps.end(); ++iter) {
        Gtk::Image *image;
        if (row < (int)_steps.size()) {
          _steps[row].second->set_text(iter->substr(1));
          image = _steps[row].first;
        } else {
          Gtk::Label *label = Gtk::manage(new Gtk::Label(iter->substr(1), 0.0, 0.5));
          image = Gtk::manage(new Gtk::Image());

          _step_table.attach(*image, 0, 1, row, row + 1, Gtk::FILL, Gtk::FILL);
          _step_table.attach(*label, 1, 2, row, row + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL);

          _steps.push_back(ImageLabel(image, label));
        }
        switch ((*iter)[0]) {
          case '*':
            image->set(icon_path + "/DotBlue.png");
            break;
          case '.':
            image->set(icon_path + "/DotGrey.png");
            break;
          case '-':
            image->set(icon_path + "/DotDisabled.png");
            break;
        }
        row++;
      }

      _step_table.show_all();
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_icon_path(const std::string &path) {
      icon_path = path;
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_allow_cancel(::mforms::Wizard *self, bool flag) {
      WizardImpl *wiz = self->get_data<WizardImpl>();
      wiz->_cancel_btn.set_sensitive(flag);
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_allow_back(::mforms::Wizard *self, bool flag) {
      WizardImpl *wiz = self->get_data<WizardImpl>();
      wiz->_back_btn.set_sensitive(flag);
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_allow_next(::mforms::Wizard *self, bool flag) {
      WizardImpl *wiz = self->get_data<WizardImpl>();
      wiz->_fwd_btn.set_sensitive(flag);
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_show_extra(::mforms::Wizard *self, bool flag) {
      WizardImpl *wiz = self->get_data<WizardImpl>();

      if (flag)
        wiz->_extra_btn.show();
      else
        wiz->_extra_btn.hide();
      wiz->_extra_btn.set_sensitive(flag);
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_extra_caption(::mforms::Wizard *self, const std::string &caption) {
      WizardImpl *wiz = self->get_data<WizardImpl>();

      wiz->_extra_label.set_text(caption);
      wiz->_extra_label.set_markup(caption);
    }

    //------------------------------------------------------------------------------
    void WizardImpl::set_next_caption(::mforms::Wizard *self, const std::string &caption) {
      WizardImpl *wiz = self->get_data<WizardImpl>();

      if (caption.empty())
        wiz->_fwd_label.set_markup_with_mnemonic(_("_Next"));
      else
        wiz->_fwd_label.set_markup_with_mnemonic(caption);
    }

    //------------------------------------------------------------------------------
    void WizardImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_wizard_impl.create = &WizardImpl::create;
      f->_wizard_impl.set_title = &WizardImpl::set_title;
      f->_wizard_impl.run_modal = &WizardImpl::run_modal;
      f->_wizard_impl.close = &WizardImpl::close;

      f->_wizard_impl.set_content = &WizardImpl::set_content;
      f->_wizard_impl.set_heading = &WizardImpl::set_heading;
      f->_wizard_impl.set_step_list = &WizardImpl::set_step_list;
      f->_wizard_impl.set_allow_cancel = &WizardImpl::set_allow_cancel;
      f->_wizard_impl.set_allow_back = &WizardImpl::set_allow_back;
      f->_wizard_impl.set_allow_next = &WizardImpl::set_allow_next;
      f->_wizard_impl.set_show_extra = &WizardImpl::set_show_extra;

      f->_wizard_impl.set_extra_caption = &WizardImpl::set_extra_caption;
      f->_wizard_impl.set_next_caption = &WizardImpl::set_next_caption;
    }

  } // end of gtk namespace
} // end of mforms namespace
