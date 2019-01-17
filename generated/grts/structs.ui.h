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
  #ifdef GRT_STRUCTS_UI_EXPORT
  #define GRT_STRUCTS_UI_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_UI_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_UI_PUBLIC
#endif

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.model.h"
#include "grts/structs.wrapper.h"

class ui_db_ConnectPanel;
typedef grt::Ref<ui_db_ConnectPanel> ui_db_ConnectPanelRef;
class ui_ObjectEditor;
typedef grt::Ref<ui_ObjectEditor> ui_ObjectEditorRef;
class ui_ModelPanel;
typedef grt::Ref<ui_ModelPanel> ui_ModelPanelRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

class GRT_STRUCTS_UI_PUBLIC ui_db_ConnectPanel : public TransientObject {
  typedef TransientObject super;

public:
  class ImplData;
  friend class ImplData;
  ui_db_ConnectPanel(grt::MetaClass *meta = nullptr)
    : TransientObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _data(nullptr) {
  }

  virtual ~ui_db_ConnectPanel();

  static std::string static_class_name() {
    return "ui.db.ConnectPanel";
  }

  /**
   * Getter for attribute connection
   *
   * 
   * \par In Python:
   *    value = obj.connection
   */
  db_mgmt_ConnectionRef connection() const;

  /**
   * Setter for attribute connection
   * 
   * 
   * \par In Python:
   *   obj.connection = value
   */
  virtual void connection(const db_mgmt_ConnectionRef &value);

  /**
   * Getter for attribute view (read-only)
   *
   * reference to the toplevel mforms View of the connect panel
   * \par In Python:
   *    value = obj.view
   */
  mforms_ObjectReferenceRef view() const;


private: // The next attribute is read-only.
public:

  /**
   * Method. initializes the Connection Panel
   * \param mgmt 
   * \return 
   */
  virtual void initialize(const db_mgmt_ManagementRef &mgmt);
  /**
   * Method. initializes the Connection Panel
   * \param mgmt 
   * \param allowedRdbmsList 
   * \return 
   */
  virtual void initializeWithRDBMSSelector(const db_mgmt_ManagementRef &mgmt, const grt::ListRef<db_mgmt_Rdbms> &allowedRdbmsList);
  /**
   * Method. save the connection with the given name. Throws an exception if the connection name is duplicate or on other errors
   * \param name 
   * \return 
   */
  virtual void saveConnectionAs(const std::string &name);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:


private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new ui_db_ConnectPanel());
  }

  static grt::ValueRef call_initialize(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<ui_db_ConnectPanel*>(self)->initialize(db_mgmt_ManagementRef::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_initializeWithRDBMSSelector(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<ui_db_ConnectPanel*>(self)->initializeWithRDBMSSelector(db_mgmt_ManagementRef::cast_from(args[0]), grt::ListRef<db_mgmt_Rdbms>::cast_from(args[1])); return grt::ValueRef(); }

  static grt::ValueRef call_saveConnectionAs(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<ui_db_ConnectPanel*>(self)->saveConnectionAs(grt::StringRef::cast_from(args[0])); return grt::ValueRef(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&ui_db_ConnectPanel::create);
    {
      void (ui_db_ConnectPanel::*setter)(const db_mgmt_ConnectionRef &) = &ui_db_ConnectPanel::connection;
      db_mgmt_ConnectionRef (ui_db_ConnectPanel::*getter)() const = &ui_db_ConnectPanel::connection;
      meta->bind_member("connection", new grt::MetaClass::Property<ui_db_ConnectPanel,db_mgmt_ConnectionRef>(getter, setter));
    }
    meta->bind_member("view", new grt::MetaClass::Property<ui_db_ConnectPanel,mforms_ObjectReferenceRef>(&ui_db_ConnectPanel::view));
    meta->bind_method("initialize", &ui_db_ConnectPanel::call_initialize);
    meta->bind_method("initializeWithRDBMSSelector", &ui_db_ConnectPanel::call_initializeWithRDBMSSelector);
    meta->bind_method("saveConnectionAs", &ui_db_ConnectPanel::call_saveConnectionAs);
  }
};

class GRT_STRUCTS_UI_PUBLIC ui_ObjectEditor : public TransientObject {
  typedef TransientObject super;

public:
  class ImplData;
  friend class ImplData;
  ui_ObjectEditor(grt::MetaClass *meta = nullptr)
    : TransientObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _customData(this, false),
      _data(nullptr) {
  }

  virtual ~ui_ObjectEditor();

  static std::string static_class_name() {
    return "ui.ObjectEditor";
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

  /**
   * Getter for attribute dockingPoint
   *
   * 
   * \par In Python:
   *    value = obj.dockingPoint
   */
  mforms_ObjectReferenceRef dockingPoint() const { return _dockingPoint; }

  /**
   * Setter for attribute dockingPoint
   * 
   * 
   * \par In Python:
   *   obj.dockingPoint = value
   */
  virtual void dockingPoint(const mforms_ObjectReferenceRef &value) {
    grt::ValueRef ovalue(_dockingPoint);
    _dockingPoint = value;
    member_changed("dockingPoint", ovalue, value);
  }

  /**
   * Getter for attribute object
   *
   * 
   * \par In Python:
   *    value = obj.object
   */
  GrtObjectRef object() const { return _object; }

  /**
   * Setter for attribute object
   * 
   * 
   * \par In Python:
   *   obj.object = value
   */
  virtual void object(const GrtObjectRef &value) {
    grt::ValueRef ovalue(_object);
    _object = value;
    member_changed("object", ovalue, value);
  }


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::DictRef _customData;
  mforms_ObjectReferenceRef _dockingPoint;
  GrtObjectRef _object;

private: // Wrapper methods for use by the grt.
  ImplData *_data;

  static grt::ObjectRef create() {
    return grt::ObjectRef(new ui_ObjectEditor());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&ui_ObjectEditor::create);
    {
      void (ui_ObjectEditor::*setter)(const grt::DictRef &) = &ui_ObjectEditor::customData;
      grt::DictRef (ui_ObjectEditor::*getter)() const = &ui_ObjectEditor::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<ui_ObjectEditor,grt::DictRef>(getter, setter));
    }
    {
      void (ui_ObjectEditor::*setter)(const mforms_ObjectReferenceRef &) = &ui_ObjectEditor::dockingPoint;
      mforms_ObjectReferenceRef (ui_ObjectEditor::*getter)() const = &ui_ObjectEditor::dockingPoint;
      meta->bind_member("dockingPoint", new grt::MetaClass::Property<ui_ObjectEditor,mforms_ObjectReferenceRef>(getter, setter));
    }
    {
      void (ui_ObjectEditor::*setter)(const GrtObjectRef &) = &ui_ObjectEditor::object;
      GrtObjectRef (ui_ObjectEditor::*getter)() const = &ui_ObjectEditor::object;
      meta->bind_member("object", new grt::MetaClass::Property<ui_ObjectEditor,GrtObjectRef>(getter, setter));
    }
  }
};

class  ui_ModelPanel : public TransientObject {
  typedef TransientObject super;

public:
  ui_ModelPanel(grt::MetaClass *meta = nullptr)
    : TransientObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _customData(this, false) {
  }

  static std::string static_class_name() {
    return "ui.ModelPanel";
  }

  /**
   * Getter for attribute commonSidebar
   *
   * 
   * \par In Python:
   *    value = obj.commonSidebar
   */
  mforms_ObjectReferenceRef commonSidebar() const { return _commonSidebar; }

  /**
   * Setter for attribute commonSidebar
   * 
   * 
   * \par In Python:
   *   obj.commonSidebar = value
   */
  virtual void commonSidebar(const mforms_ObjectReferenceRef &value) {
    grt::ValueRef ovalue(_commonSidebar);
    _commonSidebar = value;
    member_changed("commonSidebar", ovalue, value);
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

  /**
   * Getter for attribute model
   *
   * 
   * \par In Python:
   *    value = obj.model
   */
  model_ModelRef model() const { return _model; }

  /**
   * Setter for attribute model
   * 
   * 
   * \par In Python:
   *   obj.model = value
   */
  virtual void model(const model_ModelRef &value) {
    grt::ValueRef ovalue(_model);
    _model = value;
    member_changed("model", ovalue, value);
  }

protected:

  mforms_ObjectReferenceRef _commonSidebar;
  grt::DictRef _customData;
  model_ModelRef _model;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new ui_ModelPanel());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&ui_ModelPanel::create);
    {
      void (ui_ModelPanel::*setter)(const mforms_ObjectReferenceRef &) = &ui_ModelPanel::commonSidebar;
      mforms_ObjectReferenceRef (ui_ModelPanel::*getter)() const = &ui_ModelPanel::commonSidebar;
      meta->bind_member("commonSidebar", new grt::MetaClass::Property<ui_ModelPanel,mforms_ObjectReferenceRef>(getter, setter));
    }
    {
      void (ui_ModelPanel::*setter)(const grt::DictRef &) = &ui_ModelPanel::customData;
      grt::DictRef (ui_ModelPanel::*getter)() const = &ui_ModelPanel::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<ui_ModelPanel,grt::DictRef>(getter, setter));
    }
    {
      void (ui_ModelPanel::*setter)(const model_ModelRef &) = &ui_ModelPanel::model;
      model_ModelRef (ui_ModelPanel::*getter)() const = &ui_ModelPanel::model;
      meta->bind_member("model", new grt::MetaClass::Property<ui_ModelPanel,model_ModelRef>(getter, setter));
    }
  }
};



inline void register_structs_ui_xml() {
  grt::internal::ClassRegistry::register_class<ui_db_ConnectPanel>();
  grt::internal::ClassRegistry::register_class<ui_ObjectEditor>();
  grt::internal::ClassRegistry::register_class<ui_ModelPanel>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_ui_xml {
  _autoreg__structs_ui_xml() {
    register_structs_ui_xml();
  }
} __autoreg__structs_ui_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

