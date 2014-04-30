/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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


#include "stdafx.h"

#include "grtpp_module_cpp.h"
#include "grtpp_util.h"
#include "base/string_utilities.h"

#include <gmodule.h>

using namespace grt;
using namespace base;


Interface::Interface(CPPModuleLoader *loader)
  : Module(loader)
{
}


Interface *Interface::create(GRT *grt, const char *name, ...)
{
  Interface *iface= new Interface(dynamic_cast<CPPModuleLoader*>(grt->get_module_loader("cpp")));
  va_list args;
  ModuleFunctorBase *func;

  iface->_name= name;

  if (!g_str_has_suffix(name, "Impl"))
  {
    g_warning("module interface classes must have the suffix Impl to avoid confusion between implementation and wrapper classes (%s)",
              name);
  }
  else
    iface->_name= iface->_name.substr(0, iface->_name.length()-4); // truncate Impl part

  // strip the namespace if it has one
  {
    std::string::size_type pos= iface->_name.find("::");
    if (pos != std::string::npos)
      iface->_name= iface->_name.substr(pos+2);
  }

  va_start(args, name);

  while ((func= va_arg(args, ModuleFunctorBase*)))
  {
    Module::Function f;
    
    f.name= func->get_name();
    f.ret_type= func->get_return_type();
    f.arg_types= func->get_signature();

    iface->add_function(f);

    delete func;
  }
  va_end(args);

  return iface;
}


bool Interface::check_conformance(const Module *module) const
{
  for (std::vector<Function>::const_iterator f= _functions.begin();
       f != _functions.end(); ++f)
  {
    const Function *function= module->get_function(f->name);
    
    if (!function)
    {
      get_grt()->send_warning(strfmt("Module '%s' does not have function '%s'", module->name().c_str(), f->name.c_str()));
      return false;
    }

    if (!(function->ret_type == f->ret_type))
    {
      get_grt()->send_warning(strfmt("Function '%s' of module '%s' has wrong return type (expected %s, got %s)",
                                     f->name.c_str(), module->name().c_str(),
                                     fmt_type_spec(f->ret_type).c_str(), fmt_type_spec(function->ret_type).c_str()));
      return false;
    }
    
    ArgSpecList::const_iterator iarg= f->arg_types.begin(), marg= function->arg_types.begin();
    for (; iarg != f->arg_types.end() && marg != function->arg_types.end(); ++iarg, ++marg)
    {
      if (!(iarg->type == marg->type))
      {
        get_grt()->send_warning(strfmt("Function '%s' of module '%s' doesn't match argument types (expected %s, got %s)", 
                                       f->name.c_str(), module->name().c_str(),
                                       fmt_type_spec(iarg->type).c_str(), fmt_type_spec(marg->type).c_str()));
        return false;
      }
    }
    if (iarg != f->arg_types.end() || marg != function->arg_types.end())
    {
      get_grt()->send_warning(strfmt("Function '%s' of module '%s' has wrong number of arguments", f->name.c_str(), module->name().c_str()));
      return false;
    }
  }
  return true;
}


//--------------------------------------------------------------------------------


void CPPModule::register_functions(ModuleFunctorBase *first, ...)
{
  if (first)
  {
    va_list args;
    ModuleFunctorBase *func= first;

    va_start(args, first);
    do 
    {
      Function f;
      
      f.name= func->get_name();
      f.description= func->get_doc();
      f.ret_type= func->get_return_type();
      f.arg_types= func->get_signature();
      f.call= boost::bind(&ModuleFunctorBase::perform_call, func, _1);
      
      add_function(f);

      _functors.push_back(func);
    }
    while ((func= va_arg(args, ModuleFunctorBase*))!=NULL);
    va_end(args);
  }

  _interfaces= _implemented_interfaces;
}


CPPModule::CPPModule(CPPModuleLoader *loader)
  : Module(loader), _gmodule(NULL)
{
}


void CPPModule::set_name(const std::string &name)
{
  _name= name;

  if (!g_str_has_suffix(_name.c_str(), "Impl"))
  {
    g_warning("Native C++ module classes must have the suffix Impl to avoid confusion between implementation and wrapper classes (%s)",
              _name.c_str());
  }
  else
    _name= _name.substr(0, _name.size()-4); // truncate Impl part

  // strip the namespace if it has one
  {
    const char *ptr= strstr(_name.c_str(), "::");
    if (ptr)
      _name= ptr;
  }
}


CPPModule::~CPPModule()
{
  for (std::list<ModuleFunctorBase*>::iterator iter= _functors.begin();
       iter != _functors.end(); ++iter)
    delete *iter;
  
  // Modules live the whole life of the app, so there's usually no need to delete them
  // except when initialization is aborted in the middle (like when a module is duplicate)
  // Problem is that in this case of duplicate modules, it seems that closing a GModule will cause a crash (in OSX)
#ifndef __APPLE__
  if (_gmodule)
    g_module_close(_gmodule);
#endif
}


std::string CPPModule::get_module_datadir()
{
  return _path+"/modules/data";
}


std::string CPPModule::get_resource_file_path(const std::string &file)
{
  return get_module_datadir()+"/"+file;
}




//--------------------------------------------------------------------------------


CPPModuleLoader::CPPModuleLoader(GRT *grt)
  : ModuleLoader(grt)
{
}


CPPModuleLoader::~CPPModuleLoader()
{
}


Module *CPPModuleLoader::init_module(const std::string &path)
{
  GModule *gmodule;
  Module *(*module_init)(CPPModuleLoader *loader, const char* grt_version);

					    // use lazy binding
					    // so that interdependent modules can be loaded in any order
  gmodule= g_module_open(path.c_str(), (GModuleFlags)G_MODULE_BIND_LAZY);
  if (!gmodule)
  {
    if (_grt->verbose())
      _grt->send_warning(strfmt("Could not open module %s (%s)", path.c_str(), g_module_error()));
    throw grt::os_error(strfmt("Could not open module %s (%s)", path.c_str(), g_module_error()));
  }

  // locate entry point
  if (!g_module_symbol(gmodule, "grt_module_init", (gpointer*)&module_init))
  {
    if (_grt->verbose())
      _grt->send_warning(strfmt("Could not get pointer to grt_module_init in module %s (%s)", path.c_str(), g_module_error()));
    g_module_close(gmodule);
    throw std::runtime_error("Invalid module "+path);
  }

  // execute module's init function, which must return a Module*
  // object created by init_module()
  CPPModule* cppmodule;

  cppmodule= dynamic_cast<CPPModule*>((*module_init)(this, GRT_VERSION));
  if (!cppmodule)
  {
    g_module_close(gmodule);
    return 0;
  }

  cppmodule->_path= path;

  cppmodule->_gmodule= gmodule;

  return cppmodule;
}

  
void CPPModuleLoader::refresh()
{
}


bool CPPModuleLoader::check_file_extension(const std::string &path)
{
#ifdef __APPLE__
  static const char *ext= ".dylib";
#elif defined(_WIN32)
  static const char *ext= ".dll";
#else
  static const char *ext= ".so";
#endif

  return g_str_has_suffix(path.c_str(), ext) != 0;
}
