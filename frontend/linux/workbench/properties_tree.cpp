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

#include <memory>
#include <gtk/gtk.h>
#include <gtkmm/viewport.h>
#include <gtkmm/separator.h>
#include "gtk_helpers.h"
#include "properties_tree.h"
#include "grtpp_util.h"
#include "grt/grt_value_inspector.h"
#include <boost/lexical_cast.hpp>

#include "workbench/wb_context_ui.h"
//------------------------------------------------------------------------------
PropertyValue::PropertyValue(PropertyInspector* owner, const bec::NodeId& node) : _owner(owner), _prop_node(node) {
  add(_text);
  _text.set_alignment(Gtk::ALIGN_START);
  _text.set_single_line_mode(true);
  _text.set_ellipsize(Pango::ELLIPSIZE_END);
  _text.show();
  _conn = signal_event().connect(sigc::mem_fun(this, &PropertyValue::on_event));

  override_background_color(color_to_rgba(Gdk::Color("#f5f5f5")), Gtk::STATE_FLAG_NORMAL);
}

//------------------------------------------------------------------------------
PropertyValue::~PropertyValue() {
  _conn.disconnect();
}

//------------------------------------------------------------------------------
void PropertyValue::set_text(const std::string& text) {
  _text.set_text(text);
}

//------------------------------------------------------------------------------
void PropertyValue::stop_edit() {
  stop_editing();
  remove();
  add(label());
}

//------------------------------------------------------------------------------
void PropertyValue::start_edit() {
  remove();
  add(editor());

  editor().property_width_request() = get_width() - 2;
  editor().show();

  start_editing();
}

//------------------------------------------------------------------------------
bool PropertyValue::on_event(GdkEvent* event) {
  if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
    _owner->handle_click(this);
    return true;
  } else if (event->type == GDK_KEY_RELEASE) {
    const int key = event->key.keyval;
    if (key == GDK_KEY_Escape) {
      _owner->edit_canceled();
      stop_edit();
      return true;
    } else if (key == GDK_KEY_Return) {
      set_text(get_new_value());
      _owner->edit_done(this, true);
      stop_edit();
      return true;
    }
  }
  return false;
}

//==============================================================================
PropertyString::PropertyString(PropertyInspector* owner, const bec::NodeId& node) : PropertyValue(owner, node) {
  _entry.set_has_frame(false);
  _entry.set_alignment(Gtk::ALIGN_START);
}

//------------------------------------------------------------------------------
std::string PropertyString::get_new_value() const {
  return _entry.get_text();
}

//------------------------------------------------------------------------------
void PropertyString::start_editing() {
  _entry.set_text(get_text());
  _entry.show();
}

//------------------------------------------------------------------------------
Gtk::Widget& PropertyString::editor() {
  return _entry;
}

//==============================================================================
PropertyBool::PropertyBool(PropertyInspector* owner, const bec::NodeId& node) : PropertyValue(owner, node) {
  _conn = _button.signal_toggled().connect(sigc::mem_fun(this, &PropertyBool::on_value_changed));

  start_edit(); // That's because when PropertyValue set ups label its vtable points to PropertyValue
                // So we need to update Table with our control. As we have the same CheckButton
                // for label and for editor it is ok to call start_edit. It will replace label
                // widget with editor widget. But at this moment label() and editor() will
                // return PropertyBool's widgets.
}

//------------------------------------------------------------------------------
std::string PropertyBool::get_new_value() const {
  return _button.get_active() ? "1" : "0";
}

//------------------------------------------------------------------------------
void PropertyBool::set_text(const std::string& text) {
  _conn.block();
  _button.set_active(text == "1" ? 1 : 0);
  _conn.unblock();
}

//------------------------------------------------------------------------------
std::string PropertyBool::get_text() const {
  return _button.get_active() ? "1" : "0";
}

//------------------------------------------------------------------------------
void PropertyBool::start_editing() {
  _button.show();
}

//------------------------------------------------------------------------------
Gtk::Widget& PropertyBool::editor() {
  return _button;
}

//------------------------------------------------------------------------------
Gtk::Widget& PropertyBool::label() {
  return _button;
}

//------------------------------------------------------------------------------
void PropertyBool::on_value_changed() {
  PropertyValue::set_text(_button.get_active() ? "True" : "False");
  _owner->edit_done(this);
}

//==============================================================================
PropertyColor::PropertyColor(PropertyInspector* owner, const bec::NodeId& node)
  : PropertyValue(owner, node), _hbox(Gtk::ORIENTATION_HORIZONTAL), _button("...") {
  _hbox.pack_start(_entry);
  _hbox.pack_end(_button, false, false);
  _hbox.show_all();

  _button.signal_clicked().connect(sigc::mem_fun(this, &PropertyColor::show_dlg));
}

//------------------------------------------------------------------------------
std::string PropertyColor::get_new_value() const {
  return _entry.get_text();
}

//------------------------------------------------------------------------------
void PropertyColor::start_editing() {
  _entry.set_text(get_text());
}

//------------------------------------------------------------------------------
void PropertyColor::show_dlg() {
  _dlg.get_color_selection()->set_current_color(Gdk::Color(get_text()));
  const int resp = _dlg.run();

  if (resp == Gtk::RESPONSE_OK) {
    const Gdk::Color color(_dlg.get_color_selection()->get_current_color());
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "#%02x%02x%02x", color.get_red() >> 8, color.get_green() >> 8,
             color.get_blue() >> 8);
    buffer[sizeof(buffer) - 1] = 0;

    _entry.set_text(buffer);
    set_text(buffer);
    _owner->edit_done(this);
    _entry.grab_focus();
  }
  _dlg.hide();
}

//------------------------------------------------------------------------------
Gtk::Widget& PropertyColor::editor() {
  return _hbox;
}

//==============================================================================
PropertyText::PropertyText(PropertyInspector* owner, const bec::NodeId& node)
  : PropertyValue(owner, node), _ctrl(false) {
  _scroll.add(_text);
  _wnd.get_vbox()->add(_scroll);
  _wnd.set_position(Gtk::WIN_POS_MOUSE);
  if (get_mainwindow() != nullptr)
    _wnd.set_transient_for(*get_mainwindow());

  _wnd.resize(128, 96);
  _text.signal_event().connect(sigc::mem_fun(this, &PropertyText::handle_event));
}

//------------------------------------------------------------------------------
std::string PropertyText::get_new_value() const {
  Gtk::TextView* tv = const_cast<Gtk::TextView*>(&_text);
  return tv->get_buffer()->get_text();
}

//------------------------------------------------------------------------------
void PropertyText::start_editing() {
  _text.get_buffer()->set_text(get_text());
  _wnd.show_all();
}

//------------------------------------------------------------------------------
void PropertyText::stop_editing() {
  _wnd.hide();
}

//------------------------------------------------------------------------------
bool PropertyText::handle_event(GdkEvent* event) {
  if (event->type == GDK_KEY_PRESS) {
    const int key = event->key.keyval;
    if (key == GDK_KEY_Control_L || key == GDK_KEY_Control_R)
      _ctrl = true;
  }
  if (event->type == GDK_KEY_RELEASE) {
    const int key = event->key.keyval;
    if (key == GDK_KEY_Control_L || key == GDK_KEY_Control_R)
      _ctrl = false;

    if (key == GDK_KEY_Escape) {
      _owner->edit_canceled();
      stop_edit();
      return true;
    } else if (key == GDK_KEY_KP_Enter || (_ctrl && key == GDK_KEY_Return)) {
      set_text(get_new_value());
      _owner->edit_done(this, true);
      stop_edit();
      return true;
    }
  }
  return false;
}

//==============================================================================
PropertyInspector::PropertyInspector() : _edited_property(0), _updating(0) {
  _table = new Gtk::Table();
  _table->set_border_width(4);
  add(*_table);
}

//------------------------------------------------------------------------------
static void remove_child(Gtk::Widget& w) {
  w.hide();
  delete &w;
}

//------------------------------------------------------------------------------
PropertyInspector::~PropertyInspector() {
  clear();
  delete _table;
}

//------------------------------------------------------------------------------
void PropertyInspector::clear() {
  _edited_property = 0;
  _table->foreach (sigc::ptr_fun(&::remove_child));
  _properties.clear();
  remove();
  delete _table;
  _table = new Gtk::Table();
  _table->set_border_width(4);
  _table->set_row_spacings(2);
  add(*_table);
  _table->show();
  Gtk::Viewport* vp = static_cast<Gtk::Viewport*>(_table->get_parent());
  vp->override_background_color(color_to_rgba(Gdk::Color("#ffffff")), Gtk::STATE_FLAG_NORMAL);
}

//------------------------------------------------------------------------------
void PropertyInspector::populate() {
  _updating = 1;
  clear();

  _table->property_n_columns() = 2;
  const int count = _get_properties_count();
  if (count > 0) {
    _table->property_n_rows() = count + 1;

    for (int i = 0; i < count; ++i) {
      const int i2 = i * 2;
      bec::NodeId node;
      node.append(i);

      const std::string type = _get_type_slot(node);
      const std::string name = _get_value_slot(node, true); // true means name

      Gtk::Label* name_label = Gtk::manage(new Gtk::Label(name));
      name_label->property_xalign() = 0.0;
      _table->attach(*name_label, 0, 1, i2, i2 + 1, Gtk::FILL, Gtk::AttachOptions());

      // g_message("prop %s, type %s", name.c_str(), type.c_str());
      PropertyValue* prop;
      if (type.empty())
        prop = new PropertyString(this, node);
      else if ("bool" == type)
        prop = new PropertyBool(this, node);
      else if ("color" == type)
        prop = new PropertyColor(this, node);
      else if ("longtext" == type)
        prop = new PropertyText(this, node);
      else
        prop = new PropertyString(this, node);

      _table->attach(*prop, 1, 2, i2, i2 + 1, Gtk::EXPAND | Gtk::FILL, Gtk::AttachOptions());

      Gtk::HSeparator* sep = Gtk::manage(new Gtk::HSeparator());
      _table->attach(*sep, 0, 1, i2 + 1, i2 + 2, Gtk::FILL, Gtk::AttachOptions());
      sep = Gtk::manage(new Gtk::HSeparator());
      _table->attach(*sep, 1, 2, i2 + 1, i2 + 2, Gtk::FILL, Gtk::AttachOptions());

      _properties.push_back(prop);
    }
  }

  _table->show_all_children();
  _updating = 0;
}

//------------------------------------------------------------------------------
void PropertyInspector::update() {
  _updating = 1;
  const int count = _properties.size();

  if (count > 0) {
    PropertyValue* prop = 0;
    for (int i = 0; i < count; ++i) {
      prop = _properties[i];
      prop->set_text(_get_value_slot(prop->node(), false));
    }
  }
  _updating = 0;
}

//------------------------------------------------------------------------------
void PropertyInspector::handle_click(PropertyValue* property) {
  if (!_updating && _edited_property != property) {
    if (_edited_property) {
      edit_done(_edited_property);
      _edited_property->stop_edit();
    }

    _edited_property = property;
    property->start_edit();
  }
}

//------------------------------------------------------------------------------
void PropertyInspector::edit_done(PropertyValue* property, const bool finish) {
  if (!_updating) {
    _set_value_slot(property->node(), property->get_new_value(), property->type());
    property->set_text(_get_value_slot(property->node(), false));
    if (finish)
      _edited_property = 0;
  }
}

//------------------------------------------------------------------------------
void PropertyInspector::edit_canceled() {
  _edited_property = 0;
}

//------------------------------------------------------------------------------
PropertiesTree::PropertiesTree() : Gtk::Box(Gtk::ORIENTATION_VERTICAL), _inspector(0) {
  pack_start(_inspector_view, true, true);

  _inspector_view.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  _inspector_view.set_shadow_type(Gtk::SHADOW_IN);

  show_all();
}

//------------------------------------------------------------------------------
PropertiesTree::~PropertiesTree() {
  delete _inspector;
}

//------------------------------------------------------------------------------
void PropertiesTree::update() {
  std::vector<std::string> items;

  delete _inspector;
  _inspector = 0;
  _inspector =
    wb::WBContextUI::get()->create_inspector_for_selection(wb::WBContextUI::get()->get_active_main_form(), items);

  _inspector_view.clear();
  if (_inspector) {
    _inspector_view.set_count_slot(sigc::mem_fun(this, &PropertiesTree::get_properties_count));
    _inspector_view.set_value_slot_setter(sigc::mem_fun(this, &PropertiesTree::set_value));
    _inspector_view.set_value_slot_getter(sigc::mem_fun(this, &PropertiesTree::get_value));
    _inspector_view.set_type_slot_getter(sigc::mem_fun(this, &PropertiesTree::get_prop_type));

    _inspector_view.populate();
    _inspector_view.update();

    _inspector->set_refresh_ui_slot(std::bind(&PropertiesTree::refresh, this));
  }
}

//------------------------------------------------------------------------------
void PropertiesTree::refresh() {
  if (_inspector)
    _inspector_view.update();
}

//------------------------------------------------------------------------------
int PropertiesTree::get_properties_count() const {
  return _inspector ? _inspector->count() : 0;
}

//------------------------------------------------------------------------------
std::string PropertiesTree::get_prop_type(const bec::NodeId& node) const {
  std::string type;
  _inspector->get_field(node, ::bec::ValueInspectorBE::EditMethod, type);
  return type;
}

//------------------------------------------------------------------------------
void PropertiesTree::set_value(const bec::NodeId& node, const std::string& value, const grt::Type type) {
  if (node.is_valid()) {
    try {
      _inspector->set_convert_field(node, ::bec::ValueInspectorBE::Value, value);
    } catch (...) {
      g_message("PropertiesTree::set_value: can't convert value %s to ssize_t", value.c_str());
    }
  }
}

//------------------------------------------------------------------------------
std::string PropertiesTree::get_value(const bec::NodeId& node, const bool is_name) const {
  if (node.is_valid()) {
    std::string val;
    _inspector->get_field(node, is_name ? bec::ValueInspectorBE::Name : bec::ValueInspectorBE::Value, val);
    return val;
  }
  return "";
}
