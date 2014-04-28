using System;
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