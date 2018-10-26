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

#include "gtk_helpers.h"
#include "image_cache.h"

#include <gtkmm/image.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/menu.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/paned.h>
#include "text_list_columns_model.h"

#include "treemodel_wrapper.h"
#include "base/string_utilities.h"

// This list_model is used for all functions which operate on GTKListStore
static TextListColumnsModel _wb_list_model;

Glib::RefPtr<Gtk::ListStore> get_empty_model() {
  static Glib::RefPtr<Gtk::ListStore> empty_list_store;
  if (!empty_list_store)
    empty_list_store = Gtk::ListStore::create(_wb_list_model);

  return empty_list_store;
}

//------------------------------------------------------------------------------
Gtk::Box &create_icon_label(const std::string &icon, const std::string &text) {
  Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));

  Gtk::Image *image = Gtk::manage(new Gtk::Image(ImageCache::get_instance()->image_from_filename(icon)));
  Gtk::Label *label = Gtk::manage(new Gtk::Label(text));

  label->set_use_markup(true);

  hbox->pack_start(*image);
  hbox->pack_start(*label, true, true);

  hbox->show_all();

  return *hbox;
}

//------------------------------------------------------------------------------
Glib::RefPtr<Gtk::ListStore> model_from_string_list(const std::vector<std::string> &list,
                                                    TextListColumnsModel *columns) {
  Glib::RefPtr<Gtk::ListStore> model = Gtk::ListStore::create(*columns);

  std::vector<std::string>::const_iterator last = list.end();

  for (std::vector<std::string>::const_iterator iter = list.begin(); iter != last; ++iter)
    (*model->append())[columns->item] = *iter;

  return model;
}

//------------------------------------------------------------------------------
Glib::RefPtr<Gtk::ListStore> model_from_string_list(const std::vector<std::string> &list,
                                                    TextListColumnsModel **columns) {
  if (columns)
    *columns = &_wb_list_model;

  return model_from_string_list(list, &_wb_list_model);
}

//------------------------------------------------------------------------------
Glib::RefPtr<Gtk::ListStore> model_from_string_list(const std::list<std::string> &list,
                                                    TextListColumnsModel **columns) {
  if (columns)
    *columns = &_wb_list_model;

  Glib::RefPtr<Gtk::ListStore> model = Gtk::ListStore::create(_wb_list_model);

  std::list<std::string>::const_iterator last = list.end();

  for (std::list<std::string>::const_iterator iter = list.begin(); iter != last; ++iter)
    (*model->append())[_wb_list_model.item] = *iter;

  return model;
}

//------------------------------------------------------------------------------
void recreate_model_from_string_list(Glib::RefPtr<Gtk::ListStore> model, const std::vector<std::string> &list) {
  model->clear();

  std::vector<std::string>::const_iterator last = list.end();

  for (std::vector<std::string>::const_iterator iter = list.begin(); iter != last; ++iter)
    (*model->append())[_wb_list_model.item] = *iter;
}

//------------------------------------------------------------------------------
void setup_combo_for_string_list(Gtk::ComboBox *combo) {
  Gtk::CellRendererText *cell = Gtk::manage(new Gtk::CellRendererText());
  combo->pack_end(*cell, true);
  combo->add_attribute(*cell, "text", 0);
}

//------------------------------------------------------------------------------
std::string get_selected_combo_item(Gtk::ComboBox *combo) {
  Gtk::TreeIter iter = combo->get_active();
  if (iter) {
    Gtk::TreeRow row = *iter;
    std::string item = row[_wb_list_model.item];

    return item;
  }
  return "";
}

//------------------------------------------------------------------------------

bool set_selected_combo_item(Gtk::ComboBox *combo, const std::string &value) {
  Glib::RefPtr<Gtk::TreeModel> store(combo->get_model());

  for (Gtk::TreeIter end = store->children().end(), iter = store->children().begin(); iter != end; ++iter) {
    Gtk::TreeRow row = *iter;
    std::string item = row[_wb_list_model.item];
    if (item == value) {
      combo->set_active(iter);
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void set_glib_string(Glib::ValueBase &value, const std::string &str, bool escape_nuls) {
  GValue *gval = value.gobj();

  g_value_init(gval, G_TYPE_STRING);
  if (escape_nuls) {
    std::string tmp;
    std::string::size_type p = 0, e;
    std::string::size_type length = str.length();
    // skip the \0 bytes so that data is displayed as in OSX
    while (p < length) {
      e = str.find('\0', p);
      if (e == std::string::npos)
        break;
      tmp.append(str.data() + p, e - p);
      p = e + 1;
    }
    if (p < length)
      tmp.append(str.data() + p);
    g_value_set_string(gval, tmp.c_str());
  } else
    g_value_set_string(gval, str.c_str());
}

//------------------------------------------------------------------------------
void set_glib_int(Glib::ValueBase &value, const int i) {
  GValue *gval = value.gobj();

  g_value_init(gval, G_TYPE_INT);
  g_value_set_int(gval, i);
}

//------------------------------------------------------------------------------
void set_glib_bool(Glib::ValueBase &value, const bool b) {
  GValue *gval = value.gobj();

  g_value_init(gval, G_TYPE_BOOLEAN);
  g_value_set_boolean(gval, b);
}

//------------------------------------------------------------------------------
void set_glib_double(Glib::ValueBase &value, const double d) {
  GValue *gval = value.gobj();

  g_value_init(gval, G_TYPE_DOUBLE);
  g_value_set_double(gval, d);
}

//------------------------------------------------------------------------------
void fill_combo_from_string_list(Gtk::ComboBox *combo, const std::vector<std::string> &list) {
  std::vector<std::string>::const_iterator it = list.begin();
  std::vector<std::string>::const_iterator last = list.end();

  Glib::RefPtr<Gtk::ListStore> store(Glib::RefPtr<Gtk::ListStore>::cast_dynamic(combo->get_model()));
  if (!store) {
    store = get_empty_model();
    combo->set_model(store);
  }

  store->clear();

  for (; last != it; ++it) {
    Gtk::TreeIter iter = store->append();
    Gtk::TreeRow row = *iter;
    row.set_value(0, *it);
  }
}

//------------------------------------------------------------------------------
void fill_combo_from_string_list(Gtk::ComboBoxText *combo, const std::vector<std::string> &list) {
  std::vector<std::string>::const_iterator it = list.begin();
  std::vector<std::string>::const_iterator last = list.end();

  for (; last != it; ++it)
    combo->append(*it);
}

//------------------------------------------------------------------------------
static std::string file_chooser_impl(const bool is_for_save, const std::string &filter) {
  std::string filename;
  Gtk::FileChooserDialog dialog("Please choose a file",
                                is_for_save ? Gtk::FILE_CHOOSER_ACTION_SAVE : Gtk::FILE_CHOOSER_ACTION_OPEN);
  if (get_mainwindow() != nullptr)
    dialog.set_transient_for(*get_mainwindow());

  // Add response buttons the the dialog:
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(is_for_save ? Gtk::Stock::SAVE : Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

  if (!filter.empty()) {
    Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
    // filter_any.set_name("Any files");
    filter_any->add_pattern(filter);
    dialog.add_filter(filter_any);
  }
  const int result = dialog.run();

  switch (result) {
    case (Gtk::RESPONSE_OK): {
      filename = dialog.get_filename();
      break;
    }
    default:
      break;
  }

  return filename;
}

//------------------------------------------------------------------------------
std::string open_file_chooser(const std::string &filter) {
  return file_chooser_impl(false, filter); // false - is not for save
}

std::string save_file_chooser(const std::string &filter) {
  return file_chooser_impl(true, filter); // true - is for save
}

//------------------------------------------------------------------------------
// std::string run_string_dialog(const std::string& title, const std::string& init_value)
//{
//  Gtk::Entry entry;
//  Gtk::Dialog dlg;
//  entry.set_text(init_value);
//  entry.show();
//  dlg.add_action_widget(entry, 0xff);
//  dlg.set_title(title);
//  dlg.set_position(Gtk::WIN_POS_MOUSE);
//  dlg.set_transient_for(*get_mainwindow());
//  const int result = dlg.run();
//
//  std::string ret = init_value;
//  switch (result)
//  {
//    case 0xff: // for the magic number 0xff see above add_action_widget
//      ret = entry.get_text();
//      break;
//    default: break;
//  }
//
//  return ret;
//}

static void populate_popup_menu(const bec::MenuItemList &items, const int time,
                                const sigc::slot<void, std::string> &activate_slot, Gtk::Menu *popup) {
  popup->foreach (sigc::mem_fun(popup, &Gtk::Container::remove));

  bec::MenuItemList::const_iterator cur_item = items.begin();
  const bec::MenuItemList::const_iterator last_item = items.end();

  for (; last_item != cur_item; cur_item++) {
    Gtk::MenuItem *item = Gtk::manage(new Gtk::MenuItem(base::replaceString(cur_item->caption, "_", "__"), true));
    item->set_name(cur_item->accessibilityName);
    item->set_sensitive(cur_item->enabled);
    // not support in Gtk from Ubuntu 8.04
    // item->set_use_underline(false);

    // g_message("run_popup: %s", cur_item->caption.c_str());

    switch (cur_item->type) {
      case bec::MenuAction:
      case bec::MenuUnavailable: {
        if (item)
          item->signal_activate().connect(sigc::bind(activate_slot, cur_item->internalName));
        break;
      }
      case bec::MenuCascade: {
        Gtk::Menu *submenu = Gtk::manage(new Gtk::Menu());
        item->set_submenu(*submenu);
        populate_popup_menu(cur_item->subitems, time, activate_slot, submenu);
        break;
      }
      case bec::MenuRadio: {
        // g_message("%s: fake impl of menuradioitem", __FUNCTION__);
      }
      case bec::MenuCheck: {
        Gtk::CheckMenuItem *citem = Gtk::manage(new Gtk::CheckMenuItem(cur_item->caption, true));
        item = citem;
        citem->set_active(cur_item->checked);
        citem->signal_activate().connect(sigc::bind(activate_slot, cur_item->internalName));
        break;
      }
      case bec::MenuSeparator: {
        delete item;
        item = Gtk::manage(new Gtk::SeparatorMenuItem());
        break;
      }
      default: {
        g_message("%s: WARNING! unhandled menuitem type %i, '%s'", __FUNCTION__, cur_item->type,
                  cur_item->internalName.c_str());
        break;
      }
    }

    popup->append(*item);
    item->show();
  }

  popup->show();
}

void run_popup_menu(const bec::MenuItemList &items, const int time, const sigc::slot<void, std::string> &activate_slot,
                    Gtk::Menu *popup) {
  populate_popup_menu(items, time, activate_slot, popup);

  popup->popup(3, time);
}

//--------------------------------------------------------------------------------
/*
Gtk::Widget *create_closeable_tab(const Glib::ustring &title,
                                  const sigc::slot<void> &close_callback,
                                  Gtk::Label **title_label)
{
  Gtk::Box *hbox= Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 1));
  Gtk::Label *label= Gtk::manage(new Gtk::Label("\342\234\225"));
  Gtk::EventBox *evbox= Gtk::manage(new Gtk::EventBox());
  Gtk::Label *text_label= Gtk::manage(new Gtk::Label(title));

  evbox->add(*label);
  label->show();
  hbox->pack_start(*text_label);
  hbox->pack_start(*evbox);

  hbox->show_all();

  if (title_label)
    *title_label = text_label;

  return hbox;
}
*/

//--------------------------------------------------------------------------------

void swap_panned_children(Gtk::Paned *paned, bool fixed_size_1) {
  Gtk::Widget *w1 = paned->get_child1();
  Gtk::Widget *w2 = paned->get_child2();

  w1->reference();
  w2->reference();

  paned->remove(*w1);
  paned->remove(*w2);

  paned->pack1(*w2, true, fixed_size_1);
  paned->pack2(*w1, true, !fixed_size_1);

  w1->unreference();
  w2->unreference();
}

//--------------------------------------------------------------------------------

static bool disallow_select(const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::Path &path,
                            bool selected) {
  return false;
}

static bool allow_select(const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::Path &path, bool selected) {
  return true;
}

static void handle_button_press(GdkEventButton *event, Gtk::TreeView *tree) {
  Gtk::TreeModel::Path path;
  Gtk::TreeViewColumn *column;
  int cx, cy;

  if (tree->get_path_at_pos(event->x, event->y, path, column, cx, cy)) {
    if (tree->get_selection()->is_selected(path) && (event->state & 0xff) == 0) {
      tree->get_selection()->set_select_function(sigc::ptr_fun(disallow_select));
    }
  }
}

static void handle_button_release(GdkEventButton *event, Gtk::TreeView *tree) {
  tree->get_selection()->set_select_function(sigc::ptr_fun(allow_select));
}

void fix_broken_gtk_selection_handling(Gtk::TreeView *tree) {
  tree->signal_button_press_event().connect_notify(sigc::bind(sigc::ptr_fun(handle_button_press), tree));
  tree->signal_button_release_event().connect_notify(sigc::bind(sigc::ptr_fun(handle_button_release), tree));
}

//------------------------------------------------------------------------------
void gtk_paned_set_pos_ratio(Gtk::Paned *paned, const float ratio) {
  const int min_pos = paned->property_min_position();
  const int max_pos = paned->property_max_position();
  const int diff = (max_pos - min_pos) * ratio;
  if (ratio >= 1.0)
    paned->set_position(max_pos);
  else
    paned->set_position(min_pos + diff);
}

//------------------------------------------------------------------------------
float gtk_paned_get_pos_ratio(Gtk::Paned *paned) {
  const float min_pos = paned->property_min_position();
  const float max_pos = paned->property_max_position();

  return (paned->get_position() - min_pos) / (max_pos - min_pos);
}
//------------------------------------------------------------------------------
void gtk_reparent_realized(Gtk::Widget *widget, Gtk::Container *new_parent) {
  if (!widget || !new_parent)
    return;
  widget->reference();
  widget->get_parent()->remove(*widget);
  new_parent->add(*widget);
  widget->unreference();
}

Gdk::RGBA color_to_rgba(Gdk::Color c) {
  Gdk::RGBA rgba;
  rgba.set_rgba(c.get_red_p(), c.get_green_p(), c.get_blue_p());
  return rgba;
}

//------------------------------------------------------------------------------
void PanedConstrainer::size_alloc(Gtk::Allocation &_alloc) {
  if (_reentrant)
    return;
  _reentrant = true;

  if (_pan && (_top_or_left_limit > 0 || _bottom_or_right_limit > 0)) {
    if (_pan->get_position() <= _top_or_left_limit) // If Paned position is lower than specified limit.
    {
      if (_allow_sticky) {
        if (!_was_hidden && _state_notifier_cb) {
          _was_hidden = true;
          _state_notifier_cb(PANED_HIDDEN);
        }
        _pan->set_position(0); // Hide it if it should be sticky.
      } else
        _pan->set_position(_top_or_left_limit); // Prevent making it smaller if it's not sticky
    } // If Paned is smaller than minimum size allowed by the _bottom_or_right_limit
    else if (_bottom_or_right_limit >= ((_vertical ? _pan->get_height() : _pan->get_width()) - _pan->get_position())) {
      // Set it fully visible
      _pan->set_position(_pan->property_max_position());
      if (!_was_hidden && _state_notifier_cb) {
        _was_hidden = true;
        _state_notifier_cb(PANED_FULLY_VISIBLE);
      }
    } else {
      if (_was_hidden && _state_notifier_cb) {
        _was_hidden = false;
        _state_notifier_cb(PANED_VISIBLE);
      }
    }
  }

  _reentrant = false;
}

PanedConstrainer::PanedConstrainer(Gtk::Paned *pan)
  : _pan(pan), _vertical(true), _allow_sticky(true), _was_hidden(false) {
  _reentrant = false;
  _top_or_left_limit = 60;
  _bottom_or_right_limit = 60;

  if (_pan) {
    if (pan->get_orientation() == Gtk::ORIENTATION_VERTICAL)
      _vertical = true;
    else
      _vertical = false;

    _size_alloc_sig = _pan->signal_size_allocate().connect(sigc::mem_fun(this, &PanedConstrainer::size_alloc));
  }
}
PanedConstrainer *PanedConstrainer::make_constrainer(Gtk::Paned *paned, int top_or_left_limit,
                                                     int bottom_or_right_limit) {
  if (paned) {
    PanedConstrainer *pc = new PanedConstrainer(paned);
    pc->set_limit(top_or_left_limit, bottom_or_right_limit);
    paned->set_data("paned_constrainer", pc);
    paned->add_destroy_notify_callback(reinterpret_cast<void *>(pc), &PanedConstrainer::destroy);
    return pc;
  }
  throw std::logic_error("Gtk::Paned is empty");
}
void PanedConstrainer::disable_sticky(bool disable) {
  _allow_sticky = !disable;
}

void PanedConstrainer::set_state_cb(const state_notifier &cb) {
  _state_notifier_cb = cb;
}

PanedConstrainer::~PanedConstrainer() {
  _size_alloc_sig.disconnect();
}

void *PanedConstrainer::destroy(void *data) {
  PanedConstrainer *pc = reinterpret_cast<PanedConstrainer *>(data);
  if (pc)
    delete pc;

  return 0;
}

void PanedConstrainer::set_limit(int top_or_left, int bottom_or_right) {
  _top_or_left_limit = top_or_left;
  _bottom_or_right_limit = bottom_or_right;
}

Gtk::Paned *PanedConstrainer::get() {
  return _pan;
}
