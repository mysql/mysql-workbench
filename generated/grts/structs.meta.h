#pragma once

#include "grt.h"

#ifdef _MSC_VER

#pragma warning(disable : 4355) // 'this' : used in base member initializer list
#ifdef GRT_STRUCTS_META_EXPORT
#define GRT_STRUCTS_META_PUBLIC __declspec(dllexport)
#else
#define GRT_STRUCTS_META_PUBLIC __declspec(dllimport)
#endif
#else
#define GRT_STRUCTS_META_PUBLIC
#endif

#include <grts/structs.h>
#include <grts/structs.db.h>

class meta_TaggedObject;
typedef grt::Ref<meta_TaggedObject> meta_TaggedObjectRef;
class meta_Tag;
typedef grt::Ref<meta_Tag> meta_TagRef;

namespace mforms {
  class Object;
};

namespace grt {
  class AutoPyObject;
};

class meta_TaggedObject : public GrtObject {
  typedef GrtObject super;

public:
  meta_TaggedObject(grt::MetaClass *meta = 0)
    : GrtObject(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _description("")

  {
  }

  static std::string static_class_name() {
    return "meta.TaggedObject";
  }

  /** Getter for attribute description


   \par In Python:
value = obj.description
   */
  grt::StringRef description() const {
    return _description;
  }
  /** Setter for attribute description


    \par In Python:
obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  /** Getter for attribute object


   \par In Python:
value = obj.object
   */
  db_DatabaseObjectRef object() const {
    return _object;
  }
  /** Setter for attribute object


    \par In Python:
obj.object = value
   */
  virtual void object(const db_DatabaseObjectRef &value) {
    grt::ValueRef ovalue(_object);
    _object = value;
    member_changed("object", ovalue, value);
  }

protected:
  grt::StringRef _description;
  db_DatabaseObjectRef _object;

private: // wrapper methods for use by grt
  static grt::ObjectRef create() {
    return grt::ObjectRef(new meta_TaggedObject());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&meta_TaggedObject::create);
    {
      void (meta_TaggedObject::*setter)(const grt::StringRef &) = &meta_TaggedObject::description;
      grt::StringRef (meta_TaggedObject::*getter)() const = &meta_TaggedObject::description;
      meta->bind_member("description", new grt::MetaClass::Property<meta_TaggedObject, grt::StringRef>(getter, setter));
    }
    {
      void (meta_TaggedObject::*setter)(const db_DatabaseObjectRef &) = &meta_TaggedObject::object;
      db_DatabaseObjectRef (meta_TaggedObject::*getter)() const = &meta_TaggedObject::object;
      meta->bind_member("object",
                        new grt::MetaClass::Property<meta_TaggedObject, db_DatabaseObjectRef>(getter, setter));
    }
  }
};

class meta_Tag : public GrtObject {
  typedef GrtObject super;

public:
  meta_Tag(grt::MetaClass *meta = 0)
    : GrtObject(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _color(""),
      _description(""),
      _label(""),
      _objects(this, false)

  {
  }

  static std::string static_class_name() {
    return "meta.Tag";
  }

  /** Getter for attribute category


   \par In Python:
value = obj.category
   */
  GrtObjectRef category() const {
    return _category;
  }
  /** Setter for attribute category


    \par In Python:
obj.category = value
   */
  virtual void category(const GrtObjectRef &value) {
    grt::ValueRef ovalue(_category);
    _category = value;
    member_changed("category", ovalue, value);
  }

  /** Getter for attribute color


   \par In Python:
value = obj.color
   */
  grt::StringRef color() const {
    return _color;
  }
  /** Setter for attribute color


    \par In Python:
obj.color = value
   */
  virtual void color(const grt::StringRef &value) {
    grt::ValueRef ovalue(_color);
    _color = value;
    member_changed("color", ovalue, value);
  }

  /** Getter for attribute description


   \par In Python:
value = obj.description
   */
  grt::StringRef description() const {
    return _description;
  }
  /** Setter for attribute description


    \par In Python:
obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  /** Getter for attribute label


   \par In Python:
value = obj.label
   */
  grt::StringRef label() const {
    return _label;
  }
  /** Setter for attribute label


    \par In Python:
obj.label = value
   */
  virtual void label(const grt::StringRef &value) {
    grt::ValueRef ovalue(_label);
    _label = value;
    member_changed("label", ovalue, value);
  }

  /** Getter for attribute objects (read-only)


   \par In Python:
value = obj.objects
   */
  grt::ListRef<meta_TaggedObject> objects() const {
    return _objects;
  }

private: // the next attribute is read-only
  virtual void objects(const grt::ListRef<meta_TaggedObject> &value) {
    grt::ValueRef ovalue(_objects);
    _objects = value;
    member_changed("objects", ovalue, value);
  }

public:
protected:
  GrtObjectRef _category;
  grt::StringRef _color;
  grt::StringRef _description;
  grt::StringRef _label;
  grt::ListRef<meta_TaggedObject> _objects;

private: // wrapper methods for use by grt
  static grt::ObjectRef create() {
    return grt::ObjectRef(new meta_Tag());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&meta_Tag::create);
    {
      void (meta_Tag::*setter)(const GrtObjectRef &) = &meta_Tag::category;
      GrtObjectRef (meta_Tag::*getter)() const = &meta_Tag::category;
      meta->bind_member("category", new grt::MetaClass::Property<meta_Tag, GrtObjectRef>(getter, setter));
    }
    {
      void (meta_Tag::*setter)(const grt::StringRef &) = &meta_Tag::color;
      grt::StringRef (meta_Tag::*getter)() const = &meta_Tag::color;
      meta->bind_member("color", new grt::MetaClass::Property<meta_Tag, grt::StringRef>(getter, setter));
    }
    {
      void (meta_Tag::*setter)(const grt::StringRef &) = &meta_Tag::description;
      grt::StringRef (meta_Tag::*getter)() const = &meta_Tag::description;
      meta->bind_member("description", new grt::MetaClass::Property<meta_Tag, grt::StringRef>(getter, setter));
    }
    {
      void (meta_Tag::*setter)(const grt::StringRef &) = &meta_Tag::label;
      grt::StringRef (meta_Tag::*getter)() const = &meta_Tag::label;
      meta->bind_member("label", new grt::MetaClass::Property<meta_Tag, grt::StringRef>(getter, setter));
    }
    {
      void (meta_Tag::*setter)(const grt::ListRef<meta_TaggedObject> &) = &meta_Tag::objects;
      grt::ListRef<meta_TaggedObject> (meta_Tag::*getter)() const = &meta_Tag::objects;
      meta->bind_member("objects",
                        new grt::MetaClass::Property<meta_Tag, grt::ListRef<meta_TaggedObject> >(getter, setter));
    }
  }
};

inline void register_structs_meta_xml() {
  grt::internal::ClassRegistry::register_class<meta_TaggedObject>();
  grt::internal::ClassRegistry::register_class<meta_Tag>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_meta_xml {
  _autoreg__structs_meta_xml() {
    register_structs_meta_xml();
  }
} __autoreg__structs_meta_xml;
#endif
