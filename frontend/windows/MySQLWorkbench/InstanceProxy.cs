/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

﻿using System;
using System.Security.Permissions;

namespace MySQL.Workbench
{
	/// <summary>
	/// A shared object used for interprocess communication.
	/// </summary>
	[Serializable]
	[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
	internal class InstanceProxy : MarshalByRefObject
	{
		/// <summary>
		/// Gets a value indicating whether this instance is first instance.
		/// </summary>
		/// <value>
		/// 	<c>true</c> if this instance is first instance; otherwise, <c>false</c>.
		/// </value>
		public static bool IsFirstInstance { get; internal set; }

		/// <summary>
		/// Gets the command line args.
		/// </summary>
		/// <value>The command line args.</value>
		public static string[] CommandLineArgs { get; internal set; }

		/// <summary>
		/// Sets the command line args.
		/// </summary>
		/// <param name="isFirstInstance">if set to <c>true</c> [is first instance].</param>
		/// <param name="commandLineArgs">The command line args.</param>
		public void SetCommandLineArgs(bool isFirstInstance, string[] commandLineArgs)
		{
			IsFirstInstance = isFirstInstance;
			CommandLineArgs = commandLineArgs;
		}
	}

	/// <summary>
	/// 
	/// </summary>
	public class InstanceCallbackEventArgs : EventArgs
	{
		/// <summary>
		/// Initializes a new instance of the <see cref="InstanceCallbackEventArgs"/> class.
		/// </summary>
		/// <param name="isFirstInstance">if set to <c>true</c> [is first instance].</param>
		/// <param name="commandLineArgs">The command line args.</param>
		internal InstanceCallbackEventArgs(bool isFirstInstance, string[] commandLineArgs)
		{
			IsFirstInstance = isFirstInstance;
			CommandLineArgs = commandLineArgs;
		}

		/// <summary>
		/// Gets a value indicating whether this instance is first instance.
		/// </summary>
		/// <value>
		/// 	<c>true</c> if this instance is first instance; otherwise, <c>false</c>.
		/// </value>
		public bool IsFirstInstance { get; private set; }

		/// <summary>
		/// Gets or sets the command line args.
		/// </summary>
		/// <value>The command line args.</value>
		public string[] CommandLineArgs { get; private set; }
	}
}