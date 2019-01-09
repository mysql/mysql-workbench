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

#include <errno.h>
#include "wb_component_basic.h"
#include "wb_context_model.h"
#include "model/wb_model_diagram_form.h"
#include "workbench/wb_context.h"
#include "grts/structs.workbench.model.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/file_functions.h"
#include "grt/clipboard.h"

using namespace std;

using namespace wb;
using namespace grt;
using namespace bec;
using namespace base;

struct HandToolContext {
  Point mouse_pos;
  Point viewport_pos;
  bool moving;

  HandToolContext() : mouse_pos(0., 0.), viewport_pos(0, 0), moving(false) {
  }
};

WBComponentBasic::WBComponentBasic(WBContext *wb) : WBComponent(wb) {
}

WBComponentBasic::~WBComponentBasic() {
}

void WBComponentBasic::load_app_options(bool update) {
  if (!update) {
    app_ToolbarRef options_toolbar;

    options_toolbar = app_ToolbarRef::cast_from(
      grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/model_option_toolbar_layer.xml")));
    _toolbars[options_toolbar->name()] = options_toolbar;

    options_toolbar = app_ToolbarRef::cast_from(
      grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/model_option_toolbar_note.xml")));
    _toolbars[options_toolbar->name()] = options_toolbar;

    _shortcuts = grt::ListRef<app_ShortcutItem>::cast_from(
      grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/shortcuts_basic.xml")));
  }
}

app_ToolbarRef WBComponentBasic::get_tools_toolbar() {
  return app_ToolbarRef::cast_from(
    grt::GRT::get()->unserialize(base::makePath(_wb->get_datadir(), "data/tools_toolbar_basic.xml")));
}

app_ToolbarRef WBComponentBasic::get_tool_options(const std::string &tool) {
  if (_toolbars.find("options/" + tool) != _toolbars.end())
    return _toolbars["options/" + tool];
  return app_ToolbarRef();
}

void WBComponentBasic::setup_canvas_tool(ModelDiagramForm *view, const std::string &tool) {
  void *data = 0;

  if (tool == WB_TOOL_SELECT) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("select_dark");
    } else {
      view->set_cursor("select");
    }
    _wb->_frontendCallbacks->show_status_text("");
  } else if (tool == WB_TOOL_HAND) {
    data = new HandToolContext;
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("hand_dark");
    } else {
      view->set_cursor("hand");
    }
    _wb->_frontendCallbacks->show_status_text(_("Drag the canvas to move it around."));
  } else if (tool == WB_TOOL_DELETE) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("rubber_dark");
    } else {
      view->set_cursor("rubber");
    }
    _wb->_frontendCallbacks->show_status_text(_("Click the object to delete."));
  } else if (tool == WB_TOOL_LAYER) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("layer_dark");
    } else {
      view->set_cursor("layer");
    }
    _wb->_frontendCallbacks->show_status_text(_("Select an area for the new layer."));
  } else if (tool == WB_TOOL_NOTE) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("note_dark");
    } else {
      view->set_cursor("note");
    }
    _wb->_frontendCallbacks->show_status_text(_("Select an area for a text object."));
  } else if (tool == WB_TOOL_IMAGE) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("image_dark");
    } else {
      view->set_cursor("image");
    }
    _wb->_frontendCallbacks->show_status_text(_("Select a location for the image object."));
  } else if (tool == WB_TOOL_ZOOM_IN) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("zoom_in_dark");
    } else {
      view->set_cursor("zoom_in");
    }
    _wb->_frontendCallbacks->show_status_text(_("Left-click anywhere on the diagram to zoom in."));
  } else if (tool == WB_TOOL_ZOOM_OUT) {
    if (mforms::App::get()->isDarkModeActive()) {
      view->set_cursor("zoom_out_dark");
    } else {
      view->set_cursor("zoom_out");
    }
    _wb->_frontendCallbacks->show_status_text(_("Left-click anywhere on the diagram to zoom out."));
  } else {
    _wb->_frontendCallbacks->show_status_text("Invalid tool " + tool);
    return;
  }

  view->set_button_callback(std::bind(&WBComponentBasic::handle_button_event, this, std::placeholders::_1,
                                      std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                                      std::placeholders::_5, data));
  view->set_motion_callback(std::bind(&WBComponentBasic::handle_motion_event, this, std::placeholders::_1,
                                      std::placeholders::_2, std::placeholders::_3, data));
  view->set_reset_tool_callback(std::bind(&WBComponentBasic::reset_tool, this, std::placeholders::_1, data));
}

std::vector<std::string> WBComponentBasic::get_command_dropdown_items(const std::string &option) {
  std::vector<std::string> items;
  if (base::hasSuffix(option, ":Color")) {
    std::string colors = _wb->get_wb_options().get_string("workbench.model.Figure:ColorList");
    std::vector<std::string> colorList;

    colorList = base::split(colors, "\n");

    if (!colorList.empty()) {
      for (std::size_t c = colorList.size(), i = 0; i < c; i++) {
        if (!colorList[i].empty() && colorList[i][0] == '#')
          items.push_back(colorList[i]);
      }
    } else {
      items.push_back("#FFAAAA");
      items.push_back("#FFFFAA");
      items.push_back("#AAFFFF");
    }
    ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_form());
    if (form) {
      std::string selected = form->get_tool_argument(option);
      if (selected.empty())
        selected = _wb->get_wb_options().get_string(option);

      if (!selected.empty() && std::find(items.begin(), items.end(), selected) == items.end())
        items.push_back(selected);

      form->set_tool_argument(option, selected);
    }
  } else
    throw std::logic_error("Unknown option " + option);
  return items;
}

grt::ListRef<app_ShortcutItem> WBComponentBasic::get_shortcut_items() {
  return _shortcuts;
}

//--------------------------------------------------------------------------------
// Object Placement

bool WBComponentBasic::handles_figure(const model_ObjectRef &object) {
  if (object.is_instance(model_Layer::static_class_name()) ||
      object.is_instance(workbench_model_NoteFigure::static_class_name()) ||
      object.is_instance(workbench_model_ImageFigure::static_class_name()))
    return true;
  return false;
}

grt::ValueRef WBComponentBasic::place_layer(ModelDiagramForm *form, const Rect &rect) {
  try {
    model_DiagramRef view(form->get_model_diagram());

    std::string color;

    if (form->get_tool_argument("workbench.model.Layer:Color").empty())
      color = _wb->get_wb_options().get_string("workbench.model.Layer:Color");
    else
      color = form->get_tool_argument("workbench.model.Layer:Color");

    model_LayerRef layer;

    grt::AutoUndo undo;

    layer = view->placeNewLayer(rect.left(), rect.top(), rect.width(), rect.height(), _("New Layer"));

    if (layer.is_valid())
      layer->color(color);

    undo.end(_("Place New Layer"));

    _wb->_frontendCallbacks->show_status_text(_("Created new layer."));
    return layer;
  } catch (std::exception &exc) {
    _wb->show_exception(_("Place new layer"), exc);
    return grt::ValueRef();
  }
}

//-------------------------------------------------------------------------------------
// Deletion

/* unused
void WBComponentBasic::delete_selection()
{
  ModelDiagramForm *form;

  if ((form= dynamic_cast<ModelDiagramForm*>(_wb->get_active_form())))
  {
    grt::ListRef<model_Object> selection= form->get_selection();

    std::vector<model_ObjectRef> objects;

    for (size_t c= selection.count(), i= 0; i < c; i++)
    {
      if (selection.get(i).is_instance(model_Object::static_class_name()))
        objects.push_back(model_ObjectRef::cast_from(selection.get(i)));
    }

    for (size_t c= objects.size(), i= 0; i < c; i++)
      _wb->get_model_context()->delete_object(objects[i]);
  }
}*/

bool WBComponentBasic::delete_model_object(const model_ObjectRef &object, bool figure_only) {
  grt::AutoUndo undo;

  if (object.is_instance(model_Figure::static_class_name())) {
    model_FigureRef figure(model_FigureRef::cast_from(object));

    figure->layer()->figures().remove_value(figure);
    model_DiagramRef::cast_from(figure->owner())->figures().remove_value(figure);

    undo.end(strfmt(_("Delete '%s' Figure"), figure.get_metaclass()->get_attribute("caption").c_str()));
  } else if (object.is_instance(model_Layer::static_class_name())) {
    model_LayerRef layer(model_LayerRef::cast_from(object));
    model_DiagramRef view(model_DiagramRef::cast_from(layer->owner()));

    view->deleteLayer(layer);

    undo.end(strfmt(_("Delete '%s' Layer"), layer.get_metaclass()->get_attribute("caption").c_str()));
  } else
    return false;

  return true;
}

void WBComponentBasic::delete_object(ModelDiagramForm *view, const Point &pos) {
  model_ObjectRef object(view->get_object_at(pos));

  if (object.is_valid()) {
    if (_wb->get_model_context()->remove_figure(object))
      _wb->_frontendCallbacks->show_status_text(
        strfmt(_("Removed %s"), object.get_metaclass()->get_attribute("caption").c_str()));
  }
}

void WBComponentBasic::copy_object_to_clipboard(const grt::ObjectRef &object, grt::CopyContext &copy_context) {
  std::set<std::string> skip;
  skip.insert("oldName");
  //  skip.insert("filename");

  // copy table
  grt::ObjectRef copy = copy_context.copy(object, skip);

  bec::Clipboard *clip = get_wb()->get_clipboard();
  clip->append_data(copy);
}

bool WBComponentBasic::can_paste_object(const grt::ObjectRef &object) {
  if (object.is_instance(workbench_model_NoteFigure::static_class_name()) ||
      object.is_instance(workbench_model_ImageFigure::static_class_name()) ||
      object.is_instance(model_Layer::static_class_name()))
    return true;
  return false;
}

static void get_component_that_can_paste(WBComponent *compo, const grt::ObjectRef &object, WBComponent **result) {
  if (compo->can_paste_object(object))
    *result = compo;
}

model_ObjectRef WBComponentBasic::paste_object(ModelDiagramForm *view, const grt::ObjectRef &object,
                                               grt::CopyContext &copy_context) {
  model_ObjectRef copy;
  model_LayerRef destlayer(view->get_model_diagram()->rootLayer());
  grt::AutoUndo undo;

  if (object.is_instance(model_Figure::static_class_name())) {
    std::set<std::string> skip;
    model_FigureRef original(model_FigureRef::cast_from(object));
    model_FigureRef figure(model_FigureRef::cast_from(grt::copy_object(object, skip)));

    if (destlayer.is_valid()) {
      figure->layer(destlayer);
      figure->owner(destlayer->owner());

      {
        if (grt::find_named_object_in_list(destlayer->owner()->figures(), original->name()).is_valid())
          figure->name(
            grt::get_name_suggestion_for_list_object(destlayer->owner()->figures(), *original->name() + "_copy"));

        destlayer->owner()->addFigure(figure);
        undo.end("Duplicate Object");
      }
    } else {
      figure->owner(model_DiagramRef());
      figure->layer(model_LayerRef());
    }
    copy = figure;
  } else if (object.is_instance(model_Layer::static_class_name())) {
    std::set<std::string> skip;

    skip.insert("figures");

    model_LayerRef original(model_LayerRef::cast_from(object));

    model_LayerRef layer_copy(model_LayerRef::cast_from(copy_context.copy(object, skip)));

    if (destlayer.is_valid())
      layer_copy->owner(destlayer->owner());

    if (grt::find_named_object_in_list(destlayer->owner()->layers(), original->name()).is_valid())
      layer_copy->name(
        grt::get_name_suggestion_for_list_object(destlayer->owner()->layers(), *original->name() + "_copy"));

    destlayer->owner()->layers().insert(layer_copy);
    undo.end("Duplicate Object");

    // copy contained stuff that's to be duplicated
    for (std::size_t c = original->figures()->count(), i = 0; i < c; i++) {
      model_FigureRef figure(original->figures()[i]);

      // paste the object
      WBComponent *compo = 0;
      _wb->foreach_component(std::bind(get_component_that_can_paste, std::placeholders::_1, figure, &compo));
      if (compo) {
        model_ObjectRef tmp = compo->paste_object(view, figure, copy_context);
        if (tmp.is_valid() && model_FigureRef::can_wrap(tmp)) {
          model_FigureRef tmpf = model_FigureRef::cast_from(tmp);

          if (tmpf->layer() != layer_copy) {
            tmpf->layer()->figures().remove_value(tmpf);
            tmpf->layer(layer_copy);
            layer_copy->figures().insert(tmpf);

            tmpf->top(figure->top());
            tmpf->left(figure->left());
          }
        }
      }
    }

    copy_context.update_references();

    copy = layer_copy;
  }

  return copy;
}

void WBComponentBasic::activate_canvas_object(const model_ObjectRef &figure, bool newwindow) {
  if (figure.is_instance(workbench_model_NoteFigure::static_class_name()))
    bec::GRTManager::get()->open_object_editor(figure, newwindow ? bec::ForceNewWindowFlag : bec::NoFlags);
  else if (figure.is_instance(workbench_model_ImageFigure::static_class_name()))
    bec::GRTManager::get()->open_object_editor(figure, newwindow ? bec::ForceNewWindowFlag : bec::NoFlags);
  else if (figure.is_instance(model_Layer::static_class_name()))
    bec::GRTManager::get()->open_object_editor(figure, newwindow ? bec::ForceNewWindowFlag : bec::NoFlags);
}

//-------------------------------------------------------------------------------------
// Handlers

bool WBComponentBasic::handle_button_event(ModelDiagramForm *view, mdc::MouseButton button, bool press, Point pos,
                                           mdc::EventState state, void *data) {
  std::string tool = view->get_tool();

  if (tool == WB_TOOL_HAND && (button == mdc::ButtonLeft || button == mdc::ButtonMiddle)) {
    HandToolContext *hcontext = reinterpret_cast<HandToolContext *>(data);
    hcontext->moving = press;
    if (press) {
      Rect r = view->get_view()->get_viewport();
      int wx, wy;
      hcontext->viewport_pos = r.pos;
      view->get_view()->canvas_to_window(pos, wx, wy);
      hcontext->mouse_pos = Point(wx, wy);
    }
    return true;
  }

  if (button != mdc::ButtonLeft)
    return false;

  if (tool == WB_TOOL_DELETE) {
    if (press)
      delete_object(view, pos);
    return true;
  } else if (tool == WB_TOOL_LAYER) {
    if (press)
      view->get_view()->start_dragging_rectangle(pos);
    else {
      Rect rect = view->get_view()->finish_dragging_rectangle();
      Size max_size = view->get_view()->get_total_view_size();

      if (rect.width() < 5 || rect.height() < 5) {
        _wb->_frontendCallbacks->show_status_text(_("Please select a larger area."));
      } else {
        rect.pos.x = max(rect.pos.x, 0.0);
        rect.pos.y = max(rect.pos.y, 0.0);
        rect.size.width = min(rect.size.width, max_size.width);
        rect.size.height = min(rect.size.height, max_size.height);

        place_layer(view, rect);
        view->reset_tool(true);
      }
    }
    return true;
  } else if (tool == WB_TOOL_NOTE) {
    /*  if (press)
        view->get_view()->start_dragging_rectangle(pos);
      else
      {
        Rect rect= view->get_view()->finish_dragging_rectangle();

        if (rect.width() < 5 || rect.height() < 5)
        {
          _wb->_frontendCallbacks->show_status_text(_("Please select a larger area."));
        }
        else*/
    if (press) {
      workbench_model_NoteFigureRef note(
        workbench_model_NoteFigureRef::cast_from(place_object(view, pos, "workbench.model.NoteFigure")));
      if (note.is_valid()) {
        note->name(grt::get_name_suggestion_for_list_object(
          grt::ObjectListRef::cast_from(view->get_model_diagram()->figures()), "text"));

        note->text("Text");
        // note.width(rect.width());
        // note.height(rect.height());
      }
      view->reset_tool(true);
    }
    //}
    return true;
  } else if (tool == WB_TOOL_IMAGE) {
    if (press) {
      workbench_model_ImageFigureRef image;
      std::string filename =
        _wb->_frontendCallbacks->show_file_dialog("open", _("Place Image"), "PNG Image Files (*.png)|*.png");

      if (!filename.empty()) {
        {
          // if the file is not a PNG, we get a generic "out of memory" error, so try to detect that
          FILE *f = base_fopen(filename.c_str(), "r");
          if (!f) {
            mforms::Utilities::show_error(_("Cannot Open Image File"), g_strerror(errno), _("Close"));
            view->reset_tool(true);
            return true;
          }
          char buffer[10];
          if (fread(buffer, 1, sizeof(buffer), f) < 4 || strncmp(buffer, "\211PNG", 4) != 0) {
            fclose(f);
            mforms::Utilities::show_error(_("Cannot Open Image File"), _("Images must be in PNG format."), _("Close"));
            view->reset_tool(true);
            return true;
          }
          fclose(f);
        }

        image = workbench_model_ImageFigureRef::cast_from(place_object(view, pos, "workbench.model.ImageFigure"));

        if (image.is_valid())
          image->setImageFile(filename);
      }

      view->reset_tool(true);
    }
    return true;
  }
  return false;
}

bool WBComponentBasic::handle_motion_event(ModelDiagramForm *view, Point pos, mdc::EventState state, void *data) {
  std::string tool = view->get_tool();

  if (tool == WB_TOOL_HAND) {
    HandToolContext *hcontext = reinterpret_cast<HandToolContext *>(data);
    mdc::CanvasView *cview = view->get_view();

    if (hcontext->moving) {
      int wx, wy;
      cview->canvas_to_window(pos, wx, wy);
      Point wpos(wx, wy);

      Point mouse_pos = hcontext->mouse_pos;
      Point viewport_pos = hcontext->viewport_pos;

      Point newp =
        viewport_pos + Point((mouse_pos.x - wpos.x) / view->get_zoom(), (mouse_pos.y - wpos.y) / view->get_zoom());

      Rect r = cview->get_viewport();
      Size sz = cview->get_total_view_size();

      if (newp.x < 0)
        newp.x = 0;
      if (newp.y < 0)
        newp.y = 0;
      if (newp.x > (sz.width - r.size.width))
        newp.x = sz.width - r.size.width;
      if (newp.y > (sz.height - r.size.height))
        newp.y = sz.height - r.size.height;

      cview->set_offset(newp);

      return true;
    }
  }
  return false;
}

void WBComponentBasic::reset_tool(ModelDiagramForm *view, void *data) {
  if (view->get_tool() == WB_TOOL_HAND) {
    HandToolContext *hcontext = reinterpret_cast<HandToolContext *>(data);

    delete hcontext;
  }
}
