/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "../lf_toolbar.h"
#include "../lf_view.h"
#include <gtkmm/box.h>
#include <atkmm.h>
#include "image_cache.h"
#include "base/wb_iterators.h"
#include "base/log.h"
DEFAULT_LOG_DOMAIN("mforms.linux")

class ColorComboColumns : public Gtk::TreeModel::ColumnRecord {
public:
  Gtk::TreeModelColumn<std::string> color;
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > image;

  ColorComboColumns() {
    add(color);
    add(image);
  }
};

static ColorComboColumns *color_combo_columns = 0;

//------------------------------------------------------------------------------
static void process_ctrl_action(Gtk::Widget *w, mforms::ToolBarItem *item) {
  const int ignore_signal = (long)w->get_data("ignore_signal");
  if (!ignore_signal && item)
    item->callback();
}

//------------------------------------------------------------------------------
namespace {
  template <typename T>
  T cast(void *ptr) {
    return dynamic_cast<T>((Gtk::Widget *)ptr);
  }
}

static Gtk::Orientation toolbar_orientation_from_type(const mforms::ToolBarType type) {
  Gtk::Orientation dir = Gtk::ORIENTATION_HORIZONTAL;

  if (type == mforms::ToolPickerToolBar)
    dir = Gtk::ORIENTATION_VERTICAL;

  return dir;
}

//==============================================================================
//
//==============================================================================

class ToolBarImpl : public mforms::gtk::ViewImpl {
public:
  mutable Gtk::Box _toolbar;
  const mforms::ToolBarType _toolbar_type;

  virtual Gtk::Widget *get_outer() const {
    return &_toolbar;
  }

  ToolBarImpl(mforms::ToolBar *self, const mforms::ToolBarType type)
    : mforms::gtk::ViewImpl(self), _toolbar(toolbar_orientation_from_type(type)), _toolbar_type(type) {
    _toolbar.show();
  }

  virtual ~ToolBarImpl() {
  }

protected:
  virtual void set_padding_impl(int left, int top, int right, int bottom);
};

//------------------------------------------------------------------------------
Gtk::Widget *mforms::widget_for_toolbar(mforms::ToolBar *toolbar) {
  return toolbar->get_data< ::ToolBarImpl>()->get_outer();
}

//------------------------------------------------------------------------------
Gtk::Widget *mforms::widget_for_toolbar_item_named(mforms::ToolBar *toolbar, const std::string &name) {
  mforms::ToolBarItem *item = toolbar->find_item(name);
  if (item) {
    Gtk::Widget *w = cast<Gtk::Widget *>(item->get_data_ptr());
#if GTK_VERSION_LT(2, 16)
    if (item->get_type() == SearchFieldItem)
      w = cast<Gtk::Widget *>(w->get_data("entry"));
#endif
    return w;
  }
  return 0;
}

//------------------------------------------------------------------------------
bool mforms::gtk::ToolBarImpl::create_tool_bar(mforms::ToolBar *item, mforms::ToolBarType type) {
  return (new ::ToolBarImpl(item, type)) != 0;
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::insert_item(mforms::ToolBar *toolbar, int index, mforms::ToolBarItem *item) {
  ::ToolBarImpl *impl = toolbar->get_data< ::ToolBarImpl>();
  Gtk::Widget *w = cast<Gtk::Widget *>(item->get_data_ptr());
  if (!w)
    return;

  if (item && item->get_type() == SeparatorItem) {
    Gtk::Separator *sep = dynamic_cast<Gtk::Separator *>(w);
    if (sep) {
      sep->set_orientation(toolbar_orientation_from_type(impl->_toolbar_type));
      sep->show();
    }
  }

  const int size = impl->_toolbar.get_children().size();
  if (index < 0 || index >= size)
    index = -1;
  if (impl && w) {
    bool expand = false;
    bool fill = false;
    if (item->get_type() == mforms::ExpanderItem)
      expand = fill = true;

    impl->_toolbar.pack_start(*Gtk::manage(w), expand, fill);
    impl->_toolbar.reorder_child(*w, index);

    w->show_all();
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::remove_item(mforms::ToolBar *toolbar, mforms::ToolBarItem *item) {
  ::ToolBarImpl *impl = toolbar->get_data< ::ToolBarImpl>();
  Gtk::Widget *w = item ? cast<Gtk::Widget *>(item->get_data_ptr()) : 0;

  if (impl) {
    if (w) {
      impl->_toolbar.remove(*w);
    } else {
      typedef Glib::ListHandle<Gtk::Widget *> WList;
      WList list = impl->_toolbar.get_children();
      for (base::const_range<WList> it(list); it; ++it)
        impl->_toolbar.remove(*(*it));
    }
  }
}

//------------------------------------------------------------------------------
bool mforms::gtk::ToolBarImpl::create_tool_item(mforms::ToolBarItem *item, ToolBarItemType type) {
  Gtk::Widget *w = 0;
  switch (type) {
    case mforms::TextActionItem:
    case mforms::ActionItem: {
      Gtk::Button *btn = Gtk::manage(new Gtk::Button());
      btn->set_focus_on_click(false);
      btn->set_border_width(0);
      btn->set_relief(Gtk::RELIEF_NONE);
      btn->signal_clicked().connect(sigc::bind(sigc::ptr_fun(process_ctrl_action), btn, item));
      w = btn;
      break;
    }
    case mforms::SegmentedToggleItem:
    case mforms::ToggleItem: {
      Gtk::ToggleButton *btn = Gtk::manage(new Gtk::ToggleButton());
      btn->set_focus_on_click(false);
      btn->set_relief(Gtk::RELIEF_NONE);
      btn->signal_toggled().connect(sigc::bind(sigc::ptr_fun(process_ctrl_action), btn, item));
      btn->set_inconsistent(false);

      w = btn;
      break;
    }
    case mforms::SeparatorItem: {
      Gtk::Separator *sep = new Gtk::Separator(Gtk::ORIENTATION_VERTICAL);
      w = sep;
      break;
    }
    case mforms::SearchFieldItem: {
#if GTK_VERSION_GE(2, 16)
      Gtk::Entry *entry = Gtk::manage(new Gtk::Entry());
      w = entry;
      entry->set_icon_from_stock(Gtk::Stock::FIND);
#else
      Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
      w = hbox;
      Gtk::Image *image = Gtk::manage(new Gtk::Image(Gtk::Stock::FIND, Gtk::ICON_SIZE_MENU));
      Gtk::Entry *entry = Gtk::manage(new Gtk::Entry());

      hbox->pack_start(*image, false, true);
      hbox->pack_start(*entry, true, true);
      hbox->set_data("entry", entry);
      hbox->show_all();
#endif
      entry->signal_activate().connect(sigc::bind(sigc::ptr_fun(process_ctrl_action), entry, item));
      break;
    }
    case mforms::TextEntryItem: {
      Gtk::Box *hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
      w = hbox;
      Gtk::Entry *entry = Gtk::manage(new Gtk::Entry());
      hbox->pack_start(*entry, true, true);
      hbox->set_data("entry", entry);
      hbox->show_all();
      entry->signal_activate().connect(sigc::bind(sigc::ptr_fun(process_ctrl_action), entry, item));
      break;
    }
    case mforms::FlatSelectorItem:
    case mforms::SelectorItem: {
      Gtk::ComboBoxText *ct = Gtk::manage(new Gtk::ComboBoxText());
      ct->signal_changed().connect(sigc::bind(sigc::ptr_fun(process_ctrl_action), ct, item));

      w = ct;
      break;
    }
    case mforms::ColorSelectorItem: {
      if (!color_combo_columns) {
        color_combo_columns = new ColorComboColumns();
      }
      Gtk::ComboBox *ct = Gtk::manage(new Gtk::ComboBox());

      ct->pack_start(color_combo_columns->image);
      ct->signal_changed().connect(sigc::bind(sigc::ptr_fun(process_ctrl_action), ct, item));

      w = ct;

      break;
    }
    case mforms::ExpanderItem:
    case mforms::LabelItem: {
      Gtk::Label *label = Gtk::manage(new Gtk::Label("", 0.0, 0.5));
      w = label;
      break;
    }
    case mforms::ImageBoxItem: {
      Gtk::Image *image = Gtk::manage(new Gtk::Image());
      w = image;
      break;
    }
    case mforms::TitleItem: {
      Gtk::Label *label = Gtk::manage(new Gtk::Label("", 0.0, 0.5));
      w = label;
      auto provider = Gtk::CssProvider::create();
      provider->load_from_data("* { color: #333; font-weight: bold; }");
      w->get_style_context()->add_provider(provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
      break;
    }
  }

  if (w) {
    w->show();
  } else
    logError("create_tool_item, widget is 0 for passed type %i\n", type);

  item->set_data(w);

  return w != 0;
}

//------------------------------------------------------------------------------
static void free_icon(gpointer icon_ptr) {
  Gtk::Image *img = reinterpret_cast<Gtk::Image *>(icon_ptr);
  if (img) {
    delete img;
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::set_item_icon(mforms::ToolBarItem *item, const std::string &image_path) {
  Gtk::Button *btn = cast<Gtk::Button *>(item->get_data_ptr());
  if (btn) {
    static ImageCache *images = ImageCache::get_instance();
    Gtk::Image *img = new Gtk::Image(images->image_from_path(image_path));
    if (img) {
      btn->set_image(*img);
      btn->set_data("icon", img, free_icon);
      img->show();
    }
  }
}

//------------------------------------------------------------------------------
static void swap_icons(Gtk::ToggleButton *btn) {
  Gtk::Image *img = 0;

  if (btn->get_active())
    img = cast<Gtk::Image *>(btn->get_data("alt_icon"));
  else
    img = cast<Gtk::Image *>(btn->get_data("icon"));

  img->show();
  btn->set_image(*img);
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::set_item_alt_icon(mforms::ToolBarItem *item, const std::string &image_path) {
  Gtk::ToggleButton *btn = cast<Gtk::ToggleButton *>(item->get_data_ptr());
  if (btn) {
    static ImageCache *images = ImageCache::get_instance();
    Gtk::Image *img = new Gtk::Image(images->image_from_path(image_path));
    if (img) {
      btn->set_data("alt_icon", img, free_icon);
      btn->signal_toggled().connect(sigc::bind(sigc::ptr_fun(swap_icons), btn));
    }
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::set_item_text(mforms::ToolBarItem *item, const std::string &label) {
  const mforms::ToolBarItemType type = item->get_type();

  switch (type) {
    case mforms::TextActionItem:
    case mforms::ActionItem:
    case mforms::SegmentedToggleItem:
    case mforms::ToggleItem: {
      Gtk::Button *btn = cast<Gtk::Button *>(item->get_data_ptr());
      btn->add_label(label);
      btn->set_name(label);
      break;
    }
    case mforms::TitleItem:
    case mforms::LabelItem: {
      Gtk::Label *lbl = cast<Gtk::Label *>(item->get_data_ptr());
      if (lbl) {
        lbl->set_markup("<small>" + label + "</small>");
        lbl->set_name(label);
      }
      break;
    }
    case mforms::FlatSelectorItem:
    case mforms::SelectorItem: {
      Gtk::ComboBoxText *ct = cast<Gtk::ComboBoxText *>(item->get_data_ptr());
      if (ct)
        ct->set_active_text(label);
      break;
    }
    case mforms::ColorSelectorItem: {
      Gtk::ComboBox *combo = cast<Gtk::ComboBox *>(item->get_data_ptr());
      if (combo) {
        Glib::RefPtr<Gtk::TreeModel> model = combo->get_model();
        if (model) {
          const Gtk::TreeModel::Children children = model->children();
          const Gtk::TreeIter last = children.end();
          Gtk::TreeRow row;

          for (Gtk::TreeIter it = children.begin(); it != last; ++it) {
            row = *it;
            if (row.get_value(color_combo_columns->color) == label) {
              combo->set_active(it);
              break;
            }
          }
        }
      }
      break;
    }
    case mforms::SearchFieldItem:
    case mforms::TextEntryItem: {
      Gtk::Entry *e = cast<Gtk::Entry *>(item->get_data_ptr());
      if (e)
        e->set_text(label);
      break;
    }
    case mforms::SeparatorItem:
    case mforms::ExpanderItem:
    case mforms::ImageBoxItem:
      break;
  }
}

//------------------------------------------------------------------------------
std::string mforms::gtk::ToolBarImpl::get_item_text(mforms::ToolBarItem *item) {
  std::string text;

  switch (item->get_type()) {
    case mforms::FlatSelectorItem:
    case mforms::SelectorItem: {
      Gtk::ComboBoxText *ct = cast<Gtk::ComboBoxText *>(item->get_data_ptr());
      if (ct)
        text = ct->get_active_text();
      break;
    }
    case mforms::ColorSelectorItem: {
      const Gtk::ComboBox *combo = cast<Gtk::ComboBox *>(item->get_data_ptr());
      if (combo) {
        const Gtk::TreeIter iter = combo->get_active();
        const Gtk::TreeRow row = *iter;
        text = row.get_value(color_combo_columns->color);
      }
      break;
    }
    case mforms::SearchFieldItem: {
      Gtk::Entry *e = cast<Gtk::Entry *>(item->get_data_ptr());
      if (e)
        text = e->get_text();
      break;
    }
    default: {
      Gtk::Widget *btn = cast<Gtk::Widget *>(item->get_data_ptr());
      if (btn)
        text = btn->get_name();
    }
  }

  return text;
}

//------------------------------------------------------------------------------

void mforms::gtk::ToolBarImpl::set_item_name(mforms::ToolBarItem *item, const std::string &name) {
  Gtk::Widget *w = cast<Gtk::Widget *>(item->get_data_ptr());
  if (w) {
    w->set_name(name);
    {
      Glib::RefPtr<Atk::Object> acc = w->get_accessible();
      if (acc)
        acc->set_name(name);
    }
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::set_item_enabled(mforms::ToolBarItem *item, bool is_on) {
  Gtk::Widget *w = cast<Gtk::Widget *>(item->get_data_ptr());
  if (w) {
    w->set_sensitive(is_on);
    if (w->get_sensitive() != is_on)
      throw new std::runtime_error("Failed to change sensivity");
  }
}

//------------------------------------------------------------------------------
bool mforms::gtk::ToolBarImpl::get_item_enabled(mforms::ToolBarItem *item) {
  bool ret = false;
  Gtk::Widget *w = cast<Gtk::Widget *>(item->get_data_ptr());
  if (w)
    ret = w->get_sensitive();

  return ret;
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::set_item_checked(mforms::ToolBarItem *item, bool toggled) {
  Gtk::ToggleButton *btn = cast<Gtk::ToggleButton *>(item->get_data_ptr());
  if (btn) {
    btn->set_data("ignore_signal", (void *)1);
    btn->set_active(toggled);
    btn->set_data("ignore_signal", (void *)0);
  }
}

//------------------------------------------------------------------------------
bool mforms::gtk::ToolBarImpl::get_item_checked(mforms::ToolBarItem *item) {
  bool ret = false;

  Gtk::ToggleButton *btn = cast<Gtk::ToggleButton *>(item->get_data_ptr());
  if (btn)
    ret = btn->get_active();

  return ret;
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::set_item_tooltip(mforms::ToolBarItem *item, const std::string &text) {
  Gtk::Widget *w = cast<Gtk::Widget *>(item->get_data_ptr());
  if (w) {
#if GTK_VERSION_GT(2, 10)
    w->set_tooltip_text(text);
#endif
  }
}

//------------------------------------------------------------------------------
void mforms::gtk::ToolBarImpl::set_selector_items(ToolBarItem *item, const std::vector<std::string> &values) {
  if (item->get_type() == mforms::SelectorItem || item->get_type() == mforms::FlatSelectorItem) {
    Gtk::ComboBoxText *w = cast<Gtk::ComboBoxText *>(item->get_data_ptr());
    if (w) {
      w->set_data("ignore_signal", (void *)1);

      w->remove_all();
      const int size = values.size();
      for (int i = 0; i < size; ++i)
        w->append(values[i]);

      if (w->get_active_row_number() < 0 && !values.empty())
        w->set_active_text(values[0]);

      w->set_data("ignore_signal", 0);
    }
  } else if (item->get_type() == mforms::ColorSelectorItem) {
    Gtk::ComboBox *w = cast<Gtk::ComboBox *>(item->get_data_ptr());
    if (w) {
      w->set_data("ignore_signal", (void *)1);
      Glib::RefPtr<Gtk::ListStore> model = Gtk::ListStore::create(*color_combo_columns);
      const int size = values.size();
      for (int i = 0; i < size; ++i) {
        Gtk::TreeRow row = *model->append();
        Gdk::Color color(values[i]);
        Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, 16, 14);
        pixbuf->fill((guint32)color.get_red() << 24 | (guint32)color.get_green() << 16 | (guint32)color.get_blue() << 8);

        row[color_combo_columns->color] = values[i];
        row[color_combo_columns->image] = pixbuf;
      }

      w->set_model(model);

      if (w->get_active_row_number() < 0)
        w->set_active(0);

      w->set_data("ignore_signal", 0);
    }
  }
}

//------------------------------------------------------------------------------
void ::ToolBarImpl::set_padding_impl(int left, int top, int right, int bottom) {
  _toolbar.set_border_width(left);
}

//------------------------------------------------------------------------------
void mforms::gtk::lf_toolbar_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_tool_bar_impl.create_tool_bar = mforms::gtk::ToolBarImpl::create_tool_bar;
  f->_tool_bar_impl.insert_item = mforms::gtk::ToolBarImpl::insert_item;
  f->_tool_bar_impl.remove_item = mforms::gtk::ToolBarImpl::remove_item;
  f->_tool_bar_impl.create_tool_item = mforms::gtk::ToolBarImpl::create_tool_item;
  f->_tool_bar_impl.set_item_icon = mforms::gtk::ToolBarImpl::set_item_icon;
  f->_tool_bar_impl.set_item_alt_icon = mforms::gtk::ToolBarImpl::set_item_alt_icon;
  f->_tool_bar_impl.set_item_text = mforms::gtk::ToolBarImpl::set_item_text;
  f->_tool_bar_impl.get_item_text = mforms::gtk::ToolBarImpl::get_item_text;
  f->_tool_bar_impl.set_item_name = mforms::gtk::ToolBarImpl::set_item_name;
  f->_tool_bar_impl.set_item_enabled = mforms::gtk::ToolBarImpl::set_item_enabled;
  f->_tool_bar_impl.get_item_enabled = mforms::gtk::ToolBarImpl::get_item_enabled;
  f->_tool_bar_impl.set_item_checked = mforms::gtk::ToolBarImpl::set_item_checked;
  f->_tool_bar_impl.get_item_checked = mforms::gtk::ToolBarImpl::get_item_checked;
  f->_tool_bar_impl.set_item_tooltip = mforms::gtk::ToolBarImpl::set_item_tooltip;
  f->_tool_bar_impl.set_selector_items = mforms::gtk::ToolBarImpl::set_selector_items;
}
