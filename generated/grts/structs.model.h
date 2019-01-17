/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#ifndef _MSC_VER
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include "grt.h"

#ifdef _MSC_VER
  #pragma warning(disable: 4355) // 'this' : used in base member initializer list
  #ifdef GRT_STRUCTS_MODEL_EXPORT
  #define GRT_STRUCTS_MODEL_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_MODEL_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_MODEL_PUBLIC
#endif

#include "grts/structs.h"
#include "grts/structs.app.h"

class model_Marker;
typedef grt::Ref<model_Marker> model_MarkerRef;
class model_Group;
typedef grt::Ref<model_Group> model_GroupRef;
class model_Object;
typedef grt::Ref<model_Object> model_ObjectRef;
class model_Layer;
typedef grt::Ref<model_Layer> model_LayerRef;
class model_Connection;
typedef grt::Ref<model_Connection> model_ConnectionRef;
class model_Figure;
typedef grt::Ref<model_Figure> model_FigureRef;
class model_Diagram;
typedef grt::Ref<model_Diagram> model_DiagramRef;
class model_Model;
typedef grt::Ref<model_Model> model_ModelRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** a marker storing the active diagram and position on the diagram */
class  model_Marker : public GrtObject {
  typedef GrtObject super;

public:
  model_Marker(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _x(0.0),
      _y(0.0),
      _zoom(0.0) {
  }

  static std::string static_class_name() {
    return "model.Marker";
  }

  /**
   * Getter for attribute diagram
   *
   * 
   * \par In Python:
   *    value = obj.diagram
   */
  GrtObjectRef diagram() const { return _diagram; }

  /**
   * Setter for attribute diagram
   * 
   * 
   * \par In Python:
   *   obj.diagram = value
   */
  virtual void diagram(const GrtObjectRef &value) {
    grt::ValueRef ovalue(_diagram);
    _diagram = value;
    member_changed("diagram", ovalue, value);
  }

  /**
   * Getter for attribute x
   *
   * 
   * \par In Python:
   *    value = obj.x
   */
  grt::DoubleRef x() const { return _x; }

  /**
   * Setter for attribute x
   * 
   * 
   * \par In Python:
   *   obj.x = value
   */
  virtual void x(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_x);
    _x = value;
    member_changed("x", ovalue, value);
  }

  /**
   * Getter for attribute y
   *
   * 
   * \par In Python:
   *    value = obj.y
   */
  grt::DoubleRef y() const { return _y; }

  /**
   * Setter for attribute y
   * 
   * 
   * \par In Python:
   *   obj.y = value
   */
  virtual void y(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_y);
    _y = value;
    member_changed("y", ovalue, value);
  }

  /**
   * Getter for attribute zoom
   *
   * 
   * \par In Python:
   *    value = obj.zoom
   */
  grt::DoubleRef zoom() const { return _zoom; }

  /**
   * Setter for attribute zoom
   * 
   * 
   * \par In Python:
   *   obj.zoom = value
   */
  virtual void zoom(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_zoom);
    _zoom = value;
    member_changed("zoom", ovalue, value);
  }

protected:

  GrtObjectRef _diagram;
  grt::DoubleRef _x;
  grt::DoubleRef _y;
  grt::DoubleRef _zoom;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new model_Marker());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&model_Marker::create);
    {
      void (model_Marker::*setter)(const GrtObjectRef &) = &model_Marker::diagram;
      GrtObjectRef (model_Marker::*getter)() const = &model_Marker::diagram;
      meta->bind_member("diagram", new grt::MetaClass::Property<model_Marker,GrtObjectRef>(getter, setter));
    }
    {
      void (model_Marker::*setter)(const grt::DoubleRef &) = &model_Marker::x;
      grt::DoubleRef (model_Marker::*getter)() const = &model_Marker::x;
      meta->bind_member("x", new grt::MetaClass::Property<model_Marker,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Marker::*setter)(const grt::DoubleRef &) = &model_Marker::y;
      grt::DoubleRef (model_Marker::*getter)() const = &model_Marker::y;
      meta->bind_member("y", new grt::MetaClass::Property<model_Marker,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Marker::*setter)(const grt::DoubleRef &) = &model_Marker::zoom;
      grt::DoubleRef (model_Marker::*getter)() const = &model_Marker::zoom;
      meta->bind_member("zoom", new grt::MetaClass::Property<model_Marker,grt::DoubleRef>(getter, setter));
    }
  }
};

/** a group of figures */
class  model_Group : public GrtObject {
  typedef GrtObject super;

public:
  model_Group(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _description(""),
      _figures(this, false),
      _subGroups(this, false) {
  }

  static std::string static_class_name() {
    return "model.Group";
  }

  /**
   * Getter for attribute description
   *
   * 
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * 
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  /**
   * Getter for attribute figures (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.figures
   */
  grt::ListRef<model_Figure> figures() const { return _figures; }


private: // The next attribute is read-only.
  virtual void figures(const grt::ListRef<model_Figure> &value) {
    grt::ValueRef ovalue(_figures);
    _figures = value;
    member_changed("figures", ovalue, value);
  }
public:

  /**
   * Getter for attribute owner
   *
   * 
   * \par In Python:
   *    value = obj.owner
   */
  model_DiagramRef owner() const { return model_DiagramRef::cast_from(_owner); }

  /**
   * Setter for attribute owner
   * 
   * 
   * \par In Python:
   *   obj.owner = value
   */
  virtual void owner(const model_DiagramRef &value) { super::owner(value); }

  /**
   * Getter for attribute subGroups (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.subGroups
   */
  grt::ListRef<model_Group> subGroups() const { return _subGroups; }


private: // The next attribute is read-only.
  virtual void subGroups(const grt::ListRef<model_Group> &value) {
    grt::ValueRef ovalue(_subGroups);
    _subGroups = value;
    member_changed("subGroups", ovalue, value);
  }
public:

protected:

  grt::StringRef _description;
  grt::ListRef<model_Figure> _figures;
  grt::ListRef<model_Group> _subGroups;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new model_Group());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&model_Group::create);
    {
      void (model_Group::*setter)(const grt::StringRef &) = &model_Group::description;
      grt::StringRef (model_Group::*getter)() const = &model_Group::description;
      meta->bind_member("description", new grt::MetaClass::Property<model_Group,grt::StringRef>(getter, setter));
    }
    {
      void (model_Group::*setter)(const grt::ListRef<model_Figure> &) = &model_Group::figures;
      grt::ListRef<model_Figure> (model_Group::*getter)() const = &model_Group::figures;
      meta->bind_member("figures", new grt::MetaClass::Property<model_Group,grt::ListRef<model_Figure>>(getter, setter));
    }
    {
      void (model_Group::*setter)(const model_DiagramRef &) = 0;
      model_DiagramRef (model_Group::*getter)() const = 0;
      meta->bind_member("owner", new grt::MetaClass::Property<model_Group,model_DiagramRef>(getter, setter));
    }
    {
      void (model_Group::*setter)(const grt::ListRef<model_Group> &) = &model_Group::subGroups;
      grt::ListRef<model_Group> (model_Group::*getter)() const = &model_Group::subGroups;
      meta->bind_member("subGroups", new grt::MetaClass::Property<model_Group,grt::ListRef<model_Group>>(getter, setter));
    }
  }
};

/** a model object */
class GRT_STRUCTS_MODEL_PUBLIC model_Object : public GrtObject {
  typedef GrtObject super;

public:
  class ImplData;
  friend class ImplData;
  model_Object(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _visible(1),
      _data(nullptr) {
  }

  virtual ~model_Object();

  static std::string static_class_name() {
    return "model.Object";
  }

  /**
   * Getter for attribute owner
   *
   * 
   * \par In Python:
   *    value = obj.owner
   */
  model_DiagramRef owner() const { return model_DiagramRef::cast_from(_owner); }

  /**
   * Setter for attribute owner
   * 
   * 
   * \par In Python:
   *   obj.owner = value
   */
  virtual void owner(const model_DiagramRef &value) { super::owner(value); }

  /**
   * Getter for attribute visible
   *
   * 
   * \par In Python:
   *    value = obj.visible
   */
  grt::IntegerRef visible() const { return _visible; }

  /**
   * Setter for attribute visible
   * 
   * 
   * \par In Python:
   *   obj.visible = value
   */
  virtual void visible(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_visible);
    _visible = value;
    member_changed("visible", ovalue, value);
  }


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::IntegerRef _visible;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new model_Object());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&model_Object::create);
    {
      void (model_Object::*setter)(const model_DiagramRef &) = 0;
      model_DiagramRef (model_Object::*getter)() const = 0;
      meta->bind_member("owner", new grt::MetaClass::Property<model_Object,model_DiagramRef>(getter, setter));
    }
    {
      void (model_Object::*setter)(const grt::IntegerRef &) = &model_Object::visible;
      grt::IntegerRef (model_Object::*getter)() const = &model_Object::visible;
      meta->bind_member("visible", new grt::MetaClass::Property<model_Object,grt::IntegerRef>(getter, setter));
    }
  }
};

/** a layer that contains figure */
class GRT_STRUCTS_MODEL_PUBLIC model_Layer : public model_Object {
  typedef model_Object super;

public:
  class ImplData;
  friend class ImplData;
  model_Layer(grt::MetaClass *meta = nullptr)
    : model_Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _color(""),
      _description(""),
      _figures(this, false),
      _groups(this, false),
      _height(0.0),
      _left(0.0),
      _subLayers(this, false),
      _top(0.0),
      _width(0.0),
      _data(nullptr) {
  }

  virtual ~model_Layer();

  static std::string static_class_name() {
    return "model.Layer";
  }

  /**
   * Getter for attribute color
   *
   * 
   * \par In Python:
   *    value = obj.color
   */
  grt::StringRef color() const { return _color; }

  /**
   * Setter for attribute color
   * 
   * 
   * \par In Python:
   *   obj.color = value
   */
  virtual void color(const grt::StringRef &value) {
    grt::ValueRef ovalue(_color);
    _color = value;
    member_changed("color", ovalue, value);
  }

  /**
   * Getter for attribute description
   *
   * 
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * 
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  /**
   * Getter for attribute figures (read-only)
   *
   * list of figures in layer, in stacking order. Lower elements come first.
   * \par In Python:
   *    value = obj.figures
   */
  grt::ListRef<model_Figure> figures() const { return _figures; }


private: // The next attribute is read-only.
  virtual void figures(const grt::ListRef<model_Figure> &value) {
    grt::ValueRef ovalue(_figures);
    _figures = value;
    member_changed("figures", ovalue, value);
  }
public:

  /**
   * Getter for attribute groups (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.groups
   */
  grt::ListRef<model_Group> groups() const { return _groups; }


private: // The next attribute is read-only.
  virtual void groups(const grt::ListRef<model_Group> &value) {
    grt::ValueRef ovalue(_groups);
    _groups = value;
    member_changed("groups", ovalue, value);
  }
public:

  /**
   * Getter for attribute height
   *
   * 
   * \par In Python:
   *    value = obj.height
   */
  grt::DoubleRef height() const { return _height; }

  /**
   * Setter for attribute height
   * 
   * 
   * \par In Python:
   *   obj.height = value
   */
  virtual void height(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_height);
    _height = value;
    member_changed("height", ovalue, value);
  }

  /**
   * Getter for attribute left
   *
   * 
   * \par In Python:
   *    value = obj.left
   */
  grt::DoubleRef left() const { return _left; }

  /**
   * Setter for attribute left
   * 
   * 
   * \par In Python:
   *   obj.left = value
   */
  virtual void left(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_left);
    _left = value;
    member_changed("left", ovalue, value);
  }

  /**
   * Getter for attribute subLayers (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.subLayers
   */
  grt::ListRef<model_Layer> subLayers() const { return _subLayers; }


private: // The next attribute is read-only.
  virtual void subLayers(const grt::ListRef<model_Layer> &value) {
    grt::ValueRef ovalue(_subLayers);
    _subLayers = value;
    member_changed("subLayers", ovalue, value);
  }
public:

  /**
   * Getter for attribute top
   *
   * 
   * \par In Python:
   *    value = obj.top
   */
  grt::DoubleRef top() const { return _top; }

  /**
   * Setter for attribute top
   * 
   * 
   * \par In Python:
   *   obj.top = value
   */
  virtual void top(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_top);
    _top = value;
    member_changed("top", ovalue, value);
  }

  /**
   * Getter for attribute width
   *
   * 
   * \par In Python:
   *    value = obj.width
   */
  grt::DoubleRef width() const { return _width; }

  /**
   * Setter for attribute width
   * 
   * 
   * \par In Python:
   *   obj.width = value
   */
  virtual void width(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_width);
    _width = value;
    member_changed("width", ovalue, value);
  }

  /**
   * Method. 
   * \param figure 
   * \return 
   */
  virtual void lowerFigure(const model_FigureRef &figure);
  /**
   * Method. 
   * \param figure 
   * \return 
   */
  virtual void raiseFigure(const model_FigureRef &figure);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::StringRef _color;
  grt::StringRef _description;
  grt::ListRef<model_Figure> _figures;
  grt::ListRef<model_Group> _groups;
  grt::DoubleRef _height;
  grt::DoubleRef _left;
  grt::ListRef<model_Layer> _subLayers;
  grt::DoubleRef _top;
  grt::DoubleRef _width;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new model_Layer());
  }

  static grt::ValueRef call_lowerFigure(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Layer*>(self)->lowerFigure(model_FigureRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_raiseFigure(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Layer*>(self)->raiseFigure(model_FigureRef::cast_from(args[0])); return grt::ValueRef(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&model_Layer::create);
    {
      void (model_Layer::*setter)(const grt::StringRef &) = &model_Layer::color;
      grt::StringRef (model_Layer::*getter)() const = &model_Layer::color;
      meta->bind_member("color", new grt::MetaClass::Property<model_Layer,grt::StringRef>(getter, setter));
    }
    {
      void (model_Layer::*setter)(const grt::StringRef &) = &model_Layer::description;
      grt::StringRef (model_Layer::*getter)() const = &model_Layer::description;
      meta->bind_member("description", new grt::MetaClass::Property<model_Layer,grt::StringRef>(getter, setter));
    }
    {
      void (model_Layer::*setter)(const grt::ListRef<model_Figure> &) = &model_Layer::figures;
      grt::ListRef<model_Figure> (model_Layer::*getter)() const = &model_Layer::figures;
      meta->bind_member("figures", new grt::MetaClass::Property<model_Layer,grt::ListRef<model_Figure>>(getter, setter));
    }
    {
      void (model_Layer::*setter)(const grt::ListRef<model_Group> &) = &model_Layer::groups;
      grt::ListRef<model_Group> (model_Layer::*getter)() const = &model_Layer::groups;
      meta->bind_member("groups", new grt::MetaClass::Property<model_Layer,grt::ListRef<model_Group>>(getter, setter));
    }
    {
      void (model_Layer::*setter)(const grt::DoubleRef &) = &model_Layer::height;
      grt::DoubleRef (model_Layer::*getter)() const = &model_Layer::height;
      meta->bind_member("height", new grt::MetaClass::Property<model_Layer,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Layer::*setter)(const grt::DoubleRef &) = &model_Layer::left;
      grt::DoubleRef (model_Layer::*getter)() const = &model_Layer::left;
      meta->bind_member("left", new grt::MetaClass::Property<model_Layer,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Layer::*setter)(const grt::ListRef<model_Layer> &) = &model_Layer::subLayers;
      grt::ListRef<model_Layer> (model_Layer::*getter)() const = &model_Layer::subLayers;
      meta->bind_member("subLayers", new grt::MetaClass::Property<model_Layer,grt::ListRef<model_Layer>>(getter, setter));
    }
    {
      void (model_Layer::*setter)(const grt::DoubleRef &) = &model_Layer::top;
      grt::DoubleRef (model_Layer::*getter)() const = &model_Layer::top;
      meta->bind_member("top", new grt::MetaClass::Property<model_Layer,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Layer::*setter)(const grt::DoubleRef &) = &model_Layer::width;
      grt::DoubleRef (model_Layer::*getter)() const = &model_Layer::width;
      meta->bind_member("width", new grt::MetaClass::Property<model_Layer,grt::DoubleRef>(getter, setter));
    }
    meta->bind_method("lowerFigure", &model_Layer::call_lowerFigure);
    meta->bind_method("raiseFigure", &model_Layer::call_raiseFigure);
  }
};

/** a connection between figures */
class GRT_STRUCTS_MODEL_PUBLIC model_Connection : public model_Object {
  typedef model_Object super;

public:
  class ImplData;
  friend class ImplData;
  model_Connection(grt::MetaClass *meta = nullptr)
    : model_Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _drawSplit(0),
      _data(nullptr) {
  }

  virtual ~model_Connection();

  static std::string static_class_name() {
    return "model.Connection";
  }

  /**
   * Getter for attribute drawSplit
   *
   * set to 1 if the connection line should be drawn split
   * \par In Python:
   *    value = obj.drawSplit
   */
  grt::IntegerRef drawSplit() const { return _drawSplit; }

  /**
   * Setter for attribute drawSplit
   * 
   * set to 1 if the connection line should be drawn split
   * \par In Python:
   *   obj.drawSplit = value
   */
  virtual void drawSplit(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_drawSplit);
    _drawSplit = value;
    member_changed("drawSplit", ovalue, value);
  }

  /**
   * Getter for attribute endFigure
   *
   * the target figure
   * \par In Python:
   *    value = obj.endFigure
   */
  model_FigureRef endFigure() const { return _endFigure; }

  /**
   * Setter for attribute endFigure
   * 
   * the target figure
   * \par In Python:
   *   obj.endFigure = value
   */
  virtual void endFigure(const model_FigureRef &value) {
    grt::ValueRef ovalue(_endFigure);
    _endFigure = value;
    member_changed("endFigure", ovalue, value);
  }

  /**
   * Getter for attribute startFigure
   *
   * the source figure
   * \par In Python:
   *    value = obj.startFigure
   */
  model_FigureRef startFigure() const { return _startFigure; }

  /**
   * Setter for attribute startFigure
   * 
   * the source figure
   * \par In Python:
   *   obj.startFigure = value
   */
  virtual void startFigure(const model_FigureRef &value) {
    grt::ValueRef ovalue(_startFigure);
    _startFigure = value;
    member_changed("startFigure", ovalue, value);
  }


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::IntegerRef _drawSplit;
  model_FigureRef _endFigure;
  model_FigureRef _startFigure;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new model_Connection());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&model_Connection::create);
    {
      void (model_Connection::*setter)(const grt::IntegerRef &) = &model_Connection::drawSplit;
      grt::IntegerRef (model_Connection::*getter)() const = &model_Connection::drawSplit;
      meta->bind_member("drawSplit", new grt::MetaClass::Property<model_Connection,grt::IntegerRef>(getter, setter));
    }
    {
      void (model_Connection::*setter)(const model_FigureRef &) = &model_Connection::endFigure;
      model_FigureRef (model_Connection::*getter)() const = &model_Connection::endFigure;
      meta->bind_member("endFigure", new grt::MetaClass::Property<model_Connection,model_FigureRef>(getter, setter));
    }
    {
      void (model_Connection::*setter)(const model_FigureRef &) = &model_Connection::startFigure;
      model_FigureRef (model_Connection::*getter)() const = &model_Connection::startFigure;
      meta->bind_member("startFigure", new grt::MetaClass::Property<model_Connection,model_FigureRef>(getter, setter));
    }
  }
};

/** a single model figure */
class GRT_STRUCTS_MODEL_PUBLIC model_Figure : public model_Object {
  typedef model_Object super;

public:
  class ImplData;
  friend class ImplData;
  model_Figure(grt::MetaClass *meta = nullptr)
    : model_Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _color(""),
      _expanded(1),
      _height(0.0),
      _left(0.0),
      _locked(0),
      _manualSizing(0),
      _top(0.0),
      _width(0.0),
      _data(nullptr) {
  }

  virtual ~model_Figure();

  static std::string static_class_name() {
    return "model.Figure";
  }

  /**
   * Getter for attribute color
   *
   * color style name for the figure
   * \par In Python:
   *    value = obj.color
   */
  grt::StringRef color() const { return _color; }

  /**
   * Setter for attribute color
   * 
   * color style name for the figure
   * \par In Python:
   *   obj.color = value
   */
  virtual void color(const grt::StringRef &value);

  /**
   * Getter for attribute expanded
   *
   * 
   * \par In Python:
   *    value = obj.expanded
   */
  grt::IntegerRef expanded() const { return _expanded; }

  /**
   * Setter for attribute expanded
   * 
   * 
   * \par In Python:
   *   obj.expanded = value
   */
  virtual void expanded(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_expanded);
    _expanded = value;
    member_changed("expanded", ovalue, value);
  }

  /**
   * Getter for attribute group
   *
   * 
   * \par In Python:
   *    value = obj.group
   */
  model_GroupRef group() const { return _group; }

  /**
   * Setter for attribute group
   * 
   * 
   * \par In Python:
   *   obj.group = value
   */
  virtual void group(const model_GroupRef &value) {
    grt::ValueRef ovalue(_group);
    _group = value;
    member_changed("group", ovalue, value);
  }

  /**
   * Getter for attribute height
   *
   * 
   * \par In Python:
   *    value = obj.height
   */
  grt::DoubleRef height() const { return _height; }

  /**
   * Setter for attribute height
   * 
   * 
   * \par In Python:
   *   obj.height = value
   */
  virtual void height(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_height);
    _height = value;
    member_changed("height", ovalue, value);
  }

  /**
   * Getter for attribute layer
   *
   * 
   * \par In Python:
   *    value = obj.layer
   */
  model_LayerRef layer() const { return _layer; }

  /**
   * Setter for attribute layer
   * 
   * 
   * \par In Python:
   *   obj.layer = value
   */
  virtual void layer(const model_LayerRef &value);

  /**
   * Getter for attribute left
   *
   * 
   * \par In Python:
   *    value = obj.left
   */
  grt::DoubleRef left() const { return _left; }

  /**
   * Setter for attribute left
   * 
   * 
   * \par In Python:
   *   obj.left = value
   */
  virtual void left(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_left);
    _left = value;
    member_changed("left", ovalue, value);
  }

  /**
   * Getter for attribute locked
   *
   * 
   * \par In Python:
   *    value = obj.locked
   */
  grt::IntegerRef locked() const { return _locked; }

  /**
   * Setter for attribute locked
   * 
   * 
   * \par In Python:
   *   obj.locked = value
   */
  virtual void locked(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_locked);
    _locked = value;
    member_changed("locked", ovalue, value);
  }

  /**
   * Getter for attribute manualSizing
   *
   * manually resize object
   * \par In Python:
   *    value = obj.manualSizing
   */
  grt::IntegerRef manualSizing() const { return _manualSizing; }

  /**
   * Setter for attribute manualSizing
   * 
   * manually resize object
   * \par In Python:
   *   obj.manualSizing = value
   */
  virtual void manualSizing(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_manualSizing);
    _manualSizing = value;
    member_changed("manualSizing", ovalue, value);
  }

  /**
   * Getter for attribute top
   *
   * 
   * \par In Python:
   *    value = obj.top
   */
  grt::DoubleRef top() const { return _top; }

  /**
   * Setter for attribute top
   * 
   * 
   * \par In Python:
   *   obj.top = value
   */
  virtual void top(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_top);
    _top = value;
    member_changed("top", ovalue, value);
  }

  /**
   * Getter for attribute width
   *
   * 
   * \par In Python:
   *    value = obj.width
   */
  grt::DoubleRef width() const { return _width; }

  /**
   * Setter for attribute width
   * 
   * 
   * \par In Python:
   *   obj.width = value
   */
  virtual void width(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_width);
    _width = value;
    member_changed("width", ovalue, value);
  }


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::StringRef _color;
  grt::IntegerRef _expanded;
  model_GroupRef _group;
  grt::DoubleRef _height;
  model_LayerRef _layer;
  grt::DoubleRef _left;
  grt::IntegerRef _locked;
  grt::IntegerRef _manualSizing;
  grt::DoubleRef _top;
  grt::DoubleRef _width;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new model_Figure());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&model_Figure::create);
    {
      void (model_Figure::*setter)(const grt::StringRef &) = &model_Figure::color;
      grt::StringRef (model_Figure::*getter)() const = &model_Figure::color;
      meta->bind_member("color", new grt::MetaClass::Property<model_Figure,grt::StringRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const grt::IntegerRef &) = &model_Figure::expanded;
      grt::IntegerRef (model_Figure::*getter)() const = &model_Figure::expanded;
      meta->bind_member("expanded", new grt::MetaClass::Property<model_Figure,grt::IntegerRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const model_GroupRef &) = &model_Figure::group;
      model_GroupRef (model_Figure::*getter)() const = &model_Figure::group;
      meta->bind_member("group", new grt::MetaClass::Property<model_Figure,model_GroupRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const grt::DoubleRef &) = &model_Figure::height;
      grt::DoubleRef (model_Figure::*getter)() const = &model_Figure::height;
      meta->bind_member("height", new grt::MetaClass::Property<model_Figure,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const model_LayerRef &) = &model_Figure::layer;
      model_LayerRef (model_Figure::*getter)() const = &model_Figure::layer;
      meta->bind_member("layer", new grt::MetaClass::Property<model_Figure,model_LayerRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const grt::DoubleRef &) = &model_Figure::left;
      grt::DoubleRef (model_Figure::*getter)() const = &model_Figure::left;
      meta->bind_member("left", new grt::MetaClass::Property<model_Figure,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const grt::IntegerRef &) = &model_Figure::locked;
      grt::IntegerRef (model_Figure::*getter)() const = &model_Figure::locked;
      meta->bind_member("locked", new grt::MetaClass::Property<model_Figure,grt::IntegerRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const grt::IntegerRef &) = &model_Figure::manualSizing;
      grt::IntegerRef (model_Figure::*getter)() const = &model_Figure::manualSizing;
      meta->bind_member("manualSizing", new grt::MetaClass::Property<model_Figure,grt::IntegerRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const grt::DoubleRef &) = &model_Figure::top;
      grt::DoubleRef (model_Figure::*getter)() const = &model_Figure::top;
      meta->bind_member("top", new grt::MetaClass::Property<model_Figure,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Figure::*setter)(const grt::DoubleRef &) = &model_Figure::width;
      grt::DoubleRef (model_Figure::*getter)() const = &model_Figure::width;
      meta->bind_member("width", new grt::MetaClass::Property<model_Figure,grt::DoubleRef>(getter, setter));
    }
  }
};

/** a diagram of the model data */
class GRT_STRUCTS_MODEL_PUBLIC model_Diagram : public GrtObject {
  typedef GrtObject super;

public:
  class ImplData;
  friend class ImplData;
  model_Diagram(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _closed(0),
      _connections(this, false),
      _description(""),
      _figures(this, false),
      _height(0.0),
      _layers(this, false),
      _options(this, false),
      _selection(this, false),
      _updateBlocked(0),
      _width(0.0),
      _x(0.0),
      _y(0.0),
      _zoom(0.0),
      _data(nullptr) {
  }

  virtual ~model_Diagram();

  static std::string static_class_name() {
    return "model.Diagram";
  }

  // args: 
  boost::signals2::signal<void (model_ObjectRef, ssize_t)>* signal_objectActivated() { return &_signal_objectActivated; }
  // args: 
  boost::signals2::signal<void (model_ObjectRef)>* signal_refreshDisplay() { return &_signal_refreshDisplay; }
  /**
   * Getter for attribute closed
   *
   * 
   * \par In Python:
   *    value = obj.closed
   */
  grt::IntegerRef closed() const { return _closed; }

  /**
   * Setter for attribute closed
   * 
   * 
   * \par In Python:
   *   obj.closed = value
   */
  virtual void closed(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_closed);
    _closed = value;
    member_changed("closed", ovalue, value);
  }

  // connections is owned by model_Diagram
  /**
   * Getter for attribute connections (read-only)
   *
   * all connections displayed in this diagram
   * \par In Python:
   *    value = obj.connections
   */
  grt::ListRef<model_Connection> connections() const { return _connections; }


private: // The next attribute is read-only.
  virtual void connections(const grt::ListRef<model_Connection> &value) {
    grt::ValueRef ovalue(_connections);

    _connections = value;
    owned_member_changed("connections", ovalue, value);
  }
public:

  /**
   * Getter for attribute description
   *
   * 
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * 
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  // figures is owned by model_Diagram
  /**
   * Getter for attribute figures (read-only)
   *
   * all figures displayed in this diagram
   * \par In Python:
   *    value = obj.figures
   */
  grt::ListRef<model_Figure> figures() const { return _figures; }


private: // The next attribute is read-only.
  virtual void figures(const grt::ListRef<model_Figure> &value) {
    grt::ValueRef ovalue(_figures);

    _figures = value;
    owned_member_changed("figures", ovalue, value);
  }
public:

  /**
   * Getter for attribute height
   *
   * 
   * \par In Python:
   *    value = obj.height
   */
  grt::DoubleRef height() const { return _height; }

  /**
   * Setter for attribute height
   * 
   * 
   * \par In Python:
   *   obj.height = value
   */
  virtual void height(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_height);
    _height = value;
    member_changed("height", ovalue, value);
  }

  // layers is owned by model_Diagram
  /**
   * Getter for attribute layers (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.layers
   */
  grt::ListRef<model_Layer> layers() const { return _layers; }


private: // The next attribute is read-only.
  virtual void layers(const grt::ListRef<model_Layer> &value) {
    grt::ValueRef ovalue(_layers);

    _layers = value;
    owned_member_changed("layers", ovalue, value);
  }
public:

  /**
   * Getter for attribute name
   *
   * the current name of the object
   * \par In Python:
   *    value = obj.name
   */

  /**
   * Setter for attribute name
   * 
   * the current name of the object
   * \par In Python:
   *   obj.name = value
   */

  /**
   * Getter for attribute options (read-only)
   *
   * diagram specific options
   * \par In Python:
   *    value = obj.options
   */
  grt::DictRef options() const { return _options; }


private: // The next attribute is read-only.
  virtual void options(const grt::DictRef &value) {
    grt::ValueRef ovalue(_options);
    _options = value;
    member_changed("options", ovalue, value);
  }
public:

  /**
   * Getter for attribute owner
   *
   * 
   * \par In Python:
   *    value = obj.owner
   */
  model_ModelRef owner() const { return model_ModelRef::cast_from(_owner); }

  /**
   * Setter for attribute owner
   * 
   * 
   * \par In Python:
   *   obj.owner = value
   */
  virtual void owner(const model_ModelRef &value) { super::owner(value); }

  // rootLayer is owned by model_Diagram
  /**
   * Getter for attribute rootLayer
   *
   * 
   * \par In Python:
   *    value = obj.rootLayer
   */
  model_LayerRef rootLayer() const { return _rootLayer; }

  /**
   * Setter for attribute rootLayer
   * 
   * 
   * \par In Python:
   *   obj.rootLayer = value
   */
  virtual void rootLayer(const model_LayerRef &value);

  /**
   * Getter for attribute selection (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.selection
   */
  grt::ListRef<model_Object> selection() const { return _selection; }


private: // The next attribute is read-only.
  virtual void selection(const grt::ListRef<model_Object> &value) {
    grt::ValueRef ovalue(_selection);
    _selection = value;
    member_changed("selection", ovalue, value);
  }
public:

  /**
   * Getter for attribute updateBlocked
   *
   * 
   * \par In Python:
   *    value = obj.updateBlocked
   */
  grt::IntegerRef updateBlocked() const { return _updateBlocked; }

  /**
   * Setter for attribute updateBlocked
   * 
   * 
   * \par In Python:
   *   obj.updateBlocked = value
   */
  virtual void updateBlocked(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_updateBlocked);
    _updateBlocked = value;
    member_changed("updateBlocked", ovalue, value);
  }

  /**
   * Getter for attribute width
   *
   * 
   * \par In Python:
   *    value = obj.width
   */
  grt::DoubleRef width() const { return _width; }

  /**
   * Setter for attribute width
   * 
   * 
   * \par In Python:
   *   obj.width = value
   */
  virtual void width(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_width);
    _width = value;
    member_changed("width", ovalue, value);
  }

  /**
   * Getter for attribute x
   *
   * 
   * \par In Python:
   *    value = obj.x
   */
  grt::DoubleRef x() const { return _x; }

  /**
   * Setter for attribute x
   * 
   * 
   * \par In Python:
   *   obj.x = value
   */
  virtual void x(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_x);
    _x = value;
    member_changed("x", ovalue, value);
  }

  /**
   * Getter for attribute y
   *
   * 
   * \par In Python:
   *    value = obj.y
   */
  grt::DoubleRef y() const { return _y; }

  /**
   * Setter for attribute y
   * 
   * 
   * \par In Python:
   *   obj.y = value
   */
  virtual void y(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_y);
    _y = value;
    member_changed("y", ovalue, value);
  }

  /**
   * Getter for attribute zoom
   *
   * 
   * \par In Python:
   *    value = obj.zoom
   */
  grt::DoubleRef zoom() const { return _zoom; }

  /**
   * Setter for attribute zoom
   * 
   * 
   * \par In Python:
   *   obj.zoom = value
   */
  virtual void zoom(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_zoom);
    _zoom = value;
    member_changed("zoom", ovalue, value);
  }

  /**
   * Method. 
   * \param connection 
   * \return 
   */
  virtual void addConnection(const model_ConnectionRef &connection);
  /**
   * Method. 
   * \param figure 
   * \return 
   */
  virtual void addFigure(const model_FigureRef &figure);
  /**
   * Method. 
   * \param flag 
   * \return 
   */
  virtual void blockUpdates(ssize_t flag);
  /**
   * Method. 
   * \param layer 
   * \return 
   */
  virtual void deleteLayer(const model_LayerRef &layer);
  /**
   * Method. 
   * \param x 
   * \param y 
   * \param width 
   * \param height 
   * \param name 
   * \return 
   */
  virtual model_LayerRef placeNewLayer(double x, double y, double width, double height, const std::string &name) = 0;
  /**
   * Method. 
   * \param connection 
   * \return 
   */
  virtual void removeConnection(const model_ConnectionRef &connection);
  /**
   * Method. 
   * \param figure 
   * \return 
   */
  virtual void removeFigure(const model_FigureRef &figure);
  /**
   * Method. 
   * \param object 
   * \return 
   */
  virtual void selectObject(const model_ObjectRef &object);
  /**
   * Method. 
   * \param xpages 
   * \param ypages 
   * \return 
   */
  virtual void setPageCounts(ssize_t xpages, ssize_t ypages);
  /**
   * Method. 
   * \return 
   */
  virtual void unselectAll();
  /**
   * Method. 
   * \param object 
   * \return 
   */
  virtual void unselectObject(const model_ObjectRef &object);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:
  boost::signals2::signal<void (model_ObjectRef, ssize_t)> _signal_objectActivated;
  boost::signals2::signal<void (model_ObjectRef)> _signal_refreshDisplay;

  grt::IntegerRef _closed;
  grt::ListRef<model_Connection> _connections;// owned
  grt::StringRef _description;
  grt::ListRef<model_Figure> _figures;// owned
  grt::DoubleRef _height;
  grt::ListRef<model_Layer> _layers;// owned
  grt::DictRef _options;
  model_LayerRef _rootLayer;// owned
  grt::ListRef<model_Object> _selection;
  grt::IntegerRef _updateBlocked;
  grt::DoubleRef _width;
  grt::DoubleRef _x;
  grt::DoubleRef _y;
  grt::DoubleRef _zoom;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ValueRef call_addConnection(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->addConnection(model_ConnectionRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_addFigure(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->addFigure(model_FigureRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_blockUpdates(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->blockUpdates(grt::IntegerRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_deleteLayer(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->deleteLayer(model_LayerRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_placeNewLayer(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<model_Diagram*>(self)->placeNewLayer(grt::DoubleRef::cast_from(args[0]), grt::DoubleRef::cast_from(args[1]), grt::DoubleRef::cast_from(args[2]), grt::DoubleRef::cast_from(args[3]), grt::StringRef::cast_from(args[4])); }

  static grt::ValueRef call_removeConnection(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->removeConnection(model_ConnectionRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_removeFigure(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->removeFigure(model_FigureRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_selectObject(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->selectObject(model_ObjectRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_setPageCounts(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->setPageCounts(grt::IntegerRef::cast_from(args[0]), grt::IntegerRef::cast_from(args[1])); return grt::ValueRef(); }

  static grt::ValueRef call_unselectAll(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->unselectAll(); return grt::ValueRef(); }

  static grt::ValueRef call_unselectObject(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<model_Diagram*>(self)->unselectObject(model_ObjectRef::cast_from(args[0])); return grt::ValueRef(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(nullptr);
    {
      void (model_Diagram::*setter)(const grt::IntegerRef &) = &model_Diagram::closed;
      grt::IntegerRef (model_Diagram::*getter)() const = &model_Diagram::closed;
      meta->bind_member("closed", new grt::MetaClass::Property<model_Diagram,grt::IntegerRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::ListRef<model_Connection> &) = &model_Diagram::connections;
      grt::ListRef<model_Connection> (model_Diagram::*getter)() const = &model_Diagram::connections;
      meta->bind_member("connections", new grt::MetaClass::Property<model_Diagram,grt::ListRef<model_Connection>>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::StringRef &) = &model_Diagram::description;
      grt::StringRef (model_Diagram::*getter)() const = &model_Diagram::description;
      meta->bind_member("description", new grt::MetaClass::Property<model_Diagram,grt::StringRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::ListRef<model_Figure> &) = &model_Diagram::figures;
      grt::ListRef<model_Figure> (model_Diagram::*getter)() const = &model_Diagram::figures;
      meta->bind_member("figures", new grt::MetaClass::Property<model_Diagram,grt::ListRef<model_Figure>>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::DoubleRef &) = &model_Diagram::height;
      grt::DoubleRef (model_Diagram::*getter)() const = &model_Diagram::height;
      meta->bind_member("height", new grt::MetaClass::Property<model_Diagram,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::ListRef<model_Layer> &) = &model_Diagram::layers;
      grt::ListRef<model_Layer> (model_Diagram::*getter)() const = &model_Diagram::layers;
      meta->bind_member("layers", new grt::MetaClass::Property<model_Diagram,grt::ListRef<model_Layer>>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (model_Diagram::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<model_Diagram,grt::StringRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::DictRef &) = &model_Diagram::options;
      grt::DictRef (model_Diagram::*getter)() const = &model_Diagram::options;
      meta->bind_member("options", new grt::MetaClass::Property<model_Diagram,grt::DictRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const model_ModelRef &) = 0;
      model_ModelRef (model_Diagram::*getter)() const = 0;
      meta->bind_member("owner", new grt::MetaClass::Property<model_Diagram,model_ModelRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const model_LayerRef &) = &model_Diagram::rootLayer;
      model_LayerRef (model_Diagram::*getter)() const = &model_Diagram::rootLayer;
      meta->bind_member("rootLayer", new grt::MetaClass::Property<model_Diagram,model_LayerRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::ListRef<model_Object> &) = &model_Diagram::selection;
      grt::ListRef<model_Object> (model_Diagram::*getter)() const = &model_Diagram::selection;
      meta->bind_member("selection", new grt::MetaClass::Property<model_Diagram,grt::ListRef<model_Object>>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::IntegerRef &) = &model_Diagram::updateBlocked;
      grt::IntegerRef (model_Diagram::*getter)() const = &model_Diagram::updateBlocked;
      meta->bind_member("updateBlocked", new grt::MetaClass::Property<model_Diagram,grt::IntegerRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::DoubleRef &) = &model_Diagram::width;
      grt::DoubleRef (model_Diagram::*getter)() const = &model_Diagram::width;
      meta->bind_member("width", new grt::MetaClass::Property<model_Diagram,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::DoubleRef &) = &model_Diagram::x;
      grt::DoubleRef (model_Diagram::*getter)() const = &model_Diagram::x;
      meta->bind_member("x", new grt::MetaClass::Property<model_Diagram,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::DoubleRef &) = &model_Diagram::y;
      grt::DoubleRef (model_Diagram::*getter)() const = &model_Diagram::y;
      meta->bind_member("y", new grt::MetaClass::Property<model_Diagram,grt::DoubleRef>(getter, setter));
    }
    {
      void (model_Diagram::*setter)(const grt::DoubleRef &) = &model_Diagram::zoom;
      grt::DoubleRef (model_Diagram::*getter)() const = &model_Diagram::zoom;
      meta->bind_member("zoom", new grt::MetaClass::Property<model_Diagram,grt::DoubleRef>(getter, setter));
    }
    meta->bind_method("addConnection", &model_Diagram::call_addConnection);
    meta->bind_method("addFigure", &model_Diagram::call_addFigure);
    meta->bind_method("blockUpdates", &model_Diagram::call_blockUpdates);
    meta->bind_method("deleteLayer", &model_Diagram::call_deleteLayer);
    meta->bind_method("placeNewLayer", &model_Diagram::call_placeNewLayer);
    meta->bind_method("removeConnection", &model_Diagram::call_removeConnection);
    meta->bind_method("removeFigure", &model_Diagram::call_removeFigure);
    meta->bind_method("selectObject", &model_Diagram::call_selectObject);
    meta->bind_method("setPageCounts", &model_Diagram::call_setPageCounts);
    meta->bind_method("unselectAll", &model_Diagram::call_unselectAll);
    meta->bind_method("unselectObject", &model_Diagram::call_unselectObject);
  }
};

class GRT_STRUCTS_MODEL_PUBLIC model_Model : public GrtObject {
  typedef GrtObject super;

public:
  class ImplData;
  friend class ImplData;
  model_Model(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _customData(this, false),
      _diagrams(this, false),
      _markers(this, false),
      _options(this, false),
      _data(nullptr) {
  }

  virtual ~model_Model();

  static std::string static_class_name() {
    return "model.Model";
  }

  /**
   * Getter for attribute currentDiagram
   *
   * the currently active diagram
   * \par In Python:
   *    value = obj.currentDiagram
   */
  model_DiagramRef currentDiagram() const { return _currentDiagram; }

  /**
   * Setter for attribute currentDiagram
   * 
   * the currently active diagram
   * \par In Python:
   *   obj.currentDiagram = value
   */
  virtual void currentDiagram(const model_DiagramRef &value) {
    grt::ValueRef ovalue(_currentDiagram);
    _currentDiagram = value;
    member_changed("currentDiagram", ovalue, value);
  }

  /**
   * Getter for attribute customData (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.customData
   */
  grt::DictRef customData() const { return _customData; }


private: // The next attribute is read-only.
  virtual void customData(const grt::DictRef &value) {
    grt::ValueRef ovalue(_customData);
    _customData = value;
    member_changed("customData", ovalue, value);
  }
public:

  // diagrams is owned by model_Model
  /**
   * Getter for attribute diagrams (read-only)
   *
   * the list of all available diagrams
   * \par In Python:
   *    value = obj.diagrams
   */
  grt::ListRef<model_Diagram> diagrams() const { return _diagrams; }


private: // The next attribute is read-only.
  virtual void diagrams(const grt::ListRef<model_Diagram> &value) {
    grt::ValueRef ovalue(_diagrams);

    _diagrams = value;
    owned_member_changed("diagrams", ovalue, value);
  }
public:

  // markers is owned by model_Model
  /**
   * Getter for attribute markers (read-only)
   *
   * a list of markers that can be used to jump to a given diagram at a given position
   * \par In Python:
   *    value = obj.markers
   */
  grt::ListRef<model_Marker> markers() const { return _markers; }


private: // The next attribute is read-only.
  virtual void markers(const grt::ListRef<model_Marker> &value) {
    grt::ValueRef ovalue(_markers);

    _markers = value;
    owned_member_changed("markers", ovalue, value);
  }
public:

  /**
   * Getter for attribute options (read-only)
   *
   * model specific options
   * \par In Python:
   *    value = obj.options
   */
  grt::DictRef options() const { return _options; }


private: // The next attribute is read-only.
  virtual void options(const grt::DictRef &value) {
    grt::ValueRef ovalue(_options);
    _options = value;
    member_changed("options", ovalue, value);
  }
public:

  /**
   * Method. 
   * \param deferRealize 
   * \return 
   */
  virtual model_DiagramRef addNewDiagram(ssize_t deferRealize) = 0;

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  model_DiagramRef _currentDiagram;
  grt::DictRef _customData;
  grt::ListRef<model_Diagram> _diagrams;// owned
  grt::ListRef<model_Marker> _markers;// owned
  grt::DictRef _options;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ValueRef call_addNewDiagram(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<model_Model*>(self)->addNewDiagram(grt::IntegerRef::cast_from(args[0])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(nullptr);
    {
      void (model_Model::*setter)(const model_DiagramRef &) = &model_Model::currentDiagram;
      model_DiagramRef (model_Model::*getter)() const = &model_Model::currentDiagram;
      meta->bind_member("currentDiagram", new grt::MetaClass::Property<model_Model,model_DiagramRef>(getter, setter));
    }
    {
      void (model_Model::*setter)(const grt::DictRef &) = &model_Model::customData;
      grt::DictRef (model_Model::*getter)() const = &model_Model::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<model_Model,grt::DictRef>(getter, setter));
    }
    {
      void (model_Model::*setter)(const grt::ListRef<model_Diagram> &) = &model_Model::diagrams;
      grt::ListRef<model_Diagram> (model_Model::*getter)() const = &model_Model::diagrams;
      meta->bind_member("diagrams", new grt::MetaClass::Property<model_Model,grt::ListRef<model_Diagram>>(getter, setter));
    }
    {
      void (model_Model::*setter)(const grt::ListRef<model_Marker> &) = &model_Model::markers;
      grt::ListRef<model_Marker> (model_Model::*getter)() const = &model_Model::markers;
      meta->bind_member("markers", new grt::MetaClass::Property<model_Model,grt::ListRef<model_Marker>>(getter, setter));
    }
    {
      void (model_Model::*setter)(const grt::DictRef &) = &model_Model::options;
      grt::DictRef (model_Model::*getter)() const = &model_Model::options;
      meta->bind_member("options", new grt::MetaClass::Property<model_Model,grt::DictRef>(getter, setter));
    }
    meta->bind_method("addNewDiagram", &model_Model::call_addNewDiagram);
  }
};



inline void register_structs_model_xml() {
  grt::internal::ClassRegistry::register_class<model_Marker>();
  grt::internal::ClassRegistry::register_class<model_Group>();
  grt::internal::ClassRegistry::register_class<model_Object>();
  grt::internal::ClassRegistry::register_class<model_Layer>();
  grt::internal::ClassRegistry::register_class<model_Connection>();
  grt::internal::ClassRegistry::register_class<model_Figure>();
  grt::internal::ClassRegistry::register_class<model_Diagram>();
  grt::internal::ClassRegistry::register_class<model_Model>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_model_xml {
  _autoreg__structs_model_xml() {
    register_structs_model_xml();
  }
} __autoreg__structs_model_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

