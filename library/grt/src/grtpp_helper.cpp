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

#include "grtpp_helper.h"
#include "grtpp_util.h"

#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/log.h"

#include <stdio.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <chrono>

DEFAULT_LOG_DOMAIN(DOMAIN_GRT)

using namespace grt;

// XXX: convert to using C++11 and streams.

static std::string copyright =
  "/*\n"
  " * Copyright (c) 2011, " + std::string("%year%") +
  ", Oracle and/or its affiliates. All rights reserved.\n"
  " *\n"
  " * This program is free software; you can redistribute it and/or modify\n"
  " * it under the terms of the GNU General Public License, version 2.0,\n"
  " * as published by the Free Software Foundation.\n"
  " *\n"
  " * This program is also distributed with certain software (including\n"
  " * but not limited to OpenSSL) that is licensed under separate terms, as\n"
  " * designated in a particular file or component or in included license\n"
  " * documentation.  The authors of MySQL hereby grant you an additional\n"
  " * permission to link the program and your derivative works with the\n"
  " * separately licensed software that they have included with MySQL.\n"
  " * This program is distributed in the hope that it will be useful, but\n"
  " * WITHOUT ANY WARRANTY; without even the implied warranty of\n"
  " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See\n"
  " * the GNU General Public License, version 2.0, for more details.\n"
  " *\n"
  " * You should have received a copy of the GNU General Public License\n"
  " * along with this program; if not, write to the Free Software Foundation, Inc.,\n"
  " * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA\n"
  " */\n\n";

//--------------------------------------------------------------------------------------------------

static std::string cppize_class_name(std::string name) {
  std::string::size_type p;
  while ((p = name.find('.')) != std::string::npos)
    name[p] = '_';
  return name;
}

//--------------------------------------------------------------------------------------------------

static std::string format_type_cpp(const TypeSpec &type, bool unknown_as_void = false) {
  std::string s;

  switch (type.base.type) {
    case IntegerType:
      return "grt::IntegerRef";
    case DoubleType:
      return "grt::DoubleRef";
    case StringType:
      return "grt::StringRef";
    case ListType:
      switch (type.content.type) {
        case IntegerType:
          return "grt::IntegerListRef";
        case DoubleType:
          return "grt::DoubleListRef";
        case StringType:
          return "grt::StringListRef";
        case ListType:
          return "???? invalid ???";
        case DictType:
          return "grt::DictListRef";
        case ObjectType:
          return "grt::ListRef<" + cppize_class_name(type.content.object_class) + ">";
        default:
          return "??? invalid ???";
      }
    case DictType:
      return "grt::DictRef";
    case ObjectType:
      return cppize_class_name(type.base.object_class) + "Ref";
    case UnknownType:
      if (unknown_as_void)
        return "void";
      /* fall-thru */
    default:
      return "??? invalid ???";
  }
  return s;
}

//--------------------------------------------------------------------------------------------------

static std::string format_arg_list(const std::vector<ArgSpec> &args) {
  std::string s;

  for (std::vector<ArgSpec>::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
    if (!s.empty())
      s.append(", ");

    if (is_simple_type(iter->type.base.type)) {
      switch (iter->type.base.type) {
        case IntegerType:
          s.append("ssize_t ");
          break;
        case DoubleType:
          s.append("double ");
          break;
        case StringType:
          s.append("const std::string &");
          break;
        default:
          break;
      }
    } else
      s.append("const ").append(format_type_cpp(iter->type)).append(" &");
    s.append(iter->name);
  }
  return s;
}

//--------------------------------------------------------------------------------------------------

static std::string format_signal_args(const std::vector<MetaClass::SignalArg> &args) {
  std::string s;

  for (std::vector<MetaClass::SignalArg>::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
    if (!s.empty())
      s.append(", ");

    switch (iter->type) {
      case MetaClass::BoolSArg:
        s.append("bool");
        break;
      case MetaClass::IntSArg:
        s.append("ssize_t");
        break;
      case MetaClass::DoubleSArg:
        s.append("double");
        break;
      case MetaClass::StringSArg:
        s.append("std::string");
        break;
      case MetaClass::ObjectSArg:
        s.append(cppize_class_name(iter->object_class) + "Ref");
        break;
    }
  }
  return s;
}

//--------------------------------------------------------------------------------------------------

static std::string format_signal_names(const std::vector<MetaClass::SignalArg> &args) {
  std::string s;

  for (std::vector<MetaClass::SignalArg>::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
    if (!s.empty())
      s.append(", ");

    s.append(iter->name);
  }
  return s;
}

static std::string format_wraparg_list(const std::vector<ArgSpec> &args) {
  std::string s;
  int i = 0;
  for (std::vector<ArgSpec>::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
    char idx[32];
    sprintf(idx, "%i", i++);
    if (!s.empty())
      s.append(", ");
    s.append(format_type_cpp(iter->type)).append("::cast_from(args[").append(idx).append("])");
  }
  return s;
}

//--------------------------------------------------------------------------------------------------

// Object Class Generator
struct ClassImplGenerator {
  MetaClass *gstruct;
  FILE *header_file;
  std::string cname;
  std::string pname;
  const std::map<std::string, MetaClass::Member> &members;
  const std::map<std::string, MetaClass::Method> &methods;
  bool needs_body;

  ClassImplGenerator(MetaClass *gstru, FILE *hdr)
    : gstruct(gstru), members(gstruct->get_members_partial()), methods(gstruct->get_methods_partial()) {
    header_file = hdr;
    cname = cppize_class_name(gstruct->name());
    pname =
      cppize_class_name(gstruct->parent()->name() != internal::Object::static_class_name() ? gstruct->parent()->name()
                                                                                           : "grt::internal::Object");

    needs_body = false;
  }

  void output_overriden_list_reset_code(FILE *f) {
    for (std::map<std::string, MetaClass::Member>::const_iterator mem = members.begin(); mem != members.end(); ++mem) {
      if ((mem->second.type.base.type == ListType || mem->second.type.base.type == DictType) &&
          !mem->second.calculated && mem->second.overrides) {
        // we need to reset the list content type descriptor to the overridden types
        // we can't just recreate the list because constructors higher up in the
        // hierarchy could get references to the already initialized list
        fprintf(f, "    _%s.content().__retype(grt::ObjectType, \"%s\");\n", mem->first.c_str(),
                mem->second.type.content.object_class.c_str());
      }
    }
  }

  void output_constructor_init_list(FILE *f) {
    fprintf(f, "    : %s(meta != nullptr ? meta : grt::GRT::get()->get_metaclass(static_class_name()))", pname.c_str());
    for (std::map<std::string, MetaClass::Member>::const_iterator mem = members.begin(); mem != members.end(); ++mem) {
      if (mem->second.calculated || mem->second.overrides)
        continue;

      std::string defval(mem->second.default_value);

      switch (mem->second.type.base.type) {
        case ListType:
        case DictType:
          fprintf(f, ",\n      _%s(this, %s)", mem->first.c_str(), mem->second.null_content_allowed ? "true" : "false");
          break;
        case StringType:
          fprintf(f, ",\n      _%s(\"%s\")", mem->first.c_str(), defval.c_str());
          break;
        case IntegerType:
          fprintf(f, ",\n      _%s(%s)", mem->first.c_str(), defval.empty() ? "0" : defval.c_str());
          break;
        case DoubleType:
          fprintf(f, ",\n      _%s(%s)", mem->first.c_str(), defval.empty() ? "0.0" : defval.c_str());
          break;
        default:
          break;
      }
    }

    if (!gstruct->get_attribute("simple-impl-data").empty())
      fprintf(f, ",\n      _data(nullptr), _release_data(nullptr)");
    else if ((needs_body && gstruct->impl_data()))
      fprintf(f, ",\n      _data(nullptr)");
  }

  void generate_class_doc(FILE *f) {
    std::string doc = gstruct->get_attribute("desc", false);
    if (!doc.empty())
      fprintf(f, "/** %s */\n", doc.c_str());
  }

  void generate_getter_doc(FILE *f, const MetaClass::Member &member) {
    std::string doc = gstruct->get_member_attribute(member.name, "desc", false);

    fprintf(f, "  /**\n");
    fprintf(f, "   * Getter for attribute %s%s\n", member.name.c_str(), member.read_only ? " (read-only)" : "");
    fprintf(f, "   *\n");
    fprintf(f, "   * %s\n", doc.c_str());
    fprintf(f, "   * \\par In Python:\n   *    value = obj.%s\n", member.name.c_str());
    fprintf(f, "   */\n");
  }

  void generate_setter_doc(FILE *f, const MetaClass::Member &member) {
    std::string doc = gstruct->get_member_attribute(member.name, "desc", false);

    fprintf(f, "  /**\n");
    fprintf(f, "   * Setter for attribute %s\n", member.name.c_str());
    fprintf(f, "   * \n");
    fprintf(f, "   * %s\n", doc.c_str());
    fprintf(f, "   * \\par In Python:\n   *   obj.%s = value\n", member.name.c_str());
    fprintf(f, "   */\n");
  }

  void generate_method_doc(FILE *f, const MetaClass::Method &method) {
    std::string doc = gstruct->get_member_attribute(method.name, "desc", false);

    fprintf(f, "  /**\n");
    fprintf(f, "   * Method. %s\n", doc.c_str());
    for (ArgSpecList::const_iterator arg = method.arg_types.begin(); arg != method.arg_types.end(); ++arg) {
      fprintf(f, "   * \\param %s %s\n", arg->name.c_str(),
              gstruct->get_member_attribute(method.name + ":" + arg->name, "desc").c_str());
    }
    doc = gstruct->get_member_attribute(method.name + ":return", "desc", false);
    fprintf(f, "   * \\return %s\n", doc.c_str());
    fprintf(f, "   */\n");
  }

  void generate_class_header(const std::string &dll_export) {
    FILE *f = header_file;

    // check if we will need a implementation file (.cpp)
    if (gstruct->force_impl() || !methods.empty())
      needs_body = true;

    for (std::map<std::string, MetaClass::Member>::const_iterator iter = members.begin(); iter != members.end();
         ++iter) {
      if (iter->second.delegate_get || iter->second.delegate_set || iter->second.calculated) {
        needs_body = true;
        break;
      }
    }

    generate_class_doc(f);
    fprintf(f, "class %s %s : public %s {\n", needs_body ? dll_export.c_str() : "", cname.c_str(), pname.c_str());
    fprintf(f, "  typedef %s super;\n\n", pname.c_str());
    fprintf(f, "public:\n");
    std::string klass = gstruct->get_attribute("simple-impl-data");
    if (gstruct->impl_data() || !klass.empty()) {
      if (klass.empty()) {
        fprintf(f, "  class ImplData;\n");
        fprintf(f, "  friend class ImplData;\n");
      } else
        fprintf(f, "  typedef %s ImplData;\n", klass.c_str());
    }
    // generate constructors
    bool default_ctor_created = false;

    for (std::map<std::string, MetaClass::Method>::const_iterator iter = methods.begin(); iter != methods.end();
         ++iter) {
      if (iter->second.constructor) {
        fprintf(f, "  %s(%s%s, grt::MetaClass *meta = nullptr);\n", cname.c_str(), iter->second.arg_types.empty() ? "" : ", ",
                format_arg_list(iter->second.arg_types).c_str());

        if (iter->second.arg_types.empty())
          default_ctor_created = true;
      }
    }

    if (!default_ctor_created) {
      fprintf(f, "  %s(grt::MetaClass *meta = nullptr)\n", cname.c_str());

      output_constructor_init_list(f);

      fprintf(f, " {\n");
      // reinit overridden lists
      output_overriden_list_reset_code(f);
      fprintf(f, "  }\n");
      fprintf(f, "\n");
    }

    if (needs_body || !gstruct->get_attribute("simple-impl-data").empty()) {
      if (gstruct->get_attribute("simple-impl-data").empty())
        fprintf(f, "  virtual ~%s();\n\n", cname.c_str());
      else {
        fprintf(f, "  virtual ~%s() {\n    if (_release_data && _data)\n      _release_data(_data);\n  }\n\n", cname.c_str());
      }
    }

    fprintf(f, "  static std::string static_class_name() {\n    return \"%s\";\n  }\n\n", gstruct->name().c_str());

    // generate signal access methods
    for (MetaClass::SignalList::const_iterator iter = gstruct->get_signals_partial().begin();
         iter != gstruct->get_signals_partial().end(); ++iter) {
      fprintf(f, "  // args: %s\n", format_signal_names(iter->arg_types).c_str());
      fprintf(f, "  boost::signals2::signal<void (%s)>* signal_%s() { return &_signal_%s; }\n",
              format_signal_args(iter->arg_types).c_str(), iter->name.c_str(), iter->name.c_str());
    }

    // generate member variable accessors
    for (MetaClass::MemberList::const_iterator iter = members.begin(); iter != members.end(); ++iter) {
      if (iter->second.private_)
        continue;

      bool overrides_with_same_type = false;
      // if the member is overridden, check if the type has been changed
      if (iter->second.overrides && gstruct->parent()) {
        const MetaClass::Member *member = gstruct->parent()->get_member_info(iter->second.name);

        if (member && member->type == iter->second.type)
          overrides_with_same_type = true;
      }

      // getter
      if (iter->second.owned_object)
        fprintf(f, "  // %s is owned by %s\n", iter->second.name.c_str(), cname.c_str());

      generate_getter_doc(f, iter->second);
      if (iter->second.delegate_get) {
        fprintf(f, "  %s %s() const;\n", format_type_cpp(iter->second.type).c_str(), iter->second.name.c_str());
      } else {
        if (iter->second.overrides) {
          if (!overrides_with_same_type) {
            fprintf(f, "  %s %s() const { return %s::cast_from(_%s); }\n", format_type_cpp(iter->second.type).c_str(),
                    iter->second.name.c_str(), format_type_cpp(iter->second.type).c_str(), iter->second.name.c_str());
          } else {
            // if we're overriding and the setter is delegated, we need to declare the getter too
            if (iter->second.delegate_set)
              fprintf(f, "  %s %s() const { return super::%s(); }\n", format_type_cpp(iter->second.type).c_str(),
                      iter->second.name.c_str(), iter->second.name.c_str());
          }
        } else
          fprintf(f, "  %s %s() const { return _%s; }\n", format_type_cpp(iter->second.type).c_str(),
                  iter->second.name.c_str(), iter->second.name.c_str());
      }
      fprintf(f, "\n");

      // setter
      if (iter->second.read_only)
        fprintf(f, "\nprivate: // The next attribute is read-only.\n");
      else
        generate_setter_doc(f, iter->second);

      if (iter->second.overrides) {
        if (overrides_with_same_type) {
          // check if the override is for overriding the setter
          if (iter->second.delegate_set) {
            fprintf(f, "  virtual void %s(const %s &value);\n", iter->second.name.c_str(),
                    format_type_cpp(iter->second.type).c_str());
          }
        } else {
          // variable is overriding another

          // list and dict members cannot be changed
          if (iter->second.type.base.type != ListType && iter->second.type.base.type != DictType) {
            if (iter->second.delegate_set)
              fprintf(f, "  virtual void %s(const %s &value);\n", iter->second.name.c_str(),
                      format_type_cpp(iter->second.type).c_str());
            else
              fprintf(f, "  virtual void %s(const %s &value) { super::%s(value); }\n", iter->second.name.c_str(),
                      format_type_cpp(iter->second.type).c_str(), iter->second.name.c_str());
          }
        }
      } else {
        if (iter->second.delegate_set) {
          fprintf(f, "  virtual void %s(const %s &value);\n", iter->second.name.c_str(),
                  format_type_cpp(iter->second.type).c_str());
        } else { // read-only vars need setter for unserialization  if (!iter->second.read_only)
          if (!iter->second.calculated) {
            fprintf(f, "  virtual void %s%s(const %s &value) {\n",
                    /*iter->second.read_only?"__":*/ "", iter->second.name.c_str(),
                    format_type_cpp(iter->second.type).c_str());

            fprintf(f, "    grt::ValueRef ovalue(_%s);\n", iter->second.name.c_str());
            if (iter->second.owned_object) {
              // if member is owned by this object, we have to mark/unmark it as global
              // in case we're in the global tree as well (done in owned_member_changed)
              fprintf(f, "\n");
              fprintf(f, "    _%s = value;\n", iter->second.name.c_str());
              fprintf(f, "    owned_member_changed(\"%s\", ovalue, value);\n", iter->second.name.c_str());
            } else {
              fprintf(f, "    _%s = value;\n", iter->second.name.c_str());
              fprintf(f, "    member_changed(\"%s\", ovalue, value);\n", iter->second.name.c_str());
            }
            fprintf(f, "  }\n");
          }
        }
      }

      if (iter->second.read_only)
        fprintf(f, "public:\n");
      fprintf(f, "\n");
    }

    // generate methods
    for (std::map<std::string, MetaClass::Method>::const_iterator iter = methods.begin(); iter != methods.end();
         ++iter) {
      generate_method_doc(f, iter->second);
      if (iter->second.abstract)
        fprintf(f, "  virtual %s %s(%s) = 0;\n", format_type_cpp(iter->second.ret_type, true).c_str(),
                iter->second.name.c_str(), format_arg_list(iter->second.arg_types).c_str());
      else
        fprintf(f, "  virtual %s %s(%s);\n", format_type_cpp(iter->second.ret_type, true).c_str(),
                iter->second.name.c_str(), format_arg_list(iter->second.arg_types).c_str());
    }

    if (needs_body || !gstruct->get_attribute("simple-impl-data").empty()) {
      bool needs_init = true;
      if (gstruct->impl_data()) {
        if (gstruct->get_attribute("simple-impl-data").empty()) {
          fprintf(f, "\n  ImplData *get_data() const { return _data; }\n\n");
          fprintf(f, "  void set_data(ImplData *data);\n");
        } else {
          fprintf(f, "\n  ImplData *get_data() const { return _data; }\n\n");
          fprintf(f, "  void set_data(ImplData *data, void (*release)(ImplData*)) {\n");
          fprintf(f, "    if (_data == data) return;\n");
          fprintf(f, "    if (_data && _release_data) _release_data(_data);\n");
          fprintf(f, "    _data= data;\n");
          fprintf(f, "    _release_data = release;\n");
          fprintf(f, "  }\n");
          needs_init = false;
        }
      }
      if (needs_init) {
        fprintf(f, "  // default initialization function. auto-called by ObjectRef constructor\n");
        fprintf(f, "  virtual void init();\n\n");
      }
    }

    fprintf(f, "protected:\n");

    // special methods
    if (gstruct->watch_lists()) {
      fprintf(f, "  virtual void owned_list_item_added(grt::internal::OwnedList *list, const grt::ValueRef &value);\n");
      fprintf(f,
              "  virtual void owned_list_item_removed(grt::internal::OwnedList *list, const grt::ValueRef &value);\n");
    }

    if (gstruct->watch_dicts()) {
      fprintf(f, "  virtual void owned_dict_item_set(grt::internal::OwnedDict *dict, const std::string &key);\n");
      fprintf(f, "  virtual void owned_dict_item_removed(grt::internal::OwnedDict *dict, const std::string &key);\n");
    }

    // signals
    for (auto iter = gstruct->get_signals_partial().begin(); iter != gstruct->get_signals_partial().end(); ++iter) {
      fprintf(f, "  boost::signals2::signal<void (%s)> _signal_%s;\n", format_signal_args(iter->arg_types).c_str(),
              iter->name.c_str());
    }
    fprintf(f, "\n");

    // generate member variables
    for (auto iter = members.begin(); iter != members.end(); ++iter) {
      if (!iter->second.calculated && !iter->second.overrides)
        fprintf(f, "  %s _%s;%s\n", format_type_cpp(iter->second.type).c_str(), iter->second.name.c_str(),
                iter->second.owned_object ? "// owned" : "");
    }
    fprintf(f, "\n");

    fprintf(f, "private: // Wrapper methods for use by the grt.\n");
    if ((needs_body && gstruct->impl_data()) || !gstruct->get_attribute("simple-impl-data").empty()) {
      fprintf(f, "  ImplData *_data;\n");
      if (!gstruct->get_attribute("simple-impl-data").empty())
        fprintf(f, "  void (*_release_data)(ImplData *);\n");
      fprintf(f, "\n");
    }
    // function to create the object
    if (!gstruct->is_abstract()) {
      fprintf(f, "  static grt::ObjectRef create() {\n");
      fprintf(f, "    return grt::ObjectRef(new %s());\n", cname.c_str());
      fprintf(f, "  }\n");
      fprintf(f, "\n");
    }

    // generate method wrappers for grt
    for (auto iter = methods.begin(); iter != methods.end(); ++iter) {
      if (!iter->second.constructor) {
        if (iter->second.ret_type.base.type == UnknownType)
          fprintf(f,
                  "  static grt::ValueRef call_%s(grt::internal::Object *self, const grt::BaseListRef &args)"
                  "{ dynamic_cast<%s*>(self)->%s(%s); return grt::ValueRef(); }\n",
                  iter->second.name.c_str(), cname.c_str(), iter->second.name.c_str(),
                  format_wraparg_list(iter->second.arg_types).c_str());
        else
          fprintf(f,
                  "  static grt::ValueRef call_%s(grt::internal::Object *self, const grt::BaseListRef &args)"
                  "{ return dynamic_cast<%s*>(self)->%s(%s); }\n",
                  iter->second.name.c_str(), cname.c_str(), iter->second.name.c_str(),
                  format_wraparg_list(iter->second.arg_types).c_str());
        fprintf(f, "\n");
      }
    }

    // Class registration.
    fprintf(f, "public:\n");
    fprintf(f, "  static void grt_register() {\n");
    fprintf(f, "    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());\n");
    fprintf(f,
            "    if (meta == nullptr)\n      throw std::runtime_error(\"error initializing grt object class, metaclass not found\");\n");

    if (gstruct->is_abstract())
      fprintf(f, "    meta->bind_allocator(nullptr);\n");
    else
      fprintf(f, "    meta->bind_allocator(&%s::create);\n", cname.c_str());

    for (std::map<std::string, MetaClass::Member>::const_iterator iter = members.begin(); iter != members.end();
         ++iter) {
      if (iter->second.calculated) {
        if (!iter->second.delegate_set)
          fprintf(f, "    meta->bind_member(\"%s\", new grt::MetaClass::Property<%s,%s>(&%s::%s));\n",
                  iter->second.name.c_str(), cname.c_str(), format_type_cpp(iter->second.type).c_str(), cname.c_str(),
                  iter->second.name.c_str());
        else {
          fprintf(f, "    {\n");
          fprintf(f, "      void (%s::*setter)(const %s &) = &%s::%s%s;\n", cname.c_str(),
                  format_type_cpp(iter->second.type).c_str(), cname.c_str(), /*iter->second.read_only?"__":*/ "",
                  iter->second.name.c_str());
          fprintf(f, "      %s (%s::*getter)() const = &%s::%s;\n", format_type_cpp(iter->second.type).c_str(),
                  cname.c_str(), cname.c_str(), iter->second.name.c_str());

          fprintf(f, "      meta->bind_member(\"%s\", new grt::MetaClass::Property<%s,%s>(getter, setter));\n",
                  iter->second.name.c_str(), cname.c_str(), format_type_cpp(iter->second.type).c_str());
          fprintf(f, "    }\n");
        }
      } else {
        fprintf(f, "    {\n");
        if (iter->second.overrides) {
          if (iter->second.delegate_set) {
            fprintf(f, "      void (%s::*setter)(const %s &) = &%s::%s%s;\n", cname.c_str(),
                    format_type_cpp(iter->second.type).c_str(), cname.c_str(), /*iter->second.read_only?"__":*/ "",
                    iter->second.name.c_str());
          } else {
            fprintf(f, "      void (%s::*setter)(const %s &) = 0;\n", cname.c_str(),
                    format_type_cpp(iter->second.type).c_str());
          }
        } else { // read-only members should still have a setter for unserialization
          fprintf(f, "      void (%s::*setter)(const %s &) = &%s::%s%s;\n", cname.c_str(),
                  format_type_cpp(iter->second.type).c_str(), cname.c_str(), /*iter->second.read_only?"__":*/ "",
                  iter->second.name.c_str());
        }

        if (iter->second.overrides) {
          if (iter->second.delegate_get)
            fprintf(f, "      %s (%s::*getter)() const = &%s::%s;\n", format_type_cpp(iter->second.type).c_str(),
                    cname.c_str(), cname.c_str(), iter->second.name.c_str());
          else
            fprintf(f, "      %s (%s::*getter)() const = 0;\n", format_type_cpp(iter->second.type).c_str(),
                    cname.c_str());
        } else {
          fprintf(f, "      %s (%s::*getter)() const = &%s::%s;\n", format_type_cpp(iter->second.type).c_str(),
                  cname.c_str(), cname.c_str(), iter->second.name.c_str());
        }

        fprintf(f, "      meta->bind_member(\"%s\", new grt::MetaClass::Property<%s,%s>(getter, setter));\n",
                iter->second.name.c_str(), cname.c_str(), format_type_cpp(iter->second.type).c_str());
        fprintf(f, "    }\n");
      }
    }

    for (std::map<std::string, MetaClass::Method>::const_iterator iter = methods.begin(); iter != methods.end();
         ++iter) {
      fprintf(f, "    meta->bind_method(\"%s\", &%s::call_%s);\n", iter->second.name.c_str(), cname.c_str(),
              iter->second.name.c_str());
    }
    fprintf(f, "  }\n");

    fprintf(f, "};\n\n");
  }

  //------------------------------------------------------------------------------------------------

  void generate_class_body(FILE *f) {
    const char *separator =
      "//------------------------------------------------------------------------------------------------\n\n";
    fprintf(f, "%s", separator);

    if (gstruct->impl_data()) {
      fprintf(f, "class %s::ImplData {\n", cname.c_str());
      fprintf(f, "};\n\n");

      fprintf(f, "%s", separator);
      fprintf(f, "void %s::init() {\n  if (!_data) _data= new %s::ImplData();\n}\n\n", cname.c_str(), cname.c_str());
      fprintf(f, "%s", separator);
      fprintf(f, "%s::~%s() {\n  delete _data;\n}\n\n", cname.c_str(), cname.c_str());
      fprintf(f, "%s", separator);
      fprintf(f, "void %s::set_data(ImplData *data) {\n}\n\n", cname.c_str());
      fprintf(f, "%s", separator);
    } else {
      fprintf(f, "void %s::init() {\n\n}\n\n", cname.c_str());
      fprintf(f, "%s", separator);
      fprintf(f, "%s::~%s() {\n  \n}\n\n", cname.c_str(), cname.c_str());
      fprintf(f, "%s", separator);
    }

    // generate constructors
    for (std::map<std::string, MetaClass::Method>::const_iterator iter = methods.begin(); iter != methods.end();
         ++iter) {
      if (iter->second.constructor) {
        fprintf(f, "%s::%s(%s%s, grt::MetaClass *meta)\n", cname.c_str(), cname.c_str(),
                iter->second.arg_types.empty() ? "" : ", ", format_arg_list(iter->second.arg_types).c_str());
        output_constructor_init_list(f);
      }
    }

    // generate member variable accessors
    for (std::map<std::string, MetaClass::Member>::const_iterator iter = members.begin(); iter != members.end();
         ++iter) {
      if (iter->second.private_)
        continue;

      if (iter->second.delegate_get) {
        fprintf(f, "%s %s::%s() const {\n // add code here\n}\n\n", format_type_cpp(iter->second.type).c_str(),
                cname.c_str(), iter->second.name.c_str());
        fprintf(f, "%s", separator);
      }

      if (!iter->second.read_only && iter->second.delegate_set) {
        fprintf(f, "void %s::%s(const %s &value) {\n", cname.c_str(), iter->second.name.c_str(),
                format_type_cpp(iter->second.type).c_str());
        fprintf(f, "  grt::ValueRef ovalue(_%s);\n", iter->second.name.c_str());
        if (iter->second.owned_object) {
          fprintf(f, "  // this member is owned by this object\n");
          fprintf(f, "// add code here\n");
          fprintf(f, "  _%s = value;\n", iter->second.name.c_str());
          fprintf(f, "  owned_member_changed(\"%s\", ovalue, value);\n", iter->second.name.c_str());
        } else {
          fprintf(f, "// add code here\n");
          fprintf(f, "  _%s = value;\n", iter->second.name.c_str());
          fprintf(f, "  member_changed(\"%s\", ovalue, value);\n", iter->second.name.c_str());
        }

        // fprintf(f, "  _changed_signal.emit(\"%s\", ovalue);\n", iter->second.name.c_str());

        fprintf(f, "}\n\n");
        fprintf(f, "%s", separator);
      }
    }

    if (gstruct->watch_lists()) {
      fprintf(f, "void %s::owned_list_item_added(grt::internal::OwnedList *list, const grt::ValueRef &value) ",
              cname.c_str());
      fprintf(f, "{\n}\n\n");
      fprintf(f, "%s", separator);
      fprintf(f, "void %s::owned_list_item_removed(grt::internal::OwnedList *list, const grt::ValueRef &value) ",
              cname.c_str());
      fprintf(f, "{\n}\n\n");
      fprintf(f, "%s", separator);
    }

    if (gstruct->watch_dicts()) {
      fprintf(f, "void %s::owned_dict_item_set(grt::internal::OwnedDict *dict, const std::string &key) ",
              cname.c_str());
      fprintf(f, "{\n}\n\n");
      fprintf(f, "%s", separator);
      fprintf(f, "void %s::owned_dict_item_removed(grt::internal::OwnedDict *dict, const std::string &key) ",
              cname.c_str());
      fprintf(f, "{\n}\n\n");
      fprintf(f, "%s", separator);
    }

    // generate methods
    for (std::map<std::string, MetaClass::Method>::const_iterator iter = methods.begin(); iter != methods.end();
         ++iter) {
      if (!iter->second.abstract && !iter->second.constructor)
        fprintf(f, "%s %s::%s(%s) {\n  // add code here\n}\n\n", format_type_cpp(iter->second.ret_type, true).c_str(),
                cname.c_str(), iter->second.name.c_str(), format_arg_list(iter->second.arg_types).c_str());
      fprintf(f, "%s", separator);
    }
  }
};

//--------------------------------------------------------------------------------------------------

// XXX: use base::basename instead.
static std::string basename(std::string s) {
  if (s.find('/') != std::string::npos)
    s = s.substr(s.rfind('/') + 1);

  if (s.find('\\') != std::string::npos)
    s = s.substr(s.rfind('\\') + 1);

  return s;
}

//--------------------------------------------------------------------------------------------------

static std::string pkgname(std::string s) {
  std::string source(basename(s));

  if (source.find('.') != std::string::npos)
    return source.substr(0, source.rfind('.'));
  return source;
}

//--------------------------------------------------------------------------------------------------

static std::string generate_dll_export_name(const std::string &fname) {
  std::string name = basename(fname);

  name = cppize_class_name(name.substr(0, name.rfind('.')));
  for (size_t i = 0; i < name.size(); i++) {
    name[i] = g_ascii_toupper(name[i]);
  }

  return "GRT_" + name;
}

//--------------------------------------------------------------------------------------------------

static std::string used_class_name(const TypeSpec &type) {
  if (type.base.type == ObjectType)
    return type.base.object_class;
  else if (type.content.type == ObjectType)
    return type.content.object_class;
  return std::string();
}

//--------------------------------------------------------------------------------------------------

static bool is_header_included_somehow(const std::string &xml_for_header, const std::string &in_xml_for_header,
                                       const std::multimap<std::string, std::string> &requiresMap) {
  if (xml_for_header == in_xml_for_header)
    return true;

  for (std::multimap<std::string, std::string>::const_iterator r = requiresMap.find(in_xml_for_header);
       r != requiresMap.end() && r->first == in_xml_for_header; ++r) {
    if (basename(r->second) == xml_for_header ||
        is_header_included_somehow(xml_for_header, basename(r->second), requiresMap))
      return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

void grt::helper::generate_struct_code(const std::string &target_file, const std::string &outpath,
                                       const std::string &imploutpath,
                                       const std::multimap<std::string, std::string> &requires_orig) {
  std::map<std::string, FILE *> files;
  std::map<std::string, std::set<std::string>> foreign_classes; // packagename -> class list
  std::multimap<std::string, std::string> requiresMap;
  const std::list<MetaClass *> &meta(grt::GRT::get()->get_metaclasses());

  // requires list is keyed by the full path of the xml, so we add the same entries to a copy of the list with basenames
  for (std::multimap<std::string, std::string>::const_iterator r = requires_orig.begin(); r != requires_orig.end(); ++r)
    requiresMap.insert(std::make_pair(basename(r->first), r->second));

  {
    std::map<std::string, std::string> package_for_struct; // structname -> packagename
    // fill mapping from struct name to package, to know what structs are declared in the header itself
    // and what comes from other headers. Later, we will forward declare externally defined class names
    // during output
    for (std::list<MetaClass *>::const_iterator iter = meta.begin(); iter != meta.end(); ++iter)
      package_for_struct[(*iter)->name()] = basename((*iter)->source());

    // fill a list of foreign classes per file
    for (std::list<MetaClass *>::const_iterator iter = meta.begin(); iter != meta.end(); ++iter) {
      // forward declare classes that are mentioned and not in #included files
      for (MetaClass::MemberList::const_iterator mem = (*iter)->get_members_partial().begin();
           mem != (*iter)->get_members_partial().end(); ++mem) {
        std::string class_name = used_class_name(mem->second.type);
        if (!class_name.empty() && pkgname(package_for_struct[class_name]) != pkgname((*iter)->source()) &&
            !is_header_included_somehow(package_for_struct[class_name], basename((*iter)->source()), requiresMap))
          foreign_classes[(*iter)->source()].insert(class_name);
      }
      for (MetaClass::MethodList::const_iterator met = (*iter)->get_methods_partial().begin();
           met != (*iter)->get_methods_partial().end(); ++met) {
        std::string class_name = used_class_name(met->second.ret_type);
        if (!class_name.empty() && pkgname(package_for_struct[class_name]) != pkgname((*iter)->source()) &&
            !is_header_included_somehow(package_for_struct[class_name], basename((*iter)->source()), requiresMap))
          foreign_classes[(*iter)->source()].insert(class_name);
        for (ArgSpecList::const_iterator arg = met->second.arg_types.begin(); arg != met->second.arg_types.end();
             ++arg) {
          class_name = used_class_name(arg->type);
          if (!class_name.empty() && pkgname(package_for_struct[class_name]) != pkgname((*iter)->source()) &&
              !is_header_included_somehow(package_for_struct[class_name], basename((*iter)->source()), requiresMap))
            foreign_classes[(*iter)->source()].insert(class_name);
        }
      }
    }
  }

  // perform dump
  for (std::list<MetaClass *>::const_iterator iter = meta.begin(); iter != meta.end(); ++iter) {
    FILE *fhdr;
    std::string header_file;
    std::string body_file;

    if (!target_file.empty() && basename(target_file) != basename((*iter)->source()))
      continue;

    std::string package_name = (*iter)->source().substr(0, (*iter)->source().rfind('.'));
    package_name = basename(package_name);

    if (package_name.size() > strlen("structs."))
      package_name = package_name.substr(strlen("structs."));
    else
      package_name = "";

    if (files.find((*iter)->source()) == files.end()) {
      std::string file = (*iter)->source();
      std::string outfile = base::basename(file);
      char *path;

      std::size_t found = outfile.find(".xml");
      if (found == std::string::npos)
        continue;

      outfile = base::strip_extension(outfile).append(".h");

      header_file = outfile;

      path = g_build_path("/", outpath.c_str(), outfile.c_str(), NULL);

      fhdr = base_fopen(path, "w+");
      if (!fhdr) {
        g_free(path);
        throw grt::os_error(path, errno);
      }
      g_print("Creating file %s\n", path);
      g_free(path);

      time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      auto time = localtime(&now);
      base::replaceStringInplace(copyright, "%year%", std::to_string(1900 + time->tm_year));

      fprintf(fhdr, "%s", copyright.c_str());
      fprintf(fhdr, "#pragma once\n\n");
      fprintf(fhdr, "#ifndef _MSC_VER\n");
      fprintf(fhdr, "  #pragma GCC diagnostic push\n");
      fprintf(fhdr, "  #pragma GCC diagnostic ignored \"-Woverloaded-virtual\"\n");
      fprintf(fhdr, "#endif\n\n");

      fprintf(fhdr, "#include \"grt.h\"\n\n");

      fprintf(fhdr, "#ifdef _MSC_VER\n");
      fprintf(fhdr, "  #pragma warning(disable: 4355) // 'this' : used in base member initializer list\n");
      fprintf(fhdr, "  #ifdef %s_EXPORT\n", generate_dll_export_name(outfile).c_str());
      fprintf(fhdr, "  #define %s_PUBLIC __declspec(dllexport)\n", generate_dll_export_name(outfile).c_str());
      fprintf(fhdr, "#else\n");
      fprintf(fhdr, "  #define %s_PUBLIC __declspec(dllimport)\n", generate_dll_export_name(outfile).c_str());
      fprintf(fhdr, "#endif\n");
      fprintf(fhdr, "#else\n");
      fprintf(fhdr, "  #define %s_PUBLIC\n", generate_dll_export_name(outfile).c_str());
      fprintf(fhdr, "#endif\n\n");

      // include external files
      for (std::multimap<std::string, std::string>::const_iterator r = requires_orig.find(file);
           r != requires_orig.end() && r->first == file; ++r) {
        std::string tmp(r->second, 0, r->second.rfind('.'));
        fprintf(fhdr, "#include \"grts/%s.h\"\n", tmp.c_str());
      }
      fprintf(fhdr, "\n");

      // forward declare all classes
      for (std::list<MetaClass *>::const_iterator jter = meta.begin(); jter != meta.end(); ++jter) {
        if ((*jter)->source() == (*iter)->source()) {
          std::string cname = cppize_class_name((*jter)->name());

          fprintf(fhdr, "class %s;\n", cname.c_str());
          fprintf(fhdr, "typedef grt::Ref<%s> %sRef;\n", cname.c_str(), cname.c_str());
        }
      }
      fprintf(fhdr, "\n\n");

      if (foreign_classes.find((*iter)->source()) != foreign_classes.end()) {
        std::set<std::string> classes(foreign_classes[(*iter)->source()]);
        for (std::set<std::string>::const_iterator i = classes.begin(); i != classes.end(); ++i) {
          g_message("Class %s is not included in %s, forward declaring explicitly", i->c_str(),
                    (*iter)->source().c_str());

          std::string cname = cppize_class_name(*i);
          fprintf(fhdr, "class %s;\n", cname.c_str());
          fprintf(fhdr, "typedef grt::Ref<%s> %sRef;\n", cname.c_str(), cname.c_str());
        }
        fprintf(fhdr, "\n\n");
      }

      // forward declare simple-impl-data classes
      for (std::list<MetaClass *>::const_iterator jter = meta.begin(); jter != meta.end(); ++jter) {
        std::string klass = (*jter)->get_attribute("simple-impl-data");
        if (!klass.empty()) {
          std::vector<std::string> parts(base::split(klass, "::"));

          for (size_t i = 0; i < parts.size() - 1; i++) {
            fprintf(fhdr, "namespace %s { ", parts[i].c_str());
          }
          fprintf(fhdr, "\n  class %s;\n", parts.back().c_str());
          for (size_t i = 0; i < parts.size() - 1; i++) {
            fprintf(fhdr, "}; ");
          }
          fprintf(fhdr, "\n\n");
        }
      }

      files[(*iter)->source()] = fhdr;
    } else {
      fhdr = files[(*iter)->source()];

      header_file = (*iter)->source().substr(0, (*iter)->source().find(".xml"));
      header_file = header_file.substr((*iter)->source().rfind('/') + 1);
    }

    body_file = imploutpath;
    body_file.append("/");

    ClassImplGenerator gen(*iter, fhdr);

    gen.generate_class_header(generate_dll_export_name((*iter)->source()) + "_PUBLIC");
    if (gen.needs_body) {
      // create the output dir if it doesn't exist
      if (!g_file_test(imploutpath.c_str(), G_FILE_TEST_IS_DIR))
        g_mkdir_with_parents(imploutpath.c_str(), 0755);

      body_file.append(gen.cname).append(".cpp.new");
      g_message("CREATE %s", body_file.c_str());
      FILE *f = base_fopen(body_file.c_str(), "w+");

      fprintf(f, "\n#include \"grts/%s.h\"\n", header_file.c_str());
      fprintf(f, "\n#include \"grtpp_util.h\"\n");

      fprintf(f, "\n\n");

      gen.generate_class_body(f);

      fclose(f);
    }
  }

  for (std::map<std::string, FILE *>::const_iterator iter = files.begin(); iter != files.end(); ++iter) {
    if (iter->second) {
      std::string name = cppize_class_name(basename(iter->first));
      const char *sname = name.c_str();

      // output code to register all classes in this file
      fprintf(iter->second, "\n\ninline void register_%s() {\n", sname);
      for (std::list<MetaClass *>::const_iterator jter = meta.begin(); jter != meta.end(); ++jter) {
        if ((*jter)->source() == iter->first) {
          std::string cname = cppize_class_name((*jter)->name());

          fprintf(iter->second, "  grt::internal::ClassRegistry::register_class<%s>();\n", cname.c_str());
        }
      }
      fprintf(iter->second, "}\n");
      fprintf(iter->second, "\n");
      fprintf(iter->second, "#ifdef AUTO_REGISTER_GRT_CLASSES\n");
      fprintf(iter->second, "static struct _autoreg__%s {\n  _autoreg__%s() {\n    register_%s();\n  }\n} __autoreg__%s;\n", sname,
              sname, sname, sname);
      fprintf(iter->second, "#endif\n\n");

      fprintf(iter->second, "#ifndef _MSC_VER\n");
      fprintf(iter->second, "  #pragma GCC diagnostic pop\n");
      fprintf(iter->second, "#endif\n\n");

      fclose(iter->second);
    }
  }
}

//--------------------------------------------------------------------------------------------------

// Module Wrapper Generator

static const char *module_wrapper_head =
  "// Automatically generated GRT module wrapper. Do not edit.\n\n"
  "using namespace grt;\n\n";

static const char *module_base_template_h =
  "class %module_class_name% : public %parent_module_class_name% {\n"
  "protected:\n"
  "  friend class grt::GRT;\n"
  "  %module_class_name%(grt::Module *module) : %parent_module_class_name%(module) {\n  }\n"
  "\n"
  "public:\n"
  "  static const char *static_get_name() {\n    return \"%module_name%\";\n  }\n\n";

static const char *module_base_template_f = "};\n\n";

static const char *module_function_template_void =
  "  void %function_name%(%args%) {\n"
  "    grt::BaseListRef args(true);\n"
  "%make_args%\n"
  "    _module->call_function(\"%function_name%\", args);\n"
  "  }\n";

static const char *module_function_template_int =
  "  ssize_t %function_name%(%args%) {\n"
  "    grt::BaseListRef args(grt::AnyType);\n"
  "%make_args%\n"
  "    grt::ValueRef ret = _module->call_function(\"%function_name%\", args);\n"
  "    return *grt::IntegerRef::cast_from(ret);\n"
  "  }\n";

static const char *module_function_template_double =
  "  double %function_name%(%args%) {\n"
  "    grt::BaseListRef args(grt::AnyType);\n"
  "%make_args%\n"
  "    grt::ValueRef ret = _module->call_function(\"%function_name%\", args);\n"
  "    return *DoubleRef::cast_from(ret);\n"
  "  }\n";

static const char *module_function_template_string =
  "  std::string %function_name%(%args%) {\n"
  "    grt::BaseListRef args(grt::AnyType);\n"
  "%make_args%\n"
  "    grt::ValueRef ret = _module->call_function(\"%function_name%\", args);\n"
  "    return *grt::StringRef::cast_from(ret);\n"
  "  }\n";

static const char *module_function_template =
  "  %return_type% %function_name%(%args%) {\n"
  "    grt::BaseListRef args(grt::AnyType);\n"
  "%make_args%\n"
  "    grt::ValueRef ret = _module->call_function(\"%function_name%\", args);\n"
  "    return %return_type%::cast_from(ret);\n"
  "  }\n";

//--------------------------------------------------------------------------------------------------

static void export_module_function(FILE *f, const Module::Function &function) {
  unsigned int i;
  std::string func_template = module_function_template;
  std::string return_type;
  std::string args;
  std::string make_args;

  return_type = format_type_cpp(function.ret_type);

  switch (function.ret_type.base.type) {
    case IntegerType:
      func_template = module_function_template_int;
      break;
    case DoubleType:
      func_template = module_function_template_double;
      break;
    case StringType:
      func_template = module_function_template_string;
      break;
    case ListType:
    case DictType:
    case ObjectType:
      func_template = module_function_template;
      break;
    default:
      func_template = module_function_template_void;
      break;
  }

  i = 0;
  const ArgSpecList::const_iterator last = function.arg_types.end();
  for (ArgSpecList::const_iterator param = function.arg_types.begin(); param != last; ++param) {
    const char *proto_arg_type = NULL;
    const std::string arg_type = format_type_cpp(param->type);
    std::string arg_name;

    switch (param->type.base.type) {
      case IntegerType:
        proto_arg_type = "ssize_t";
        break;
      case DoubleType:
        proto_arg_type = "double";
        break;
      case StringType:
        proto_arg_type = "const std::string &";
        break;
      case ListType:
      case DictType:
      case ObjectType:
        break;
      default:
        logWarning("invalid parameter type found in module function %s\n", function.name.c_str());
    }

    if (param->name.empty()) {
      char buf[40];
      sprintf(buf, "param%i", i++);
      arg_name = buf;
    } else
      arg_name = param->name;

    if (!args.empty())
      args.append(", ");
    if (!make_args.empty())
      make_args.append("\n");

    if (proto_arg_type) {
      args.append(proto_arg_type).append(" ").append(arg_name);
      make_args.append("    args.ginsert(").append(arg_type).append("(").append(arg_name).append("));");
    } else {
      args.append("const ").append(arg_type).append("& ").append(arg_name);
      make_args.append("    args.ginsert(").append(arg_name).append(");");
      ;
    }
  }

  {
    std::string code = func_template;
    base::replaceStringInplace(code, "%return_type%", return_type);
    base::replaceStringInplace(code, "%function_name%", function.name);
    base::replaceStringInplace(code, "%args%", args);
    base::replaceStringInplace(code, "%make_args%", make_args);

    fprintf(f, "%s", code.c_str());
  }
}

//--------------------------------------------------------------------------------------------------

void grt::helper::generate_module_wrappers(const std::string &outpath, const std::vector<Module *> &modules) {
  FILE *f = base_fopen(outpath.c_str(), "w+");
  if (!f)
    throw grt::os_error(errno);

  std::string header_name = base::basename(outpath);
  base::replaceStringInplace(header_name, ".", "_");

  time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  auto time = localtime(&now);
  base::replaceStringInplace(copyright, "%year%", std::to_string(1900 + time->tm_year));

  fprintf(f, "%s", copyright.c_str());
  fprintf(f, "#pragma once\n\n");
  fprintf(f, "%s", module_wrapper_head);

  for (std::vector<Module *>::const_iterator module = modules.begin(); module != modules.end(); ++module) {
    std::string code = module_base_template_h;
    base::replaceStringInplace(code, "%module_name%", (*module)->name());

    base::replaceStringInplace(code, "%module_class_name%", base::strfmt("%sWrapper", (*module)->name().c_str()));

    if (!(*module)->extends().empty())
      base::replaceStringInplace(code, "%parent_module_class_name%",
                                 base::strfmt("%sWrapper", (*module)->extends().c_str()));
    else
      base::replaceStringInplace(code, "%parent_module_class_name%", "grt::ModuleWrapper");

    fprintf(f, "%s", code.c_str());

    for (std::vector<Module::Function>::const_iterator func = (*module)->get_functions().begin();
         func != (*module)->get_functions().end(); ++func) {
      export_module_function(f, *func);
    }

    fprintf(f, "%s", module_base_template_f);
  }

  fclose(f);
}

//--------------------------------------------------------------------------------------------------
