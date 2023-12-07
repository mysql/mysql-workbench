/*
 * Copyright (c) 2007, 2023, Oracle and/or its affiliates. All rights reserved.
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

#include <regex>
#include "base/string_utilities.h"
#include "grtpp_shell.h"

using namespace grt;

#define O_VECTOR_COUNT 64 // max # of ()*2+2

std::string get_value_from_text_ex(const std::string& txt, const std::string& regexpr, unsigned int substringNr) {
  std::regex regex(regexpr, std::regex::icase);
  auto begin = std::sregex_iterator(txt.begin(), txt.end(), regex);
  auto end = std::sregex_iterator();
  if(std::distance(begin, end) >= substringNr)
    return "";

  unsigned int idx = 0; 
  for (std::sregex_iterator i = begin; i != end; ++i, ++idx) {
    if (idx == substringNr) {
      std::smatch match = *i;
      return match.str();
    }
  }

  return "";
}


//----------------------------------------------------------------------------------------------------------------------

Shell::Shell() {
}

Shell::~Shell() {
}

#define MAX_NESTING 100

std::string Shell::get_abspath(const std::string &curpath, const std::string &dir) {
  if (dir.empty() || dir == ".") {
    // No real path info to add. Simply return a duplicate of what is current.
    return curpath;
  } else if (dir[0] == '/')
    return dir;
  else {
    int I;
    gchar **Run;
    gchar *NewPath;
    gchar *New[MAX_NESTING];
    gchar **Current;
    gchar **Append;

    // Split the current and new paths into single tokens.
    Current = g_strsplit(curpath.c_str(), "/", MAX_NESTING);
    Append = g_strsplit(dir.c_str(), "/", MAX_NESTING);
    memset(New, 0, sizeof(New));

    // Fill the new parts array with the current path parts initially.
    // In any case we need the root slash, so start from 1 instead 0.
    I = 0;
    New[I++] = (gchar *)"";
    Run = Current;
    while (I < MAX_NESTING && *Run != NULL) {
      if (*Run != NULL && **Run != '\0')
        New[I++] = *Run;
      ++Run;
    };

    // Now look through the path to append piece by piece and collect the final path.
    Run = Append;
    while (I < MAX_NESTING && *Run != NULL) {
      // Nothing to do if only a single dot was given (meaning the current dir) or no part at all (e.g. //).
      if ((**Run != '\0') && (strcmp(*Run, ".") != 0)) {
        if (strcmp(*Run, "..") == 0) {
          // One level up. Check that we do not go beyond the top level.
          if (I > 1)
            New[--I] = NULL;
        } else {
          // Anything else is considered a normal path part. Add it to the new list.
          New[I++] = *Run;
        };
      };
      ++Run;
    };

    // Finally create a new path by joining all new path parts.
    // If there is only the root part then the join call will not add a single slash. Do it manually.
    if (New[1] == NULL)
      NewPath = g_strdup("/");
    else
      NewPath = g_strjoinv("/", New);

    std::string ret(NewPath);
    g_free(NewPath);
    g_strfreev(Current);
    g_strfreev(Append);

    return ret;
  };
}

void Shell::print(const std::string &str) {
  grt::GRT::get()->send_output(str);
}

bool Shell::set_disable_quit(bool flag) {
  bool o = _disable_quit;
  _disable_quit = flag;
  return o;
}

/**
 ****************************************************************************
 * @brief Executes a Python command in the Python shell
 *
 * This will execute the given Python command in the Python Shell from the
 * GRT environment. Some pre-processing will be performed in the command
 * to handle special GRT shell commands.
 *
 * @param grt The GRT environment the shell belongs to.
 * @param linebuf Line containing the command to be executed.
 *
 * @return
 ****************************************************************************
 */
ShellCommand Shell::execute(const std::string &linebuf) {
    ShellCommand res = ShellCommandUnknown;
  char *preprocessed_cmd = NULL;
  
  /*line = g_strdup(linebuf.c_str());
  cmd = g_strchug(g_strchomp(line));
  cmd_len = (unsigned int)strlen(cmd);*/

  std::string cmd = base::trim(linebuf);
  std::string cmdParam;

  // Help command
  if (strcmp(cmd.c_str(), "help") == 0) {
    show_help("");

    res = ShellCommandHelp;
  } else if (!(cmdParam = get_value_from_text_ex(cmd, "^(help|\\\\h)\\s+([\\w\\/\\.]*)", 2)).empty()) {
    show_help(cmdParam);

    res = ShellCommandHelp;
  } else if (!(cmdParam = get_value_from_text_ex(cmd, "^(\\?|\\-\\?)\\s*([\\w\\/\\.]*)", 2)).empty()) {
    show_help(cmdParam);

    res = ShellCommandHelp;
  }
  // Quit command
  else if ((strcmp(cmd.c_str(), "quit") == 0) || (strcmp(cmd.c_str(), "quit;") == 0) ||
           (strcmp(cmd.c_str(), "exit") == 0) || (strcmp(cmd.c_str(), "exit;") == 0) ||
           (strcmp(cmd.c_str(), "\\q") == 0) || (strcmp(cmd.c_str(), "q") == 0) || (strcmp(cmd.c_str(), "\\e") == 0)) {
    if (_disable_quit)
      print("Command not available\n");
    else {
      print("Exiting...\n");

      res = ShellCommandExit;
    }
  } else if ((strcmp(cmd.c_str(), "run") == 0) || (g_str_has_prefix(cmd.c_str(), "\\r")) ||
             (g_str_has_prefix(cmd.c_str(), "run "))) {
    std::string fileName = get_value_from_text_ex(cmd, "(run|\\\\r)\\s+(.+)", 2);

    if (!fileName.empty()) {
      preprocessed_cmd = g_strdup_printf("run(\"%s\")\n", fileName.c_str());
      res = ShellCommandStatement;
    } else {
      show_help("run");
      res = ShellCommandHelp;
    }
  }
  // Automatically convert cd.. to cd(".." and
  //   cd objectname to cd("objectname")
  else if ((strcmp(cmd.c_str(), "cd") == 0) || (strcmp(cmd.c_str(), "cd..") == 0) ||
           (g_str_has_prefix(cmd.c_str(), "cd "))) {
    std::string path = get_value_from_text_ex(cmd, "cd\\s*(.+)", 1);

    if (!path.empty()) {
      preprocessed_cmd = g_strdup_printf("cd(\"%s\")\n", path.c_str());
      res = ShellCommandStatement;
    } else {
      preprocessed_cmd = g_strdup_printf("print(pwd())\n");
      res = ShellCommandStatement;
    }
  }
  // Automatically convert ls -t to table.foreach(x, print)
  else if (g_str_has_prefix(cmd.c_str(), "ls -t ")) {
    std::string path = get_value_from_text_ex(cmd, "ls\\s+\\-t\\s+(.+)", 1);

    if (!path.empty()) {
      preprocessed_cmd = g_strdup_printf("table.foreach(%s, print)\n", path.c_str());
      res = ShellCommandStatement;
    }
  }
  // Automatically convert ls -m module to grtM.show()
  else if (g_str_has_prefix(cmd.c_str(), "ls -m ")) {
    std::string path = get_value_from_text_ex(cmd, "ls\\s+\\-m\\s+(.+)", 1);

    if (!path.empty()) {
      preprocessed_cmd = g_strdup_printf("grtM.show(\"%s\")\n", path.c_str());
      res = ShellCommandStatement;
    }
  }
  // Automatically convert ls -m to grtM.list()
  else
    // TODO: Parsing for the poor. What if there is more than a space char between the command and its parameter?
    if ((strcmp(cmd.c_str(), "ls -m") == 0) || (strcmp(cmd.c_str(), "dir -m") == 0)) {
    preprocessed_cmd = g_strdup("grtM.list()\n");
    res = ShellCommandStatement;
  }
  // Automatically convert ls -s to grtS.list()
  else if ((strcmp(cmd.c_str(), "ls -s") == 0) || (strcmp(cmd.c_str(), "dir -s") == 0)) {
    preprocessed_cmd = g_strdup("grtS.list()\n");
    res = ShellCommandStatement;
  }
  // Automatically convert ls -m module to grtS.show()
  else if (g_str_has_prefix(cmd.c_str(), "ls -s ")) {
    std::string path = get_value_from_text_ex(cmd, "ls\\s+\\-s\\s+(.+)", 1);

    if (!path.empty()) {
      preprocessed_cmd = g_strdup_printf("grtS.show(\"%s\")\n", path.c_str());
      res = ShellCommandStatement;
    }
  }
  // Automatically convert ls to ls()
  else if ((strcmp(cmd.c_str(), "ls") == 0) || (strcmp(cmd.c_str(), "dir") == 0) ||
           g_str_has_prefix(cmd.c_str(), "ls ") || g_str_has_prefix(cmd.c_str(), "dir ")) {
    preprocessed_cmd = g_strdup("ls()\n");
    res = ShellCommandStatement;
  }
  // Automatically convert show to show(grt2Lua(pwd()))
  else if (strcmp(cmd.c_str(), "show") == 0) {
    preprocessed_cmd = g_strdup_printf("print(" MYX_SHELL_CURNODE ")\n");
    res = ShellCommandStatement;
  }
  // Automatically convert show objectname to show(getGlobal("objectname"))
  else if (g_str_has_prefix(cmd.c_str(), "show ")) {
    std::string path = get_value_from_text_ex(cmd, "show\\s+(.+)", 1);

    if (!path.empty()) {
      preprocessed_cmd = g_strdup_printf("print(grtV.getGlobal(\"%s\"))\n", path.c_str());
      res = ShellCommandStatement;
    }
  }

  // If the command is still unknown, it needs to be a Python command
  if ((res == ShellCommandUnknown) || (res == ShellCommandStatement)) {
    int i;

    if (!preprocessed_cmd)
      i = execute_line(linebuf);
    else
      i = execute_line(preprocessed_cmd);

    if (i > 0)
      res = ShellCommandUnknown;
    else if (i < 0)
      res = ShellCommandError;
    else
      res = ShellCommandStatement;
  }

  // g_free(cmd);
  g_free(preprocessed_cmd);

  return res;
}
