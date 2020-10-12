/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Windows.Forms;

using Microsoft.Win32;

using MySQL.Forms;
using MySQL.Grt;
using MySQL.Utilities;
using MySQL.Utilities.SysUtils;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  static class Program
  {
    #region Static Variables and Enums

    // The types of application metadata information
    public enum ApplicationMetaInfo { Company, Copyright, Version, Revision, Configuration, ReleaseType };

    // The Workbench Context
    private static WbContext wbContext = null;

    // The GRT Manager
    private static GrtManager grtManager = null;

    // Timer to guarantee idle handler is called periodically when nothing happens
    private static System.Windows.Forms.Timer timer = null;

    // Flag to collect garbage
    private static bool gcRequested = false;
    public static void CollectGarbageOnIdle()
    {
      gcRequested = true;
    }

    // Used to synchronize with for messages triggered by background threads.
    private static MainForm mainForm = null;

    private static WbFrontendCallbacks callbacks = null;

    #endregion
    
    /// <summary>
    /// The main entry point for the application.
    /// </summary>
    
    [STAThread]
    static void Main(string[] Args)
    {
      // Connect the application to console to have proper output there if requested.
      bool consoleRedirectionWorked = Win32Api.RedirectConsole();

      // Start with command line parsing.
      string userDir = System.IO.Path.Combine(System.IO.Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
        "MySQL"), "Workbench");
      Logger.InitLogger(userDir);

      if (!consoleRedirectionWorked)
        Logger.LogError("Workbench", "Console redirection failed.\n");

      System.Reflection.Assembly asm = System.Reflection.Assembly.GetEntryAssembly();
      string baseDir = System.IO.Path.GetDirectoryName(asm.Location);
      WbOptions wbOptions = new WbOptions(baseDir, userDir, true);

      if (!wbOptions.parse_args(Args, asm.Location))
      {
        Logger.LogInfo("Workbench", "Command line params told us to shut down.\n");
        return;
      }

      PrintInitialLogInfo();

      Application.EnableVisualStyles();
      Application.SetCompatibleTextRenderingDefault(false);

      // Hook into the exception handling to establish our own handling.
      AppDomain currentDomain = AppDomain.CurrentDomain; // CLR
      currentDomain.UnhandledException += new UnhandledExceptionEventHandler(OnUnhandledException);

      Application.ThreadException += // Windows Forms
         new System.Threading.ThreadExceptionEventHandler(OnGuiUnhandledException);

      // Read some early values which cannot be stored in the preferences (since they are loaded
      // later) from registry.
      bool singleInstance = true;
      string lastVersion = "";
      string currentVersion = GetApplicationMetaInfo(ApplicationMetaInfo.Version);
      Logger.LogInfo("Workbench", "Current version given by meta info is: " + currentVersion + '\n');
      RegistryKey wbKey = Registry.CurrentUser;
      try
      {
        wbKey = wbKey.OpenSubKey(@"Software\Oracle\MySQL Workbench", false);
        if (wbKey != null)
        {
          if (wbKey.GetValue("DisableSingleInstance", 0).ToString() == "1")
            singleInstance = false;
          lastVersion = wbKey.GetValue("LastStartedAs", "").ToString();
        }
        else
          Registry.CurrentUser.CreateSubKey(@"Software\Oracle\MySQL Workbench");
      }
      catch (Exception e)
      {
        Logger.LogError("Workbench", "Error while checking single instance reg key: " + e.Message + '\n');
      }
      finally
      {
        if (wbKey != null)
          wbKey.Close();
      }

      // First check if this is the first instance of Workbench (if enabled).
      // The setting for single-instance is stored in the registry as it is Windows-only
      // and loading of the application settings happens later.
      if (singleInstance)
      {
        if (!ApplicationInstanceManager.CreateSingleInstance(
          Assembly.GetExecutingAssembly().GetName().Name, Args, SingleInstanceCallback))
        {
          Logger.LogInfo("Workbench", "Exiting as another instance of WB is already running.\n");
          return;
        }
      }

      // Give the main thread a proper name, so we can later check for it when needed.
      Thread.CurrentThread.Name = "mainthread";

      // Change the working dir to to application path.
      // This is necessary because all our internal data files etc. are located under the app dir
      // and WB could have been called from a different dir.
      string workdir = System.IO.Directory.GetCurrentDirectory();
      System.IO.Directory.SetCurrentDirectory(baseDir);

      // Next check if this is the first start of a new version of WB. In this case remove all
      // compiled python files. They will be automatically recreated and can produce problems
      // under certain circumstances.
      if (currentVersion != lastVersion)
      {
        Logger.LogInfo("Workbench", "This is the first start of a new version. Doing some clean up.\n");
        List<string> failed = new List<string>();
        RemoveCompiledPythonFiles(baseDir, failed);

        // TODO: decide if we wanna ask the user to remove those files manually or just ignore them.
      }

      // Some people don't have c:\windows\system32 in PATH, so we need to set it here
      // for WBA to find the needed commands
      String systemFolder = Environment.GetFolderPath(Environment.SpecialFolder.System);
      String cleanedPath = Environment.GetEnvironmentVariable("PATH");

      String []paths= cleanedPath.Split(new char[]{';'});
      cleanedPath = "";

      // Strip all python related dirs from PATH to avoid conflicts with other Python installations.
      foreach (String path in paths)
      {
        if (!path.ToLower().Contains("python")) 
          cleanedPath = cleanedPath + ";" + path;
      }
      Environment.SetEnvironmentVariable("PATH", systemFolder + cleanedPath);
      Logger.LogInfo("Workbench", "Setting PATH to: " + systemFolder + cleanedPath + '\n');

      // Clear PYTHONPATH environment variable, as we do not need it but our python impl
      // seriously gets confused with it.
      Environment.SetEnvironmentVariable("PYTHONPATH", workdir + "\\python\\Lib;" + workdir + "\\python\\DLLs;" + workdir + "\\python");
      Environment.SetEnvironmentVariable("PYTHONHOME", workdir + "\\python");

      // Initialize forms stuff.
      MySQL.Forms.Manager formsManager = MySQL.Forms.Manager.get_instance(); // Creates the singleton.

      // init extra mforms things that are delegated to the frontend, indirectly through RecordsetWrapper in wbpublic
      MySQL.Grt.Db.RecordsetWrapper.init_mforms(MySQL.Grt.Db.RecordsetView.create);

      #region Runtime path check

      // Currently WB has trouble running from a path containing non-ASCII characters.
      // Actually, our third party libraries have (namely lua, python, ctemplate), 
      // as they don't consider Unicode file names (encoded as UTF-8) which leads to file-not-found
      // errors. Refuse to work in such a path for now.
      foreach (Char c in baseDir)
        if (c > 0x7f)
        {
          MessageBox.Show("MySQL Workbench cannot be executed from a path that contains non-ASCII characters.\n"+
            "This problem is imposed by used third-party libraries.\n" +
            "Please run this application from the default installation path or at least a path which is all ASCII characters.",
            "MySQL Workbench Execution Problem", MessageBoxButtons.OK, MessageBoxIcon.Error);
          return;
        }

      #endregion

      #region Release check (outdated beta or rc version)

      // check the date of the executable and suggest to install a new version if this is a beta or rc
      if (GetApplicationMetaInfo(ApplicationMetaInfo.Configuration).ToUpper().IndexOf("BETA") >= 0 ||
        GetApplicationMetaInfo(ApplicationMetaInfo.Configuration).ToUpper().IndexOf("RC") >= 0)
      {
        DateTime fileDate = System.IO.File.GetCreationTime(Application.ExecutablePath);

        if (DateTime.Now.Subtract(fileDate).TotalDays > 45)
        {
          Logger.LogInfo("Workbench", "Found an old WB pre release. Showing warning.\n");
          if (MessageBox.Show("This version of MySQL Workbench is older than 45 days and most probably outdated. "
            + Environment.NewLine
            + "It is recommended to upgrade to a newer version if available. "
            + Environment.NewLine
            + "Press [OK] to check for a new version and exit the application. "
            + "Press [Cancel] to continue using this version.",
            "MySQL Workbench Version Outdated", MessageBoxButtons.OKCancel,
            MessageBoxIcon.Warning, MessageBoxDefaultButton.Button1) == DialogResult.OK)
          {
            CheckForNewVersion();
            return;
          }
        }
      }

      #endregion

      #region Variables and Splashscreen

      #endregion

      #region Initialize GRT

      // Try to instantiate the Workbench context and the GRT Manager and catch exceptions
      try
      {
        // Create Workbench Context
        wbContext = new WbContext(wbOptions.Verbose);

        if (wbContext != null)
        {
          // Create the GRT Manager instance
          grtManager = wbContext.get_grt_manager();
        }
      }
      catch (Exception ex)
      {
        HandleException(ex);
      }

      #endregion

      // If the Workbench Context and GRT Manager were successfully created, 
      // initialize the application
      if (wbContext != null && grtManager != null)
      {
        #region Initialize Callbacks and Mainform

        mainForm = new MainForm(wbContext);

        // Initialize the Workbench context
        ManagedApplication formsApplication = new ManagedApplication(
          new AppCommandDelegate(mainForm.ApplicationCommand),
          mainForm.dockDelegate);

        callbacks = new WbFrontendCallbacks(
          new WbFrontendCallbacks.StrStrStrStrDelegate(mainForm.ShowFileDialog),
          new WbFrontendCallbacks.VoidStrDelegate(mainForm.ShowStatusText),
          new WbFrontendCallbacks.BoolStrStrFloatDelegate(mainForm.ShowProgress),
          new WbFrontendCallbacks.CanvasViewStringStringDelegate(mainForm.CreateNewDiagram),
          new WbFrontendCallbacks.VoidCanvasViewDelegate(mainForm.DestroyView),
          new WbFrontendCallbacks.VoidCanvasViewDelegate(mainForm.SwitchedView),
          new WbFrontendCallbacks.VoidCanvasViewDelegate(mainForm.ToolChanged),
          new WbFrontendCallbacks.IntPtrGRTManagerModuleStrStrGrtListFlagsDelegate(mainForm.OpenPlugin),
          new WbFrontendCallbacks.VoidIntPtrDelegate(mainForm.ShowPlugin),
          new WbFrontendCallbacks.VoidIntPtrDelegate(mainForm.HidePlugin),
          new WbFrontendCallbacks.VoidRefreshTypeStringIntPtrDelegate(mainForm.RefreshGUI),
          new WbFrontendCallbacks.VoidBoolDelegate(mainForm.LockGUI),
          new WbFrontendCallbacks.VoidStrDelegate(mainForm.PerformCommand),
          new WbFrontendCallbacks.BoolDelegate(mainForm.QuitApplication));

        // TODO: check return value and show error message.
        // Currently the return value is always true. In case of an error an exception is raised.
        // That should change.
        wbContext.init(callbacks, wbOptions,
          new WbContext.VoidStrUIFormDelegate(mainForm.CreateMainFormView)
        );

        // command registration must be done after WBContext init
        mainForm.PostInit();

        // Set the Application.Idle event handler
        Application.Idle += new EventHandler(OnApplicationIdle);

        // Don't call the idle handler too often.
        timer = new System.Windows.Forms.Timer();
        timer.Interval = 100;
        timer.Tick += new EventHandler(timer_Tick);
        timer.Start();

        // Trigger GRT idle tasks
        grtManager.perform_idle_tasks();
        
        // Setup Menus
        wbContext.validate_edit_menu();
        mainForm.Show();
        Logger.LogInfo("Workbench", "UI is up\n");

        // Tell the backend our main UI is ready. This will also load a model if it was given via command line
        // and opens the overview form for it.
        wbContext.finished_loading(wbOptions);

        // Right before we go to work and everything was loaded write the current version to registry
        // to allow us later to find out if we ran a new version the first time.
        try
        {
          wbKey = Registry.CurrentUser.OpenSubKey(@"Software\Oracle\MySQL Workbench", true);
          if (wbKey != null)
            wbKey.SetValue("LastStartedAs", currentVersion);
        }
        catch (Exception e)
        {
          Logger.LogError("Workbench", "Couldn't write regkey LastStartedAs: " + e.Message + '\n');
        }
        finally
        {
          if (wbKey != null)
            wbKey.Close();
        }

        // Start the Application if we are not already shutting down.
        if (!wbContext.is_quitting())
        {
          try
          {
            Logger.LogInfo("Workbench", "Running the application\n");
            Application.Run(new ApplicationContext(mainForm));
          }
          catch (Exception e)
          {
            HandleException(e);
          }
        }

        #endregion

        Logger.LogInfo("Workbench", "Shutting down Workbench\n");

        timer.Stop();
        timer.Dispose();

        // shutdown wb context
        if (wbContext != null)
        {
          while (wbContext.is_busy())
            wbContext.flush_idle_tasks(true);

          wbContext.finalize();
          wbContext.Dispose();
        }
        formsApplication.Dispose();
        formsManager.Dispose();

        GC.Collect();
      }
      
      Win32Api.ReleaseConsole();

      Logger.LogInfo("Workbench", "Done\n");
    }

    /// <summary>
    /// Prints some general info to the log file.
    /// </summary>
    private static void PrintInitialLogInfo()
    {
      Logger.LogInfo("Workbench", "Starting up Workbench\n");
      Logger.LogInfo("Workbench", string.Format("Current environment:\n\tCommand line: {0}\n\tCurrentDirectory: {1}\n" + 
        "\tHasShutdownStarted: {2}\n\tOSVersion: {3}\n\tSystemDirectory: {4}\n" +
        "\tTickCount: {5}\n\tUserInteractive: {6}\n\tVersion: {7}\n\tWorkingSet: {8}\n",
        Environment.CommandLine, Environment.CurrentDirectory, Environment.HasShutdownStarted,
        Environment.OSVersion.ToString(), Environment.SystemDirectory,
        Environment.TickCount, Environment.UserInteractive,
        Environment.Version.ToString(), Environment.WorkingSet));
      
      IDictionary environmentVariables = Environment.GetEnvironmentVariables();
      string variables = "";
      foreach (DictionaryEntry entry in environmentVariables)
        variables += string.Format("\t{0} = {1}\n", entry.Key, entry.Value);
      Logger.LogInfo("Workbench", "Environment variables:\n" + variables);
    }
    
    #region Application handlers

    static void timer_Tick(object sender, EventArgs e)
    {
      System.Windows.Forms.Timer timer = sender as System.Windows.Forms.Timer;

      // flush_idle_tasks needs to be reentrant so that functions
      // that crosses threads have their own idle tasks executed
      if (wbContext != null)
      {
        try
        {
          wbContext.flush_idle_tasks(false);
        }
        catch (Exception exception)
        {
          // temporarily stop the timer so that we don't get into a loop from showing the msgbox
          timer.Stop();
          HandleException(exception);
          timer.Start();
        }
      }
    }

    private static void OnApplicationIdle(object sender, EventArgs e)
    {
      if (gcRequested)
      {
        gcRequested = false;
        GC.Collect();
      }
    }

    /// <summary>
    /// Handler that gets called if another instance of the application was started. It passes
    /// the command line arguments from the other instance on so we can act accordingly here.
    /// </summary>
    /// <param name="sender">The sender.</param>
    /// <param name="args">The instance containing the event data.</param>
    private static void SingleInstanceCallback(object sender, InstanceCallbackEventArgs args)
    {
      if (args == null || mainForm == null || !mainForm.IsHandleCreated)
        return;

      Action<bool> d = (bool x) =>
      {
        mainForm.Activate(x);

        // Parse command line and extract the model file name if there is one.
        WbOptions options = new WbOptions("", "", false);
        System.Reflection.Assembly asm = System.Reflection.Assembly.GetEntryAssembly();
        options.parse_args(args.CommandLineArgs, asm.Location);
        wbContext.finished_loading(options);
      };
      mainForm.Invoke(d, true);
    }

    // CLR unhandled exception.
    private static void OnUnhandledException(Object sender, UnhandledExceptionEventArgs e)
    {
      HandleException(e.ExceptionObject);
    }

    // Windows Forms unhandled exception.
    private static void OnGuiUnhandledException(Object sender, ThreadExceptionEventArgs e)
    {
      HandleException(e.Exception);
    }

    #endregion

    #region Message Callbacks

    private static int ShowStringInputDialog(String Title, String Prompt,
      ref String InputString)
    {
      DialogResult result;
      result = StringInputForm.ShowModal(Title, "", Prompt, ref InputString);
      if (result == DialogResult.OK)
        return 1;
      else
        return 0;
    }

    #endregion
    
    #region Various Static Functions

    /// <summary>
    /// Retrieves application metadata and returns it as a string
    /// </summary>
    /// <param name="kind">The type of metadata information to return</param>
    /// <returns></returns>
    static public string GetApplicationMetaInfo(ApplicationMetaInfo kind)
    {
      object[] attributes;
      Version version;

      Assembly assembly = Assembly.GetExecutingAssembly();

      string value = "";

      switch (kind)
      {
        case ApplicationMetaInfo.Company:
          attributes = assembly.GetCustomAttributes(typeof(AssemblyCompanyAttribute), false);
          if (attributes.Length > 0)
            value = (attributes[0] as AssemblyCompanyAttribute).Company;
          break;

        case ApplicationMetaInfo.Copyright:
          attributes = assembly.GetCustomAttributes(typeof(AssemblyCopyrightAttribute), false);
          if (attributes.Length > 0)
            value = (attributes[0] as AssemblyCopyrightAttribute).Copyright;
          break;

        case ApplicationMetaInfo.Configuration:
          attributes = assembly.GetCustomAttributes(typeof(AssemblyConfigurationAttribute), false);
          if (attributes.Length > 0)
            value = (attributes[0] as AssemblyConfigurationAttribute).Configuration;
          break;

        case ApplicationMetaInfo.Version:
          version = new Version(Application.ProductVersion);
          value = string.Format("{0}.{1}.{2}",
            version.Major, version.Minor, version.Build);
          break;

        case ApplicationMetaInfo.Revision:
          version = new Version(Application.ProductVersion);
          value = string.Format("{0}", version.MinorRevision);
          break;
        case ApplicationMetaInfo.ReleaseType:
          attributes = assembly.GetCustomAttributes(typeof(AssemblyReleaseTypeAttribute), false);
          if (attributes.Length > 0)
            value = (attributes[0] as AssemblyReleaseTypeAttribute).ReleaseType;
          break;
      }

      return value;
    }

    static public bool CheckForNewVersion()
    {
      try
      {
        System.Diagnostics.Process.Start("http://wb.mysql.com/workbench/version-check.php?config="
          + GetApplicationMetaInfo(ApplicationMetaInfo.Configuration).Replace(' ', '_')
          + "&version="
          + GetApplicationMetaInfo(ApplicationMetaInfo.Version)
          + "&revision="
          + GetApplicationMetaInfo(ApplicationMetaInfo.Revision));
      }
      catch (Exception e)
      {
        MessageBox.Show(e.Message.ToString(), "Error Opening Browser", 
          MessageBoxButtons.OK, MessageBoxIcon.Error, MessageBoxDefaultButton.Button1);
      }

      return true;
    }

    public static void HandleException(Object o)
    {
      Exception e = o as Exception;
      String message;
      String info;

      bool isFontProblem = false;
      if (e != null)
      {
        // Report System.Exception info
        message = e.Message;

        info = "Exception = " + e.GetType() + "\n";
        info += "Message = " + e.Message + "\n";
        info += "FullText = " + e.ToString();

        isFontProblem = (e is ArgumentException) && message.Contains("'Regular'") && info.Contains(".CreateNativeFont");
      }
      else
      { // Report exception Object info
        message = "Exception = " + o.GetType();
        info = "Exception = " + o.GetType() + "\n";
        info += "FullText = " + o.ToString();
      }

      Logger.LogError("Workbench", message + "\n" + info + '\n');

      // Check for blocked files (Windows "security" feature).
      if (info.Contains("0x80131515"))
      {
        MessageBox.Show("A blocked library could not be loaded. You probably downloaded MySQL Workbench " +
          "as no-installation zip package. Windows has locked this, preventing so its full operation.\n\n" +
          "When you click \"OK\" MySQL Workbench will try to automatically unblock all its files. Restart the application " +
          "for this change to take effect!\n\nThis operation might fail " +
          "e.g. on read-only volumes or for other reasons. If that is the case then manually unblock the zip package " +
          "(see its Properties in Explorer) and unzip it again.",
          "Blocked DLL Detected", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);

        Win32Api.UnblockWorkbenchFiles(System.IO.Directory.GetCurrentDirectory());
      }
      else
      {
        // There's a relatively common error with missing 'Regular' style even on system
        // standard fonts, which is extremely weird, but not caused by WB. However, we show
        // a hint and close WB, so the user knows he can fix it and doesn't enter new bug reports for this.
        if (isFontProblem)
        {
          MessageBox.Show("There was a problem with one of the system fonts, which indicates this font " +
          "is corrupt. The original message is:\n" + message + "\nMySQL Workbench needs to close now. You " +
          "should re-install the font in question to avoid this problem in the future.",
          "Invalid Font Reported", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
          Application.Exit();
        }
        else
          ExceptionDialog.Show(message, info, wbContext);
      }
    }

    /// <summary>
    /// Removes all pyc files from the given dir and its sub dirs.
    /// </summary>
    private static void RemoveCompiledPythonFiles(string folder, List<string> failed)
    {
      foreach (string file in Directory.GetFiles(folder, "*.pyc"))
        try
        {
          File.Delete(file);
        }
        catch (Exception)
        {
          failed.Add(file);
        }

      foreach (string subfolder in Directory.GetDirectories(folder))
        RemoveCompiledPythonFiles(subfolder, failed);
    }
    #endregion

  }
}
