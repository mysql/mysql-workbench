/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::CompilerServices;
using namespace System::Security::Permissions;
using namespace Runtime::InteropServices;

//
// General Information about an assembly is controlled through the following
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
//
[assembly:AssemblyTitleAttribute("MySQL Workbench Windows mforms wrapper")];
[assembly:AssemblyDescriptionAttribute("")];
[assembly:AssemblyConfigurationAttribute("")];
[assembly:AssemblyCompanyAttribute("Oracle Corporation")];
[assembly:AssemblyProductAttribute("mforms.wr")];
[assembly:AssemblyCopyrightAttribute("Copyright (c) 2010, 2017, Oracle and/or its affiliates")];
[assembly:AssemblyTrademarkAttribute("")];
[assembly:AssemblyCultureAttribute("")];

//
// Version information for an assembly consists of the following four values:
//
//      Major Version
//      Minor Version
//      Build Number
//      Revision
//
// You can specify all the value or you can default the Revision and Build Numbers
// by using the '*' as shown below:
// ml: avoid using the star, it will occasionally bring up the
// "Inconsistent field declarations in duplicated types" error.
// See also:
// https://connect.microsoft.com/VisualStudio/feedback/details/442615/managed-incremental-build-doesnt-work-if-change-to-dependent-project-was-insignificant
[assembly:AssemblyVersionAttribute("1.0.0.1")];

[assembly:ComVisible(false)];

[assembly:CLSCompliantAttribute(true)];
