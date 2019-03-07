/*
 * Copyright (c) 2010, 2019, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Diagnostics;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Ipc;
using System.Threading;
using System.Windows.Forms;

namespace MySQL.Workbench
{
  /// <summary>
  /// Application Instance Manager
  /// </summary>
  public static class ApplicationInstanceManager
  {
    /// <summary>
    /// Creates the single instance.
    /// </summary>
    /// <param name="name">The name.</param>
    /// <param name="callback">The callback.</param>
    /// <returns></returns>
    public static bool CreateSingleInstance(string name, String[] args, 
      EventHandler<InstanceCallbackEventArgs> callback)
    {
      Logger.LogDebug("Workbench", 1, "Creating single instance setup\n");

      EventWaitHandle eventWaitHandle = null;
      string eventName = String.Format("{0}.{1}.{2}", Environment.MachineName, Environment.UserName, name);

      InstanceProxy.IsFirstInstance = false;
      InstanceProxy.CommandLineArgs = args;

      try
      {
        // Try opening existing wait handle.
        Logger.LogDebug("Workbench", 2, "Trying to open existing event wait handle\n");
        eventWaitHandle = EventWaitHandle.OpenExisting(eventName);
      }
      catch
      {
        // Got exception => handle wasn't created yet.
        InstanceProxy.IsFirstInstance = true;
      }

      if (InstanceProxy.IsFirstInstance)
      {
        Logger.LogDebug("Workbench", 2, "This is the first application instance\n");
        
        // Since this is the first instance we need to set up our communication infrastructure.
        eventWaitHandle = new EventWaitHandle(false, EventResetMode.AutoReset, eventName);
        ThreadPool.RegisterWaitForSingleObject(eventWaitHandle, WaitOrTimerCallback, callback,
          Timeout.Infinite, false);
        eventWaitHandle.Close();

        // Register shared type (used to pass data between processes).
        RegisterRemoteType(name);
      }
      else
      {
        Logger.LogDebug("Workbench", 2, "Another application instance is already running\n");
        
        // We are second in a row, so pass application arguments to the shared object and quit.
        UpdateRemoteObject(name);

        if (eventWaitHandle != null)
          eventWaitHandle.Set();
      }

      return InstanceProxy.IsFirstInstance;
    }

    /// <summary>
    /// Sends the stored command line arguments over to the first instance.
    /// </summary>
    /// <param name="uri">The name used to identify the application.</param>
    private static void UpdateRemoteObject(string uri)
    {
      Logger.LogDebug("Workbench", 2, "Sending our command line arguments to the first instance\n");
      
      // Open an inter-process-communication channel to the target application.
      var clientChannel = new IpcClientChannel();
      ChannelServices.RegisterChannel(clientChannel, true);

      // Get shared object from other process.
      var proxy = Activator.GetObject(typeof(InstanceProxy),
        String.Format("ipc://{0}.{1}.{2}/{2}", Environment.MachineName, Environment.UserName, uri)) as InstanceProxy;

      // If we got a proxy object then pass the current command line arguments on.
      if (proxy != null)
        try
        {
          proxy.SetCommandLineArgs(InstanceProxy.IsFirstInstance, InstanceProxy.CommandLineArgs);
        }
        catch
        {
          Logger.LogError("Workbench", "Sending command line parameters to existing instance failed.\n");
          MessageBox.Show("MySQL Workbench encountered a problem when trying to pass on command line parameters" +
            " to the already running Workbench instance. Maybe there's a hanging Workbench process that is" +
            " pretending to be the current instance.\n\nPlease kill the hanging process and try again.",
            "MySQL Workbench Execution Problem", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

      // Finally clean up. We are done.
      ChannelServices.UnregisterChannel(clientChannel);
    }

    /// <summary>
    /// Registers our instance proxy so we can use it in IPC.
    /// </summary>
    /// <param name="uri">The name used to identify the application.</param>
    private static void RegisterRemoteType(string uri)
    {
      Logger.LogDebug("Workbench", 2, "Registering IPC channel and instance proxy\n");

      // Create and register the IPC channel for communication.
      var serverChannel = new IpcServerChannel(
        String.Format("{0}.{1}.{2}", Environment.MachineName, Environment.UserName, uri));
      ChannelServices.RegisterChannel(serverChannel, true);

      // Register the proxy type...
      RemotingConfiguration.RegisterWellKnownServiceType(
        typeof(InstanceProxy), uri, WellKnownObjectMode.Singleton);

      // ... and take care that things are cleaned up when the process goes down.
      Process process = Process.GetCurrentProcess();
      process.Exited += delegate { ChannelServices.UnregisterChannel(serverChannel); };
    }

    /// <summary>
    /// Callback triggered if the wait handle is set. This means a new process was started and
    /// we have to pass on its command line parameters to the first instance.
    /// </summary>
    /// <param name="state">The application callback.</param>
    /// <param name="timedOut">Can never be true as we have an infinite timeout.</param>
    private static void WaitOrTimerCallback(object state, bool timedOut)
    {
      Logger.LogDebug("Workbench", 2, "Application instance wait handle was triggered\n");
      
      var callback = state as EventHandler<InstanceCallbackEventArgs>;
      if (callback == null)
        return;

      // Invoke the first instance's application callback so it can do what it needs to do
      // with the given parameters.
      Logger.LogDebug("Workbench", 2, "Sending passed-in command line arguments to the callback\n");
      callback(state,
        new InstanceCallbackEventArgs(InstanceProxy.IsFirstInstance, InstanceProxy.CommandLineArgs)
      );
    }
  }
}
