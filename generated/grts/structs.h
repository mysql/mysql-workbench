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
  #ifdef GRT_STRUCTS_EXPORT
  #define GRT_STRUCTS_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_PUBLIC
#endif


class GrtObject;
typedef grt::Ref<GrtObject> GrtObjectRef;
class GrtVersion;
typedef grt::Ref<GrtVersion> GrtVersionRef;
class GrtMessage;
typedef grt::Ref<GrtMessage> GrtMessageRef;
class GrtLogEntry;
typedef grt::Ref<GrtLogEntry> GrtLogEntryRef;
class GrtLogObject;
typedef grt::Ref<GrtLogObject> GrtLogObjectRef;
class GrtNamedObject;
typedef grt::Ref<GrtNamedObject> GrtNamedObjectRef;
class GrtStoredNote;
typedef grt::Ref<GrtStoredNote> GrtStoredNoteRef;
class TransientObject;
typedef grt::Ref<TransientObject> TransientObjectRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** the parent of all other objects */
class  GrtObject : public grt::internal::Object {
  typedef grt::internal::Object super;

public:
  GrtObject(grt::MetaClass *meta = nullptr)
    : grt::internal::Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _name("") {
  }

  static std::string static_class_name() {
    return "GrtObject";
  }

  /**
   * Getter for attribute name
   *
   * the object's name
   * \par In Python:
   *    value = obj.name
   */
  grt::StringRef name() const { return _name; }

  /**
   * Setter for attribute name
   * 
   * the object's name
   * \par In Python:
   *   obj.name = value
   */
  virtual void name(const grt::StringRef &value) {
    grt::ValueRef ovalue(_name);
    _name = value;
    member_changed("name", ovalue, value);
  }

  /**
   * Getter for attribute owner
   *
   * the object that owns this object
   * \par In Python:
   *    value = obj.owner
   */
  GrtObjectRef owner() const { return _owner; }

  /**
   * Setter for attribute owner
   * 
   * the object that owns this object
   * \par In Python:
   *   obj.owner = value
   */
  virtual void owner(const GrtObjectRef &value) {
    grt::ValueRef ovalue(_owner);
    _owner = value;
    member_changed("owner", ovalue, value);
  }

  /**
   * Getter for attribute guid (read-only)
   *
   * Internal object Global Unique Identifier
   * \par In Python:
   *    value = obj.guid
   */
  grt::StringRef guid() const { return _id; }

protected:

  grt::StringRef _name;
  GrtObjectRef _owner;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new GrtObject());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&GrtObject::create);
    {
      void (GrtObject::*setter)(const grt::StringRef &) = &GrtObject::name;
      grt::StringRef (GrtObject::*getter)() const = &GrtObject::name;
      meta->bind_member("name", new grt::MetaClass::Property<GrtObject,grt::StringRef>(getter, setter));
    }
    {
      void (GrtObject::*setter)(const GrtObjectRef &) = &GrtObject::owner;
      GrtObjectRef (GrtObject::*getter)() const = &GrtObject::owner;
      meta->bind_member("owner", new grt::MetaClass::Property<GrtObject,GrtObjectRef>(getter, setter));
    }
    meta->bind_member("guid", new grt::MetaClass::Property<GrtObject,grt::StringRef>(&GrtObject::guid));
  }
};

/** version information for an object */
class  GrtVersion : public GrtObject {
  typedef GrtObject super;

public:
  GrtVersion(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _buildNumber(0),
      _majorNumber(0),
      _minorNumber(0),
      _releaseNumber(0),
      _status(0) {
  }

  static std::string static_class_name() {
    return "GrtVersion";
  }

  /**
   * Getter for attribute buildNumber
   *
   * build number
   * \par In Python:
   *    value = obj.buildNumber
   */
  grt::IntegerRef buildNumber() const { return _buildNumber; }

  /**
   * Setter for attribute buildNumber
   * 
   * build number
   * \par In Python:
   *   obj.buildNumber = value
   */
  virtual void buildNumber(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_buildNumber);
    _buildNumber = value;
    member_changed("buildNumber", ovalue, value);
  }

  /**
   * Getter for attribute majorNumber
   *
   * major version
   * \par In Python:
   *    value = obj.majorNumber
   */
  grt::IntegerRef majorNumber() const { return _majorNumber; }

  /**
   * Setter for attribute majorNumber
   * 
   * major version
   * \par In Python:
   *   obj.majorNumber = value
   */
  virtual void majorNumber(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_majorNumber);
    _majorNumber = value;
    member_changed("majorNumber", ovalue, value);
  }

  /**
   * Getter for attribute minorNumber
   *
   * minor version
   * \par In Python:
   *    value = obj.minorNumber
   */
  grt::IntegerRef minorNumber() const { return _minorNumber; }

  /**
   * Setter for attribute minorNumber
   * 
   * minor version
   * \par In Python:
   *   obj.minorNumber = value
   */
  virtual void minorNumber(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_minorNumber);
    _minorNumber = value;
    member_changed("minorNumber", ovalue, value);
  }

  /**
   * Getter for attribute releaseNumber
   *
   * release number
   * \par In Python:
   *    value = obj.releaseNumber
   */
  grt::IntegerRef releaseNumber() const { return _releaseNumber; }

  /**
   * Setter for attribute releaseNumber
   * 
   * release number
   * \par In Python:
   *   obj.releaseNumber = value
   */
  virtual void releaseNumber(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_releaseNumber);
    _releaseNumber = value;
    member_changed("releaseNumber", ovalue, value);
  }

  /**
   * Getter for attribute status
   *
   * the status, 0 for GA, 1 for alpha, 2 for beta, 3 for RC
   * \par In Python:
   *    value = obj.status
   */
  grt::IntegerRef status() const { return _status; }

  /**
   * Setter for attribute status
   * 
   * the status, 0 for GA, 1 for alpha, 2 for beta, 3 for RC
   * \par In Python:
   *   obj.status = value
   */
  virtual void status(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_status);
    _status = value;
    member_changed("status", ovalue, value);
  }

protected:

  grt::IntegerRef _buildNumber;
  grt::IntegerRef _majorNumber;
  grt::IntegerRef _minorNumber;
  grt::IntegerRef _releaseNumber;
  grt::IntegerRef _status;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new GrtVersion());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&GrtVersion::create);
    {
      void (GrtVersion::*setter)(const grt::IntegerRef &) = &GrtVersion::buildNumber;
      grt::IntegerRef (GrtVersion::*getter)() const = &GrtVersion::buildNumber;
      meta->bind_member("buildNumber", new grt::MetaClass::Property<GrtVersion,grt::IntegerRef>(getter, setter));
    }
    {
      void (GrtVersion::*setter)(const grt::IntegerRef &) = &GrtVersion::majorNumber;
      grt::IntegerRef (GrtVersion::*getter)() const = &GrtVersion::majorNumber;
      meta->bind_member("majorNumber", new grt::MetaClass::Property<GrtVersion,grt::IntegerRef>(getter, setter));
    }
    {
      void (GrtVersion::*setter)(const grt::IntegerRef &) = &GrtVersion::minorNumber;
      grt::IntegerRef (GrtVersion::*getter)() const = &GrtVersion::minorNumber;
      meta->bind_member("minorNumber", new grt::MetaClass::Property<GrtVersion,grt::IntegerRef>(getter, setter));
    }
    {
      void (GrtVersion::*setter)(const grt::IntegerRef &) = &GrtVersion::releaseNumber;
      grt::IntegerRef (GrtVersion::*getter)() const = &GrtVersion::releaseNumber;
      meta->bind_member("releaseNumber", new grt::MetaClass::Property<GrtVersion,grt::IntegerRef>(getter, setter));
    }
    {
      void (GrtVersion::*setter)(const grt::IntegerRef &) = &GrtVersion::status;
      grt::IntegerRef (GrtVersion::*getter)() const = &GrtVersion::status;
      meta->bind_member("status", new grt::MetaClass::Property<GrtVersion,grt::IntegerRef>(getter, setter));
    }
  }
};

/** a dictionary containing a GRT message */
class  GrtMessage : public GrtObject {
  typedef GrtObject super;

public:
  GrtMessage(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _details(this, false),
      _msg(""),
      _msgType(0) {
  }

  static std::string static_class_name() {
    return "GrtMessage";
  }

  /**
   * Getter for attribute details (read-only)
   *
   * A list of detail information strings belonging to the message
   * \par In Python:
   *    value = obj.details
   */
  grt::StringListRef details() const { return _details; }


private: // The next attribute is read-only.
  virtual void details(const grt::StringListRef &value) {
    grt::ValueRef ovalue(_details);
    _details = value;
    member_changed("details", ovalue, value);
  }
public:

  /**
   * Getter for attribute msg
   *
   * The message string
   * \par In Python:
   *    value = obj.msg
   */
  grt::StringRef msg() const { return _msg; }

  /**
   * Setter for attribute msg
   * 
   * The message string
   * \par In Python:
   *   obj.msg = value
   */
  virtual void msg(const grt::StringRef &value) {
    grt::ValueRef ovalue(_msg);
    _msg = value;
    member_changed("msg", ovalue, value);
  }

  /**
   * Getter for attribute msgType
   *
   * The type of the message, 0 stands for a normal message, 1 for a warning and 2 for an error
   * \par In Python:
   *    value = obj.msgType
   */
  grt::IntegerRef msgType() const { return _msgType; }

  /**
   * Setter for attribute msgType
   * 
   * The type of the message, 0 stands for a normal message, 1 for a warning and 2 for an error
   * \par In Python:
   *   obj.msgType = value
   */
  virtual void msgType(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_msgType);
    _msgType = value;
    member_changed("msgType", ovalue, value);
  }

  /**
   * Getter for attribute name
   *
   * the message's title
   * \par In Python:
   *    value = obj.name
   */

  /**
   * Setter for attribute name
   * 
   * the message's title
   * \par In Python:
   *   obj.name = value
   */

protected:

  grt::StringListRef _details;
  grt::StringRef _msg;
  grt::IntegerRef _msgType;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new GrtMessage());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&GrtMessage::create);
    {
      void (GrtMessage::*setter)(const grt::StringListRef &) = &GrtMessage::details;
      grt::StringListRef (GrtMessage::*getter)() const = &GrtMessage::details;
      meta->bind_member("details", new grt::MetaClass::Property<GrtMessage,grt::StringListRef>(getter, setter));
    }
    {
      void (GrtMessage::*setter)(const grt::StringRef &) = &GrtMessage::msg;
      grt::StringRef (GrtMessage::*getter)() const = &GrtMessage::msg;
      meta->bind_member("msg", new grt::MetaClass::Property<GrtMessage,grt::StringRef>(getter, setter));
    }
    {
      void (GrtMessage::*setter)(const grt::IntegerRef &) = &GrtMessage::msgType;
      grt::IntegerRef (GrtMessage::*getter)() const = &GrtMessage::msgType;
      meta->bind_member("msgType", new grt::MetaClass::Property<GrtMessage,grt::IntegerRef>(getter, setter));
    }
    {
      void (GrtMessage::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (GrtMessage::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<GrtMessage,grt::StringRef>(getter, setter));
    }
  }
};

/** an individual object log entry */
class  GrtLogEntry : public GrtObject {
  typedef GrtObject super;

public:
  GrtLogEntry(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _customData(this, false),
      _entryType(0) {
  }

  static std::string static_class_name() {
    return "GrtLogEntry";
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
   * Getter for attribute entryType
   *
   * type of the log entry, 0 for a normal message, 1 for a warning and 2 for an error
   * \par In Python:
   *    value = obj.entryType
   */
  grt::IntegerRef entryType() const { return _entryType; }

  /**
   * Setter for attribute entryType
   * 
   * type of the log entry, 0 for a normal message, 1 for a warning and 2 for an error
   * \par In Python:
   *   obj.entryType = value
   */
  virtual void entryType(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_entryType);
    _entryType = value;
    member_changed("entryType", ovalue, value);
  }

  /**
   * Getter for attribute name
   *
   * the log message
   * \par In Python:
   *    value = obj.name
   */

  /**
   * Setter for attribute name
   * 
   * the log message
   * \par In Python:
   *   obj.name = value
   */

protected:

  grt::DictRef _customData;
  grt::IntegerRef _entryType;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new GrtLogEntry());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&GrtLogEntry::create);
    {
      void (GrtLogEntry::*setter)(const grt::DictRef &) = &GrtLogEntry::customData;
      grt::DictRef (GrtLogEntry::*getter)() const = &GrtLogEntry::customData;
      meta->bind_member("customData", new grt::MetaClass::Property<GrtLogEntry,grt::DictRef>(getter, setter));
    }
    {
      void (GrtLogEntry::*setter)(const grt::IntegerRef &) = &GrtLogEntry::entryType;
      grt::IntegerRef (GrtLogEntry::*getter)() const = &GrtLogEntry::entryType;
      meta->bind_member("entryType", new grt::MetaClass::Property<GrtLogEntry,grt::IntegerRef>(getter, setter));
    }
    {
      void (GrtLogEntry::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (GrtLogEntry::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<GrtLogEntry,grt::StringRef>(getter, setter));
    }
  }
};

/** an object log */
class  GrtLogObject : public GrtObject {
  typedef GrtObject super;

public:
  GrtLogObject(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _entries(this, false) {
  }

  static std::string static_class_name() {
    return "GrtLogObject";
  }

  // entries is owned by GrtLogObject
  /**
   * Getter for attribute entries (read-only)
   *
   * the generated log messages
   * \par In Python:
   *    value = obj.entries
   */
  grt::ListRef<GrtLogEntry> entries() const { return _entries; }


private: // The next attribute is read-only.
  virtual void entries(const grt::ListRef<GrtLogEntry> &value) {
    grt::ValueRef ovalue(_entries);

    _entries = value;
    owned_member_changed("entries", ovalue, value);
  }
public:

  /**
   * Getter for attribute logObject
   *
   * a link to the object
   * \par In Python:
   *    value = obj.logObject
   */
  GrtObjectRef logObject() const { return _logObject; }

  /**
   * Setter for attribute logObject
   * 
   * a link to the object
   * \par In Python:
   *   obj.logObject = value
   */
  virtual void logObject(const GrtObjectRef &value) {
    grt::ValueRef ovalue(_logObject);
    _logObject = value;
    member_changed("logObject", ovalue, value);
  }

  /**
   * Getter for attribute refObject
   *
   * an optional link to a referenced object
   * \par In Python:
   *    value = obj.refObject
   */
  GrtObjectRef refObject() const { return _refObject; }

  /**
   * Setter for attribute refObject
   * 
   * an optional link to a referenced object
   * \par In Python:
   *   obj.refObject = value
   */
  virtual void refObject(const GrtObjectRef &value) {
    grt::ValueRef ovalue(_refObject);
    _refObject = value;
    member_changed("refObject", ovalue, value);
  }

protected:

  grt::ListRef<GrtLogEntry> _entries;// owned
  GrtObjectRef _logObject;
  GrtObjectRef _refObject;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new GrtLogObject());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&GrtLogObject::create);
    {
      void (GrtLogObject::*setter)(const grt::ListRef<GrtLogEntry> &) = &GrtLogObject::entries;
      grt::ListRef<GrtLogEntry> (GrtLogObject::*getter)() const = &GrtLogObject::entries;
      meta->bind_member("entries", new grt::MetaClass::Property<GrtLogObject,grt::ListRef<GrtLogEntry>>(getter, setter));
    }
    {
      void (GrtLogObject::*setter)(const GrtObjectRef &) = &GrtLogObject::logObject;
      GrtObjectRef (GrtLogObject::*getter)() const = &GrtLogObject::logObject;
      meta->bind_member("logObject", new grt::MetaClass::Property<GrtLogObject,GrtObjectRef>(getter, setter));
    }
    {
      void (GrtLogObject::*setter)(const GrtObjectRef &) = &GrtLogObject::refObject;
      GrtObjectRef (GrtLogObject::*getter)() const = &GrtLogObject::refObject;
      meta->bind_member("refObject", new grt::MetaClass::Property<GrtLogObject,GrtObjectRef>(getter, setter));
    }
  }
};

/** an object that tracks name changes */
class  GrtNamedObject : public GrtObject {
  typedef GrtObject super;

public:
  GrtNamedObject(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _comment(""),
      _oldName("") {
  }

  static std::string static_class_name() {
    return "GrtNamedObject";
  }

  /**
   * Getter for attribute comment
   *
   * a text describing the object
   * \par In Python:
   *    value = obj.comment
   */
  grt::StringRef comment() const { return _comment; }

  /**
   * Setter for attribute comment
   * 
   * a text describing the object
   * \par In Python:
   *   obj.comment = value
   */
  virtual void comment(const grt::StringRef &value) {
    grt::ValueRef ovalue(_comment);
    _comment = value;
    member_changed("comment", ovalue, value);
  }

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
   * Getter for attribute oldName
   *
   * used to keep track of the old, original name of the object if the object gets renamed
   * \par In Python:
   *    value = obj.oldName
   */
  grt::StringRef oldName() const { return _oldName; }

  /**
   * Setter for attribute oldName
   * 
   * used to keep track of the old, original name of the object if the object gets renamed
   * \par In Python:
   *   obj.oldName = value
   */
  virtual void oldName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_oldName);
    _oldName = value;
    member_changed("oldName", ovalue, value);
  }

protected:

  grt::StringRef _comment;
  grt::StringRef _oldName;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new GrtNamedObject());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&GrtNamedObject::create);
    {
      void (GrtNamedObject::*setter)(const grt::StringRef &) = &GrtNamedObject::comment;
      grt::StringRef (GrtNamedObject::*getter)() const = &GrtNamedObject::comment;
      meta->bind_member("comment", new grt::MetaClass::Property<GrtNamedObject,grt::StringRef>(getter, setter));
    }
    {
      void (GrtNamedObject::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (GrtNamedObject::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<GrtNamedObject,grt::StringRef>(getter, setter));
    }
    {
      void (GrtNamedObject::*setter)(const grt::StringRef &) = &GrtNamedObject::oldName;
      grt::StringRef (GrtNamedObject::*getter)() const = &GrtNamedObject::oldName;
      meta->bind_member("oldName", new grt::MetaClass::Property<GrtNamedObject,grt::StringRef>(getter, setter));
    }
  }
};

/** a note */
class GRT_STRUCTS_PUBLIC GrtStoredNote : public GrtNamedObject {
  typedef GrtNamedObject super;

public:
  GrtStoredNote(grt::MetaClass *meta = nullptr)
    : GrtNamedObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _createDate(""),
      _filename(""),
      _lastChangeDate("") {
  }

  virtual ~GrtStoredNote();

  static std::string static_class_name() {
    return "GrtStoredNote";
  }

  /**
   * Getter for attribute createDate
   *
   * 
   * \par In Python:
   *    value = obj.createDate
   */
  grt::StringRef createDate() const { return _createDate; }

  /**
   * Setter for attribute createDate
   * 
   * 
   * \par In Python:
   *   obj.createDate = value
   */
  virtual void createDate(const grt::StringRef &value) {
    grt::ValueRef ovalue(_createDate);
    _createDate = value;
    member_changed("createDate", ovalue, value);
  }

  /**
   * Getter for attribute filename
   *
   * 
   * \par In Python:
   *    value = obj.filename
   */
  grt::StringRef filename() const { return _filename; }

  /**
   * Setter for attribute filename
   * 
   * 
   * \par In Python:
   *   obj.filename = value
   */
  virtual void filename(const grt::StringRef &value) {
    grt::ValueRef ovalue(_filename);
    _filename = value;
    member_changed("filename", ovalue, value);
  }

  /**
   * Getter for attribute lastChangeDate
   *
   * 
   * \par In Python:
   *    value = obj.lastChangeDate
   */
  grt::StringRef lastChangeDate() const { return _lastChangeDate; }

  /**
   * Setter for attribute lastChangeDate
   * 
   * 
   * \par In Python:
   *   obj.lastChangeDate = value
   */
  virtual void lastChangeDate(const grt::StringRef &value) {
    grt::ValueRef ovalue(_lastChangeDate);
    _lastChangeDate = value;
    member_changed("lastChangeDate", ovalue, value);
  }

  /**
   * Method. 
   * \return 
   */
  virtual grt::StringRef getText();
  /**
   * Method. 
   * \param text 
   * \return 
   */
  virtual void setText(const std::string &text);
  // default initialization function. auto-called by ObjectRef constructor
  virtual void init();

protected:

  grt::StringRef _createDate;
  grt::StringRef _filename;
  grt::StringRef _lastChangeDate;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new GrtStoredNote());
  }

  static grt::ValueRef call_getText(grt::internal::Object *self, const grt::BaseListRef &args){ return dynamic_cast<GrtStoredNote*>(self)->getText(); }

  static grt::ValueRef call_setText(grt::internal::Object *self, const grt::BaseListRef &args){ dynamic_cast<GrtStoredNote*>(self)->setText(grt::StringRef::cast_from(args[0])); return grt::ValueRef(); }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&GrtStoredNote::create);
    {
      void (GrtStoredNote::*setter)(const grt::StringRef &) = &GrtStoredNote::createDate;
      grt::StringRef (GrtStoredNote::*getter)() const = &GrtStoredNote::createDate;
      meta->bind_member("createDate", new grt::MetaClass::Property<GrtStoredNote,grt::StringRef>(getter, setter));
    }
    {
      void (GrtStoredNote::*setter)(const grt::StringRef &) = &GrtStoredNote::filename;
      grt::StringRef (GrtStoredNote::*getter)() const = &GrtStoredNote::filename;
      meta->bind_member("filename", new grt::MetaClass::Property<GrtStoredNote,grt::StringRef>(getter, setter));
    }
    {
      void (GrtStoredNote::*setter)(const grt::StringRef &) = &GrtStoredNote::lastChangeDate;
      grt::StringRef (GrtStoredNote::*getter)() const = &GrtStoredNote::lastChangeDate;
      meta->bind_member("lastChangeDate", new grt::MetaClass::Property<GrtStoredNote,grt::StringRef>(getter, setter));
    }
    meta->bind_method("getText", &GrtStoredNote::call_getText);
    meta->bind_method("setText", &GrtStoredNote::call_setText);
  }
};

/** the parent of all transient (non persistent) objects */
class  TransientObject : public grt::internal::Object {
  typedef grt::internal::Object super;

public:
  TransientObject(grt::MetaClass *meta = nullptr)
    : grt::internal::Object(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())) {
  }

  static std::string static_class_name() {
    return "TransientObject";
  }

protected:


private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new TransientObject());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&TransientObject::create);
  }
};



inline void register_structs_xml() {
  grt::internal::ClassRegistry::register_class<GrtObject>();
  grt::internal::ClassRegistry::register_class<GrtVersion>();
  grt::internal::ClassRegistry::register_class<GrtMessage>();
  grt::internal::ClassRegistry::register_class<GrtLogEntry>();
  grt::internal::ClassRegistry::register_class<GrtLogObject>();
  grt::internal::ClassRegistry::register_class<GrtNamedObject>();
  grt::internal::ClassRegistry::register_class<GrtStoredNote>();
  grt::internal::ClassRegistry::register_class<TransientObject>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_xml {
  _autoreg__structs_xml() {
    register_structs_xml();
  }
} __autoreg__structs_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

