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
  #ifdef GRT_STRUCTS_WORKBENCH_LOGICAL_EXPORT
  #define GRT_STRUCTS_WORKBENCH_LOGICAL_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_WORKBENCH_LOGICAL_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_WORKBENCH_LOGICAL_PUBLIC
#endif

#include "grts/structs.h"
#include "grts/structs.model.h"
#include "grts/structs.eer.h"

class workbench_logical_Connection;
typedef grt::Ref<workbench_logical_Connection> workbench_logical_ConnectionRef;
class workbench_logical_Relationship;
typedef grt::Ref<workbench_logical_Relationship> workbench_logical_RelationshipRef;
class workbench_logical_Entity;
typedef grt::Ref<workbench_logical_Entity> workbench_logical_EntityRef;
class workbench_logical_Diagram;
typedef grt::Ref<workbench_logical_Diagram> workbench_logical_DiagramRef;
class workbench_logical_Model;
typedef grt::Ref<workbench_logical_Model> workbench_logical_ModelRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** a model connection */
class  workbench_logical_Connection : public model_Connection {
  typedef model_Connection super;

public:
  workbench_logical_Connection(grt::MetaClass *meta = nullptr)
    : model_Connection(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _comment(""),
      _endCaption(""),
      _endCaptionXOffs(0.0),
      _endCaptionYOffs(0.0),
      _endMany(0),
      _startCaption(""),
      _startCaptionXOffs(0.0),
      _startCaptionYOffs(0.0),
      _startMany(0) {
  }

  static std::string static_class_name() {
    return "workbench.logical.Connection";
  }

  /**
   * Getter for attribute comment
   *
   * a comment about the relationship
   * \par In Python:
   *    value = obj.comment
   */
  grt::StringRef comment() const { return _comment; }

  /**
   * Setter for attribute comment
   * 
   * a comment about the relationship
   * \par In Python:
   *   obj.comment = value
   */
  virtual void comment(const grt::StringRef &value) {
    grt::ValueRef ovalue(_comment);
    _comment = value;
    member_changed("comment", ovalue, value);
  }

  /**
   * Getter for attribute endCaption
   *
   * caption at the end of of the relationship
   * \par In Python:
   *    value = obj.endCaption
   */
  grt::StringRef endCaption() const { return _endCaption; }

  /**
   * Setter for attribute endCaption
   * 
   * caption at the end of of the relationship
   * \par In Python:
   *   obj.endCaption = value
   */
  virtual void endCaption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_endCaption);
    _endCaption = value;
    member_changed("endCaption", ovalue, value);
  }

  /**
   * Getter for attribute endCaptionXOffs
   *
   * X offset of the end caption
   * \par In Python:
   *    value = obj.endCaptionXOffs
   */
  grt::DoubleRef endCaptionXOffs() const { return _endCaptionXOffs; }

  /**
   * Setter for attribute endCaptionXOffs
   * 
   * X offset of the end caption
   * \par In Python:
   *   obj.endCaptionXOffs = value
   */
  virtual void endCaptionXOffs(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_endCaptionXOffs);
    _endCaptionXOffs = value;
    member_changed("endCaptionXOffs", ovalue, value);
  }

  /**
   * Getter for attribute endCaptionYOffs
   *
   * Y offset of the end caption
   * \par In Python:
   *    value = obj.endCaptionYOffs
   */
  grt::DoubleRef endCaptionYOffs() const { return _endCaptionYOffs; }

  /**
   * Setter for attribute endCaptionYOffs
   * 
   * Y offset of the end caption
   * \par In Python:
   *   obj.endCaptionYOffs = value
   */
  virtual void endCaptionYOffs(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_endCaptionYOffs);
    _endCaptionYOffs = value;
    member_changed("endCaptionYOffs", ovalue, value);
  }

  /**
   * Getter for attribute endFigure
   *
   * the target figure
   * \par In Python:
   *    value = obj.endFigure
   */

  /**
   * Setter for attribute endFigure
   * 
   * the target figure
   * \par In Python:
   *   obj.endFigure = value
   */

  /**
   * Getter for attribute endMany
   *
   * 
   * \par In Python:
   *    value = obj.endMany
   */
  grt::IntegerRef endMany() const { return _endMany; }

  /**
   * Setter for attribute endMany
   * 
   * 
   * \par In Python:
   *   obj.endMany = value
   */
  virtual void endMany(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_endMany);
    _endMany = value;
    member_changed("endMany", ovalue, value);
  }

  /**
   * Getter for attribute startCaption
   *
   * caption at the start of of the relationship
   * \par In Python:
   *    value = obj.startCaption
   */
  grt::StringRef startCaption() const { return _startCaption; }

  /**
   * Setter for attribute startCaption
   * 
   * caption at the start of of the relationship
   * \par In Python:
   *   obj.startCaption = value
   */
  virtual void startCaption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_startCaption);
    _startCaption = value;
    member_changed("startCaption", ovalue, value);
  }

  /**
   * Getter for attribute startCaptionXOffs
   *
   * X offset of the start caption
   * \par In Python:
   *    value = obj.startCaptionXOffs
   */
  grt::DoubleRef startCaptionXOffs() const { return _startCaptionXOffs; }

  /**
   * Setter for attribute startCaptionXOffs
   * 
   * X offset of the start caption
   * \par In Python:
   *   obj.startCaptionXOffs = value
   */
  virtual void startCaptionXOffs(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_startCaptionXOffs);
    _startCaptionXOffs = value;
    member_changed("startCaptionXOffs", ovalue, value);
  }

  /**
   * Getter for attribute startCaptionYOffs
   *
   * Y offset of the start caption
   * \par In Python:
   *    value = obj.startCaptionYOffs
   */
  grt::DoubleRef startCaptionYOffs() const { return _startCaptionYOffs; }

  /**
   * Setter for attribute startCaptionYOffs
   * 
   * Y offset of the start caption
   * \par In Python:
   *   obj.startCaptionYOffs = value
   */
  virtual void startCaptionYOffs(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_startCaptionYOffs);
    _startCaptionYOffs = value;
    member_changed("startCaptionYOffs", ovalue, value);
  }

  /**
   * Getter for attribute startFigure
   *
   * the source figure
   * \par In Python:
   *    value = obj.startFigure
   */

  /**
   * Setter for attribute startFigure
   * 
   * the source figure
   * \par In Python:
   *   obj.startFigure = value
   */

  /**
   * Getter for attribute startMany
   *
   * 
   * \par In Python:
   *    value = obj.startMany
   */
  grt::IntegerRef startMany() const { return _startMany; }

  /**
   * Setter for attribute startMany
   * 
   * 
   * \par In Python:
   *   obj.startMany = value
   */
  virtual void startMany(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_startMany);
    _startMany = value;
    member_changed("startMany", ovalue, value);
  }

protected:

  grt::StringRef _comment;
  grt::StringRef _endCaption;
  grt::DoubleRef _endCaptionXOffs;
  grt::DoubleRef _endCaptionYOffs;
  grt::IntegerRef _endMany;
  grt::StringRef _startCaption;
  grt::DoubleRef _startCaptionXOffs;
  grt::DoubleRef _startCaptionYOffs;
  grt::IntegerRef _startMany;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_logical_Connection());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_logical_Connection::create);
    {
      void (workbench_logical_Connection::*setter)(const grt::StringRef &) = &workbench_logical_Connection::comment;
      grt::StringRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::comment;
      meta->bind_member("comment", new grt::MetaClass::Property<workbench_logical_Connection,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const grt::StringRef &) = &workbench_logical_Connection::endCaption;
      grt::StringRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::endCaption;
      meta->bind_member("endCaption", new grt::MetaClass::Property<workbench_logical_Connection,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const grt::DoubleRef &) = &workbench_logical_Connection::endCaptionXOffs;
      grt::DoubleRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::endCaptionXOffs;
      meta->bind_member("endCaptionXOffs", new grt::MetaClass::Property<workbench_logical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const grt::DoubleRef &) = &workbench_logical_Connection::endCaptionYOffs;
      grt::DoubleRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::endCaptionYOffs;
      meta->bind_member("endCaptionYOffs", new grt::MetaClass::Property<workbench_logical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const model_FigureRef &) = 0;
      model_FigureRef (workbench_logical_Connection::*getter)() const = 0;
      meta->bind_member("endFigure", new grt::MetaClass::Property<workbench_logical_Connection,model_FigureRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const grt::IntegerRef &) = &workbench_logical_Connection::endMany;
      grt::IntegerRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::endMany;
      meta->bind_member("endMany", new grt::MetaClass::Property<workbench_logical_Connection,grt::IntegerRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const grt::StringRef &) = &workbench_logical_Connection::startCaption;
      grt::StringRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::startCaption;
      meta->bind_member("startCaption", new grt::MetaClass::Property<workbench_logical_Connection,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const grt::DoubleRef &) = &workbench_logical_Connection::startCaptionXOffs;
      grt::DoubleRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::startCaptionXOffs;
      meta->bind_member("startCaptionXOffs", new grt::MetaClass::Property<workbench_logical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const grt::DoubleRef &) = &workbench_logical_Connection::startCaptionYOffs;
      grt::DoubleRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::startCaptionYOffs;
      meta->bind_member("startCaptionYOffs", new grt::MetaClass::Property<workbench_logical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const model_FigureRef &) = 0;
      model_FigureRef (workbench_logical_Connection::*getter)() const = 0;
      meta->bind_member("startFigure", new grt::MetaClass::Property<workbench_logical_Connection,model_FigureRef>(getter, setter));
    }
    {
      void (workbench_logical_Connection::*setter)(const grt::IntegerRef &) = &workbench_logical_Connection::startMany;
      grt::IntegerRef (workbench_logical_Connection::*getter)() const = &workbench_logical_Connection::startMany;
      meta->bind_member("startMany", new grt::MetaClass::Property<workbench_logical_Connection,grt::IntegerRef>(getter, setter));
    }
  }
};

/** a model figure representing a relationship */
class  workbench_logical_Relationship : public model_Figure {
  typedef model_Figure super;

public:
  workbench_logical_Relationship(grt::MetaClass *meta = nullptr)
    : model_Figure(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _attributesExpanded(1) {
  }

  static std::string static_class_name() {
    return "workbench.logical.Relationship";
  }

  /**
   * Getter for attribute attributesExpanded
   *
   * indicates whether the columns list is expanded
   * \par In Python:
   *    value = obj.attributesExpanded
   */
  grt::IntegerRef attributesExpanded() const { return _attributesExpanded; }

  /**
   * Setter for attribute attributesExpanded
   * 
   * indicates whether the columns list is expanded
   * \par In Python:
   *   obj.attributesExpanded = value
   */
  virtual void attributesExpanded(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_attributesExpanded);
    _attributesExpanded = value;
    member_changed("attributesExpanded", ovalue, value);
  }

  /**
   * Getter for attribute relationship
   *
   * the relationship that is represented
   * \par In Python:
   *    value = obj.relationship
   */
  eer_RelationshipRef relationship() const { return _relationship; }

  /**
   * Setter for attribute relationship
   * 
   * the relationship that is represented
   * \par In Python:
   *   obj.relationship = value
   */
  virtual void relationship(const eer_RelationshipRef &value) {
    grt::ValueRef ovalue(_relationship);
    _relationship = value;
    member_changed("relationship", ovalue, value);
  }

protected:

  grt::IntegerRef _attributesExpanded;
  eer_RelationshipRef _relationship;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_logical_Relationship());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_logical_Relationship::create);
    {
      void (workbench_logical_Relationship::*setter)(const grt::IntegerRef &) = &workbench_logical_Relationship::attributesExpanded;
      grt::IntegerRef (workbench_logical_Relationship::*getter)() const = &workbench_logical_Relationship::attributesExpanded;
      meta->bind_member("attributesExpanded", new grt::MetaClass::Property<workbench_logical_Relationship,grt::IntegerRef>(getter, setter));
    }
    {
      void (workbench_logical_Relationship::*setter)(const eer_RelationshipRef &) = &workbench_logical_Relationship::relationship;
      eer_RelationshipRef (workbench_logical_Relationship::*getter)() const = &workbench_logical_Relationship::relationship;
      meta->bind_member("relationship", new grt::MetaClass::Property<workbench_logical_Relationship,eer_RelationshipRef>(getter, setter));
    }
  }
};

/** a model figure representing a table */
class  workbench_logical_Entity : public model_Figure {
  typedef model_Figure super;

public:
  workbench_logical_Entity(grt::MetaClass *meta = nullptr)
    : model_Figure(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _attributesExpanded(1) {
  }

  static std::string static_class_name() {
    return "workbench.logical.Entity";
  }

  /**
   * Getter for attribute attributesExpanded
   *
   * indicates whether the columns list is expanded
   * \par In Python:
   *    value = obj.attributesExpanded
   */
  grt::IntegerRef attributesExpanded() const { return _attributesExpanded; }

  /**
   * Setter for attribute attributesExpanded
   * 
   * indicates whether the columns list is expanded
   * \par In Python:
   *   obj.attributesExpanded = value
   */
  virtual void attributesExpanded(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_attributesExpanded);
    _attributesExpanded = value;
    member_changed("attributesExpanded", ovalue, value);
  }

  /**
   * Getter for attribute entity
   *
   * the entity this figure represents
   * \par In Python:
   *    value = obj.entity
   */
  eer_EntityRef entity() const { return _entity; }

  /**
   * Setter for attribute entity
   * 
   * the entity this figure represents
   * \par In Python:
   *   obj.entity = value
   */
  virtual void entity(const eer_EntityRef &value) {
    grt::ValueRef ovalue(_entity);
    _entity = value;
    member_changed("entity", ovalue, value);
  }

protected:

  grt::IntegerRef _attributesExpanded;
  eer_EntityRef _entity;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_logical_Entity());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_logical_Entity::create);
    {
      void (workbench_logical_Entity::*setter)(const grt::IntegerRef &) = &workbench_logical_Entity::attributesExpanded;
      grt::IntegerRef (workbench_logical_Entity::*getter)() const = &workbench_logical_Entity::attributesExpanded;
      meta->bind_member("attributesExpanded", new grt::MetaClass::Property<workbench_logical_Entity,grt::IntegerRef>(getter, setter));
    }
    {
      void (workbench_logical_Entity::*setter)(const eer_EntityRef &) = &workbench_logical_Entity::entity;
      eer_EntityRef (workbench_logical_Entity::*getter)() const = &workbench_logical_Entity::entity;
      meta->bind_member("entity", new grt::MetaClass::Property<workbench_logical_Entity,eer_EntityRef>(getter, setter));
    }
  }
};

/** a model diagram holding layers */
class GRT_STRUCTS_WORKBENCH_LOGICAL_PUBLIC workbench_logical_Diagram : public model_Diagram {
  typedef model_Diagram super;

public:
  class ImplData;
  friend class ImplData;
  workbench_logical_Diagram(grt::MetaClass *meta = nullptr)
    : model_Diagram(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
  }

  virtual ~workbench_logical_Diagram();

  static std::string static_class_name() {
    return "workbench.logical.Diagram";
  }

  /**
   * Method. 
   * \param x 
   * \param y 
   * \param width 
   * \param height 
   * \param name 
   * \return 
   */
  virtual model_LayerRef placeNewLayer(double x, double y, double width, double height, const std::string &name);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:


private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_logical_Diagram());
  }

  static grt::ValueRef call_placeNewLayer(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_logical_Diagram*>(self)->placeNewLayer(grt::DoubleRef::cast_from(args[0]), grt::DoubleRef::cast_from(args[1]), grt::DoubleRef::cast_from(args[2]), grt::DoubleRef::cast_from(args[3]), grt::StringRef::cast_from(args[4])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_logical_Diagram::create);
    meta->bind_method("placeNewLayer", &workbench_logical_Diagram::call_placeNewLayer);
  }
};

/** a logical model holding diagrams */
class GRT_STRUCTS_WORKBENCH_LOGICAL_PUBLIC workbench_logical_Model : public model_Model {
  typedef model_Model super;

public:
  class ImplData;
  friend class ImplData;
  workbench_logical_Model(grt::MetaClass *meta = nullptr)
    : model_Model(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
    _diagrams.content().__retype(grt::ObjectType, "workbench.logical.Diagram");
  }

  virtual ~workbench_logical_Model();

  static std::string static_class_name() {
    return "workbench.logical.Model";
  }

  // diagrams is owned by workbench_logical_Model
  /**
   * Getter for attribute diagrams (read-only)
   *
   * the list of all available diagrams
   * \par In Python:
   *    value = obj.diagrams
   */
  grt::ListRef<workbench_logical_Diagram> diagrams() const { return grt::ListRef<workbench_logical_Diagram>::cast_from(_diagrams); }


private: // The next attribute is read-only.
public:

  /**
   * Method. 
   * \param deferRealize 
   * \return 
   */
  virtual model_DiagramRef addNewDiagram(ssize_t deferRealize);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:


private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_logical_Model());
  }

  static grt::ValueRef call_addNewDiagram(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_logical_Model*>(self)->addNewDiagram(grt::IntegerRef::cast_from(args[0])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_logical_Model::create);
    {
      void (workbench_logical_Model::*setter)(const grt::ListRef<workbench_logical_Diagram> &) = 0;
      grt::ListRef<workbench_logical_Diagram> (workbench_logical_Model::*getter)() const = 0;
      meta->bind_member("diagrams", new grt::MetaClass::Property<workbench_logical_Model,grt::ListRef<workbench_logical_Diagram>>(getter, setter));
    }
    meta->bind_method("addNewDiagram", &workbench_logical_Model::call_addNewDiagram);
  }
};



inline void register_structs_workbench_logical_xml() {
  grt::internal::ClassRegistry::register_class<workbench_logical_Connection>();
  grt::internal::ClassRegistry::register_class<workbench_logical_Relationship>();
  grt::internal::ClassRegistry::register_class<workbench_logical_Entity>();
  grt::internal::ClassRegistry::register_class<workbench_logical_Diagram>();
  grt::internal::ClassRegistry::register_class<workbench_logical_Model>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_workbench_logical_xml {
  _autoreg__structs_workbench_logical_xml() {
    register_structs_workbench_logical_xml();
  }
} __autoreg__structs_workbench_logical_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

