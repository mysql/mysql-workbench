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

#include "gtk/lf_mforms.h"
#include "gtk/lf_view.h"
#include "mforms/find_panel.h"
#include "mforms/code_editor.h"
#include "mforms/app.h"
#include "base/string_utilities.h"
#include <gtkmm.h>
#include <gtkmm/menushell.h>

using namespace mforms;

#if GTK_VERSION_GT(2, 16)
static void clear_text_clicked(Gtk::EntryIconPosition pos, const GdkEventButton *, Gtk::Entry *entry) {
  if (pos == Gtk::ENTRY_ICON_SECONDARY)
    entry->set_text("");
}

static void text_changed(Gtk::Entry *entry) {
  if (!entry->get_text().empty())
    entry->set_icon_from_stock(Gtk::Stock::CLEAR, Gtk::ENTRY_ICON_SECONDARY);
  else
    entry->set_icon_from_pixbuf(Glib::RefPtr<Gdk::Pixbuf>(), Gtk::ENTRY_ICON_SECONDARY);
}
#endif

static void toggle_bool(bool &b, Gtk::CheckMenuItem *item) {
  b = item->get_active();
}

class FindPanelImpl : public mforms::gtk::ViewImpl {
  Glib::RefPtr<Gtk::Builder> _find_panel;
  Gtk::Container *_container;

  Gtk::Entry *_find_entry;
  Gtk::Entry *_replace_entry;
  Gtk::Label *_find_status;
  Gtk::Menu *_search_menu;
  Gtk::RadioButton *_find_radio_button;
  Gtk::RadioButton *_replace_radio_button;
  Gtk::Box *_replace_box;
  Gtk::Label *_replace_filler_label;
  bool _search_match_whole_word;
  bool _search_ignore_case;
  bool _search_wrap_around;
  bool _use_regex;

public:
  FindPanelImpl(FindPanel *owner) : mforms::gtk::ViewImpl(owner) {
    std::string path = App::get()->get_resource_path("embedded_find.glade");

    _find_panel = Gtk::Builder::create_from_file(path);

    _find_panel->get_widget("container", _container);
    _container->reference();
    _container->unparent();
    _container->hide();
    _container->show();

    _search_match_whole_word = false;
    _search_ignore_case = true;
    _search_wrap_around = true;
    _use_regex = false;

    Gtk::Button *btn;
    _find_panel->get_widget("close_button", btn);
    btn->signal_clicked().connect(sigc::mem_fun(owner->get_editor(), &CodeEditor::hide_find_panel));

    _find_panel->get_widget("result_label", _find_status);

    _find_panel->get_widget("find_radio", _find_radio_button);
    _find_radio_button->signal_clicked().connect(sigc::bind(&FindPanelImpl::find_clicked, this));
    _find_panel->get_widget("replace_radio", _replace_radio_button);
    _replace_radio_button->signal_clicked().connect(sigc::bind(&FindPanelImpl::replace_clicked, this));

    _find_panel->get_widget("replace_all_button", btn);
    btn->signal_clicked().connect(
      sigc::hide_return(sigc::bind(sigc::mem_fun(this, &FindPanelImpl::perform_action), ReplaceAll)));
    _find_panel->get_widget("find_replace_button", btn);
    btn->signal_clicked().connect(
      sigc::hide_return(sigc::bind(sigc::mem_fun(this, &FindPanelImpl::perform_action), FindAndReplace)));
    _find_panel->get_widget("next_button", btn);
    btn->signal_clicked().connect(
      sigc::hide_return(sigc::bind(sigc::mem_fun(this, &FindPanelImpl::perform_action), FindNext)));
    _find_panel->get_widget("previous_button", btn);
    btn->signal_clicked().connect(
      sigc::hide_return(sigc::bind(sigc::mem_fun(this, &FindPanelImpl::perform_action), FindPrevious)));

    _find_panel->get_widget("search_menu", _search_menu);

    _find_panel->get_widget("find_entry", _find_entry);
    _find_entry->signal_activate().connect(
      sigc::hide_return(sigc::bind(sigc::mem_fun(this, &FindPanelImpl::perform_action), FindNext)));
    _find_entry->signal_key_press_event().connect(sigc::mem_fun(this, &FindPanelImpl::on_find_key_press));
    _find_entry->signal_changed().connect(sigc::mem_fun(this, &FindPanelImpl::find_text_changed));
#if GTK_VERSION_GT(2, 16)
    _find_entry->signal_changed().connect(sigc::bind(sigc::ptr_fun(text_changed), _find_entry));
#endif
    _find_panel->get_widget("replace_entry", _replace_entry);
    _replace_entry->signal_activate().connect(
      sigc::hide_return(sigc::bind(sigc::mem_fun(this, &FindPanelImpl::perform_action), FindAndReplace)));
    _replace_entry->signal_key_press_event().connect(sigc::mem_fun(this, &FindPanelImpl::on_find_key_press));
#if GTK_VERSION_GT(2, 16)
    _replace_entry->signal_changed().connect(sigc::bind(sigc::ptr_fun(text_changed), _replace_entry));
#endif

#if GTK_VERSION_GT(2, 16)
    _find_entry->signal_icon_press().connect(sigc::bind(sigc::ptr_fun(clear_text_clicked), _find_entry));
    _find_entry->signal_icon_press().connect(sigc::mem_fun(this, &FindPanelImpl::find_icon_press));
    _replace_entry->signal_icon_press().connect(sigc::bind(sigc::ptr_fun(clear_text_clicked), _replace_entry));
#endif

    Gtk::MenuItem *mitem;
    _find_panel->get_widget("clear_item", mitem);
    mitem->signal_activate().connect(sigc::mem_fun(this, &FindPanelImpl::clear_search_history));
    mitem->set_sensitive(false);
    Gtk::CheckMenuItem *citem;
    _find_panel->get_widget("wrap_item", citem);
    citem->signal_activate().connect(sigc::bind(sigc::ptr_fun(toggle_bool), sigc::ref(_search_wrap_around), citem));
    _find_panel->get_widget("case_item", citem);
    citem->signal_activate().connect(sigc::bind(sigc::ptr_fun(toggle_bool), sigc::ref(_search_ignore_case), citem));
    _find_panel->get_widget("word_item", citem);
    citem->signal_activate().connect(
      sigc::bind(sigc::ptr_fun(toggle_bool), sigc::ref(_search_match_whole_word), citem));
    _find_panel->get_widget("regex_item", citem);
    citem->signal_activate().connect(sigc::bind(sigc::ptr_fun(toggle_bool), sigc::ref(_use_regex), citem));
    
    _find_panel->get_widget("replace_bbox", _replace_box);
    _find_panel->get_widget("filler_1", _replace_filler_label);
    
  }

  void find_clicked() {
    if (_find_radio_button->get_active()) {
        _replace_entry->hide();
        _replace_box->hide();
        _replace_filler_label->hide();
    }
  }

  void replace_clicked() {
    if (_replace_radio_button->get_active())
      _container->show_all();
  }

  bool on_find_key_press(GdkEventKey *key) {
    if (key->keyval == GDK_KEY_Escape) {
      dynamic_cast<mforms::FindPanel *>(owner)->get_editor()->hide_find_panel();
      return true;
    }
    return false;
  }

  void find_icon_press(Gtk::EntryIconPosition pos, const GdkEventButton *ev) {
    if (ev->button == 1 && pos == Gtk::ENTRY_ICON_PRIMARY) {
      // update the menu
      {
        Gtk::CheckMenuItem *mitem;
        _find_panel->get_widget("wrap_item", mitem);
        mitem->set_active(_search_wrap_around);
        _find_panel->get_widget("case_item", mitem);
        mitem->set_active(_search_ignore_case);
        _find_panel->get_widget("word_item", mitem);
        mitem->set_active(_search_match_whole_word);
        _find_panel->get_widget("regex_item", mitem);
        mitem->set_active(_use_regex);
      }
      _search_menu->popup(ev->button, ev->time);

    }
  }

  void find_text_changed() {
    if (_find_status)
      _find_status->set_text("");
  }

  void clear_search_history() {
    if (_search_menu) {
      std::vector<Gtk::Widget *> children = _search_menu->get_children();
      while (children.size() > 8) {
        Gtk::Widget *w = children.back();
        _search_menu->remove(*w);
        children.pop_back();
      }
      Gtk::Widget *w = children.back();
      if (w)
        w->set_sensitive(false);
    }
  }

  virtual Gtk::Widget *get_outer() const {
    return _container;
  }

  size_t perform_action(FindPanelAction action) {
    std::string find_text = _find_entry->get_text();
    std::string repl_text = _replace_entry->get_text();
    CodeEditor *editor = dynamic_cast<FindPanel *>(owner)->get_editor();
    FindFlags flags = FindDefault;

    if (_search_match_whole_word)
      flags |= FindWholeWords;
    if (!_search_ignore_case)
      flags |= FindMatchCase;
    if (_search_wrap_around)
      flags |= FindWrapAround;
    if (_use_regex)
      flags |= FindRegex;

    switch (action) {
      case FindNext:
        if (find_text.empty())
          _find_status->set_text("");
        else {
          if (editor->find_and_highlight_text(find_text, flags, true, false)) {
            _find_status->set_text("Found match");
            return 1;
          } else
            _find_status->set_text("Not found");
        }
        break;
      case FindPrevious:
        if (find_text.empty())
          _find_status->set_text("");
        else {
          if (editor->find_and_highlight_text(find_text, flags, true, true)) {
            _find_status->set_text("Found match");
            return 1;
          } else
            _find_status->set_text("Not found");
        }
        break;
      case FindAndReplace:
        if (!find_text.empty())
          return editor->find_and_replace_text(find_text, repl_text, flags, false) > 0;

        break;
      case ReplaceAll:
        if (!find_text.empty()) {
          int count;
          if ((count = editor->find_and_replace_text(find_text, repl_text, flags, true)) > 0)
            _find_status->set_text(base::strfmt("Replaced %i matches", count));
          else
            _find_status->set_text("No matches found");
          return count;
        }
        break;
      default:
        g_message("unhandled FindPanel action %i", action);
        break;
    }
    return 0;
  }

public:
  static bool create(FindPanel *fp) {
    return new FindPanelImpl(fp) != 0;
  }

  static size_t perform_action(FindPanel *fp, FindPanelAction action) {
    FindPanelImpl *self = fp->get_data<FindPanelImpl>();
    return self->perform_action(action);
  }

  static void focus(FindPanel *fp) {
    FindPanelImpl *self = fp->get_data<FindPanelImpl>();
    self->_find_entry->grab_focus();
  }

  static void enable_replace(FindPanel *fp, bool flag) {
    FindPanelImpl *self = fp->get_data<FindPanelImpl>();
    flag ? self->_replace_radio_button->set_active(true) : self->_find_radio_button->set_active(true);
    // all find only widgets are marked No Show All, so we can use hide/show-all to hide the replace part
    if (flag)
      self->_container->show_all();
    else {
      self->_container->hide();
      self->_container->show();
    }
  }
};

void lf_findpanel_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_findpanel_impl.create = &FindPanelImpl::create;
  f->_findpanel_impl.perform_action = &FindPanelImpl::perform_action;
  f->_findpanel_impl.focus = &FindPanelImpl::focus;
  f->_findpanel_impl.enable_replace = &FindPanelImpl::enable_replace;
}
