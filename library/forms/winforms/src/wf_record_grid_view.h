/*
* Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#pragma once

using namespace System;
using namespace System::Windows::Forms;

namespace MySQL {
  namespace Forms {
    public delegate MySQL::Base::IRecordsetView ^CreateRecordGridDelegate(IntPtr  /* to a boost::shared_ptr<Recordset> ptr */ rset);

    ref class ColumnCallbackWrapper;

    public class RecordGridViewWrapper : public NativeWrapper
    {
      static gcroot<CreateRecordGridDelegate^> factory;
      gcroot<ColumnCallbackWrapper^> column_callback_delegate;

    public:
      static mforms::RecordGrid *create(boost::shared_ptr<class ::Recordset> rset);
      static void init(CreateRecordGridDelegate ^creator);

      RecordGridViewWrapper(mforms::RecordGrid *backend);
      virtual ~RecordGridViewWrapper();
    };

    public ref class RecordGridViewHelper
    {
    public:
      static void init(CreateRecordGridDelegate ^creator) { RecordGridViewWrapper::init(creator); }
    };
  }
}
