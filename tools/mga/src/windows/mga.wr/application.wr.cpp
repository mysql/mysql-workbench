/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma unmanaged
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <string>
#include <list>
#include <map>
#include <vector>
#include "application-context.h"


#pragma managed
#include "application.wr.h"

using namespace mga;
using namespace System::Text;

//------------------------------------------------------------------------------------------------

ApplicationContextWr::ApplicationContextWr() {
}

//------------------------------------------------------------------------------------------------

ApplicationContextWr::~ApplicationContextWr() {
}

//------------------------------------------------------------------------------------------------

int mga::ApplicationContextWr::initialize() {
  return ApplicationContext::get().initialize();
}

//------------------------------------------------------------------------------------------------

int mga::ApplicationContextWr::parseParams(array<String^>^ args, String ^ appPath, array<String^>^ env) {
  //Convert to UTF8 and keep the returned arrays as long as we are parsing that.
  // Add the application's path at the tip as the parse routine wants it so.
  array<array<unsigned char>^>^ managedArgs = gcnew array<array<unsigned char>^>(args->Length + 1);
  managedArgs[0] = Encoding::UTF8->GetBytes(appPath);
  for (int i = 0; i < args->Length; i++)
    managedArgs[i + 1] = Encoding::UTF8->GetBytes(args[i]);

  char** arguments = new char*[managedArgs->Length];


  array<array<unsigned char>^>^ managedEnv = gcnew array<array<unsigned char>^>(env->Length + 1);
  for (int i = 0; i < env->Length; i++)
    managedEnv[i] = Encoding::UTF8->GetBytes(env[i]);

  char** environmentVariables = new char*[managedEnv->Length];

  try {
    for (int i = 0; i < managedArgs->Length; i++) {
      pin_ptr<unsigned char> chars1 = &managedArgs[i][0];
      arguments[i] = (char*)chars1;
    }

    for (int i = 0; i < managedEnv->Length - 1; i++) {
      pin_ptr<unsigned char> chars2 = &managedEnv[i][0];
      environmentVariables[i] = (char*)chars2;
    }
    environmentVariables[env->Length] = nullptr;

    int ret = ApplicationContext::get().parseParams(managedArgs->Length, (const char **)arguments, environmentVariables);
    if (!ret)
      return ret;
  }
  finally {
    delete arguments;
    delete environmentVariables;
  }
  return 0;
}

//------------------------------------------------------------------------------------------------

void mga::ApplicationContextWr::run()
{
  ApplicationContext::get().run();
}

//------------------------------------------------------------------------------------------------

int mga::ApplicationContextWr::shutDown()
{ 
  return ApplicationContext::get().shutDown();
}

//------------------------------------------------------------------------------------------------