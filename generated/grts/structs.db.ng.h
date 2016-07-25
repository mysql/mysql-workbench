#pragma once

#ifndef _WIN32
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include "grt.h"

#ifdef _WIN32
  #pragma warning(disable: 4355) // 'this' : used in base member initializer list
  #ifdef GRT_STRUCTS_DB_NG_EXPORT
  #define GRT_STRUCTS_DB_NG_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_DB_NG_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_DB_NG_PUBLIC
#endif

#include "grts/structs.h"
#include "grts/structs.ui.h"


class db_ng_Editor;
typedef grt::Ref<db_ng_Editor> db_ng_EditorRef;
class db_ng_Sheet;
typedef grt::Ref<db_ng_Sheet> db_ng_SheetRef;
class db_ng_Ide;
typedef grt::Ref<db_ng_Ide> db_ng_IdeRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

  /** a proxy to an instance of NgEditor.\n */
class GRT_STRUCTS_DB_NG_PUBLIC db_ng_Editor : public GrtObject
{
  typedef GrtObject super;
public:
  class ImplData;
  friend class ImplData;
  db_ng_Editor(grt::MetaClass *meta=0)
  : GrtObject(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
    _data(nullptr)

  {
  }

  virtual ~db_ng_Editor();

  static std::string static_class_name() { return "db.ng.Editor"; }

  /** Getter for attribute connection (read-only)
   
    connection data
   \par In Python:
value = obj.connection
   */
  db_mgmt_ConnectionRef connection() const;
private: // the next attribute is read-only
public:

  /** Getter for attribute script (read-only)
   
    editor contents
   \par In Python:
value = obj.script
   */
  grt::StringRef script() const;
private: // the next attribute is read-only
public:


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

private: // wrapper methods for use by grt
  ImplData *_data;

  static grt::ObjectRef create()
  {
    return grt::ObjectRef(new db_ng_Editor());
  }


public:
  static void grt_register()
  {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_ng_Editor::create);
    meta->bind_member("connection", new grt::MetaClass::Property<db_ng_Editor,db_mgmt_ConnectionRef >(&db_ng_Editor::connection));
    meta->bind_member("script", new grt::MetaClass::Property<db_ng_Editor,grt::StringRef >(&db_ng_Editor::script));
  }
};


  /** a proxy to an instance of NGShell.\n This object cannot be instantiated directly. */
class GRT_STRUCTS_DB_NG_PUBLIC db_ng_Sheet : public GrtObject
{
  typedef GrtObject super;
public:
  class ImplData;
  friend class ImplData;
  db_ng_Sheet(grt::MetaClass *meta=0)
  : GrtObject(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
    _customData(this, false),
    _data(nullptr)

  {
  }

  virtual ~db_ng_Sheet();

  static std::string static_class_name() { return "db.ng.Sheet"; }

  /** Getter for attribute connection (read-only)
   
    connection data
   \par In Python:
value = obj.connection
   */
  db_mgmt_ConnectionRef connection() const;
private: // the next attribute is read-only
public:

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
  mforms_ObjectReferenceRef dockingPoint() const { return _dockingPoint; }
  /** Setter for attribute dockingPoint
   
    
    \par In Python:
obj.dockingPoint = value
   */
  virtual void dockingPoint(const mforms_ObjectReferenceRef &value)
  {
    grt::ValueRef ovalue(_dockingPoint);
   _dockingPoint= value;
    member_changed("dockingPoint", ovalue, value);
  }

  /** Getter for attribute editor (read-only)
   
    editor that is currently selected
   \par In Python:
value = obj.editor
   */
  db_ng_EditorRef editor() const;
private: // the next attribute is read-only
  virtual void editor(const db_ng_EditorRef &value)
  {
    grt::ValueRef ovalue(_editor);
   _editor= value;
    member_changed("editor", ovalue, value);
  }
public:

  /** Getter for attribute sidebar
   
    
   \par In Python:
value = obj.sidebar
   */
  mforms_ObjectReferenceRef sidebar() const { return _sidebar; }
  /** Setter for attribute sidebar
   
    
    \par In Python:
obj.sidebar = value
   */
  virtual void sidebar(const mforms_ObjectReferenceRef &value)
  {
    grt::ValueRef ovalue(_sidebar);
   _sidebar= value;
    member_changed("sidebar", ovalue, value);
  }

  /** Method. Execute code in the active NgEditor
  \param currentStatementOnly 
  \return 

   */
  virtual void execute(ssize_t currentStatementOnly);

  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::DictRef _customData;
  mforms_ObjectReferenceRef _dockingPoint;
  db_ng_EditorRef _editor;
  mforms_ObjectReferenceRef _sidebar;
private: // wrapper methods for use by grt
  ImplData *_data;

  static grt::ObjectRef create()
  {
    return grt::ObjectRef(new db_ng_Sheet());
  }

  static grt::ValueRef call_execute(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<db_ng_Sheet*>(self)->execute(grt::IntegerRef::cast_from(args[0])); return grt::ValueRef(); }


public:
  static void grt_register()
  {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_ng_Sheet::create);
    meta->bind_member("connection", new grt::MetaClass::Property<db_ng_Sheet,db_mgmt_ConnectionRef >(&db_ng_Sheet::connection));
    {
      void (db_ng_Sheet::*setter)(const grt::DictRef &)= &db_ng_Sheet::customData;
      grt::DictRef (db_ng_Sheet::*getter)() const= &db_ng_Sheet::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<db_ng_Sheet,grt::DictRef >(getter,setter));
    }
    {
      void (db_ng_Sheet::*setter)(const mforms_ObjectReferenceRef &)= &db_ng_Sheet::dockingPoint;
      mforms_ObjectReferenceRef (db_ng_Sheet::*getter)() const= &db_ng_Sheet::dockingPoint;
      meta->bind_member("dockingPoint", new grt::MetaClass::Property<db_ng_Sheet,mforms_ObjectReferenceRef >(getter,setter));
    }
    {
      void (db_ng_Sheet::*setter)(const db_ng_EditorRef &)= &db_ng_Sheet::editor;
      db_ng_EditorRef (db_ng_Sheet::*getter)() const= &db_ng_Sheet::editor;
      meta->bind_member("editor", new grt::MetaClass::Property<db_ng_Sheet,db_ng_EditorRef >(getter,setter));
    }
    {
      void (db_ng_Sheet::*setter)(const mforms_ObjectReferenceRef &)= &db_ng_Sheet::sidebar;
      mforms_ObjectReferenceRef (db_ng_Sheet::*getter)() const= &db_ng_Sheet::sidebar;
      meta->bind_member("sidebar", new grt::MetaClass::Property<db_ng_Sheet,mforms_ObjectReferenceRef >(getter,setter));
    }
    meta->bind_method("execute", &db_ng_Sheet::call_execute);
  }
};


  /** a proxy to an instance of NGIde.\n This object cannot be instantiated directly. */
class GRT_STRUCTS_DB_NG_PUBLIC db_ng_Ide : public GrtObject
{
  typedef GrtObject super;
public:
  class ImplData;
  friend class ImplData;
  db_ng_Ide(grt::MetaClass *meta=0)
  : GrtObject(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
    _ngSheets(this, false),
    _data(nullptr)

  {
  }

  virtual ~db_ng_Ide();

  static std::string static_class_name() { return "db.ng.Ide"; }

  /** Getter for attribute activeNgSheet (read-only)
   
    sheet that is currently selected
   \par In Python:
value = obj.activeNgSheet
   */
  db_ng_SheetRef activeNgSheet() const;
private: // the next attribute is read-only
  virtual void activeNgSheet(const db_ng_SheetRef &value)
  {
    grt::ValueRef ovalue(_activeNgSheet);
   _activeNgSheet= value;
    member_changed("activeNgSheet", ovalue, value);
  }
public:

  // ngSheets is owned by db_ng_Ide
  /** Getter for attribute ngSheets (read-only)
   
    list of open ng sheets. This list cannot be modified
   \par In Python:
value = obj.ngSheets
   */
  grt::ListRef<db_ng_Sheet> ngSheets() const { return _ngSheets; }
private: // the next attribute is read-only
  virtual void ngSheets(const grt::ListRef<db_ng_Sheet> &value)
  {
    grt::ValueRef ovalue(_ngSheets);

    _ngSheets= value;
    owned_member_changed("ngSheets", ovalue, value);
  }
public:


  ImplData *get_data() const { return _data; }

  void set_data(ImplData *data);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  db_ng_SheetRef _activeNgSheet;
  grt::ListRef<db_ng_Sheet> _ngSheets;// owned
private: // wrapper methods for use by grt
  ImplData *_data;

  static grt::ObjectRef create()
  {
    return grt::ObjectRef(new db_ng_Ide());
  }


public:
  static void grt_register()
  {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta) throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&db_ng_Ide::create);
    {
      void (db_ng_Ide::*setter)(const db_ng_SheetRef &)= &db_ng_Ide::activeNgSheet;
      db_ng_SheetRef (db_ng_Ide::*getter)() const= &db_ng_Ide::activeNgSheet;
      meta->bind_member("activeNgSheet", new grt::MetaClass::Property<db_ng_Ide,db_ng_SheetRef >(getter,setter));
    }
    {
      void (db_ng_Ide::*setter)(const grt::ListRef<db_ng_Sheet> &)= &db_ng_Ide::ngSheets;
      grt::ListRef<db_ng_Sheet> (db_ng_Ide::*getter)() const= &db_ng_Ide::ngSheets;
      meta->bind_member("ngSheets", new grt::MetaClass::Property<db_ng_Ide,grt::ListRef<db_ng_Sheet> >(getter,setter));
    }
  }
};




inline void register_structs_db_ng_xml()
{
  grt::internal::ClassRegistry::register_class<db_ng_Editor>();
  grt::internal::ClassRegistry::register_class<db_ng_Sheet>();
  grt::internal::ClassRegistry::register_class<db_ng_Ide>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_db_ng_xml { _autoreg__structs_db_ng_xml() { register_structs_db_ng_xml(); } } __autoreg__structs_db_ng_xml;
#endif

#ifndef _WIN32
  #pragma GCC diagnostic pop
#endif

