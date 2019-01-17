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
  #ifdef GRT_STRUCTS_WORKBENCH_MODEL_REPORTING_EXPORT
  #define GRT_STRUCTS_WORKBENCH_MODEL_REPORTING_PUBLIC __declspec(dllexport)
#else
  #define GRT_STRUCTS_WORKBENCH_MODEL_REPORTING_PUBLIC __declspec(dllimport)
#endif
#else
  #define GRT_STRUCTS_WORKBENCH_MODEL_REPORTING_PUBLIC
#endif

#include "grts/structs.h"

class workbench_model_reporting_TemplateStyleInfo;
typedef grt::Ref<workbench_model_reporting_TemplateStyleInfo> workbench_model_reporting_TemplateStyleInfoRef;
class workbench_model_reporting_TemplateInfo;
typedef grt::Ref<workbench_model_reporting_TemplateInfo> workbench_model_reporting_TemplateInfoRef;


namespace mforms { 
  class Object;
}; 

namespace grt { 
  class AutoPyObject;
}; 

/** information about a model reporting template */
class  workbench_model_reporting_TemplateStyleInfo : public GrtObject {
  typedef GrtObject super;

public:
  workbench_model_reporting_TemplateStyleInfo(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _canUseHTMLMarkup(0),
      _description(""),
      _previewImageFileName(""),
      _styleTagValue("") {
  }

  static std::string static_class_name() {
    return "workbench.model.reporting.TemplateStyleInfo";
  }

  /**
   * Getter for attribute canUseHTMLMarkup
   *
   * A flag that indicates if Workbench can use HTML markup for SQL syntax highlighting.
   * \par In Python:
   *    value = obj.canUseHTMLMarkup
   */
  grt::IntegerRef canUseHTMLMarkup() const { return _canUseHTMLMarkup; }

  /**
   * Setter for attribute canUseHTMLMarkup
   * 
   * A flag that indicates if Workbench can use HTML markup for SQL syntax highlighting.
   * \par In Python:
   *   obj.canUseHTMLMarkup = value
   */
  virtual void canUseHTMLMarkup(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_canUseHTMLMarkup);
    _canUseHTMLMarkup = value;
    member_changed("canUseHTMLMarkup", ovalue, value);
  }

  /**
   * Getter for attribute description
   *
   * the note text
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * the note text
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  /**
   * Getter for attribute name
   *
   * the name of the style
   * \par In Python:
   *    value = obj.name
   */

  /**
   * Setter for attribute name
   * 
   * the name of the style
   * \par In Python:
   *   obj.name = value
   */

  /**
   * Getter for attribute previewImageFileName
   *
   * the file name of the preview image, has to match preview_*.png
   * \par In Python:
   *    value = obj.previewImageFileName
   */
  grt::StringRef previewImageFileName() const { return _previewImageFileName; }

  /**
   * Setter for attribute previewImageFileName
   * 
   * the file name of the preview image, has to match preview_*.png
   * \par In Python:
   *   obj.previewImageFileName = value
   */
  virtual void previewImageFileName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_previewImageFileName);
    _previewImageFileName = value;
    member_changed("previewImageFileName", ovalue, value);
  }

  /**
   * Getter for attribute styleTagValue
   *
   * the value the style tag should be set to, e.g. the name of the css file
   * \par In Python:
   *    value = obj.styleTagValue
   */
  grt::StringRef styleTagValue() const { return _styleTagValue; }

  /**
   * Setter for attribute styleTagValue
   * 
   * the value the style tag should be set to, e.g. the name of the css file
   * \par In Python:
   *   obj.styleTagValue = value
   */
  virtual void styleTagValue(const grt::StringRef &value) {
    grt::ValueRef ovalue(_styleTagValue);
    _styleTagValue = value;
    member_changed("styleTagValue", ovalue, value);
  }

protected:

  grt::IntegerRef _canUseHTMLMarkup;
  grt::StringRef _description;
  grt::StringRef _previewImageFileName;
  grt::StringRef _styleTagValue;

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_model_reporting_TemplateStyleInfo());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_model_reporting_TemplateStyleInfo::create);
    {
      void (workbench_model_reporting_TemplateStyleInfo::*setter)(const grt::IntegerRef &) = &workbench_model_reporting_TemplateStyleInfo::canUseHTMLMarkup;
      grt::IntegerRef (workbench_model_reporting_TemplateStyleInfo::*getter)() const = &workbench_model_reporting_TemplateStyleInfo::canUseHTMLMarkup;
      meta->bind_member("canUseHTMLMarkup", new grt::MetaClass::Property<workbench_model_reporting_TemplateStyleInfo,grt::IntegerRef>(getter, setter));
    }
    {
      void (workbench_model_reporting_TemplateStyleInfo::*setter)(const grt::StringRef &) = &workbench_model_reporting_TemplateStyleInfo::description;
      grt::StringRef (workbench_model_reporting_TemplateStyleInfo::*getter)() const = &workbench_model_reporting_TemplateStyleInfo::description;
      meta->bind_member("description", new grt::MetaClass::Property<workbench_model_reporting_TemplateStyleInfo,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_reporting_TemplateStyleInfo::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (workbench_model_reporting_TemplateStyleInfo::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<workbench_model_reporting_TemplateStyleInfo,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_reporting_TemplateStyleInfo::*setter)(const grt::StringRef &) = &workbench_model_reporting_TemplateStyleInfo::previewImageFileName;
      grt::StringRef (workbench_model_reporting_TemplateStyleInfo::*getter)() const = &workbench_model_reporting_TemplateStyleInfo::previewImageFileName;
      meta->bind_member("previewImageFileName", new grt::MetaClass::Property<workbench_model_reporting_TemplateStyleInfo,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_reporting_TemplateStyleInfo::*setter)(const grt::StringRef &) = &workbench_model_reporting_TemplateStyleInfo::styleTagValue;
      grt::StringRef (workbench_model_reporting_TemplateStyleInfo::*getter)() const = &workbench_model_reporting_TemplateStyleInfo::styleTagValue;
      meta->bind_member("styleTagValue", new grt::MetaClass::Property<workbench_model_reporting_TemplateStyleInfo,grt::StringRef>(getter, setter));
    }
  }
};

/** information about a model reporting template */
class  workbench_model_reporting_TemplateInfo : public GrtObject {
  typedef GrtObject super;

public:
  workbench_model_reporting_TemplateInfo(grt::MetaClass *meta = nullptr)
    : GrtObject(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _description(""),
      _mainFileName(""),
      _styles(this, false) {
  }

  static std::string static_class_name() {
    return "workbench.model.reporting.TemplateInfo";
  }

  /**
   * Getter for attribute description
   *
   * A short description of the type of template.
   * \par In Python:
   *    value = obj.description
   */
  grt::StringRef description() const { return _description; }

  /**
   * Setter for attribute description
   * 
   * A short description of the type of template.
   * \par In Python:
   *   obj.description = value
   */
  virtual void description(const grt::StringRef &value) {
    grt::ValueRef ovalue(_description);
    _description = value;
    member_changed("description", ovalue, value);
  }

  /**
   * Getter for attribute mainFileName
   *
   * the name of the file that will be opened after the report has been created
   * \par In Python:
   *    value = obj.mainFileName
   */
  grt::StringRef mainFileName() const { return _mainFileName; }

  /**
   * Setter for attribute mainFileName
   * 
   * the name of the file that will be opened after the report has been created
   * \par In Python:
   *   obj.mainFileName = value
   */
  virtual void mainFileName(const grt::StringRef &value) {
    grt::ValueRef ovalue(_mainFileName);
    _mainFileName = value;
    member_changed("mainFileName", ovalue, value);
  }

  /**
   * Getter for attribute name
   *
   * the name of the template
   * \par In Python:
   *    value = obj.name
   */

  /**
   * Setter for attribute name
   * 
   * the name of the template
   * \par In Python:
   *   obj.name = value
   */

  // styles is owned by workbench_model_reporting_TemplateInfo
  /**
   * Getter for attribute styles (read-only)
   *
   * 
   * \par In Python:
   *    value = obj.styles
   */
  grt::ListRef<workbench_model_reporting_TemplateStyleInfo> styles() const { return _styles; }


private: // The next attribute is read-only.
  virtual void styles(const grt::ListRef<workbench_model_reporting_TemplateStyleInfo> &value) {
    grt::ValueRef ovalue(_styles);

    _styles = value;
    owned_member_changed("styles", ovalue, value);
  }
public:

protected:

  grt::StringRef _description;
  grt::StringRef _mainFileName;
  grt::ListRef<workbench_model_reporting_TemplateStyleInfo> _styles;// owned

private: // Wrapper methods for use by the grt.
  static grt::ObjectRef create() {
    return grt::ObjectRef(new workbench_model_reporting_TemplateInfo());
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (meta == nullptr)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&workbench_model_reporting_TemplateInfo::create);
    {
      void (workbench_model_reporting_TemplateInfo::*setter)(const grt::StringRef &) = &workbench_model_reporting_TemplateInfo::description;
      grt::StringRef (workbench_model_reporting_TemplateInfo::*getter)() const = &workbench_model_reporting_TemplateInfo::description;
      meta->bind_member("description", new grt::MetaClass::Property<workbench_model_reporting_TemplateInfo,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_reporting_TemplateInfo::*setter)(const grt::StringRef &) = &workbench_model_reporting_TemplateInfo::mainFileName;
      grt::StringRef (workbench_model_reporting_TemplateInfo::*getter)() const = &workbench_model_reporting_TemplateInfo::mainFileName;
      meta->bind_member("mainFileName", new grt::MetaClass::Property<workbench_model_reporting_TemplateInfo,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_reporting_TemplateInfo::*setter)(const grt::StringRef &) = 0;
      grt::StringRef (workbench_model_reporting_TemplateInfo::*getter)() const = 0;
      meta->bind_member("name", new grt::MetaClass::Property<workbench_model_reporting_TemplateInfo,grt::StringRef>(getter, setter));
    }
    {
      void (workbench_model_reporting_TemplateInfo::*setter)(const grt::ListRef<workbench_model_reporting_TemplateStyleInfo> &) = &workbench_model_reporting_TemplateInfo::styles;
      grt::ListRef<workbench_model_reporting_TemplateStyleInfo> (workbench_model_reporting_TemplateInfo::*getter)() const = &workbench_model_reporting_TemplateInfo::styles;
      meta->bind_member("styles", new grt::MetaClass::Property<workbench_model_reporting_TemplateInfo,grt::ListRef<workbench_model_reporting_TemplateStyleInfo>>(getter, setter));
    }
  }
};



inline void register_structs_workbench_model_reporting_xml() {
  grt::internal::ClassRegistry::register_class<workbench_model_reporting_TemplateStyleInfo>();
  grt::internal::ClassRegistry::register_class<workbench_model_reporting_TemplateInfo>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_workbench_model_reporting_xml {
  _autoreg__structs_workbench_model_reporting_xml() {
    register_structs_workbench_model_reporting_xml();
  }
} __autoreg__structs_workbench_model_reporting_xml;
#endif

#ifndef _MSC_VER
  #pragma GCC diagnostic pop
#endif

