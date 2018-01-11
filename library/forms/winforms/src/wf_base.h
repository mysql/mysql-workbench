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

#include "mforms/base.h"

#include "ConvUtils.h"

// Define dummy structures for incomplete native types used in this managed lib. Otherwise
// the linker will emit warnings for those as not having a definition in MSIL code
// (which doesn't make sense for native code).
struct _cairo {};

struct _cairo_surface {};

namespace MySQL {
  namespace Forms {

    // A wrapper for unmanaged data used in drag'n drop.
    [System::Serializable] ref class DataWrapper {
    private:
      void *_data;

    public:
      DataWrapper::DataWrapper(void *data) {
        _data = data;
      }

      void *GetData() {
        return _data;
      };
    };

  public
    class ObjectWrapper {
    private:
      gcroot<System::ComponentModel::Component ^> component;

    protected:
      System::IntPtr ^ GetBackendReference();

      virtual void Initialize(){};

    public:
      ObjectWrapper(mforms::Object *object);
      virtual ~ObjectWrapper();

      template <class T>
        static T ^
        Create(mforms::Object *backend, ObjectWrapper *wrapper) {
          T ^ item = gcnew T();
          ConnectParts(backend, wrapper, item);

          return item;
        }

        template <class T>
        static void ConnectParts(mforms::Object *backend, ObjectWrapper *wrapper, T ^ item) {
        item->Tag = gcnew IntPtr(backend);
        wrapper->component = item;

        wrapper->Initialize();
      }

      // Returns the managed control/dialog etc. for which this is the wrapper.
      template <class T>
        T ^
        GetManagedObject() {
          System::ComponentModel::Component ^ value = component;
          return dynamic_cast<T ^>(value);
        }

        // A GetManagedObject variant without template for use by other assemblies.
        // Only for Controls, however.
        System::Windows::Forms::Control
        ^ GetControl() { return GetManagedObject<System::Windows::Forms::Control>(); }

        // Returns the managed control/dialog etc. for the given backend.
        template <class T>
        static T ^
        GetManagedObject(const mforms::Object *backend) {
          ObjectWrapper *wrapper = backend->get_data<ObjectWrapper>();
          System::ComponentModel::Component ^ value = wrapper->component;
          return dynamic_cast<T ^>(value);
        }

        // A convenience wrapper for the parametrized GetManagedObject function.
        static System::Windows::Forms::Control
        ^ GetControl(mforms::Object *backend) { return GetManagedObject<System::Windows::Forms::Control>(backend); }

        template <class T>
        static T *GetWrapper(System::Object ^ object) {
        mforms::Object *backend = GetBackend<mforms::Object>(object);

        if (backend != NULL)
          return backend->get_data<T>();
        return NULL;
      }

      template <class T>
      T *GetBackend() {
        return GetBackend<T>(component);
      }

      template <class T>
      static T *GetBackend(System::Object ^ object) {
        IntPtr ^ reference = nullptr;
        if (is<ToolStripItem>(object))
          reference = dynamic_cast<IntPtr ^>(((ToolStripItem ^)object)->Tag);
        else if (is<CommonDialog>(object))
          reference = dynamic_cast<IntPtr ^>(((CommonDialog ^)object)->Tag);
        else if (is<Control>(object))
          reference = dynamic_cast<IntPtr ^>(((Control ^)object)->Tag);

        if (reference != nullptr)
          return reinterpret_cast<T *>(reference->ToPointer());
        return NULL;
      }
    };

  public
    ref class ObjectMapper {
    public:
      static System::ComponentModel::Component ^ GetManagedComponent(mforms::Object *backend);
      static mforms::Object *GetUnmanagedControl(System::Windows::Forms::Control ^ control);
      static void *ManagedToNativeDragData(System::Windows::Forms::IDataObject ^ dataObject, System::String ^ format);
    };
  };
};
