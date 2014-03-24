#ifndef __grts_structs_ui_h__
#define __grts_structs_ui_h__

#include <grtpp.h>

#ifdef _WIN32
  #pragma warning(disable: 4355) // 'this' : used in base member initizalizer list
  #ifdef GRT_STRUCTS_UI_EXPORT
  #define GRT_STRUCTS_UI_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_UI_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_UI_PUBLIC
#endif

#include <grts/structs.h>
#include <grts/structs.db.mgmt.h>
#include <grts/structs.model.h>


class TransientObject;
typedef grt::Ref<TransientObject> TransientObjectRef;
class ui_db_ConnectPanel;
typedef grt::Ref<ui_db_ConnectPanel> ui_db_ConnectPanelRef;
class ui_ObjectEditor;
typedef grt::Ref<ui_ObjectEditor> ui_ObjectEditorRef;
class ui_ModelPanel;
typedef grt::Ref<ui_ModelPanel> ui_ModelPanelRef;
class mforms_ObjectReference;
typedef grt::Ref<mforms_ObjectReference> mforms_ObjectReferenceRef;
class grt_PyObject;
typedef grt::Ref<grt_PyObject> grt_PyObjectRef;


namespace mforms { 
	class Object;
}; 

namespace grt { 
	class AutoPyObject;
}; 

  /** the parent of all transient (non persistent) objects */
class  TransientObject : public grt::internal::Object
{
  typedef grt::internal::Object super;
public:
  TransientObject(grt::GRT *grt, grt::MetaClass *meta=0)
  : grt::internal::Object(grt, meta ? meta : grt->get_metaclass(static_class_name()))

  {
  }

  static std::string static_class_name() { return "TransientObject"; }

protected:

private: // wrapper methods for use by grt
  static grt::ObjectRef create(grt::GRT *grt)
  {
    return grt::ObjectRef(new TransientObject(grt));
  }


public:
  static void grt_register(grt::GRT *grt)
  {
    grt::MetaClass *meta= grt->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&TransientObject::create);
  }
};


class GRT_STRUCTS_UI_PUBLIC ui_db_ConnectPanel : public TransientObject
{
  typedef TransientObject super;
public:
  class ImplData;
  friend class ImplData;
  ui_db_ConnectPanel(grt::GRT *grt, grt::MetaClass *meta=0)
  : TransientObject(grt, meta ? meta : grt->get_metaclass(static_class_name())),
    _data(0)

  {
  }

  virtual ~ui_db_ConnectPanel();

  static std::string static_class_name() { return "ui.db.ConnectPanel"; }

  /** Getter for attribute connection
   
    
   \par In Python:
value = obj.connection
   */
  grt::Ref<db_mgmt_Connection> connection() const;
  /** Setter for attribute connection
   
    
    \par In Python:
obj.connection = value
   */
  virtual void connection(const grt::Ref<db_mgmt_Connection> &value);

  /** Getter for attribute view (read-only)
   
    reference to the toplevel mforms View of the connect panel
   \par In Python:
value = obj.view
   */
  grt::Ref<mforms_ObjectReference> view() const;
private: // the next attribute is read-only
public:

  /** Method. initializes the Connection Panel
  \param mgmt 
  \return 

   */
  virtual void initialize(const grt::Ref<db_mgmt_Management> &mgmt);
  /** Method. initializes the Connection Panel
  \param mgmt 
  \param allowedRdbmsList 
  \return 

   */
  virtual void initializeWithRDBMSSelector(const grt::Ref<db_mgmt_Management> &mgmt, const grt::ListRef<db_mgmt_Rdbms> &allowedRdbmsList);
  /** Method. save the connection with the given name. Throws an exception if the connection name is duplicate or on other errors
  \param name 
  \return 

   */
  virtual void saveConnectionAs(const std::string &name);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by Ref<Object> constructor
  virtual void init();

protected:

private: // wrapper methods for use by grt
  ImplData *_data;

  static grt::ObjectRef create(grt::GRT *grt)
  {
    return grt::ObjectRef(new ui_db_ConnectPanel(grt));
  }

  static grt::ValueRef call_initialize(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<ui_db_ConnectPanel*>(self)->initialize(grt::Ref<db_mgmt_Management>::cast_from(args[0])); return grt::ValueRef(); }

  static grt::ValueRef call_initializeWithRDBMSSelector(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<ui_db_ConnectPanel*>(self)->initializeWithRDBMSSelector(grt::Ref<db_mgmt_Management>::cast_from(args[0]), grt::ListRef<db_mgmt_Rdbms>::cast_from(args[1])); return grt::ValueRef(); }

  static grt::ValueRef call_saveConnectionAs(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<ui_db_ConnectPanel*>(self)->saveConnectionAs(grt::StringRef::cast_from(args[0])); return grt::ValueRef(); }


public:
  static void grt_register(grt::GRT *grt)
  {
    grt::MetaClass *meta= grt->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&ui_db_ConnectPanel::create);
    {
      void (ui_db_ConnectPanel::*setter)(const grt::Ref<db_mgmt_Connection> &)= &ui_db_ConnectPanel::connection;
      grt::Ref<db_mgmt_Connection> (ui_db_ConnectPanel::*getter)() const= &ui_db_ConnectPanel::connection;
      meta->bind_member("connection", new grt::MetaClass::Property<ui_db_ConnectPanel,grt::Ref<db_mgmt_Connection> >(getter, setter));
    }
    meta->bind_member("view", new grt::MetaClass::Property<ui_db_ConnectPanel,grt::Ref<mforms_ObjectReference> >(&ui_db_ConnectPanel::view));
    meta->bind_method("initialize", &ui_db_ConnectPanel::call_initialize);
    meta->bind_method("initializeWithRDBMSSelector", &ui_db_ConnectPanel::call_initializeWithRDBMSSelector);
    meta->bind_method("saveConnectionAs", &ui_db_ConnectPanel::call_saveConnectionAs);
  }
};


class GRT_STRUCTS_UI_PUBLIC ui_ObjectEditor : public TransientObject
{
  typedef TransientObject super;
public:
  class ImplData;
  friend class ImplData;
  ui_ObjectEditor(grt::GRT *grt, grt::MetaClass *meta=0)
  : TransientObject(grt, meta ? meta : grt->get_metaclass(static_class_name())),
    _customData(grt, this, false),
    _data(0)

  {
  }

  virtual ~ui_ObjectEditor();

  static std::string static_class_name() { return "ui.ObjectEditor"; }

  /** Getter for attribute customData (read-only)
   
    
   \par In Python:
value = obj.customData
   */
  grt::DictRef customData() const { return _customData; }
private: // the next attribute is read-only
  virtual void customData(const grt::DictRef &value)
  {
    grt::ValueRef ovalue(_customData);
   _customData= value;
    member_changed("customData", ovalue, value);
  }
public:

  /** Getter for attribute dockingPoint
   
    
   \par In Python:
value = obj.dockingPoint
   */
  grt::Ref<mforms_ObjectReference> dockingPoint() const { return _dockingPoint; }
  /** Setter for attribute dockingPoint
   
    
    \par In Python:
obj.dockingPoint = value
   */
  virtual void dockingPoint(const grt::Ref<mforms_ObjectReference> &value)
  {
    grt::ValueRef ovalue(_dockingPoint);
   _dockingPoint= value;
    member_changed("dockingPoint", ovalue, value);
  }

  /** Getter for attribute object
   
    
   \par In Python:
value = obj.object
   */
  grt::Ref<GrtObject> object() const { return _object; }
  /** Setter for attribute object
   
    
    \par In Python:
obj.object = value
   */
  virtual void object(const grt::Ref<GrtObject> &value)
  {
    grt::ValueRef ovalue(_object);
   _object= value;
    member_changed("object", ovalue, value);
  }


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by Ref<Object> constructor
  virtual void init();

protected:

  grt::DictRef _customData;
  grt::Ref<mforms_ObjectReference> _dockingPoint;
  grt::Ref<GrtObject> _object;
private: // wrapper methods for use by grt
  ImplData *_data;

  static grt::ObjectRef create(grt::GRT *grt)
  {
    return grt::ObjectRef(new ui_ObjectEditor(grt));
  }


public:
  static void grt_register(grt::GRT *grt)
  {
    grt::MetaClass *meta= grt->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&ui_ObjectEditor::create);
    {
      void (ui_ObjectEditor::*setter)(const grt::DictRef &)= &ui_ObjectEditor::customData;
      grt::DictRef (ui_ObjectEditor::*getter)() const= &ui_ObjectEditor::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<ui_ObjectEditor,grt::DictRef >(getter,setter));
    }
    {
      void (ui_ObjectEditor::*setter)(const grt::Ref<mforms_ObjectReference> &)= &ui_ObjectEditor::dockingPoint;
      grt::Ref<mforms_ObjectReference> (ui_ObjectEditor::*getter)() const= &ui_ObjectEditor::dockingPoint;
      meta->bind_member("dockingPoint", new grt::MetaClass::Property<ui_ObjectEditor,grt::Ref<mforms_ObjectReference> >(getter,setter));
    }
    {
      void (ui_ObjectEditor::*setter)(const grt::Ref<GrtObject> &)= &ui_ObjectEditor::object;
      grt::Ref<GrtObject> (ui_ObjectEditor::*getter)() const= &ui_ObjectEditor::object;
      meta->bind_member("object", new grt::MetaClass::Property<ui_ObjectEditor,grt::Ref<GrtObject> >(getter,setter));
    }
  }
};


class  ui_ModelPanel : public TransientObject
{
  typedef TransientObject super;
public:
  ui_ModelPanel(grt::GRT *grt, grt::MetaClass *meta=0)
  : TransientObject(grt, meta ? meta : grt->get_metaclass(static_class_name())),
    _customData(grt, this, false)

  {
  }

  static std::string static_class_name() { return "ui.ModelPanel"; }

  /** Getter for attribute commonSidebar
   
    
   \par In Python:
value = obj.commonSidebar
   */
  grt::Ref<mforms_ObjectReference> commonSidebar() const { return _commonSidebar; }
  /** Setter for attribute commonSidebar
   
    
    \par In Python:
obj.commonSidebar = value
   */
  virtual void commonSidebar(const grt::Ref<mforms_ObjectReference> &value)
  {
    grt::ValueRef ovalue(_commonSidebar);
   _commonSidebar= value;
    member_changed("commonSidebar", ovalue, value);
  }

  /** Getter for attribute customData (read-only)
   
    
   \par In Python:
value = obj.customData
   */
  grt::DictRef customData() const { return _customData; }
private: // the next attribute is read-only
  virtual void customData(const grt::DictRef &value)
  {
    grt::ValueRef ovalue(_customData);
   _customData= value;
    member_changed("customData", ovalue, value);
  }
public:

  /** Getter for attribute model
   
    
   \par In Python:
value = obj.model
   */
  grt::Ref<model_Model> model() const { return _model; }
  /** Setter for attribute model
   
    
    \par In Python:
obj.model = value
   */
  virtual void model(const grt::Ref<model_Model> &value)
  {
    grt::ValueRef ovalue(_model);
   _model= value;
    member_changed("model", ovalue, value);
  }

protected:

  grt::Ref<mforms_ObjectReference> _commonSidebar;
  grt::DictRef _customData;
  grt::Ref<model_Model> _model;
private: // wrapper methods for use by grt
  static grt::ObjectRef create(grt::GRT *grt)
  {
    return grt::ObjectRef(new ui_ModelPanel(grt));
  }


public:
  static void grt_register(grt::GRT *grt)
  {
    grt::MetaClass *meta= grt->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&ui_ModelPanel::create);
    {
      void (ui_ModelPanel::*setter)(const grt::Ref<mforms_ObjectReference> &)= &ui_ModelPanel::commonSidebar;
      grt::Ref<mforms_ObjectReference> (ui_ModelPanel::*getter)() const= &ui_ModelPanel::commonSidebar;
      meta->bind_member("commonSidebar", new grt::MetaClass::Property<ui_ModelPanel,grt::Ref<mforms_ObjectReference> >(getter,setter));
    }
    {
      void (ui_ModelPanel::*setter)(const grt::DictRef &)= &ui_ModelPanel::customData;
      grt::DictRef (ui_ModelPanel::*getter)() const= &ui_ModelPanel::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<ui_ModelPanel,grt::DictRef >(getter,setter));
    }
    {
      void (ui_ModelPanel::*setter)(const grt::Ref<model_Model> &)= &ui_ModelPanel::model;
      grt::Ref<model_Model> (ui_ModelPanel::*getter)() const= &ui_ModelPanel::model;
      meta->bind_member("model", new grt::MetaClass::Property<ui_ModelPanel,grt::Ref<model_Model> >(getter,setter));
    }
  }
};


  /** an object representing a reference to a mforms object */
class GRT_STRUCTS_UI_PUBLIC mforms_ObjectReference : public TransientObject
{
  typedef TransientObject super;
public:
  typedef mforms::Object ImplData;
  mforms_ObjectReference(grt::GRT *grt, grt::MetaClass *meta=0)
  : TransientObject(grt, meta ? meta : grt->get_metaclass(static_class_name())),
     _type(""),
    _data(0), _release_data(NULL)

  {
  }

  virtual ~mforms_ObjectReference() { if (_release_data && _data) _release_data(_data);  }

  static std::string static_class_name() { return "mforms.ObjectReference"; }

  /** Getter for attribute type
   
    the specific type of mforms object
   \par In Python:
value = obj.type
   */
  grt::StringRef type() const { return _type; }
  /** Setter for attribute type
   
    the specific type of mforms object
    \par In Python:
obj.type = value
   */
  virtual void type(const grt::StringRef &value)
  {
    grt::ValueRef ovalue(_type);
   _type= value;
    member_changed("type", ovalue, value);
  }

  /** Getter for attribute valid (read-only)
   
    whether the object is currently valid
   \par In Python:
value = obj.valid
   */
  grt::IntegerRef valid() const;
private: // the next attribute is read-only
public:

  /** Method. checks whether the reference points to the same view as another refrence
  \param other 
  \return 

   */
  virtual grt::IntegerRef isEqualTo(const grt::Ref<mforms_ObjectReference> &other);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data, void (*release)(ImplData*))
  {
    if (_data == data) return;
    if (_data && _release_data) _release_data(_data);
    _data= data;
    _release_data= release;
  }
protected:

  grt::StringRef _type;
private: // wrapper methods for use by grt
  ImplData *_data;
  void (*_release_data)(ImplData *);

  static grt::ObjectRef create(grt::GRT *grt)
  {
    return grt::ObjectRef(new mforms_ObjectReference(grt));
  }

  static grt::ValueRef call_isEqualTo(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<mforms_ObjectReference*>(self)->isEqualTo(grt::Ref<mforms_ObjectReference>::cast_from(args[0])); }


public:
  static void grt_register(grt::GRT *grt)
  {
    grt::MetaClass *meta= grt->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&mforms_ObjectReference::create);
    {
      void (mforms_ObjectReference::*setter)(const grt::StringRef &)= &mforms_ObjectReference::type;
      grt::StringRef (mforms_ObjectReference::*getter)() const= &mforms_ObjectReference::type;
      meta->bind_member("type", new grt::MetaClass::Property<mforms_ObjectReference,grt::StringRef >(getter,setter));
    }
    meta->bind_member("valid", new grt::MetaClass::Property<mforms_ObjectReference,grt::IntegerRef >(&mforms_ObjectReference::valid));
    meta->bind_method("isEqualTo", &mforms_ObjectReference::call_isEqualTo);
  }
};


  /** wraps a Python object reference so it can be stored in a GRT container */
class GRT_STRUCTS_UI_PUBLIC grt_PyObject : public TransientObject
{
  typedef TransientObject super;
public:
  typedef grt::AutoPyObject ImplData;
  grt_PyObject(grt::GRT *grt, grt::MetaClass *meta=0)
  : TransientObject(grt, meta ? meta : grt->get_metaclass(static_class_name())),
    _data(0), _release_data(NULL)

  {
  }

  virtual ~grt_PyObject() { if (_release_data && _data) _release_data(_data);  }

  static std::string static_class_name() { return "grt.PyObject"; }

  /** Method. checks whether the reference points to the same object as another refrence
  \param other 
  \return 

   */
  virtual grt::IntegerRef isEqualTo(const grt::Ref<grt_PyObject> &other);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data, void (*release)(ImplData*))
  {
    if (_data == data) return;
    if (_data && _release_data) _release_data(_data);
    _data= data;
    _release_data= release;
  }
protected:

private: // wrapper methods for use by grt
  ImplData *_data;
  void (*_release_data)(ImplData *);

  static grt::ObjectRef create(grt::GRT *grt)
  {
    return grt::ObjectRef(new grt_PyObject(grt));
  }

  static grt::ValueRef call_isEqualTo(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<grt_PyObject*>(self)->isEqualTo(grt::Ref<grt_PyObject>::cast_from(args[0])); }


public:
  static void grt_register(grt::GRT *grt)
  {
    grt::MetaClass *meta= grt->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&grt_PyObject::create);
    meta->bind_method("isEqualTo", &grt_PyObject::call_isEqualTo);
  }
};




inline void register_structs_ui_xml()
{
  grt::internal::ClassRegistry::register_class<TransientObject>();
  grt::internal::ClassRegistry::register_class<ui_db_ConnectPanel>();
  grt::internal::ClassRegistry::register_class<ui_ObjectEditor>();
  grt::internal::ClassRegistry::register_class<ui_ModelPanel>();
  grt::internal::ClassRegistry::register_class<mforms_ObjectReference>();
  grt::internal::ClassRegistry::register_class<grt_PyObject>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_ui_xml { _autoreg__structs_ui_xml() { register_structs_ui_xml(); } } __autoreg__structs_ui_xml;
#endif

#endif
