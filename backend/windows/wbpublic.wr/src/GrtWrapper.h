/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "grt.h"
#include "grt/grt_value_inspector.h"
#include "grt/grt_manager.h"
#include "grt/grt_shell.h"
#include "ConvUtils.h"

namespace MySQL {
  namespace Grt {

  public
    enum class GrtValueType {
      AnyValue = ::grt::AnyType,
      IntValue = ::grt::IntegerType,
      RealValue = ::grt::DoubleType,
      DoubleValue = ::grt::DoubleType,
      StringValue = ::grt::StringType,
      ObjectValue = ::grt::ObjectType,
      ListValue = ::grt::ListType,
      DictValue = ::grt::DoubleType
    };

  public
    ref class GrtValue {
    private:
      grt::ValueRef *inner;

    public:
      GrtValue(GrtValue ^ val) {
        inner = new grt::ValueRef(val->get_unmanaged_object());
      }

      GrtValue(grt::ValueRef *inn) {
        inner = new grt::ValueRef(*inn);
      }

      GrtValue(const grt::ValueRef &inn) {
        inner = new grt::ValueRef(inn);
      }

      GrtValue() : inner(new grt::ValueRef) {
      }

      ~GrtValue() {
        this->!GrtValue();
      }

      !GrtValue() {
        delete inner;
      }

      GrtValue ^ operator=(grt::ValueRef *val) {
        delete inner;
        if (val == NULL)
          inner = new grt::ValueRef;
        else
          inner = new grt::ValueRef(*val);
        return this;
      }

      GrtValue ^ operator=(GrtValue ^ val) {
        delete inner;
        inner = new grt::ValueRef(val->get_unmanaged_object());
        return this;
      }

      const grt::ValueRef &get_unmanaged_object() {
        return *inner;
      }

      GrtValueType get_type() {
        return (GrtValueType)inner->type();
      }
      bool is_object_instance_of(System::String ^ struct_name) {
        if (get_type() == GrtValueType::ObjectValue)
          return ::grt::ObjectRef::cast_from(*inner).is_instance(NativeToCppString(struct_name));
        return false;
      }

      System::String ^ object_id() {
        if (get_type() != GrtValueType::ObjectValue || !inner || ::grt::ObjectRef::cast_from(*inner).id().empty())
          return gcnew System::String("");

        return CppStringToNative(::grt::ObjectRef::cast_from(*inner).id());
      }
    };

  public
    ref class GrtModule {
    private:
      grt::Module *inner;

    public:
      GrtModule(GrtModule ^ module) {
        inner = module->get_unmanaged_object();
      }

      GrtModule(grt::Module *inn) : inner(inn) {
      }

      GrtModule() : inner(0) {
      }

      grt::Module *get_unmanaged_object() {
        return inner;
      }
    };

  public
    ref class GRT {
    public:
      GRT() {
      }

      ::grt::GRT *get_unmanaged_object() {
        return grt::GRT::get().get();
      }

      static System::String ^ version() { return CppStringToNative(GRT_VERSION); }

    };

#ifdef notused
  public
    ref class GenericValue {
    protected:
      ::grt::ValueRef *inner;

    public:
      // void __replace_value(MYX_GRT_VALUE *nvalue)

      // int refcount() { return myx_grt_value_get_current_reference_count(_value); }

      GenericValue() : inner(new ::grt::ValueRef()) {
      }

      GenericValue(GenericValue % other) : inner(other.get_unmanaged_object()) {
      }

      explicit GenericValue(::grt::ValueRef *value) : inner(new ::grt::GenericValue(value)) {
      }

      GenericValue(::grt::ValueRef *value) : inner(value) {
      }

      virtual ~GenericValue() {
        delete inner;
      }

      inline ::grt::ValueRef *get_unmanaged_object() {
        return inner;
      }

      inline void release() {
        inner->release();
      }

      inline GenericValue % retain() {
        inner->retain();
        return *this;
      }

      inline void reset() {
        inner->reset();
      }

      inline bool is_valid() {
        return inner->is_valid();
      }

      inline GrtValue ^ grt_value() { return gcnew GrtValue(inner->grt_value()); }

        // static _clr_MYX_GRT_VALUE_TYPE class_type()
        //  { return static_cast<_clr_MYX_GRT_VALUE_TYPE>(::grt::GenericValue::class_type()); }

        /*inline _clr_MYX_GRT_VALUE_TYPE type()
          { return static_cast<_clr_MYX_GRT_VALUE_TYPE>(inner->type()); }*/
        inline GrtValueType type() {
        return static_cast<GrtValueType>(inner->type());
      }

      GenericValue % operator=(GenericValue % other) {
        inner->operator=(*(other.get_unmanaged_object()));
        return *this;
      }

      // std::string repr() const { char *tmp= myx_grt_value_formated_as_string(_value); std::string s= tmp;
      // g_free(tmp); return s; }
    };

  public
    ref class IntValue : public GenericValue {
    public:
      IntValue() {
      }

      explicit IntValue(MYX_GRT_VALUE *ivalue) : GenericValue(new ::grt::IntValue(ivalue)) {
      }

      IntValue(IntValue % ivalue) : GenericValue(ivalue.get_unmanaged_object()) {
      }

      explicit IntValue(int value) : GenericValue(new ::grt::IntValue(value)) {
      }

      inline ::grt::IntValue *get_unmanaged_object() {
        return static_cast<::grt::IntValue *>(inner);
      }

      inline operator int() {
        return get_unmanaged_object()->operator int();
      }

      IntValue % operator=(IntValue % other) {
        get_unmanaged_object()->operator=(*(other.get_unmanaged_object()));
        return *this;
      }

      IntValue % operator=(int value) {
        get_unmanaged_object()->operator=(value);
        return *this;
      }
    };

  public
    ref class DoubleValue : public GenericValue {
    public:
      DoubleValue() {
      }

      explicit DoubleValue(MYX_GRT_VALUE *ivalue) : GenericValue(new ::grt::DoubleValue(ivalue)) {
      }

      DoubleValue(DoubleValue % ivalue) : GenericValue(ivalue.get_unmanaged_object()) {
      }

      /*explicit*/ DoubleValue(double value) : GenericValue(new ::grt::DoubleValue(value)) {
      }

      inline ::grt::DoubleValue *get_unmanaged_object() {
        return static_cast<::grt::DoubleValue *>(inner);
      }

      // static MYX_GRT_VALUE_TYPE class_type() { return MYX_REAL_VALUE; }

      inline operator double() {
        return get_unmanaged_object()->operator double();
      }

      DoubleValue % operator=(DoubleValue % other) {
        get_unmanaged_object()->operator=(*(other.get_unmanaged_object()));
        return *this;
      }

      DoubleValue % operator=(double value) {
        get_unmanaged_object()->operator=(value);
        return *this;
      }
    };

  public
    ref class StringValue : public GenericValue {
    public:
      StringValue() {
      }

      explicit StringValue(MYX_GRT_VALUE *svalue) : GenericValue(::new ::grt::StringValue(svalue)) {
      }

      StringValue(StringValue % svalue) : GenericValue(svalue.get_unmanaged_object()) {
      }

      /*explicit*/ StringValue(String ^ svalue) : GenericValue(::new ::grt::StringValue(NativeToCppString(svalue))) {
      }

      // StringValue(const std::string& svalue)
      //  : GenericValue(::new ::grt::StringValue(svalue))
      //  {}

      inline ::grt::StringValue *get_unmanaged_object() {
        return static_cast<::grt::StringValue *>(inner);
      }

      // inline operator std::string () { return myx_grt_value_as_string(_value); }

      StringValue % operator=(StringValue % other) {
        get_unmanaged_object()->operator=(*(other.get_unmanaged_object()));
        return *this;
      }

      StringValue % operator=(const std::string &value) {
        get_unmanaged_object()->operator=(value);
        return *this;
      }
    };

  public
    ref class BaseListValue : public GenericValue {
    public:
      BaseListValue() : GenericValue(new ::grt::BaseListValue) {
      }

      // explicit BaseListValue(MYX_GRT_VALUE *lvalue)
      //  : inner(new ::grt::BaseListValue(lvalue))
      //  {}

      BaseListValue(::grt::BaseListValue *value) : GenericValue(value) {
      }

      inline ::grt::BaseListValue *get_unmanaged_object() {
        return static_cast<::grt::BaseListValue *>(inner);
      }

      inline void init(GRT ^ grt, GrtValueType type) {
        get_unmanaged_object()->init(grt::GRT::get()->get_unmanaged_object(), static_cast<MYX_GRT_VALUE_TYPE>(type));
      }

      // inline GRT *grt()
      //  { return BaseListValue::grt(); }

      // static MYX_GRT_VALUE_TYPE class_type()
      //  { return inner->class_type(); }

      inline GrtValueType content_type() {
        return static_cast<GrtValueType>(get_unmanaged_object()->content_type());
      }

      // inline std::string content_struct_name() const
      //  { return inner->content_struct_name(); }

      // static BaseListValue cast_from(const GenericValue &value)
      //  { return inner->cast_from(value); }
      //
      // inline bool can_contain(const GenericValue &value)
      //  { return inner->can_contain(value); }

      // inline void remove(unsigned int index)
      //  { return inner->remove(index); }

      inline unsigned int count() {
        return (unsigned int)get_unmanaged_object()->count();
      }

      // inline GenericValue operator[](unsigned int index) const
      //  { return inner->operator [] (index); }

      // inline GenericValue get(unsigned int index) const
      //  { return inner->get(index); }

      // inline void gset(unsigned int index, const GenericValue &value)
      //  { return inner->gset(index, value); }

      // void ginsert(const ::grt::GenericValue &value, int index)
      //  { return inner->ginsert(value, index); }

      void ginsert(GenericValue ^ value) {
        return get_unmanaged_object()->ginsert(*(value->get_unmanaged_object()), -1);
      }

      // void add_listener(const boost::function1<void (MYX_GRT_NOTIFICATION*)> &listener, const char *name= 0)
      //  { return inner->add_listener(listener, name); }
    };

  public
    ref class IntListValue : public BaseListValue {
    public:
      ref class Reference // inherits public attribute from parent
      {
        ::grt::IntListValue::Reference *inner;

      public:
        Reference(const ::grt::IntListValue::Reference &inn) : inner(new ::grt::IntListValue::Reference(inn)) {
        }

        Reference(IntListValue % list, unsigned int index)
          : inner(new ::grt::IntListValue::Reference(list.get_unmanaged_object(), index)) {
        }

        Reference(Reference % other) : inner(new ::grt::IntListValue::Reference(*other.get_unmanaged_object())) {
        }

        ::grt::IntListValue::Reference *get_unmanaged_object() {
          return inner;
        }

        Reference % operator=(IntValue % value) {
          inner->operator=(*(value.get_unmanaged_object()));
          return *this;
        }

        inline operator int() {
          return inner->operator int();
        }
      };

      IntListValue() {
      }

      explicit IntListValue(MYX_GRT_VALUE *lvalue) : BaseListValue(new ::grt::IntListValue(lvalue)) {
      }

      inline ::grt::IntListValue *get_unmanaged_object() {
        return static_cast<::grt::IntListValue *>(inner);
      }

      void init(GRT ^ grt) {
        get_unmanaged_object()->init(grt::GRT::get()->get_unmanaged_object());
      }

      static inline bool can_wrap(GenericValue % value) {
        return ::grt::IntListValue::can_wrap(*value.get_unmanaged_object());
      }

      // inline void insert(IntValue% value, int index)
      //  { inner->insert(*(value.get_unmanaged_object()), index); }

      // inline void insert(IntValue% value)
      //  { inner->insert(*(value.get_unmanaged_object()), -1); }

      inline void insert(int value, int index) {
        get_unmanaged_object()->insert(value, index);
      }

      inline void insert(int value) {
        get_unmanaged_object()->insert(value, -1);
      }

      inline Reference operator[](unsigned int index) {
        return Reference(get_unmanaged_object()->operator[](index));
      }

      // inline IntValue operator[](unsigned int index) const
      //  { return new  }

      // inline IntValue^ get(unsigned int index)
      //  { return gcnew IntValue(get_unmanaged_object()->get(index)); }

      inline void set(unsigned int index, IntValue % value) {
        get_unmanaged_object()->set(index, *(value.get_unmanaged_object()));
      }
    };

  public
    ref class StringListValue : public BaseListValue {
    public:
      ref class Reference {
        ::grt::StringListValue::Reference *inner;

      public:
        const Reference(const ::grt::StringListValue::Reference &inn)
          : inner(new ::grt::StringListValue::Reference(inn)) {
        }

        Reference(StringListValue % list, unsigned int index)
          : inner(new ::grt::StringListValue::Reference(list.get_unmanaged_object(), index)) {
        }

        Reference(Reference % other) : inner(new ::grt::StringListValue::Reference(*other.get_unmanaged_object())) {
        }

        inline ::grt::StringListValue::Reference *get_unmanaged_object() {
          return inner;
        }

        Reference % operator=(StringValue % value) {
          inner->operator=(*(value.get_unmanaged_object()));
          return *this;
        }
      };

      StringListValue() {
      }

      explicit StringListValue(MYX_GRT_VALUE *lvalue) : BaseListValue(new ::grt::StringListValue(lvalue)) {
      }

      ::grt::StringListValue *get_unmanaged_object() {
        return static_cast<::grt::StringListValue *>(inner);
      }

      void init(GRT ^ grt) {
        get_unmanaged_object()->init(grt::GRT::get()->get_unmanaged_object());
      }

      static inline bool can_wrap(GenericValue ^ value) {
        return ::grt::StringListValue::can_wrap(*value->get_unmanaged_object());
      }

      inline void insert(StringValue ^ value, int index) {
        get_unmanaged_object()->insert(*value->get_unmanaged_object(), index);
      }

      inline void insert(StringValue ^ value) {
        get_unmanaged_object()->insert(*value->get_unmanaged_object(), -1);
      }

      inline Reference operator[](unsigned int index) {
        return Reference(get_unmanaged_object()->operator[](index));
      }

      // inline StringValue operator[](unsigned int index) const
      //  {}

      inline StringValue ^
        get(unsigned int index) { return gcnew StringValue(get_unmanaged_object()->get(index).grt_value()); }

        inline void set(unsigned int index, StringValue ^ value) {
        get_unmanaged_object()->set(index, *value->get_unmanaged_object());
      }
    };

  public
    ref class DictValue : public GenericValue {
    public:
      ref class Reference {
        ::grt::DictValue::Reference *inner;

      public:
        Reference(DictValue % dict, const std::string &key)
          : inner(new ::grt::DictValue::Reference(dict.get_unmanaged_object(), key)) {
        }

        Reference(Reference % other) : inner(new ::grt::DictValue::Reference(*other.get_unmanaged_object())) {
        }

        Reference(const ::grt::DictValue::Reference &inn) : inner(new ::grt::DictValue::Reference(inn)) {
        }

        ::grt::DictValue::Reference *get_unmanaged_object() {
          return inner;
        }

        //~Reference()

        inline GenericValue operator->() {
          return GenericValue(inner->operator->());
        }
      };

      DictValue() {
      }

      explicit DictValue(MYX_GRT_VALUE *dvalue) : GenericValue(new ::grt::DictValue(dvalue)) {
      }

      inline ::grt::DictValue *get_unmanaged_object() {
        return static_cast<::grt::DictValue *>(inner);
      }

      inline void init(GRT ^ grt, GrtValueType type) {
        get_unmanaged_object()->init(grt::GRT::get()->get_unmanaged_object(), static_cast<MYX_GRT_VALUE_TYPE>(type),
                                     "");
      }

      inline void init(GRT ^ grt, GrtValueType type, const std::string &cstruct) {
        get_unmanaged_object()->init(grt::GRT::get()->get_unmanaged_object(), static_cast<MYX_GRT_VALUE_TYPE>(type),
                                     cstruct);
      }

      // inline ::()
      //  { return inner->grt(); }

      // static MYX_GRT_VALUE_TYPE class_type()
      //  { return ::grt::DictValue::class_type(); }

      inline MYX_GRT_VALUE_TYPE content_type() {
        return get_unmanaged_object()->content_type();
      }

      inline std::string content_struct_name() {
        return get_unmanaged_object()->content_struct_name();
      }

      // static DictValue cast_from(GenericValue% ivalue)
      //  { return ::grt::DictValue::cast_from(*ivalue.get_unmanaged_object())); }

      // inline bool can_contain(GenericValue% value)
      //  { return get_unmanaged_object()->can_contain(*value.get_unmanaged_object()); }

      inline bool has_key(const std::string &k) {
        return get_unmanaged_object()->has_key(k);
      }

      // inline GenericValue operator[](const std::string &k) const
      //  { return GenericValue(inner->operator[](k)); }

      inline Reference operator[](const std::string &k) {
        return Reference(get_unmanaged_object()->operator[](k));
      }

      void remove(const std::string &k) {
        get_unmanaged_object()->remove(k);
      }

      GenericValue get(const std::string &k) {
        return GenericValue(&get_unmanaged_object()->get(k));
      }

      void set(String ^ k, GenericValue ^ value) {
        return get_unmanaged_object()->set(NativeToCppString(k), *value->get_unmanaged_object());
      }

      inline unsigned int count() {
        return (unsigned int)get_unmanaged_object()->count();
      }

      inline bool get_by_index(unsigned int index, std::string &k, GenericValue % v) {
        return get_unmanaged_object()->get_by_index(index, k, *v.get_unmanaged_object());
      }

      // void add_listener(const boost::function<void (MYX_GRT_NOTIFICATION*)> &listener, const char *name= 0);
    };

#endif

  } // namespace Grt
} // namespace MySQL
