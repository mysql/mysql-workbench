/* Copyright (c) 2011, 2013 Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Deployment.WindowsInstaller;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;

namespace MySQLCA
{
  public class CustomActions
  {
    [CustomAction]
    public static ActionResult PrecompilePythonFiles(Session session)
    {
      session.Log("Entering CustomAction: PrecompilePythonFiles");

      try
      {
        string installpath = GetCustomActionDataArguments(session)[0];
        Process proc = new Process();

        proc.StartInfo.FileName = installpath + "\\python.exe";
        proc.StartInfo.Arguments = "-mcompileall ..";

        proc.StartInfo.UseShellExecute = true;
        proc.StartInfo.WindowStyle = ProcessWindowStyle.Hidden;
        proc.StartInfo.WorkingDirectory = installpath + "\\python";
        proc.Start();
        proc.WaitForExit();

      }
      catch (Exception ex)
      {
        session.Log("Exception in CustomAction PrecompilePythonFiles: "+ex.Message);
        return ActionResult.Failure;
      }

      session.Log("Exiting CustomAction: PrecompilePythonFiles");

      return ActionResult.Success;
    }

    [CustomAction]
    public static ActionResult DeleteFolders(Session session)
    {
        ActionResult thisResult = ActionResult.Failure;

        session.Log("Entering CustomAction: DeleteFolders");

        string[] arguments = session.CustomActionData.ToString().Split(',');

        if (arguments == null || arguments.Length != 1)
        {
            session.Log("Error retrieving directories to delete");
        }
        else
        {
            session.Log("Attempting to remove directory");

            try
            {
                string targetDirectory = arguments[0].ToString().TrimEnd('\\');
                string targetParent = Directory.GetParent(targetDirectory).FullName;

                session.Log(String.Format("Deleting: {0}", targetDirectory));
                Directory.Delete(targetDirectory, true);
                session.Log(String.Format("Deleted: {0}", targetDirectory));

                session.Log(String.Format("Checking for empty parent: {0}", targetParent));
                if (IsDirectoryEmpty(targetParent))
                {
                    session.Log(String.Format("Deleting: {0}", targetParent));
                    Directory.Delete(targetParent, true);
                    session.Log(String.Format("Deleted: {0}", targetParent));
                }

                thisResult = ActionResult.Success;
            }
            catch (Exception ex)
            {
                session.Log("Exception in CustomAction DeleteFolders: " + ex.Message);
                thisResult = ActionResult.Failure;
            }
        }

        session.Log("Exiting CustomAction: DeleteFolders");

        return thisResult;
    }


    [CustomAction]
    public static ActionResult DeleteFilesWildcard(Session session)
    {
        ActionResult thisResult = ActionResult.Failure;

        session.Log("Entering CustomAction: DeleteFilesWildcard");

        string[] arguments = session.CustomActionData.ToString().Split(',');

        if (arguments == null || arguments.Length != 2)
        {
            session.Log("Error retrieving file pattern to delete");
        }
        else
        {
            session.Log("Attempting to remove file(s)");

            try
            {
                string   targetDirectory = arguments[0].ToString().TrimEnd('\\');
                string   targetPattern = arguments[1].ToString();
                string[] fileList = Directory.GetFiles(targetDirectory, targetPattern);


                // Delete files from the list
                foreach (string f in fileList)
                {
                    session.Log(String.Format("Deleting: {0}", f));
                    File.Delete(f);
                }

                thisResult = ActionResult.Success;
            }
            catch (Exception ex)
            {
                session.Log("Exception in CustomAction DeleteFilesWildcard: " + ex.Message);
                thisResult = ActionResult.Failure;
            }
        }

        session.Log("Exiting CustomAction: DeleteFilesWildcard");

        return thisResult;
    }


    private static bool IsDirectoryEmpty(string targetDirectory)
    {
        bool directoryEmpty = false;

        if (Directory.Exists(targetDirectory))
        {
            string[] directoryListing = Directory.GetDirectories(targetDirectory);
            string[] fileListing = Directory.GetFiles(targetDirectory);

            directoryEmpty = ((directoryListing.Length == 0) && (fileListing.Length == 0));
        }

        return directoryEmpty;
    }

    private static string[] GetCustomActionDataArguments(Session session)
    {
      string[] keys = new string[session.CustomActionData.Keys.Count];
      session.CustomActionData.Keys.CopyTo(keys, 0);
      return keys[0].Split(',');
    }
  }
}
