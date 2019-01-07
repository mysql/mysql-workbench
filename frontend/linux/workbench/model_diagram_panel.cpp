/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/../gtk/lf_view.h"
#include <gtk/gtkpaned.h>

#include "model_diagram_panel.h"
#include "image_cache.h"
#include "grtdb/db_object_helpers.h"
#include "model/wb_context_model.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_context.h"
#include "mforms/../gtk/lf_toolbar.h"
#include "mforms/../gtk/lf_menubar.h"

#include "properties_tree.h"
#include "navigator_box.h"
#include "documentation_box.h"
#include "base/string_utilities.h"

#include "gtk_helpers.h"

#include <gtkmm.h>
#include <gdkmm/cursor.h>
#include <gdk/gdkkeysyms.h>

#include "base/log.h"
DEFAULT_LOG_DOMAIN("fe")

class CanvasViewer : public mdc::GtkCanvas {
  wb::ModelDiagramForm *_be;

  // override the event handlers from the base canvas widget
  virtual bool on_button_press_event(GdkEventButton *event) {
    mdc::MouseButton button = mdc::ButtonLeft;

    grab_focus();

    switch (event->button) {
      case 1:
        button = mdc::ButtonLeft;
        break;
      case 2:
        button = mdc::ButtonMiddle;
        break;
      case 3:
        button = mdc::ButtonRight;
        break;
    }

    if (event->type == GDK_2BUTTON_PRESS)
      _be->handle_mouse_double_click(button, event->x, event->y, get_event_state(event->state));
    else
      _be->handle_mouse_button(button, true, event->x, event->y, get_event_state(event->state));

    return true;
  }

  virtual bool on_button_release_event(GdkEventButton *event) {
    mdc::MouseButton button = mdc::ButtonLeft;

    switch (event->button) {
      case 1:
        button = mdc::ButtonLeft;
        break;
      case 2:
        button = mdc::ButtonMiddle;
        break;
      case 3:
        button = mdc::ButtonRight;
        break;
    }

    _be->handle_mouse_button(button, false, event->x, event->y, get_event_state(event->state));

    return true;
  }

  virtual bool on_motion_notify_event(GdkEventMotion *event) {
    _be->handle_mouse_move(event->x, event->y, get_event_state(event->state));

    return true;
  }

  mdc::KeyInfo getKeyInfo(GdkEventKey *e) {
    static struct KeyCodeMapping {
      guint key;
      mdc::KeyCode kcode;
    } keycodes[] = {{GDK_KEY_BackSpace, mdc::KBackspace},
                    {GDK_KEY_Delete, mdc::KDelete},
                    {GDK_KEY_Down, mdc::KDown},
                    {GDK_KEY_End, mdc::KEnd},
                    {GDK_KEY_Return, mdc::KEnter},
                    {GDK_KEY_Escape, mdc::KEscape},
                    {GDK_KEY_F1, mdc::KF1},
                    {GDK_KEY_F2, mdc::KF2},
                    {GDK_KEY_F3, mdc::KF3},
                    {GDK_KEY_F4, mdc::KF4},
                    {GDK_KEY_F5, mdc::KF5},
                    {GDK_KEY_F6, mdc::KF6},
                    {GDK_KEY_F7, mdc::KF7},
                    {GDK_KEY_F8, mdc::KF8},
                    {GDK_KEY_F9, mdc::KF9},
                    {GDK_KEY_F10, mdc::KF10},
                    {GDK_KEY_F11, mdc::KF11},
                    {GDK_KEY_F12, mdc::KF12},
                    {GDK_KEY_Home, mdc::KHome},
                    {GDK_KEY_Insert, mdc::KInsert},
                    {GDK_KEY_Left, mdc::KLeft},
                    {GDK_KEY_Next, mdc::KPageDown},
                    {GDK_KEY_Page_Down, mdc::KPageDown},
                    {GDK_KEY_Page_Up, mdc::KPageUp},
                    {GDK_KEY_Prior, mdc::KPageUp},
                    {GDK_KEY_Return, mdc::KEnter},
                    {GDK_KEY_Shift_L, mdc::KShift},
                    {GDK_KEY_Shift_R, mdc::KShift},
                    {GDK_KEY_Tab, mdc::KTab},

                    {GDK_KEY_plus, mdc::KPlus},
                    {GDK_KEY_minus, mdc::KMinus},
                    {GDK_KEY_space, mdc::KSpace},
                    {GDK_KEY_period, mdc::KPeriod},
                    {GDK_KEY_comma, mdc::KComma},
                    {GDK_KEY_semicolon, mdc::KSemicolon}};

    mdc::KeyInfo k;

    k.keycode = mdc::KNone;
    k.string = "";
    for (unsigned int i = 0; i < sizeof(keycodes) / sizeof(*keycodes); i++) {
      if (keycodes[i].key == e->keyval) {
        k.keycode = keycodes[i].kcode;
        break;
      }
    }

    if (k.keycode == 0 && e->string) {
      k.string = e->string;
    }

    return k;
  }

  virtual bool on_key_press_event(GdkEventKey *event) {
    mdc::KeyInfo key = getKeyInfo(event);
    mdc::EventState state = get_event_state(event->state);

    _be->handle_key(key, true, state);
    return true;
  }

  virtual bool on_key_release_event(GdkEventKey *event) {
    mdc::KeyInfo key = getKeyInfo(event);
    mdc::EventState state = get_event_state(event->state);

    _be->handle_key(key, false, state);

    return true;
  }

  virtual void on_zoom_in_event() {
    _be->zoom_in();
  }

  virtual void on_zoom_out_event() {
    _be->zoom_out();
  }

public:
  CanvasViewer(wb::ModelDiagramForm *be, bool use_gl)
    : mdc::GtkCanvas(use_gl ? mdc::GtkCanvas::OpenGLCanvasType : mdc::GtkCanvas::BufferedXlibCanvasType), _be(be) {
  }
};

//--------------------------------------------------------------------------------

ModelDiagramPanel::InlineEditor::InlineEditor(ModelDiagramPanel *owner) : _owner(owner) {
  _editing = false;
  _edit_field = Gtk::manage(new Gtk::Entry());
}

void ModelDiagramPanel::InlineEditor::begin_editing(int x, int y, int width, int height, const std::string &text) {
  if (!_edit_field->get_parent()) {
    _owner->_canvas->put(*_edit_field, x, y);
  }
  _edit_field->grab_focus();
  _edit_field->select_region(0, -1);
  _edit_field->set_text(text);
  _edit_field->show();
}

void ModelDiagramPanel::InlineEditor::end_editing() {
  _edit_field->hide();
}

void ModelDiagramPanel::InlineEditor::set_font_size(float size) {
}

void ModelDiagramPanel::InlineEditor::set_multiline(bool flag) {
}

//--------------------------------------------------------------------------------

ModelDiagramPanel *ModelDiagramPanel::create() {
  Glib::RefPtr<Gtk::Builder> xml =
    Gtk::Builder::create_from_file(bec::GRTManager::get()->get_data_file_path("diagram_view.glade"));

  ModelDiagramPanel *panel = 0;
  xml->get_widget_derived("diagram_pane", panel);
  panel->post_construct();

  return panel;
}

void ModelDiagramPanel::on_activate() {
  mforms::View *sidebar = wb::WBContextUI::get()->get_wb()->get_model_context()->shared_secondary_sidebar();
  Gtk::Widget *w = mforms::widget_for_view(sidebar);
  Gtk::Frame *secondary_sidebar;
  _xml->get_widget("sidebar_frame", secondary_sidebar);
  if (w->get_parent())
    gtk_reparent_realized(w, secondary_sidebar);
  else
    secondary_sidebar->add(*w);
  w->show();
}

ModelDiagramPanel::ModelDiagramPanel(GtkPaned *paned, const Glib::RefPtr<Gtk::Builder> &builder)
  : Gtk::Paned(paned),
    FormViewBase("ModelDiagram"),
    _top_box(Gtk::ORIENTATION_VERTICAL, 0),
    _tools_toolbar(0),
    _vbox(0),
    _diagram_hbox(0),
    _be(0),
    _canvas(0),
    _cursor(0),
    _inline_editor(this),
    _editor_paned(0),
    _sidebar(0),
    _side_model_pane2(0),
    _navigator_box(0),
    _catalog_tree(0),
    _usertypes_list(0),
    _history_list(0),
    _documentation_box(0),
    _properties_tree(0),
    _xml(builder) {
}

void ModelDiagramPanel::post_construct() {
  _diagram_hbox = 0;
  _xml->get_widget("diagram_hbox", _diagram_hbox);
  _top_box.show();

  _top_box.pack_end(*this, true, true);
  show();

  _xml->get_widget("diagram_pane", _sidebar1_pane);
  _xml->get_widget("diagram_pane2", _sidebar2_pane);

  // xml->get_widget("tools_toolbar", _tools_toolbar);
  _xml->get_widget("diagram_box", _vbox);

  _xml->get_widget("editor_note", _editor_note);
  _xml->get_widget("content", _editor_paned);

  _xml->get_widget("model_sidebar", _sidebar);
  _xml->get_widget("side_model_pane2", _side_model_pane2);
}

void ModelDiagramPanel::view_realized() {
  _canvas->get_canvas()->set_user_data(dynamic_cast<FormViewBase *>(this));

  // changing zoom or scrolling should cancel any editing in place
  _be->scoped_connect(_canvas->get_canvas()->signal_viewport_changed(),
                      sigc::mem_fun(_be, &wb::ModelDiagramForm::stop_editing));
}

void ModelDiagramPanel::init(const std::string &view_id) {
  _be = wb::WBContextUI::get()->get_wb()->get_model_context()->get_diagram_form_for_diagram_id(view_id);
  _be->set_frontend_data(dynamic_cast<FormViewBase *>(this));
  _canvas = Gtk::manage(new CanvasViewer(_be, false)); //_wb->get_wb()->using_opengl()));

  _toolbar = _be->get_toolbar();

  {
    Gtk::Notebook *note;
    Gtk::Label *label;
    Gtk::Paned *pan;

    //_xml->get_widget("model_sidebar", pan);
    PanedConstrainer::make_constrainer(_sidebar, 64, 90);

    //_xml->get_widget("side_model_pane2", pan);
    PanedConstrainer::make_constrainer(_side_model_pane2, 64, 90);

    _xml->get_widget("diagram_pane", pan);
    PanedConstrainer::make_constrainer(pan, 130, 0);

    // setup sidebar stuff for model
    _xml->get_widget("side_model_note0", note);
    _navigator_box = Gtk::manage(new NavigatorBox());
    label = Gtk::manage(new Gtk::Label(_("<small>Navigator</small>")));
    note->append_page(*_navigator_box, *label);
    label->set_use_markup(true);

    _xml->get_widget("side_model_note1", note);

    _catalog_tree = _be->get_catalog_tree();
    label = Gtk::manage(new Gtk::Label(_("<small>Catalog</small>")));
    note->append_page(*mforms::widget_for_view(_catalog_tree), *label);
    label->set_use_markup(true);

    label = Gtk::manage(new Gtk::Label(_("<small>Layers</small>")));
    note->append_page(*mforms::widget_for_view(_be->get_layer_tree()), *label);
    label->set_use_markup(true);

    _usertypes_list = wb::WBContextUI::get()->get_wb()->get_model_context()->create_user_type_list();
    label = Gtk::manage(new Gtk::Label(_("<small>User Types</small>")));
    note->append_page(*mforms::widget_for_view(_usertypes_list), *label);
    label->set_use_markup(true);

    _xml->get_widget("side_model_note2", note);
    _documentation_box = Gtk::manage(new DocumentationBox());
    label = Gtk::manage(new Gtk::Label(_("<small>Description</small>")));
    note->append_page(*_documentation_box, *label);
    label->set_use_markup(true);

    _properties_tree = Gtk::manage(new PropertiesTree());
    label = Gtk::manage(new Gtk::Label(_("<small>Properties</small>")));
    note->append_page(*_properties_tree, *label);
    label->set_use_markup(true);

    _history_list = wb::WBContextUI::get()->get_wb()->get_model_context()->create_history_tree();
    label = Gtk::manage(new Gtk::Label(_("<small>History</small>")));
    note->append_page(*mforms::widget_for_view(_history_list), *label);
    label->set_use_markup(true);
  }

  {
    mforms::MenuBar *menubar = _be->get_menubar();
    if (menubar) {
      Gtk::Widget *w = mforms::widget_for_menubar(menubar);
      _top_box.pack_start(*w, false, true);
      w->show();
    }
    mforms::ToolBar *toolbar = _be->get_toolbar();
    if (toolbar) {
      Gtk::Widget *w = mforms::widget_for_toolbar(toolbar);
      _top_box.pack_start(*w, false, true);
      w->show();
      _top_box.pack_start(*(w = Gtk::manage(new Gtk::HSeparator)), false, true);
      w->show();
    }

    mforms::ToolBar *tools_toolbar = _be->get_tools_toolbar();
    logDebug3("ModelDiagramPanel::init got tools toolbar %p\n", tools_toolbar);
    if (tools_toolbar) {
      _tools_toolbar = dynamic_cast<Gtk::Box *>(mforms::widget_for_toolbar(tools_toolbar));
      logDebug3("ModelDiagramPanel::init cast _tools_toolbar = %p\n", _tools_toolbar);
      if (_tools_toolbar) {
        _diagram_hbox->pack_start(*_tools_toolbar, false, true);
        _diagram_hbox->reorder_child(*_tools_toolbar, 0);
        _tools_toolbar->show();
      }
    }

    mforms::ToolBar *options_toolbar = _be->get_options_toolbar();
    if (toolbar) {
      Gtk::Widget *w = mforms::widget_for_toolbar(options_toolbar);
      _vbox->pack_start(*w, false, true);
      w->show();
    }
  }

  _be->set_inline_editor_context(&_inline_editor);
  _vbox->pack_start(_scroller, true, true);

  _scroller.add(*_canvas);

  std::vector<Gtk::TargetEntry> targets;
  std::vector<std::string> types = _be->get_accepted_drop_types();
  for (std::vector<std::string>::const_iterator iter = types.begin(); iter != types.end(); ++iter)
    targets.push_back(Gtk::TargetEntry(*iter, Gtk::TARGET_SAME_APP));
  _canvas->drag_dest_set(targets);

  _canvas->signal_drag_motion().connect(sigc::mem_fun(this, &ModelDiagramPanel::drag_motion));
  _canvas->signal_drag_drop().connect(sigc::mem_fun(this, &ModelDiagramPanel::drag_drop));
  _canvas->signal_drag_data_received().connect(sigc::mem_fun(this, &ModelDiagramPanel::drag_data_received));

  _canvas->signal_realize().connect_notify(sigc::mem_fun(this, &ModelDiagramPanel::view_realized));

  show_all();

  get_panel()->signal_show().connect_notify((sigc::bind(sigc::mem_fun(_be, &wb::ModelDiagramForm::set_closed), false)));

  //  _catalog_tree->refresh();

  // restore widths of sidebars when shown
  _sig_restore_sidebar =
    Glib::signal_idle().connect(sigc::bind_return(sigc::bind(sigc::mem_fun(this, &FormViewBase::restore_sidebar_layout), 275, 200), false));

  //  Set the sidebar pane sizes
  _sidebar->set_position(
    bec::GRTManager::get()->get_app_option_int("Sidebar:VBox1:Position", _sidebar->get_position()));

  _side_model_pane2->set_position(
    bec::GRTManager::get()->get_app_option_int("Sidebar:VBox2:Position", _side_model_pane2->get_position()));
}
bool ModelDiagramPanel::drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time) {
  context->drag_status(context->get_suggested_action(), time);

  this->drag_highlight();

  return true;
}

ModelDiagramPanel::~ModelDiagramPanel() {
  _sig_restore_sidebar.disconnect();
  delete _catalog_tree;
  delete _usertypes_list;
  delete _history_list;
}

bool ModelDiagramPanel::drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time) {
  std::vector<std::string> targets(context->list_targets());
  if (!targets.empty())
    drag_get_data(context, targets[0], time);

  return true;
}

void ModelDiagramPanel::drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                                           const Gtk::SelectionData &selection_data, guint, guint time) {
  mforms::gtk::DataWrapper *dwrapper = (mforms::gtk::DataWrapper *)selection_data.get_data();

  if (!dwrapper)
    return;

  std::string tmpstr = std::vector<std::string>(context->list_targets())[0];

  std::string type = selection_data.get_data_type();
  if (type == WB_DBOBJECT_DRAG_TYPE) {
    _be->perform_drop(x, y, type, *reinterpret_cast<std::list<GrtObjectRef> *>(dwrapper->GetData()));
    context->drag_finish(true, false, time);
  } else
    context->drag_finish(false, false, time);
}

static Glib::RefPtr<Gdk::Cursor> load_cursor(const std::string &path) {
  gsize size;
  guint8 *buffer;
  Glib::RefPtr<Gdk::Pixbuf> pixbuf;

  if (g_file_get_contents(path.c_str(), (gchar **)&buffer, &size, NULL)) {
    if (buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 2 || buffer[3] != 0) {
      g_free(buffer);
      return Glib::RefPtr<Gdk::Cursor>();
    }
    // read directory info
    int width = buffer[6 + 0];
    int height = buffer[6 + 1];
    int colors = buffer[6 + 2];
    int xspot = buffer[6 + 4] | buffer[6 + 5] << 8;
    int yspot = buffer[6 + 6] | buffer[6 + 7] << 8;
    gint32 bmp_size = GINT32_FROM_LE(*(guint32 *)&buffer[6 + 8]);
    gint32 bmp_offset = GINT32_FROM_LE(*(guint32 *)&buffer[6 + 12]);

    if (colors == 2 && width == 32 && height == 32 && bmp_offset + bmp_size <= (gint32)size) {
      // read dib info
      guint32 bi_size = GINT32_FROM_LE(*(guint32 *)&buffer[bmp_offset + 0]);
      gint bi_width = GINT32_FROM_LE(*(guint32 *)&buffer[bmp_offset + 4]);
      gint bi_height = GINT32_FROM_LE(*(guint32 *)&buffer[bmp_offset + 8]);
      // gint16 bi_planes; // 12
      gint16 bi_bitcount = GINT16_FROM_LE(*(guint16 *)&buffer[bmp_offset + 14]);
      // guint32 bi_compression; // 16
      // guint32 bi_sizeimage= GUINT32_FROM_LE(*(guint32*)&buffer[bmp_offset+20]);
      // gint32 bi_resx, bi_resy;
      // guint32 bi_clrused;
      // guint32 bi_clrimportant;

      guint8 *color_table = &buffer[bmp_offset + bi_size];

      const guint8 *xor_mask = (guint8 *)&buffer[bmp_offset + bi_size + 4 * colors];
      const guint8 *and_mask = (guint8 *)&buffer[bmp_offset + bi_size + 4 * colors + bi_width * bi_height / 16];

      guint8 *image = (guint8 *)g_malloc0(width * height * 4);
      int bytes_per_line = (((bi_width * bi_bitcount) + 31) & ~31) >> 3;

      for (int y = 0; y < height; y++) {
        const guint8 *and_bytes = and_mask + y * bytes_per_line;
        const guint8 *xor_bytes = xor_mask + y * bytes_per_line;

        for (int x = 0; x < width; x++) {
          int offs = ((height - 1 - y) * width + x) * 4;

          if ((xor_bytes[x / 8] & (0x80 >> (x % 8))) != 0) {
            image[offs + 0] = color_table[1 * 4 + 0];
            image[offs + 1] = color_table[1 * 4 + 1];
            image[offs + 2] = color_table[1 * 4 + 2];
          } else {
            image[offs + 0] = color_table[0 * 4 + 0];
            image[offs + 1] = color_table[0 * 4 + 1];
            image[offs + 2] = color_table[0 * 4 + 2];
          }

          if ((and_bytes[x / 8] & (0x80 >> (x % 8))) != 0)
            image[offs + 3] = 0;
          else
            image[offs + 3] = 0xff;
        }
      }

      pixbuf = Gdk::Pixbuf::create_from_data(image, Gdk::COLORSPACE_RGB, TRUE, 8, width, height, width * 4,
                                             sigc::ptr_fun((void (*)(const guint8 *))g_free));
    } else
      g_message("unsupported icon format in %s", path.c_str());

    g_free(buffer);

    if (pixbuf)
      return Gdk::Cursor::create(Gdk::Display::get_default(), pixbuf, xspot, yspot);
    return Glib::RefPtr<Gdk::Cursor>();
  }
  return Glib::RefPtr<Gdk::Cursor>();
}

void ModelDiagramPanel::update_tool_cursor() {
  if (_canvas->get_realized()) {
    std::string cursor = _be->get_cursor();
    std::string path = bec::IconManager::get_instance()->get_icon_path(cursor + ".png");

    _be->get_options_toolbar(); // refresh options toolbar

    if (path.empty()) {
      _canvas->get_window()->set_cursor();
      return;
    }

    _cursor = load_cursor(path);
    if (_cursor)
      _canvas->get_window()->set_cursor(_cursor);
    else
      _canvas->get_window()->set_cursor();
  }
}

bool ModelDiagramPanel::on_close() {
  bec::GRTManager::get()->set_app_option("Sidebar:VBox1:Position", grt::IntegerRef(_sidebar->get_position()));

  bec::GRTManager::get()->set_app_option("Sidebar:VBox2:Position", grt::IntegerRef(_side_model_pane2->get_position()));

  _be->set_closed(true);
  get_parent()->hide(); // hide the container

  return false; // don't close
}

void ModelDiagramPanel::refresh_catalog(bool hard) {
}

void ModelDiagramPanel::setup_navigator() {
  _navigator_box->set_model(_be);
}

void ModelDiagramPanel::refresh_zoom() {
  _navigator_box->refresh();
}

void ModelDiagramPanel::selection_changed() {
  _properties_tree->update();

  _documentation_box->update_for_form(_be);
}

void ModelDiagramPanel::find_text(const std::string &text) {
  _be->search_and_focus_object(text);
}
