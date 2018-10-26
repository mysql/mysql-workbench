/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "workbench/wb_overview.h"
#include "overview_panel.h"
#include "treemodel_wrapper.h"
#include "multi_view.h"

#include "grtdb/db_object_helpers.h"
#include "workbench/wb_model_file.h"
#include "workbench/wb_context.h"

#include <glib/gstdio.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <gtkmm.h>
#pragma GCC diagnostic pop
#include "gtk_helpers.h"
#include "base/string_utilities.h"

using base::strfmt;

class OverviewDivision : public Gtk::Box {
  bool header_button_release(GdkEventButton *e) {
    if (!e || e->button == 1) {
      if (_arrow->property_arrow_type() == Gtk::ARROW_DOWN)
        toggle(false);
      else
        toggle(true);
    }
    return false;
  }

  void view_mode_changed(Gtk::ToggleButton *btn, wb::OverviewBE::OverviewDisplayMode mode) {
    if (_view_mode_changing)
      return;
    _view_mode_changing = true;
    for (std::vector<Gtk::ToggleButton *>::iterator iter = _switch_buttons.begin(); iter != _switch_buttons.end();
         ++iter) {
      if (*iter != btn) {
        if ((*iter)->get_active())
          (*iter)->set_active(false);
      }
    }
    if (!btn->get_active())
      btn->set_active(true);
    _view_mode_changing = false;

    _view_mode_change.emit(mode);
  }

  Gtk::Button *add_mode_switch_button(Gtk::Box *view_mode_box, const std::string &filename,
                                      wb::OverviewBE::OverviewDisplayMode mode) {
    // Create button
    Gtk::ToggleButton *btn = Gtk::manage(new Gtk::ToggleButton());
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->set_name("Group Toggle");

    btn->property_can_focus() = false;

    // Assign image to it. filename is just a name of the file w/o leading path
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = ImageCache::get_instance()->image_from_filename(filename);
    btn->set_image(*Gtk::manage(new Gtk::Image(pixbuf)));

    // Pack and show button
    view_mode_box->pack_start(*btn, false, false);
    btn->show();

    btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(this, &OverviewDivision::view_mode_changed), btn, mode));

    _switch_buttons.push_back(btn);

    return btn;
  }

  Gtk::Button *add_action_button(Gtk::Box *view_mode_box, const std::string &filename, const std::string &tooltip,
                                 const sigc::slot<void> &callback) {
    // Create button
    Gtk::Button *btn = Gtk::manage(new Gtk::Button());
    btn->set_relief(Gtk::RELIEF_NONE);
    btn->set_name("Group Toggle");
#if GTK_VERSION_GT(2, 10)
    btn->set_tooltip_text(tooltip);
#endif
    btn->property_can_focus() = false;

    // Assign image to it. filename is just a name of the file w/o leading path
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = ImageCache::get_instance()->image_from_filename(filename);
    btn->set_image(*Gtk::manage(new Gtk::Image(pixbuf)));

    // Pack and show button
    view_mode_box->pack_start(*btn, false, false);
    btn->show();

    // Connect button
    btn->signal_clicked().connect(callback);

    return btn;
  }

  void create_header(const std::string &text, Gtk::EventBox **ebox_dptr, Gtk::Box **hbox_dptr) {
    // Let us get events from the header's widgets by creating EventBox
    Gtk::EventBox *ebox = *ebox_dptr = Gtk::manage(new Gtk::EventBox());
    ebox->set_name("Overview Header");
    ebox->set_size_request(-1, 24);
    ebox->show();

    Gtk::Box *hdr_box = *hbox_dptr = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    ebox->add(*hdr_box);

    _arrow = Gtk::manage(new Gtk::Arrow(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE));
    hdr_box->pack_start(*_arrow, false, false);
    _arrow->set_name("Toggle");

    Gtk::Label *label = Gtk::manage(new Gtk::Label(text));
    label->set_alignment(0.0, 0.5);
    hdr_box->pack_start(*label, true, true);
    label->set_name("Caption");

    hdr_box->show_all();
  }

  sigc::signal<void, wb::OverviewBE::OverviewDisplayMode> _view_mode_change;
  std::vector<Gtk::ToggleButton *> _switch_buttons;
  bec::NodeId _node;
  Gtk::HSeparator _sep;
  Gtk::Arrow *_arrow;
  Gtk::Box *_header_box;
  wb::OverviewBE *_overview;
  wb::OverviewBE::OverviewDisplayMode _display_mode;
  bool _view_mode_changing;

public:
  sigc::signal<void, wb::OverviewBE::OverviewDisplayMode> signal_view_mode_change() {
    return _view_mode_change;
  }

  OverviewDivision(wb::OverviewBE *overview, const bec::NodeId &node, const std::string &text, bool view_switch,
                   bool no_header = false)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), _node(node), _overview(overview), _view_mode_changing(false) {
    Gtk::EventBox *ebox = 0;

    _display_mode = wb::OverviewBE::MSmallIcon;
    _arrow = 0;

    if (!no_header) {
      create_header(text, &ebox, &_header_box);
      pack_start(*ebox, true, true);

      pack_start(_sep, false, true);
      _sep.show();

      if (view_switch) {
        Gtk::Button *btn;

        btn = add_mode_switch_button(_header_box, "collapsing_panel_grid_large_icons2.png", wb::OverviewBE::MLargeIcon);
#if GTK_VERSION_GT(2, 10)
        btn->set_tooltip_text(_("View items as large icons"));
#endif
        btn = add_mode_switch_button(_header_box, "collapsing_panel_grid_small_icons2.png", wb::OverviewBE::MSmallIcon);
#if GTK_VERSION_GT(2, 10)
        btn->set_tooltip_text(_("View items as small icons"));
#endif
        btn = add_mode_switch_button(_header_box, "collapsing_panel_grid_details2.png", wb::OverviewBE::MList);
#if GTK_VERSION_GT(2, 10)
        btn->set_tooltip_text(_("View items as a list"));
#endif
      }

      ebox->show_all();

      // Connect button click event to the method responsible for collapsing/expanding of the division
      ebox->signal_button_release_event().connect(sigc::mem_fun(this, &OverviewDivision::header_button_release));
    }
    set_name("Overview Division");
  }

  void add_edit_buttons(
    const std::string &obj_type) // const sgic::slot<void> &add_slot, const sgic::slot<void> &del_slot)
  {
    add_action_button(_header_box, "collapsing_panel_header_tab_add2.png", strfmt(_("Add a new %s"), obj_type.c_str()),
                      sigc::mem_fun(this, &OverviewDivision::add_clicked));
    //      btn->set_tooltip_text(_("Add new item"));
    add_action_button(_header_box, "collapsing_panel_header_tab_del2.png",
                      strfmt(_("Delete the selected %s"), obj_type.c_str()),
                      sigc::mem_fun(this, &OverviewDivision::delete_clicked));
    //      btn->set_tooltip_text(_("Delete selected item"));
  }

  void set_display_mode(wb::OverviewBE::OverviewDisplayMode mode) {
    _display_mode = mode;

    if (_switch_buttons.empty()) {
      _view_mode_change.emit(mode);
      return;
    }
    switch (mode) {
      case wb::OverviewBE::MList:
        _switch_buttons[2]->clicked();
        break;
      case wb::OverviewBE::MSmallIcon:
        _switch_buttons[1]->clicked();
        break;
      case wb::OverviewBE::MLargeIcon:
        _switch_buttons[0]->clicked();
        break;
      default:
        break;
    }
  }

  wb::OverviewBE::OverviewDisplayMode get_display_mode() {
    return _display_mode;
  }

  void add_clicked() {
    _overview->request_add_object(_node);
  }

  void delete_clicked() {
    if (_overview->count_children(_node) > 0)
      _overview->request_delete_object(_overview->get_focused_child(_node));
  }

  void toggle(bool flag) {
    std::vector<Gtk::Widget *> children(get_children());
    std::vector<Gtk::Widget *>::iterator iter = children.begin();

    while (iter != children.end() && (*iter)->gobj() != (GtkWidget *)_sep.gobj())
      ++iter;
    ++iter;

    if (flag) {
      if (_arrow)
        _arrow->set(Gtk::ARROW_DOWN, Gtk::SHADOW_NONE);
      while (iter != children.end()) {
        (*iter)->show();
        ++iter;
      }
    } else {
      if (_arrow)
        _arrow->set(Gtk::ARROW_RIGHT, Gtk::SHADOW_NONE);
      while (iter != children.end()) {
        (*iter)->hide();
        ++iter;
      }
    }
  }
};

class OverviewItemContainer : public MultiView {
protected:
  wb::OverviewBE *_overview;
  bec::NodeId _node;

  std::string _drag_tmp_file;

  void activate_item(const Gtk::TreeModel::Path &path);
  // on_selection_changed is called from MultiView before MultiView::signal_selection_changed is emitted
  virtual void on_selection_changed(const std::vector<bec::NodeId> &sel);

  bec::NodeId get_selected_node();

  void drag_begin(const Glib::RefPtr<Gdk::DragContext> &context);
  void drag_data_get(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::SelectionData &data, guint, guint time);
  void drag_data_delete(const Glib::RefPtr<Gdk::DragContext> &context);
  bool drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int, int, guint time);
  bool drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int, int, guint time, Gtk::Widget *target);
  void drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, const Gtk::SelectionData &data,
                          guint info, guint time);
  bool row_draggable(const Gtk::TreeModel::Path &path);

public:
  enum DragDropType { DragOnly, DropOnly, DragAndDrop };

  OverviewItemContainer(wb::OverviewBE *overview, const bec::NodeId &node, bool items_as_tree, bool items_as_icons);
  virtual ~OverviewItemContainer();
  void enable_drag_drop(const std::vector<Gtk::TargetEntry> *entries = 0,
                        Gdk::DragAction src_actions = Gdk::ACTION_MOVE, DragDropType type = DragAndDrop);
  void enable_drag_drop_type(const std::string &drag_type);
  void refresh_info(const bec::NodeId &node);

  const bec::NodeId &get_base_node() {
    return _node;
  }

  void update_base_node(const bec::NodeId &node) {
    _node = node;

    get_tree_model()->update_root_node(_node);
    get_icon_model()->update_root_node(_node);
  }

  void set_display_mode(wb::OverviewBE::OverviewDisplayMode mode);
};

//------------------------------------------------------------------------------
void OverviewItemContainer::activate_item(const Gtk::TreeModel::Path &path) {
  bec::NodeId node(_node);

  std::for_each(path.begin(), path.end(), sigc::mem_fun(node, &bec::NodeId::append));

  _overview->activate_node(node);
}

//------------------------------------------------------------------------------
void OverviewItemContainer::on_selection_changed(const std::vector<bec::NodeId> &sel) {
  _overview->begin_selection_marking();
  ssize_t node_type;

  for (int i = sel.size() - 1; i >= 0; --i) {
    node_type = -1;
    _overview->get_field(sel[i], wb::OverviewBE::NodeType, node_type);
    if (::wb::OverviewBE::OItem == node_type)
      _overview->select_node(sel[i]);
  }
  _overview->end_selection_marking();
}

//------------------------------------------------------------------------------
bec::NodeId OverviewItemContainer::get_selected_node() {
  Gtk::TreeModel::Path path(get_selected());
  bec::NodeId node(_node);

  if (!path.empty())
    std::for_each(path.begin(), path.end(), sigc::mem_fun(node, &bec::NodeId::append));

  return node;
}

//------------------------------------------------------------------------------
void OverviewItemContainer::drag_begin(const Glib::RefPtr<Gdk::DragContext> &context) {
  //  bec::NodeId node(get_selected_node());
  //
  //  if (!node.is_valid())
  //  {}
  //  else
  //  {}
}

//------------------------------------------------------------------------------
void OverviewItemContainer::drag_data_get(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::SelectionData &data,
                                          guint, guint time) {
  bec::NodeId node(get_selected_node());
  if (node.is_valid()) {
    // const char *bytes= 0;
    // size_t length;
    std::string name;

    _overview->get_field(node, wb::OverviewBE::Label, name);

    if (data.get_target() == "text/uri-list") {
      std::string file = _overview->get_file_for_node(node);

      if (name.empty())
        name = file;

      if (!file.empty()) {
        _drag_tmp_file = bec::GRTManager::get()->get_tmp_dir() + "/" + name;

        wb::ModelFile::copy_file(file, _drag_tmp_file);
        {
          std::vector<Glib::ustring> uris;
          uris.push_back("file://" + _drag_tmp_file);
          data.set_uris(uris);
        }
      }
    } else if (data.get_target() == WB_DBOBJECT_DRAG_TYPE) {
      grt::ListRef<GrtObject> sel = _overview->get_selection();
      std::list<db_DatabaseObjectRef> objects;

      for (grt::ListRef<GrtObject>::const_iterator obj = sel.begin(); obj != sel.end(); ++obj) {
        if ((*obj).is_valid() && db_DatabaseObjectRef::can_wrap(*obj)) {
          objects.push_back(db_DatabaseObjectRef::cast_from(*obj));
        }
      }

      if (!objects.empty()) {
        std::string text = bec::CatalogHelper::dbobject_list_to_dragdata(objects);

        data.set(data.get_target(), text);
      }
    }
  }
}

//------------------------------------------------------------------------------
void OverviewItemContainer::drag_data_delete(const Glib::RefPtr<Gdk::DragContext> &context) {
  if (!_drag_tmp_file.empty())
    ::g_remove(_drag_tmp_file.c_str());
}

//------------------------------------------------------------------------------
bool OverviewItemContainer::drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int, int, guint time) {
  std::vector<std::string> targets(context->list_targets());
  std::vector<std::string>::iterator iter;

  if ((iter = std::find(targets.begin(), targets.end(), "text/uri-list")) != targets.end()) {
    Glib::ustring type = *iter;
    g_message("ask for %s", iter->c_str());

    drag_get_data(context, type, time);

    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool OverviewItemContainer::drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int, int, guint time,
                                        Gtk::Widget *target) {
  std::vector<std::string> targets(context->list_targets());

  if (!target->drag_dest_find_target(context).empty()) {
    context->drag_status(context->get_suggested_action(), time);

    target->drag_highlight();

    return true;
  } else
    context->drag_refuse(time);
  return false;
}

//------------------------------------------------------------------------------
void OverviewItemContainer::drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                                               const Gtk::SelectionData &data, guint info, guint time) {
  if (data.targets_include_uri() || data.get_data_type() == "text/uri-list") {
    std::vector<Glib::ustring> uri(data.get_uris());

    if (!uri.empty() && Glib::str_has_prefix(uri[0], "file://")) {
      _overview->add_file_to_node(_node, uri[0].substr(7));

      context->drag_finish(true, false, time);
      return;
    }
  }
  context->drag_finish(false, false, time);
}

//------------------------------------------------------------------------------
bool OverviewItemContainer::row_draggable(const Gtk::TreeModel::Path &path) {
  bec::NodeId node(_node);

  if (!path.empty())
    std::for_each(path.begin(), path.end(), sigc::mem_fun(node, &bec::NodeId::append));

  return !_overview->get_node_unique_id(node).empty();
}

//------------------------------------------------------------------------------
OverviewItemContainer::OverviewItemContainer(wb::OverviewBE *overview, const bec::NodeId &node, bool items_as_tree,
                                             bool items_as_icons)
  : MultiView(items_as_tree, items_as_icons), _overview(overview), _node(node) {
  Glib::RefPtr<TreeModelWrapper> tv_model(
    TreeModelWrapper::create(_overview, get_tree_view(), "OverviewItemContainer", node, true));
  Glib::RefPtr<TreeModelWrapper> iv_model(TreeModelWrapper::create(_overview, 0, "OverviewItemContainer", node, true));
  iv_model->set_iconview(get_icon_view());

  tv_model->set_row_draggable_slot(sigc::mem_fun(this, &OverviewItemContainer::row_draggable));
  iv_model->set_row_draggable_slot(sigc::mem_fun(this, &OverviewItemContainer::row_draggable));

  if (get_tree_view()) {
    // create columns for the tree view
    tv_model->model().append_string_column(wb::OverviewBE::Label, _("Name"), EDITABLE_WO_FIRST, WITH_ICON);

    int columnCount = _overview->get_details_field_count(node);
    for (int counter = 0; counter < columnCount; counter++) {
      std::string columnCaption = _overview->get_field_name(node, wb::OverviewBE::FirstDetailField + counter);
      tv_model->model().append_string_column(wb::OverviewBE::FirstDetailField + counter, columnCaption);

      get_tree_view()->get_column(get_tree_view()->get_columns().size() - 1)->property_max_width() = 200;
    }
  }
  if (get_icon_view())
    iv_model->model().set_text_column(wb::OverviewBE::Label, true, get_icon_view());

  set_tree_model(tv_model);
  set_icon_model(iv_model);

  signal_activate_item().connect(sigc::mem_fun(this, &OverviewItemContainer::activate_item));
}

//------------------------------------------------------------------------------
OverviewItemContainer::~OverviewItemContainer() {
  get_tree_model()->invalidate();
  get_icon_model()->invalidate();
}

//------------------------------------------------------------------------------
void OverviewItemContainer::enable_drag_drop_type(const std::string &drag_type) {
  std::vector<Gtk::TargetEntry> targets;
  if (drag_type == "file") {
    enable_drag_drop();
  } else {
    targets.push_back(Gtk::TargetEntry(drag_type, Gtk::TARGET_SAME_APP));
    enable_drag_drop(&targets, Gdk::ACTION_COPY, OverviewItemContainer::DragOnly);
  }
}

//------------------------------------------------------------------------------
void OverviewItemContainer::enable_drag_drop(const std::vector<Gtk::TargetEntry> *entries, Gdk::DragAction src_actions,
                                             DragDropType type) {
  std::vector<Gtk::TargetEntry> targets;

  if (entries)
    targets = *entries;
  else {
    targets.push_back(Gtk::TargetEntry("text/uri-list", Gtk::TargetFlags(0)));
    targets.push_back(Gtk::TargetEntry("text/x-sql", Gtk::TargetFlags(0)));
    targets.push_back(Gtk::TargetEntry("text/plain", Gtk::TargetFlags(0)));
    targets.push_back(Gtk::TargetEntry("UTF8_STRING", Gtk::TargetFlags(0)));
    targets.push_back(Gtk::TargetEntry("STRING", Gtk::TargetFlags(0)));
    targets.push_back(Gtk::TargetEntry("TEXT", Gtk::TargetFlags(0)));
  }

  Gtk::IconView *icon_view = get_icon_view();
  if (icon_view) {
    if (type != DropOnly)
      icon_view->enable_model_drag_source(targets, Gdk::MODIFIER_MASK, src_actions);
    // icon_view->drag_source_set(targets, Gdk::MODIFIER_MASK, src_actions);

    if (type != DragOnly)
      icon_view->drag_dest_set(targets, Gtk::DEST_DEFAULT_ALL, Gdk::ACTION_COPY);

    icon_view->signal_drag_motion().connect(
      sigc::bind(sigc::mem_fun(this, &OverviewItemContainer::drag_motion), icon_view));
    icon_view->signal_drag_drop().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_drop));
    icon_view->signal_drag_data_received().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_data_received));

    icon_view->signal_drag_begin().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_begin));
    icon_view->signal_drag_data_get().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_data_get));
    icon_view->signal_drag_data_delete().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_data_delete));
  }

  Gtk::TreeView *tree_view = get_tree_view();
  if (tree_view) {
    if (type != DropOnly)
      tree_view->drag_source_set(targets);
    if (type != DragOnly)
      tree_view->drag_dest_set(targets);

    tree_view->signal_drag_motion().connect(
      sigc::bind(sigc::mem_fun(this, &OverviewItemContainer::drag_motion), tree_view));
    tree_view->signal_drag_drop().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_drop));
    tree_view->signal_drag_data_received().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_data_received));

    tree_view->signal_drag_begin().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_begin));
    tree_view->signal_drag_data_get().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_data_get));
    tree_view->signal_drag_data_delete().connect(sigc::mem_fun(this, &OverviewItemContainer::drag_data_delete));
  }
}

//------------------------------------------------------------------------------
void OverviewItemContainer::refresh_info(const bec::NodeId &node) {
  Gtk::TreePath path(node.back());
  Gtk::TreeIter iter(get_icon_model()->get_iter(path));

  std::string s;
  _overview->get_field(node, 0, s);

  get_tree_model()->row_changed(path, iter);
  get_icon_model()->row_changed(path, iter);

  queue_draw();
}

//------------------------------------------------------------------------------
void OverviewItemContainer::set_display_mode(wb::OverviewBE::OverviewDisplayMode mode) {
  switch (mode) {
    case wb::OverviewBE::MNone:
      break;
    case wb::OverviewBE::MLargeIcon:
      get_icon_model()->set_icon_size(bec::Icon32);
      set_icon_mode(true, false);
      break;
    case wb::OverviewBE::MSmallIcon:
      get_icon_model()->set_icon_size(bec::Icon16);
      set_icon_mode(true, true);
      break;
    case wb::OverviewBE::MList:
      get_tree_model()->set_icon_size(bec::Icon16);
      set_icon_mode(false, false);
      break;
  }
}

//===============================================================================
class OverviewSection : public OverviewItemContainer {
  Gtk::Label _label;

public:
  OverviewSection(wb::OverviewBE *overview, const bec::NodeId &node, bool items_as_tree, bool items_as_icons)
    : OverviewItemContainer(overview, node, items_as_tree, items_as_icons), _label("", 0.0, 0.5) {
    // remove all widgets and readd them at the end after the label
    std::vector<Gtk::Widget *> children = get_children();
    for (std::vector<Gtk::Widget *>::iterator iter = children.begin(); iter != children.end(); ++iter) {
      (*iter)->reference();
      remove(**iter);
    }

    _label.set_use_markup(true);
    _label.show();
    add(_label);

    update_label();

    Gtk::Separator *separator = Gtk::manage(new Gtk::HSeparator());
    add(*separator);
    separator->show();

    for (std::vector<Gtk::Widget *>::iterator iter = children.begin(); iter != children.end(); ++iter) {
      add(**iter);
      (*iter)->unreference();
    }
  }

  void update_label() {
    std::string label;
    _overview->get_field(_node, wb::OverviewBE::Label, label);

    _label.set_markup(
      strfmt("<b>%s</b> <small>(%zi items)</small>", label.c_str(), _overview->count_children(_node) - 1));
  }

  virtual void refresh() {
    OverviewItemContainer::refresh();
    update_label();
  }
};

class OverviewGroup : public Gtk::Box {
  wb::OverviewBE *_overview;
  bec::NodeId _node;
  std::string _uid;
  Gtk::Label *_label;

  void invalidate_children(Gtk::Widget &widget) {
    OverviewItemContainer *items = dynamic_cast<OverviewItemContainer *>(&widget);
    if (items) {
      items->get_tree_model()->invalidate();
      items->get_icon_model()->invalidate();
    }
  }

  void update_base_node_children(Gtk::Widget &widget, const bec::NodeId &node) {
    OverviewItemContainer *items = dynamic_cast<OverviewItemContainer *>(&widget);
    if (items) {
      bec::NodeId new_node(node);

      new_node.append(items->get_base_node().back());

      items->update_base_node(new_node);
    }
  }

public:
  OverviewGroup(wb::OverviewBE *overview, const bec::NodeId &node, Gtk::Label *label)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), _overview(overview), _node(node), _label(label) {
    set_homogeneous(false);
    set_border_width(12);
    set_name("Overview Note Page");
    _uid = _overview->get_node_unique_id(node);
  }

  virtual ~OverviewGroup() {
    invalidate();
  }

  void invalidate() {
    foreach (sigc::mem_fun(this, &OverviewGroup::invalidate_children))
      ;
  }

  std::string get_unique_id() {
    return _uid;
  }

  void update_base_node(const bec::NodeId &node) {
    if (_node.back() != node.back()) {
      foreach (sigc::bind(sigc::mem_fun(this, &OverviewGroup::update_base_node_children), node))
        ;
      _node = node;
    }
  }

  bec::NodeId get_base_node() {
    return _node;
  }

  void update_label() {
    std::string text, descr;

    _overview->get_field(_node, wb::OverviewBE::Label, text);
    descr = _overview->get_field_description(_node, wb::OverviewBE::Label);

    _label->set_markup(strfmt("<b>%s</b>\n<small>%s</small>", text.c_str(), descr.c_str()));
  }
};

class OverviewGroupContainer : public Gtk::Notebook {
  wb::OverviewBE *_overview;
  bec::NodeId _node;
  Gtk::Menu _context_menu;
  bool _is_focus_node_enabled;
  int _current_page_index;

public:
  bec::NodeId node() {
    return _node;
  }
  int current_page_index() {
    return _current_page_index;
  }

private:
  void page_switched(Gtk::Widget *page, guint num) {
    if ((size_t)num < _overview->count_children(_node))
      _overview->focus_node(_overview->get_child(_node, num));
    _current_page_index = num;
  }

  void activate_context_menu(const std::string &item, const std::vector<bec::NodeId> &nodes) {
    _overview->activate_popup_item_for_nodes(item, nodes);
  }

  void on_tab_button_press(GdkEventButton *ev, OverviewGroup *group) {
    if (ev->button == 3) {
      std::vector<bec::NodeId> nodes;
      nodes.push_back(group->get_base_node());
      bec::MenuItemList items = _overview->get_popup_items_for_nodes(nodes);

      if (!items.empty())
        run_popup_menu(items, ev->time,
                       sigc::bind(sigc::mem_fun(this, &OverviewGroupContainer::activate_context_menu), nodes),
                       &_context_menu);
    } else if (ev->button == 1 && ev->type == GDK_2BUTTON_PRESS) {
      // double click tab hedaer
      _overview->activate_node(group->get_base_node());
    }
  }

  Gtk::EventBox *create_group_heading(const bec::NodeId &node, Gtk::Label **tab_label) {
    Gtk::EventBox *tab_box = Gtk::manage(new Gtk::EventBox());
    Gtk::Box *tab = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
    std::string icon_path;
    std::string text, descr;
    int icon;

    _overview->get_field(node, wb::OverviewBE::Label, text);
    descr = _overview->get_field_description(node, wb::OverviewBE::Label);
    icon = _overview->get_field_icon(node, wb::OverviewBE::Label, bec::Icon32);

    icon_path = bec::IconManager::get_instance()->get_icon_path(icon);

    tab_box->add(*tab);
    tab_box->set_name("Overview Note Header");

    Gtk::Image *image = Gtk::manage(new Gtk::Image(icon_path));
    tab->pack_start(*image, false, false);

    *tab_label =
      Gtk::manage(new Gtk::Label(strfmt("<b>%s</b>\n<small>%s</small>", text.c_str(), descr.c_str()), 0.0, 0.5));
    tab->pack_start(**tab_label, false, false);
    (*tab_label)->set_use_markup(true);

    tab_box->show_all();

    return tab_box;
  }

protected:
  virtual void on_size_allocate(Gdk::Rectangle &rect) {
    bool was_focus_node_enabled = _is_focus_node_enabled;
    _is_focus_node_enabled = false;
    Notebook::on_size_allocate(rect);
    _is_focus_node_enabled = was_focus_node_enabled;
  }

public:
  OverviewGroupContainer(wb::OverviewBE *be, const bec::NodeId &node)
    : _overview(be), _node(node), _is_focus_node_enabled(true), _current_page_index(-1) {
    set_name("Overview Note");

    set_scrollable(true);

    signal_switch_page().connect(sigc::mem_fun(this, &OverviewGroupContainer::page_switched));
  }

  void enable_set_focus_node(bool value) {
    _is_focus_node_enabled = value;
  }
  bool enable_set_focus_node() {
    return _is_focus_node_enabled;
  }

  OverviewGroup *add_group(const bec::NodeId &node, int position = -1) {
    Gtk::Label *label;

    Gtk::EventBox *tab = create_group_heading(node, &label);

    OverviewGroup *page = Gtk::manage(new OverviewGroup(_overview, node, label));

    tab->signal_button_press_event().connect_notify(
      sigc::bind(sigc::mem_fun(this, &OverviewGroupContainer::on_tab_button_press), page));

    bool was_focus_node_enabled = _is_focus_node_enabled;
    _is_focus_node_enabled = false;

    if (position < 0)
      append_page(*page, *tab);
    else
      insert_page(*page, *tab, position);

    page->show();

    _is_focus_node_enabled = was_focus_node_enabled;

    return page;
  }

  void refresh_info(const bec::NodeId &node) {
    if (get_n_pages() > (ssize_t)node.back())
      ((OverviewGroup *)get_nth_page(node.back()))->update_label();
  }
};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

OverviewPanel::OverviewPanel(wb::OverviewBE *overview)
  : _container(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0))), _overview_be(overview) {
  add(*_container);
  _container->get_parent()->set_name("Overview Viewport");

  set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
  _container->show();

  //_container->modify_bg(Gtk::STATE_NORMAL, _container->get_style()->get_white());
  //_container->get_parent()->modify_bg(Gtk::STATE_NORMAL, _container->get_style()->get_white());

  _overview_be->pre_refresh_groups = std::bind(&OverviewPanel::pre_refresh_groups, this);

  _freeze = true;
  _rebuilding = false;
}

//----------------------------------------------------------------------------------------------------

static void delete_container_contents(Gtk::Container *container) {
  std::vector<Gtk::Widget *> children = container->get_children();
  for (std::vector<Gtk::Widget *>::const_iterator i = children.begin(); i != children.end(); ++i) {
    container->remove(**i);
    delete *i;
  }
}

void OverviewPanel::reset() {
  _freeze = true;

  _group_containers_by_id.clear();
  _item_containers_by_id.clear();

  delete_container_contents(_container);

  Gtk::Layout *filler = Gtk::manage(new Gtk::Layout());
  Gtk::Image *img = Gtk::manage(new Gtk::Image(bec::IconManager::get_instance()->get_icon_path("background.png")));
  filler->put(*img, 0, 0);
  filler->set_name("Overview Filler");
  _container->pack_start(*filler, true, true);
  filler->show();
}

//----------------------------------------------------------------------------------------------------
void OverviewPanel::update_for_resize() {
  // XXX gtk bug with icon reflow, this should be optimized to only refresh all icon views
  rebuild_all();
}

void OverviewPanel::rebuild_all() {
  if (_rebuilding)
    return;

  _freeze = false;
  _rebuilding = true;

  delete_container_contents(_container);

  _group_containers_by_id.clear();
  _item_containers_by_id.clear();

  bec::NodeId root = _overview_be->get_root();

  for (size_t i = 0; i < _overview_be->count_children(root); i++) {
    bec::NodeId node = _overview_be->get_child(root, i);
    ssize_t type;

    _overview_be->get_field(node, wb::OverviewBE::NodeType, type);

    if (type != wb::OverviewBE::ODivision)
      throw std::logic_error("unhandled node type at this context");

    build_division(_container, node);
  }

  Gtk::Layout *filler = Gtk::manage(new Gtk::Layout());
  Gtk::Image *img = Gtk::manage(new Gtk::Image(bec::IconManager::get_instance()->get_icon_path("background.png")));
  filler->put(*img, 0, 0);

  filler->set_name("Overview Filler");

  _container->pack_start(*filler, true, true);
  filler->show();

  {
    size_t group_index = 0;
    if (_groups->current_page_index() >= 0)
      group_index = _groups->current_page_index();
    if (group_index < _overview_be->count_children(_groups->node()))
      _overview_be->focus_node(_overview_be->get_child(_groups->node(), group_index));
  }

  _rebuilding = false;
}

void OverviewPanel::item_list_selection_changed(const std::vector<bec::NodeId> &nodes, MultiView *mview) {
  if (nodes.empty())
    return;
  for (std::map<std::string, OverviewItemContainer *>::iterator iter = _item_containers_by_id.begin();
       iter != _item_containers_by_id.end(); ++iter) {
    if (iter->second && iter->second != mview)
      iter->second->select_node(bec::NodeId());
  }
}

//------------------------------------------------------------------------------
// Build division e.g. EER Diagrams, Physical Schemata,
// Schema Privileges in the model overview. @pnode points to a subtree
void OverviewPanel::build_division(Gtk::Box *container, const bec::NodeId &pnode) {
  // Fetch division name, wb::OverviewBE::Label is a enum defined in
  // backend/workbench/wb_overview.h so it is passed as a column index
  // to wb::OverviewBE (_overview) method get_field
  std::string text;
  _overview_be->get_field(pnode, wb::OverviewBE::Label, text);

  // Check if division is expanded
  ssize_t default_expanded = 1;
  _overview_be->get_field(pnode, wb::OverviewBE::Expanded, default_expanded);

  ssize_t child_type;
  _overview_be->get_field(pnode, wb::OverviewBE::ChildNodeType, child_type);

  OverviewDivision *division = 0;
  division = Gtk::manage(new OverviewDivision(_overview_be, pnode, text, true, false));

  container->pack_start(*division, false, true);

  if (child_type == wb::OverviewBE::OGroup) {
    // The notebook (tabwidget) will represent databases
    _groups = Gtk::manage(new OverviewGroupContainer(_overview_be, pnode));

    division->pack_start(*_groups, false, false);

    division->add_edit_buttons(_("Schema"));

    _group_containers_by_id[pnode.toString()] = _groups;

    // assumes child nodes are sections
    // Build groups, like 'mydb' tab and its content for a databases
    _overview_be->refresh_node(pnode, true);
    for (size_t i = 0; i < _overview_be->count_children(pnode); i++) {
      bec::NodeId node = _overview_be->get_child(pnode, i);

      build_group(division, _groups, node);
    }

    division->set_display_mode(wb::OverviewBE::MSmallIcon);
  } else if (child_type == wb::OverviewBE::OItem) {
    OverviewItemContainer *item_list = Gtk::manage(new OverviewItemContainer(_overview_be, pnode, true, true));

    item_list->signal_selection_changed().connect(
      sigc::bind(sigc::mem_fun(this, &OverviewPanel::item_list_selection_changed), item_list));
    item_list->signal_popup_menu().connect(sigc::bind(sigc::mem_fun(this, &OverviewPanel::item_popup_menu), item_list));

    division->pack_start(*item_list, false, false);
    division->signal_view_mode_change().connect(sigc::mem_fun(*item_list, &OverviewItemContainer::set_display_mode));
    item_list->show();

    _item_containers_by_id[pnode.toString()] = item_list;

    division->set_display_mode(wb::OverviewBE::MLargeIcon);

    // for scripts/notes pane, we allow drag/drop of files
    std::string drag_type = _overview_be->get_node_drag_type(pnode);
    if (!drag_type.empty())
      item_list->enable_drag_drop_type(drag_type);
  } else if (child_type == wb::OverviewBE::OSection) {
    for (size_t i = 0; i < _overview_be->count_children(pnode); i++) {
      bec::NodeId node = _overview_be->get_child(pnode, i);
      OverviewSection *section = Gtk::manage(new OverviewSection(_overview_be, node, true, true));

      section->signal_selection_changed().connect(
        sigc::bind(sigc::mem_fun(this, &OverviewPanel::item_list_selection_changed), section));
      _item_containers_by_id[node.toString()] = section;

      division->pack_start(*section, false, false);
      section->show();

      section->signal_popup_menu().connect(sigc::bind(sigc::mem_fun(this, &OverviewPanel::item_popup_menu), section));
      section->set_border_width(8);

      division->signal_view_mode_change().connect(sigc::mem_fun(*section, &OverviewItemContainer::set_display_mode));
    }

    division->set_display_mode(wb::OverviewBE::MSmallIcon);
  } else {
    // content_box->set_border_width(12);
    // for (int i= 0; i < _overview_be->count_children(pnode); i++)
    //{
    // bec::NodeId node= _overview_be->get_child(pnode, i);

    //  build_node(content_box, node);
    //}
  }

  division->toggle(default_expanded);
  division->show();
}

//------------------------------------------------------------------------------
void OverviewPanel::build_group(OverviewDivision *division, OverviewGroupContainer *group_container,
                                const bec::NodeId &pnode, int position) {
  Gtk::Box *page = group_container->add_group(pnode, position);

  page->set_spacing(8);

  build_group_contents(division, page, pnode);
}

void OverviewPanel::build_group_contents(OverviewDivision *division, Gtk::Box *page, const bec::NodeId &pnode) {
  ssize_t type;
  _overview_be->get_field(pnode, wb::OverviewBE::ChildNodeType, type);
  if (type != wb::OverviewBE::OSection)
    throw std::logic_error("unexpected child node type");

  std::string child_name;
  for (size_t i = 0; i < _overview_be->count_children(pnode); i++) {
    bec::NodeId node = _overview_be->get_child(pnode, i);
    OverviewSection *section = Gtk::manage(new OverviewSection(_overview_be, node, true, true));
    section->signal_selection_changed().connect(
      sigc::bind(sigc::mem_fun(this, &OverviewPanel::item_list_selection_changed), section));
    section->signal_popup_menu().connect(sigc::bind(sigc::mem_fun(this, &OverviewPanel::item_popup_menu), section));
    _item_containers_by_id[node.toString()] = section;

    page->pack_start(*section, false, false);
    section->show();

    division->signal_view_mode_change().connect(sigc::mem_fun(*section, &OverviewItemContainer::set_display_mode));

    _overview_be->get_field(node, wb::OverviewBE::Label, child_name);

    std::string drag_type = _overview_be->get_node_drag_type(node);
    if (!drag_type.empty()) {
      section->enable_drag_drop_type(drag_type);
    }
  }
}

void OverviewPanel::pre_refresh_groups() {
  for (int i = _groups->get_n_pages() - 1; i >= 0; --i) {
    OverviewGroup *group = (OverviewGroup *)_groups->get_nth_page(i);
    bool was_focus_node_enabled = _groups->enable_set_focus_node();
    _groups->enable_set_focus_node(false);
    group->hide();
    group->invalidate();
    _groups->remove_page(*group);
    _groups->enable_set_focus_node(was_focus_node_enabled);
  }
}

void OverviewPanel::refresh_active_group_node_children() {
  bec::NodeId group_node_id = _groups->node();
  ssize_t current_page_index = _groups->current_page_index();
  if (-1 == current_page_index || _groups->get_n_pages() < (ssize_t)_overview_be->count_children(group_node_id)) {
    rebuild_all();
    select_default_group_page();
    current_page_index = _groups->current_page_index();
  }
  if ((current_page_index != -1) && (current_page_index < (ssize_t)_overview_be->count_children(group_node_id))) {
    bec::NodeId schema_node_id = _overview_be->get_child(group_node_id, current_page_index);

    _overview_be->refresh_node(group_node_id, true);

    OverviewGroupContainer *group_container = _group_containers_by_id[group_node_id.toString()];
    OverviewDivision *division = dynamic_cast<OverviewDivision *>(group_container->get_parent());
    bec::NodeId node(group_node_id);

    _overview_be->focus_node(schema_node_id);

    std::string schema_uid = _overview_be->get_node_unique_id(schema_node_id);

    for (int i = group_container->get_n_pages() - 1; i >= 0; --i) {
      OverviewGroup *group = (OverviewGroup *)group_container->get_nth_page(i);
      std::string uid = group->get_unique_id();

      if (uid == schema_uid) {
        delete_container_contents(group);

        build_group_contents(division, group, schema_node_id);

        division->set_display_mode(wb::OverviewBE::MSmallIcon);
        break;
      }
    }
  }
}

void OverviewPanel::update_group_note(OverviewGroupContainer *group_container, const bec::NodeId &node) {
  OverviewDivision *division = dynamic_cast<OverviewDivision *>(group_container->get_parent());

  for (int i = group_container->get_n_pages() - 1; i >= 0; --i) {
    OverviewGroup *group = (OverviewGroup *)group_container->get_nth_page(i);
    std::string uid = group->get_unique_id();
    /*
    bool found= false;

    // check if the group still exists
    for (size_t k= _overview_be->count_children(node), j= 0; j < k; j++)
    {
      bec::NodeId child(_overview_be->get_child(node, j));

      if (_overview_be->get_node_unique_id(child) == uid)
      {
        found= true;
        break;
      }
    }
    // group is gone, remove it it
     if (!found)*/
    // just remove everything then rebuild everything from scratch
    {
      bool was_focus_node_enabled = _groups->enable_set_focus_node();
      _groups->enable_set_focus_node(false);
      group->hide();
      group->invalidate();

      std::vector<Gtk::Widget *> children(group->get_children());
      std::map<std::string, OverviewItemContainer *>::iterator it;
      for (std::vector<Gtk::Widget *>::iterator iter = children.begin(); iter != children.end(); ++iter) {
        OverviewItemContainer *items = dynamic_cast<OverviewItemContainer *>(*iter);
        if (items) {
          it = _item_containers_by_id.find(items->get_base_node().toString());
          if (it != _item_containers_by_id.end()) {
            _item_containers_by_id.erase(it);
          }
        }
      }

      group_container->remove_page(*group);
      _groups->enable_set_focus_node(was_focus_node_enabled);
    }
  }

  _overview_be->focus_node(node);

  //  int j= 0;
  // go through groups and insert the ones that don't exit yet
  for (size_t c = _overview_be->count_children(node), i = 0; i < c; i++) {
    bec::NodeId child(_overview_be->get_child(node, i));
    /*
    std::string uid= _overview_be->get_node_unique_id(child);
    bool found= false;

    if (group_container->get_n_pages() > j)
    {
      OverviewGroup *group= (OverviewGroup*)group_container->get_nth_page(j);

      if (group->get_unique_id() == uid)
      {
        group->update_base_node(child);
        found= true;
        ++j;
        continue;
      }
    }
    if (!found)
     */
    {
      build_group(division, group_container, child, -1);

      division->set_display_mode(wb::OverviewBE::MSmallIcon);
    }
  }
}

//------------------------------------------------------------------------------
void OverviewPanel::select_default_group_page() {
  if (!_groups)
    return;

  int default_page_index = _overview_be->get_default_tab_page_index();
  if (-1 != default_page_index) {
    if (default_page_index < _groups->get_n_pages()) {
      _groups->set_current_page(default_page_index);
    }
  }
}

//------------------------------------------------------------------------------
void OverviewPanel::refresh_children(const bec::NodeId &node) {
  ssize_t type;

  if (!node.is_valid()) {
    rebuild_all();
    return;
  }

  if (_freeze)
    return;

  _overview_be->refresh_node(node, true);

  _overview_be->get_field(node, wb::OverviewBE::ChildNodeType, type);

  switch ((wb::OverviewBE::OverviewNodeType)type) {
    case wb::OverviewBE::OGroup:
      update_group_note(_group_containers_by_id[node.toString()], node);
      if (_groups->enable_set_focus_node()) {
        if ((_groups->current_page_index() >= 0) &&
            (_groups->current_page_index() < (ssize_t)_overview_be->count_children(_groups->node()))) {
          _overview_be->focus_node(_overview_be->get_child(_groups->node(), _groups->current_page_index()));
        }
      }
      break;

    case wb::OverviewBE::OItem:
      _item_containers_by_id[node.toString()]->refresh();
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------
void OverviewPanel::refresh_node(const bec::NodeId &node) {
  if (_freeze)
    return;

  ssize_t type;
  bec::NodeId parent(_overview_be->get_parent(node));
  _overview_be->get_field(node, wb::OverviewBE::NodeType, type);

  _overview_be->refresh_node(node, false);

  switch ((wb::OverviewBE::OverviewNodeType)type) {
    case wb::OverviewBE::OGroup:
      _group_containers_by_id[parent.toString()]->refresh_info(node);
      break;

    case wb::OverviewBE::OItem:
      //_item_containers_by_id[parent.toString()]->refresh_info(node);
      // so that renaming a node will refresh the order too, if needed
      refresh_children(parent);
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------
void OverviewPanel::select_node(const bec::NodeId &node) {
  if (_freeze)
    return;

  ssize_t type;
  bec::NodeId parent(_overview_be->get_parent(node));
  _overview_be->get_field(node, wb::OverviewBE::NodeType, type);

  switch ((wb::OverviewBE::OverviewNodeType)type) {
    case wb::OverviewBE::OItem:
      _item_containers_by_id[parent.toString()]->select_node(node);
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------
void OverviewPanel::item_popup_menu(const Gtk::TreeModel::Path &path, guint32 time, OverviewItemContainer *sender) {
  bec::NodeId node(sender->get_base_node());

  std::for_each(path.begin(), path.end(), sigc::mem_fun(node, &bec::NodeId::append));

  std::vector<bec::NodeId> nodes(1);
  nodes.push_back(node);

  bec::MenuItemList menuitems = _overview_be->get_popup_items_for_nodes(nodes);
  if (!menuitems.empty())
    run_popup_menu(menuitems, time,
                   sigc::mem_fun(wb::WBContextUI::get()->get_command_ui(),
                                 (void (wb::CommandUI::*)(const std::string &)) & wb::CommandUI::activate_command),
                   &_context_menu);
}
