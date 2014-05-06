/*
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _GTK_HELPERS_H_
#define _GTK_HELPERS_H_

#include <gdk/gdk.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <gtkmm/eventbox.h>

namespace Gtk
{
class TreeView;
class Widget;
class Entry;
class ListStore;
class EntryCompletion;
class HBox;
class ComboBox;
class ComboBoxText;
class ComboBoxEntryText;
class Window;
class Menu;
class Label;
class Paned;
}

#include <glibmm/refptr.h>
#include <gtkmm/treemodelcolumn.h>
#include <vector>
#include <string>
#include <grt/tree_model.h>

#include <sigc++/sigc++.h>

//!
//! \addtogroup linuxutils Linux utils
//! @{
//!
class TextListColumnsModel;
class TreeModelWrapper;
void expand_tree_nodes_as_in_be(const Glib::RefPtr<TreeModelWrapper> &model, Gtk::TreeView *tv);

Gtk::HBox &create_icon_label(const std::string &icon, const std::string &label);


//Gtk::Widget *create_closeable_tab(const Glib::ustring &title, const sigc::slot<void> &close_callback,
//                                  Gtk::Label **title_label);


void swap_panned_children(Gtk::Paned *paned, bool fixed_size1);

Glib::RefPtr<Gtk::ListStore> model_from_string_list(const std::vector<std::string>& list, TextListColumnsModel* columns);
Glib::RefPtr<Gtk::ListStore> model_from_string_list(const std::vector<std::string>& list, TextListColumnsModel** columns = 0);
Glib::RefPtr<Gtk::ListStore> model_from_string_list(const std::list<std::string>& list, TextListColumnsModel** columns = 0);
void recreate_model_from_string_list(Glib::RefPtr<Gtk::ListStore>, const std::vector<std::string>& list);
void setup_combo_for_string_list(Gtk::ComboBox *combo);

std::string get_selected_combo_item(Gtk::ComboBox *combo);
bool set_selected_combo_item(Gtk::ComboBox *combo, const std::string &value);

//! Wrapper to set string values to a Glib::ValueBase
//! Used in ListModelWrapper
void set_glib_string(Glib::ValueBase& value, const std::string& str, bool escape_nuls=false);
void set_glib_int(Glib::ValueBase& value, const int i);
void set_glib_bool(Glib::ValueBase& value, const bool b);
void set_glib_double(Glib::ValueBase& value, const double d);

void fill_combo_from_string_list(Gtk::ComboBox* combo, const std::vector<std::string>& list);
void fill_combo_from_string_list(Gtk::ComboBoxEntryText* combo, const std::vector<std::string>& list);

//! get_mainwindow is declared here as extern while it is implemented in frontend/linux/workbench/Program.cpp
//! and frontend/linux/shell/shell.cpp. get_mainwindow is needed by some functions/methods to
//! set dialog transient. Returns ptr to Gtk::Window
extern void* get_mainwindow_impl();
inline Gtk::Window* get_mainwindow()
{
  return (Gtk::Window*)get_mainwindow_impl();
}

extern std::string open_file_chooser(const std::string &filter = "*");
extern std::string save_file_chooser(const std::string &filter = "*");

struct GtkAutoLock
{
  GtkAutoLock() {gdk_threads_enter();}
  ~GtkAutoLock() {gdk_threads_leave();}
};

template <typename MutexType>
class Locker
{
  public:
    Locker(MutexType& m) : _m(m) {_m.lock();}
    ~Locker() {_m.unlock();}
  private:
    MutexType &_m;
};

void run_popup_menu(const bec::MenuItemList &items, const int time, 
                    const sigc::slot<void, std::string> &activate_slot, Gtk::Menu *popup);

void fix_broken_gtk_selection_handling(Gtk::TreeView *tree);

Glib::RefPtr<Gtk::ListStore> get_empty_model();

void gtk_paned_set_pos_ratio(Gtk::Paned* paned, const float ratio);
float gtk_paned_get_pos_ratio(Gtk::Paned* paned);

void gtk_reparent_realized(Gtk::Widget *widget, Gtk::Container *new_parent);

class PanedConstrainer {
  Gtk::Paned* _pan;
  bool _reentrant;
  int _margin_min;
  int _margin_max;
  bool _vertical;
  sigc::connection _size_alloc_sig;

  void size_alloc(Gtk::Allocation &_alloc);
  PanedConstrainer(Gtk::Paned *pan);

public:
  static void make_constrainer(Gtk::Paned *paned, int min_size, int max_size);

  ~PanedConstrainer();
  static void *destroy(void *data);

  void set_margin(int min = 0, int max = 0);
  Gtk::Paned* get();
};

//!
//! }@
//!

#endif /* _GTK_HELPERS_H_ */
