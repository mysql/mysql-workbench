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
  #ifdef GRT_STRUCTS_WORKBENCH_PHYSICAL_EXPORT
  #define GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC
#endif

#include "grts/structs.h"
#include "grts/structs.model.h"
#include "grts/structs.meta.h"
#include "grts/structs.db.h"
#include "grts/structs.db.mgmt.h"

class workbench_physical_Layer;
typedef grt::Ref<workbench_physical_Layer> workbench_physical_LayerRef;
class workbench_physical_Connection;
typedef grt::Ref<workbench_physical_Connection> workbench_physical_ConnectionRef;
class workbench_physical_RoutineGroupFigure;
typedef grt::Ref<workbench_physical_RoutineGroupFigure> workbench_physical_RoutineGroupFigureRef;
class workbench_physical_ViewFigure;
typedef grt::Ref<workbench_physical_ViewFigure> workbench_physical_ViewFigureRef;
class workbench_physical_TableFigure;
typedef grt::Ref<workbench_physical_TableFigure> workbench_physical_TableFigureRef;
class workbench_physical_Diagram;
typedef grt::Ref<workbench_physical_Diagram> workbench_physical_DiagramRef;
class workbench_physical_Model;
typedef grt::Ref<workbench_physical_Model> workbench_physical_ModelRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

class  workbench_physical_Layer : public model_Layer {
  typedef model_Layer super;

public:
  class ImplData;
  friend class ImplData;
  workbench_physical_Layer(grt::MetaClass *meta = nullptr)
    : model_Layer(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "workbench.physical.Layer";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_physical_Layer());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_physical_Layer::create);
  }
};

/** a model connection */
class GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC workbench_physical_Connection : public model_Connection {
  typedef model_Connection super;

public:
  class ImplData;
  friend class ImplData;
  workbench_physical_Connection(grt::MetaClass *meta = nullptr)
    : model_Connection(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _caption(""),
      _captionXOffs(0.0),
      _captionYOffs(0.0),
      _comment(""),
      _endCaptionXOffs(0.0),
      _endCaptionYOffs(0.0),
      _extraCaption(""),
      _extraCaptionXOffs(0.0),
      _extraCaptionYOffs(0.0),
      _middleSegmentOffset(0.0),
      _startCaptionXOffs(0.0),
      _startCaptionYOffs(0.0),
      _data(nullptr) {
  }

  virtual ~workbench_physical_Connection();

  static std::string static_class_name() {
    return "workbench.physical.Connection";
  }

  /**
   * Getter for attribute caption
   *
   * center caption
   * \par In Python:
   *    value = obj.caption
   */
  grt::StringRef caption() const { return _caption; }

  /**
   * Setter for attribute caption
   * 
   * center caption
   * \par In Python:
   *   obj.caption = value
   */
  virtual void caption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_caption);
    _caption = value;
    member_changed("caption", ovalue, value);
  }

  /**
   * Getter for attribute captionXOffs
   *
   * X offset of the caption
   * \par In Python:
   *    value = obj.captionXOffs
   */
  grt::DoubleRef captionXOffs() const { return _captionXOffs; }

  /**
   * Setter for attribute captionXOffs
   * 
   * X offset of the caption
   * \par In Python:
   *   obj.captionXOffs = value
   */
  virtual void captionXOffs(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_captionXOffs);
    _captionXOffs = value;
    member_changed("captionXOffs", ovalue, value);
  }

  /**
   * Getter for attribute captionYOffs
   *
   * Y offset of the caption
   * \par In Python:
   *    value = obj.captionYOffs
   */
  grt::DoubleRef captionYOffs() const { return _captionYOffs; }

  /**
   * Setter for attribute captionYOffs
   * 
   * Y offset of the caption
   * \par In Python:
   *   obj.captionYOffs = value
   */
  virtual void captionYOffs(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_captionYOffs);
    _captionYOffs = value;
    member_changed("captionYOffs", ovalue, value);
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
   * Getter for attribute extraCaption
   *
   * additional center caption
   * \par In Python:
   *    value = obj.extraCaption
   */
  grt::StringRef extraCaption() const { return _extraCaption; }

  /**
   * Setter for attribute extraCaption
   * 
   * additional center caption
   * \par In Python:
   *   obj.extraCaption = value
   */
  virtual void extraCaption(const grt::StringRef &value) {
    grt::ValueRef ovalue(_extraCaption);
    _extraCaption = value;
    member_changed("extraCaption", ovalue, value);
  }

  /**
   * Getter for attribute extraCaptionXOffs
   *
   * X offset of the caption
   * \par In Python:
   *    value = obj.extraCaptionXOffs
   */
  grt::DoubleRef extraCaptionXOffs() const { return _extraCaptionXOffs; }

  /**
   * Setter for attribute extraCaptionXOffs
   * 
   * X offset of the caption
   * \par In Python:
   *   obj.extraCaptionXOffs = value
   */
  virtual void extraCaptionXOffs(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_extraCaptionXOffs);
    _extraCaptionXOffs = value;
    member_changed("extraCaptionXOffs", ovalue, value);
  }

  /**
   * Getter for attribute extraCaptionYOffs
   *
   * Y offset of the caption
   * \par In Python:
   *    value = obj.extraCaptionYOffs
   */
  grt::DoubleRef extraCaptionYOffs() const { return _extraCaptionYOffs; }

  /**
   * Setter for attribute extraCaptionYOffs
   * 
   * Y offset of the caption
   * \par In Python:
   *   obj.extraCaptionYOffs = value
   */
  virtual void extraCaptionYOffs(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_extraCaptionYOffs);
    _extraCaptionYOffs = value;
    member_changed("extraCaptionYOffs", ovalue, value);
  }

  /**
   * Getter for attribute foreignKey
   *
   * the foreign key this corresponds to
   * \par In Python:
   *    value = obj.foreignKey
   */
  db_ForeignKeyRef foreignKey() const { return _foreignKey; }

  /**
   * Setter for attribute foreignKey
   * 
   * the foreign key this corresponds to
   * \par In Python:
   *   obj.foreignKey = value
   */
  virtual void foreignKey(const db_ForeignKeyRef &value);

  /**
   * Getter for attribute middleSegmentOffset
   *
   * offset of the middle segment of the line, if applicable
   * \par In Python:
   *    value = obj.middleSegmentOffset
   */
  grt::DoubleRef middleSegmentOffset() const { return _middleSegmentOffset; }

  /**
   * Setter for attribute middleSegmentOffset
   * 
   * offset of the middle segment of the line, if applicable
   * \par In Python:
   *   obj.middleSegmentOffset = value
   */
  virtual void middleSegmentOffset(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_middleSegmentOffset);
    _middleSegmentOffset = value;
    member_changed("middleSegmentOffset", ovalue, value);
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


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::StringRef _caption;
  grt::DoubleRef _captionXOffs;
  grt::DoubleRef _captionYOffs;
  grt::StringRef _comment;
  grt::DoubleRef _endCaptionXOffs;
  grt::DoubleRef _endCaptionYOffs;
  grt::StringRef _extraCaption;
  grt::DoubleRef _extraCaptionXOffs;
  grt::DoubleRef _extraCaptionYOffs;
  db_ForeignKeyRef _foreignKey;
  grt::DoubleRef _middleSegmentOffset;
  grt::DoubleRef _startCaptionXOffs;
  grt::DoubleRef _startCaptionYOffs;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_physical_Connection());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_physical_Connection::create);
    {
      void (workbench_physical_Connection::*setter)(const grt::StringRef &) = &workbench_physical_Connection::caption;
      grt::StringRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::caption;
      meta->bind_member("caption", new grt::MetaClass::Property<workbench_physical_Connection,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::captionXOffs;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::captionXOffs;
      meta->bind_member("captionXOffs", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::captionYOffs;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::captionYOffs;
      meta->bind_member("captionYOffs", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::StringRef &) = &workbench_physical_Connection::comment;
      grt::StringRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::comment;
      meta->bind_member("comment", new grt::MetaClass::Property<workbench_physical_Connection,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::endCaptionXOffs;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::endCaptionXOffs;
      meta->bind_member("endCaptionXOffs", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::endCaptionYOffs;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::endCaptionYOffs;
      meta->bind_member("endCaptionYOffs", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::StringRef &) = &workbench_physical_Connection::extraCaption;
      grt::StringRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::extraCaption;
      meta->bind_member("extraCaption", new grt::MetaClass::Property<workbench_physical_Connection,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::extraCaptionXOffs;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::extraCaptionXOffs;
      meta->bind_member("extraCaptionXOffs", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::extraCaptionYOffs;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::extraCaptionYOffs;
      meta->bind_member("extraCaptionYOffs", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const db_ForeignKeyRef &) = &workbench_physical_Connection::foreignKey;
      db_ForeignKeyRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::foreignKey;
      meta->bind_member("foreignKey", new grt::MetaClass::Property<workbench_physical_Connection,db_ForeignKeyRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::middleSegmentOffset;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::middleSegmentOffset;
      meta->bind_member("middleSegmentOffset", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::startCaptionXOffs;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::startCaptionXOffs;
      meta->bind_member("startCaptionXOffs", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
    {
      void (workbench_physical_Connection::*setter)(const grt::DoubleRef &) = &workbench_physical_Connection::startCaptionYOffs;
      grt::DoubleRef (workbench_physical_Connection::*getter)() const = &workbench_physical_Connection::startCaptionYOffs;
      meta->bind_member("startCaptionYOffs", new grt::MetaClass::Property<workbench_physical_Connection,grt::DoubleRef>(getter, setter));
    }
  }
};

/** a model figure representing a collection of routines */
class GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC workbench_physical_RoutineGroupFigure : public model_Figure {
  typedef model_Figure super;

public:
  class ImplData;
  friend class ImplData;
  workbench_physical_RoutineGroupFigure(grt::MetaClass *meta = nullptr)
    : model_Figure(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
  }

  virtual ~workbench_physical_RoutineGroupFigure();

  static std::string static_class_name() {
    return "workbench.physical.RoutineGroupFigure";
  }

  /**
   * Getter for attribute routineGroup
   *
   * the routine group this figure represents
   * \par In Python:
   *    value = obj.routineGroup
   */
  db_RoutineGroupRef routineGroup() const { return _routineGroup; }

  /**
   * Setter for attribute routineGroup
   * 
   * the routine group this figure represents
   * \par In Python:
   *   obj.routineGroup = value
   */
  virtual void routineGroup(const db_RoutineGroupRef &value);


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  db_RoutineGroupRef _routineGroup;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_physical_RoutineGroupFigure());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_physical_RoutineGroupFigure::create);
    {
      void (workbench_physical_RoutineGroupFigure::*setter)(const db_RoutineGroupRef &) = &workbench_physical_RoutineGroupFigure::routineGroup;
      db_RoutineGroupRef (workbench_physical_RoutineGroupFigure::*getter)() const = &workbench_physical_RoutineGroupFigure::routineGroup;
      meta->bind_member("routineGroup", new grt::MetaClass::Property<workbench_physical_RoutineGroupFigure,db_RoutineGroupRef>(getter, setter));
    }
  }
};

/** a model figure representing a view */
class GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC workbench_physical_ViewFigure : public model_Figure {
  typedef model_Figure super;

public:
  class ImplData;
  friend class ImplData;
  workbench_physical_ViewFigure(grt::MetaClass *meta = nullptr)
    : model_Figure(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
  }

  virtual ~workbench_physical_ViewFigure();

  static std::string static_class_name() {
    return "workbench.physical.ViewFigure";
  }

  /**
   * Getter for attribute view
   *
   * the view this figure represents
   * \par In Python:
   *    value = obj.view
   */
  db_ViewRef view() const { return _view; }

  /**
   * Setter for attribute view
   * 
   * the view this figure represents
   * \par In Python:
   *   obj.view = value
   */
  virtual void view(const db_ViewRef &value);


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  db_ViewRef _view;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_physical_ViewFigure());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_physical_ViewFigure::create);
    {
      void (workbench_physical_ViewFigure::*setter)(const db_ViewRef &) = &workbench_physical_ViewFigure::view;
      db_ViewRef (workbench_physical_ViewFigure::*getter)() const = &workbench_physical_ViewFigure::view;
      meta->bind_member("view", new grt::MetaClass::Property<workbench_physical_ViewFigure,db_ViewRef>(getter, setter));
    }
  }
};

/** a model figure representing a table */
class GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC workbench_physical_TableFigure : public model_Figure {
  typedef model_Figure super;

public:
  class ImplData;
  friend class ImplData;
  workbench_physical_TableFigure(grt::MetaClass *meta = nullptr)
    : model_Figure(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _columnsExpanded(1),
      _foreignKeysExpanded(0),
      _indicesExpanded(0),
      _summarizeDisplay(-1),
      _triggersExpanded(0),
      _data(nullptr) {
  }

  virtual ~workbench_physical_TableFigure();

  static std::string static_class_name() {
    return "workbench.physical.TableFigure";
  }

  /**
   * Getter for attribute columnsExpanded
   *
   * indicates whether the columns list is expanded
   * \par In Python:
   *    value = obj.columnsExpanded
   */
  grt::IntegerRef columnsExpanded() const { return _columnsExpanded; }

  /**
   * Setter for attribute columnsExpanded
   * 
   * indicates whether the columns list is expanded
   * \par In Python:
   *   obj.columnsExpanded = value
   */
  virtual void columnsExpanded(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_columnsExpanded);
    _columnsExpanded = value;
    member_changed("columnsExpanded", ovalue, value);
  }

  /**
   * Getter for attribute foreignKeysExpanded
   *
   * indicates whether the foreign keys list is expanded
   * \par In Python:
   *    value = obj.foreignKeysExpanded
   */
  grt::IntegerRef foreignKeysExpanded() const { return _foreignKeysExpanded; }

  /**
   * Setter for attribute foreignKeysExpanded
   * 
   * indicates whether the foreign keys list is expanded
   * \par In Python:
   *   obj.foreignKeysExpanded = value
   */
  virtual void foreignKeysExpanded(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_foreignKeysExpanded);
    _foreignKeysExpanded = value;
    member_changed("foreignKeysExpanded", ovalue, value);
  }

  /**
   * Getter for attribute indicesExpanded
   *
   * indicates whether the indices list is expanded
   * \par In Python:
   *    value = obj.indicesExpanded
   */
  grt::IntegerRef indicesExpanded() const { return _indicesExpanded; }

  /**
   * Setter for attribute indicesExpanded
   * 
   * indicates whether the indices list is expanded
   * \par In Python:
   *   obj.indicesExpanded = value
   */
  virtual void indicesExpanded(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_indicesExpanded);
    _indicesExpanded = value;
    member_changed("indicesExpanded", ovalue, value);
  }

  /**
   * Getter for attribute summarizeDisplay
   *
   * set to -1 for showing table in summarized view mode if there's too many columns, 0 to show all columns and 1 to force summary view
   * \par In Python:
   *    value = obj.summarizeDisplay
   */
  grt::IntegerRef summarizeDisplay() const { return _summarizeDisplay; }

  /**
   * Setter for attribute summarizeDisplay
   * 
   * set to -1 for showing table in summarized view mode if there's too many columns, 0 to show all columns and 1 to force summary view
   * \par In Python:
   *   obj.summarizeDisplay = value
   */
  virtual void summarizeDisplay(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_summarizeDisplay);
    _summarizeDisplay = value;
    member_changed("summarizeDisplay", ovalue, value);
  }

  /**
   * Getter for attribute table
   *
   * the table this figure represents
   * \par In Python:
   *    value = obj.table
   */
  db_TableRef table() const { return _table; }

  /**
   * Setter for attribute table
   * 
   * the table this figure represents
   * \par In Python:
   *   obj.table = value
   */
  virtual void table(const db_TableRef &value);

  /**
   * Getter for attribute triggersExpanded
   *
   * indicates whether the triggers list is expanded
   * \par In Python:
   *    value = obj.triggersExpanded
   */
  grt::IntegerRef triggersExpanded() const { return _triggersExpanded; }

  /**
   * Setter for attribute triggersExpanded
   * 
   * indicates whether the triggers list is expanded
   * \par In Python:
   *   obj.triggersExpanded = value
   */
  virtual void triggersExpanded(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_triggersExpanded);
    _triggersExpanded = value;
    member_changed("triggersExpanded", ovalue, value);
  }


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::IntegerRef _columnsExpanded;
  grt::IntegerRef _foreignKeysExpanded;
  grt::IntegerRef _indicesExpanded;
  grt::IntegerRef _summarizeDisplay;
  db_TableRef _table;
  grt::IntegerRef _triggersExpanded;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_physical_TableFigure());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_physical_TableFigure::create);
    {
      void (workbench_physical_TableFigure::*setter)(const grt::IntegerRef &) = &workbench_physical_TableFigure::columnsExpanded;
      grt::IntegerRef (workbench_physical_TableFigure::*getter)() const = &workbench_physical_TableFigure::columnsExpanded;
      meta->bind_member("columnsExpanded", new grt::MetaClass::Property<workbench_physical_TableFigure,grt::IntegerRef>(getter, setter));
    }
    {
      void (workbench_physical_TableFigure::*setter)(const grt::IntegerRef &) = &workbench_physical_TableFigure::foreignKeysExpanded;
      grt::IntegerRef (workbench_physical_TableFigure::*getter)() const = &workbench_physical_TableFigure::foreignKeysExpanded;
      meta->bind_member("foreignKeysExpanded", new grt::MetaClass::Property<workbench_physical_TableFigure,grt::IntegerRef>(getter, setter));
    }
    {
      void (workbench_physical_TableFigure::*setter)(const grt::IntegerRef &) = &workbench_physical_TableFigure::indicesExpanded;
      grt::IntegerRef (workbench_physical_TableFigure::*getter)() const = &workbench_physical_TableFigure::indicesExpanded;
      meta->bind_member("indicesExpanded", new grt::MetaClass::Property<workbench_physical_TableFigure,grt::IntegerRef>(getter, setter));
    }
    {
      void (workbench_physical_TableFigure::*setter)(const grt::IntegerRef &) = &workbench_physical_TableFigure::summarizeDisplay;
      grt::IntegerRef (workbench_physical_TableFigure::*getter)() const = &workbench_physical_TableFigure::summarizeDisplay;
      meta->bind_member("summarizeDisplay", new grt::MetaClass::Property<workbench_physical_TableFigure,grt::IntegerRef>(getter, setter));
    }
    {
      void (workbench_physical_TableFigure::*setter)(const db_TableRef &) = &workbench_physical_TableFigure::table;
      db_TableRef (workbench_physical_TableFigure::*getter)() const = &workbench_physical_TableFigure::table;
      meta->bind_member("table", new grt::MetaClass::Property<workbench_physical_TableFigure,db_TableRef>(getter, setter));
    }
    {
      void (workbench_physical_TableFigure::*setter)(const grt::IntegerRef &) = &workbench_physical_TableFigure::triggersExpanded;
      grt::IntegerRef (workbench_physical_TableFigure::*getter)() const = &workbench_physical_TableFigure::triggersExpanded;
      meta->bind_member("triggersExpanded", new grt::MetaClass::Property<workbench_physical_TableFigure,grt::IntegerRef>(getter, setter));
    }
  }
};

/** a model diagram holding layers and figures */
class GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC workbench_physical_Diagram : public model_Diagram {
  typedef model_Diagram super;

public:
  class ImplData;
  friend class ImplData;
  workbench_physical_Diagram(grt::MetaClass *meta = nullptr)
    : model_Diagram(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
  }

  virtual ~workbench_physical_Diagram();

  static std::string static_class_name() {
    return "workbench.physical.Diagram";
  }

  /**
   * Method. 
   * \param objects 
   * \return 
   */
  virtual void autoPlaceDBObjects(const grt::ListRef<db_DatabaseObject> &objects);
  /**
   * Method. 
   * \param fk 
   * \return 
   */
  virtual workbench_physical_ConnectionRef createConnectionForForeignKey(const db_ForeignKeyRef &fk);
  /**
   * Method. 
   * \param table 
   * \return 
   */
  virtual grt::IntegerRef createConnectionsForTable(const db_TableRef &table);
  /**
   * Method. 
   * \param table 
   * \return 
   */
  virtual void deleteConnectionsForTable(const db_TableRef &table);
  /**
   * Method. 
   * \param fk 
   * \return 
   */
  virtual workbench_physical_ConnectionRef getConnectionForForeignKey(const db_ForeignKeyRef &fk);
  /**
   * Method. 
   * \param object 
   * \return 
   */
  virtual model_FigureRef getFigureForDBObject(const db_DatabaseObjectRef &object);
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
  /**
   * Method. 
   * \param routineGroup 
   * \param x 
   * \param y 
   * \return 
   */
  virtual workbench_physical_RoutineGroupFigureRef placeRoutineGroup(const db_RoutineGroupRef &routineGroup, double x, double y);
  /**
   * Method. 
   * \param table 
   * \param x 
   * \param y 
   * \return 
   */
  virtual workbench_physical_TableFigureRef placeTable(const db_TableRef &table, double x, double y);
  /**
   * Method. 
   * \param view 
   * \param x 
   * \param y 
   * \return 
   */
  virtual workbench_physical_ViewFigureRef placeView(const db_ViewRef &view, double x, double y);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:


private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_physical_Diagram());
  }

  static grt::ValueRef call_autoPlaceDBObjects(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<workbench_physical_Diagram*>(self)->autoPlaceDBObjects(grt::ListRef<db_DatabaseObject>::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_createConnectionForForeignKey(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Diagram*>(self)->createConnectionForForeignKey(db_ForeignKeyRef::cast_from(args[0])); }

  static grt::ValueRef call_createConnectionsForTable(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Diagram*>(self)->createConnectionsForTable(db_TableRef::cast_from(args[0])); }

  static grt::ValueRef call_deleteConnectionsForTable(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<workbench_physical_Diagram*>(self)->deleteConnectionsForTable(db_TableRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_getConnectionForForeignKey(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Diagram*>(self)->getConnectionForForeignKey(db_ForeignKeyRef::cast_from(args[0])); }

  static grt::ValueRef call_getFigureForDBObject(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Diagram*>(self)->getFigureForDBObject(db_DatabaseObjectRef::cast_from(args[0])); }

  static grt::ValueRef call_placeNewLayer(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Diagram*>(self)->placeNewLayer(grt::DoubleRef::cast_from(args[0]), grt::DoubleRef::cast_from(args[1]), grt::DoubleRef::cast_from(args[2]), grt::DoubleRef::cast_from(args[3]), grt::StringRef::cast_from(args[4])); }

  static grt::ValueRef call_placeRoutineGroup(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Diagram*>(self)->placeRoutineGroup(db_RoutineGroupRef::cast_from(args[0]), grt::DoubleRef::cast_from(args[1]), grt::DoubleRef::cast_from(args[2])); }

  static grt::ValueRef call_placeTable(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Diagram*>(self)->placeTable(db_TableRef::cast_from(args[0]), grt::DoubleRef::cast_from(args[1]), grt::DoubleRef::cast_from(args[2])); }

  static grt::ValueRef call_placeView(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Diagram*>(self)->placeView(db_ViewRef::cast_from(args[0]), grt::DoubleRef::cast_from(args[1]), grt::DoubleRef::cast_from(args[2])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_physical_Diagram::create);
    meta->bind_method("autoPlaceDBObjects", &workbench_physical_Diagram::call_autoPlaceDBObjects);
    meta->bind_method("createConnectionForForeignKey", &workbench_physical_Diagram::call_createConnectionForForeignKey);
    meta->bind_method("createConnectionsForTable", &workbench_physical_Diagram::call_createConnectionsForTable);
    meta->bind_method("deleteConnectionsForTable", &workbench_physical_Diagram::call_deleteConnectionsForTable);
    meta->bind_method("getConnectionForForeignKey", &workbench_physical_Diagram::call_getConnectionForForeignKey);
    meta->bind_method("getFigureForDBObject", &workbench_physical_Diagram::call_getFigureForDBObject);
    meta->bind_method("placeNewLayer", &workbench_physical_Diagram::call_placeNewLayer);
    meta->bind_method("placeRoutineGroup", &workbench_physical_Diagram::call_placeRoutineGroup);
    meta->bind_method("placeTable", &workbench_physical_Diagram::call_placeTable);
    meta->bind_method("placeView", &workbench_physical_Diagram::call_placeView);
  }
};

/** a physical model holding diagrams */
class GRT_STRUCTS_WORKBENCH_PHYSICAL_PUBLIC workbench_physical_Model : public model_Model {
  typedef model_Model super;

public:
  class ImplData;
  friend class ImplData;
  workbench_physical_Model(grt::MetaClass *meta = nullptr)
    : model_Model(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _connectionNotation(""),
      _connections(this, false),
      _figureNotation(""),
      _notes(this, false),
      _scripts(this, false),
      _syncProfiles(this, false),
      _tagCategories(this, false),
      _tags(this, false),
      _data(nullptr) {
    _diagrams.content().__retype(grt::ObjectType, "workbench.physical.Diagram");
  }

  virtual ~workbench_physical_Model();

  static std::string static_class_name() {
    return "workbench.physical.Model";
  }

  // catalog is owned by workbench_physical_Model
  /**
   * Getter for attribute catalog
   *
   * 
   * \par In Python:
   *    value = obj.catalog
   */
  db_CatalogRef catalog() const { return _catalog; }

  /**
   * Setter for attribute catalog
   * 
   * 
   * \par In Python:
   *   obj.catalog = value
   */
  virtual void catalog(const db_CatalogRef &value) {
    grt::ValueRef ovalue(_catalog);

    _catalog = value;
    owned_member_changed("catalog", ovalue, value);
  }

  /**
   * Getter for attribute connectionNotation
   *
   * 
   * \par In Python:
   *    value = obj.connectionNotation
   */
  grt::StringRef connectionNotation() const { return _connectionNotation; }

  /**
   * Setter for attribute connectionNotation
   * 
   * 
   * \par In Python:
   *   obj.connectionNotation = value
   */
  virtual void connectionNotation(const grt::StringRef &value) {
    grt::ValueRef ovalue(_connectionNotation);
    _connectionNotation = value;
    member_changed("connectionNotation", ovalue, value);
  }

  // connections is owned by workbench_physical_Model
  /**
   * Getter for attribute connections (read-only)
   *
   * all connections that should be used for a full synchronisation
   * \par In Python:
   *    value = obj.connections
   */
  grt::ListRef<db_mgmt_Connection> connections() const { return _connections; }


private: // The next attribute is read-only.
  virtual void connections(const grt::ListRef<db_mgmt_Connection> &value) {
    grt::ValueRef ovalue(_connections);

    _connections = value;
    owned_member_changed("connections", ovalue, value);
  }
public:

  /**
   * Getter for attribute currentConnection
   *
   * the connection used for reverse engineering and synchronisation
   * \par In Python:
   *    value = obj.currentConnection
   */
  db_mgmt_ConnectionRef currentConnection() const { return _currentConnection; }

  /**
   * Setter for attribute currentConnection
   * 
   * the connection used for reverse engineering and synchronisation
   * \par In Python:
   *   obj.currentConnection = value
   */
  virtual void currentConnection(const db_mgmt_ConnectionRef &value) {
    grt::ValueRef ovalue(_currentConnection);
    _currentConnection = value;
    member_changed("currentConnection", ovalue, value);
  }

  // diagrams is owned by workbench_physical_Model
  /**
   * Getter for attribute diagrams (read-only)
   *
   * the list of all available diagrams
   * \par In Python:
   *    value = obj.diagrams
   */
  grt::ListRef<workbench_physical_Diagram> diagrams() const { return grt::ListRef<workbench_physical_Diagram>::cast_from(_diagrams); }


private: // The next attribute is read-only.
public:

  /**
   * Getter for attribute figureNotation
   *
   * 
   * \par In Python:
   *    value = obj.figureNotation
   */
  grt::StringRef figureNotation() const { return _figureNotation; }

  /**
   * Setter for attribute figureNotation
   * 
   * 
   * \par In Python:
   *   obj.figureNotation = value
   */
  virtual void figureNotation(const grt::StringRef &value) {
    grt::ValueRef ovalue(_figureNotation);
    _figureNotation = value;
    member_changed("figureNotation", ovalue, value);
  }

  // notes is owned by workbench_physical_Model
  /**
   * Getter for attribute notes (read-only)
   *
   * a list of notes that are stored with the model
   * \par In Python:
   *    value = obj.notes
   */
  grt::ListRef<GrtStoredNote> notes() const { return _notes; }


private: // The next attribute is read-only.
  virtual void notes(const grt::ListRef<GrtStoredNote> &value) {
    grt::ValueRef ovalue(_notes);

    _notes = value;
    owned_member_changed("notes", ovalue, value);
  }
public:

  /**
   * Getter for attribute rdbms
   *
   * the rdbms used for the document
   * \par In Python:
   *    value = obj.rdbms
   */
  db_mgmt_RdbmsRef rdbms() const { return _rdbms; }

  /**
   * Setter for attribute rdbms
   * 
   * the rdbms used for the document
   * \par In Python:
   *   obj.rdbms = value
   */
  virtual void rdbms(const db_mgmt_RdbmsRef &value) {
    grt::ValueRef ovalue(_rdbms);
    _rdbms = value;
    member_changed("rdbms", ovalue, value);
  }

  // scripts is owned by workbench_physical_Model
  /**
   * Getter for attribute scripts (read-only)
   *
   * a list of scripts that are stored with the model
   * \par In Python:
   *    value = obj.scripts
   */
  grt::ListRef<db_Script> scripts() const { return _scripts; }


private: // The next attribute is read-only.
  virtual void scripts(const grt::ListRef<db_Script> &value) {
    grt::ValueRef ovalue(_scripts);

    _scripts = value;
    owned_member_changed("scripts", ovalue, value);
  }
public:

  /**
   * Getter for attribute syncProfiles (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.syncProfiles
   */
  grt::DictRef syncProfiles() const { return _syncProfiles; }


private: // The next attribute is read-only.
  virtual void syncProfiles(const grt::DictRef &value) {
    grt::ValueRef ovalue(_syncProfiles);
    _syncProfiles = value;
    member_changed("syncProfiles", ovalue, value);
  }
public:

  // tagCategories is owned by workbench_physical_Model
  /**
   * Getter for attribute tagCategories (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.tagCategories
   */
  grt::ListRef<GrtObject> tagCategories() const { return _tagCategories; }


private: // The next attribute is read-only.
  virtual void tagCategories(const grt::ListRef<GrtObject> &value) {
    grt::ValueRef ovalue(_tagCategories);

    _tagCategories = value;
    owned_member_changed("tagCategories", ovalue, value);
  }
public:

  // tags is owned by workbench_physical_Model
  /**
   * Getter for attribute tags (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.tags
   */
  grt::ListRef<meta_Tag> tags() const { return _tags; }


private: // The next attribute is read-only.
  virtual void tags(const grt::ListRef<meta_Tag> &value) {
    grt::ValueRef ovalue(_tags);

    _tags = value;
    owned_member_changed("tags", ovalue, value);
  }
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

  db_CatalogRef _catalog;// owned
  grt::StringRef _connectionNotation;
  grt::ListRef<db_mgmt_Connection> _connections;// owned
  db_mgmt_ConnectionRef _currentConnection;
  grt::StringRef _figureNotation;
  grt::ListRef<GrtStoredNote> _notes;// owned
  db_mgmt_RdbmsRef _rdbms;
  grt::ListRef<db_Script> _scripts;// owned
  grt::DictRef _syncProfiles;
  grt::ListRef<GrtObject> _tagCategories;// owned
  grt::ListRef<meta_Tag> _tags;// owned

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_physical_Model());
  }

  static grt::ValueRef call_addNewDiagram(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<workbench_physical_Model*>(self)->addNewDiagram(grt::IntegerRef::cast_from(args[0])); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_physical_Model::create);
    {
      void (workbench_physical_Model::*setter)(const db_CatalogRef &) = &workbench_physical_Model::catalog;
      db_CatalogRef (workbench_physical_Model::*getter)() const = &workbench_physical_Model::catalog;
      meta->bind_member("catalog", new grt::MetaClass::Property<workbench_physical_Model,db_CatalogRef>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::StringRef &) = &workbench_physical_Model::connectionNotation;
      grt::StringRef (workbench_physical_Model::*getter)() const = &workbench_physical_Model::connectionNotation;
      meta->bind_member("connectionNotation", new grt::MetaClass::Property<workbench_physical_Model,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::ListRef<db_mgmt_Connection> &) = &workbench_physical_Model::connections;
      grt::ListRef<db_mgmt_Connection> (workbench_physical_Model::*getter)() const = &workbench_physical_Model::connections;
      meta->bind_member("connections", new grt::MetaClass::Property<workbench_physical_Model,grt::ListRef<db_mgmt_Connection>>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const db_mgmt_ConnectionRef &) = &workbench_physical_Model::currentConnection;
      db_mgmt_ConnectionRef (workbench_physical_Model::*getter)() const = &workbench_physical_Model::currentConnection;
      meta->bind_member("currentConnection", new grt::MetaClass::Property<workbench_physical_Model,db_mgmt_ConnectionRef>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::ListRef<workbench_physical_Diagram> &) = 0;
      grt::ListRef<workbench_physical_Diagram> (workbench_physical_Model::*getter)() const = 0;
      meta->bind_member("diagrams", new grt::MetaClass::Property<workbench_physical_Model,grt::ListRef<workbench_physical_Diagram>>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::StringRef &) = &workbench_physical_Model::figureNotation;
      grt::StringRef (workbench_physical_Model::*getter)() const = &workbench_physical_Model::figureNotation;
      meta->bind_member("figureNotation", new grt::MetaClass::Property<workbench_physical_Model,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::ListRef<GrtStoredNote> &) = &workbench_physical_Model::notes;
      grt::ListRef<GrtStoredNote> (workbench_physical_Model::*getter)() const = &workbench_physical_Model::notes;
      meta->bind_member("notes", new grt::MetaClass::Property<workbench_physical_Model,grt::ListRef<GrtStoredNote>>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const db_mgmt_RdbmsRef &) = &workbench_physical_Model::rdbms;
      db_mgmt_RdbmsRef (workbench_physical_Model::*getter)() const = &workbench_physical_Model::rdbms;
      meta->bind_member("rdbms", new grt::MetaClass::Property<workbench_physical_Model,db_mgmt_RdbmsRef>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::ListRef<db_Script> &) = &workbench_physical_Model::scripts;
      grt::ListRef<db_Script> (workbench_physical_Model::*getter)() const = &workbench_physical_Model::scripts;
      meta->bind_member("scripts", new grt::MetaClass::Property<workbench_physical_Model,grt::ListRef<db_Script>>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::DictRef &) = &workbench_physical_Model::syncProfiles;
      grt::DictRef (workbench_physical_Model::*getter)() const = &workbench_physical_Model::syncProfiles;
      meta->bind_member("syncProfiles", new grt::MetaClass::Property<workbench_physical_Model,grt::DictRef>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::ListRef<GrtObject> &) = &workbench_physical_Model::tagCategories;
      grt::ListRef<GrtObject> (workbench_physical_Model::*getter)() const = &workbench_physical_Model::tagCategories;
      meta->bind_member("tagCategories", new grt::MetaClass::Property<workbench_physical_Model,grt::ListRef<GrtObject>>(getter, setter));
    }
    {
      void (workbench_physical_Model::*setter)(const grt::ListRef<meta_Tag> &) = &workbench_physical_Model::tags;
      grt::ListRef<meta_Tag> (workbench_physical_Model::*getter)() const = &workbench_physical_Model::tags;
      meta->bind_member("tags", new grt::MetaClass::Property<workbench_physical_Model,grt::ListRef<meta_Tag>>(getter, setter));
    }
    meta->bind_method("addNewDiagram", &workbench_physical_Model::call_addNewDiagram);
  }
};



inline void register_structs_workbench_physical_xml() {
  grt::internal::ClassRegistry::register_class<workbench_physical_Layer>();
  grt::internal::ClassRegistry::register_class<workbench_physical_Connection>();
  grt::internal::ClassRegistry::register_class<workbench_physical_RoutineGroupFigure>();
  grt::internal::ClassRegistry::register_class<workbench_physical_ViewFigure>();
  grt::internal::ClassRegistry::register_class<workbench_physical_TableFigure>();
  grt::internal::ClassRegistry::register_class<workbench_physical_Diagram>();
  grt::internal::ClassRegistry::register_class<workbench_physical_Model>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_workbench_physical_xml {
  _autoreg__structs_workbench_physical_xml() {
    register_structs_workbench_physical_xml();
  }
} __autoreg__structs_workbench_physical_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

