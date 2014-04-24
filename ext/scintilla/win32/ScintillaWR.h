/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

/**
 * This is an additional file to export and import classes used by other projects
 * so we don't need to modify the original Scintilla code.
 */

#ifndef _SCINTILLAIMPORTEXPORT_H_
#define _SCINTILLAIMPORTEXPORT_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>

#include "Scintilla.h"
#include "ILexer.h"
#include "../lexlib/LexerModule.h"
#include "../lexlib/WordList.h"
#include "../src/Catalogue.h"
#include "../lexlib/PropSetSimple.h"
#include "../lexlib/LexAccessor.h"
#include "../lexlib/Accessor.h"

#pragma warning (disable: 4275) // Use of dll-interface for a class derived from one without dll-interface.

#ifdef DECL_SCI_EXPORT
  #define SCI_IMPORT_EXPORT __declspec(dllexport)
#else
  #define SCI_IMPORT_EXPORT __declspec(dllimport)
#endif

namespace ScintillaWrapper {

class SCI_IMPORT_EXPORT LexerModule : public Scintilla::LexerModule {};
class SCI_IMPORT_EXPORT PropSetSimple : public Scintilla::PropSetSimple {};

class SCI_IMPORT_EXPORT WordList : public Scintilla::WordList
{
public:
  WordList(bool onlyLineEnds_ = false): Scintilla::WordList(onlyLineEnds_) {};

  void Set(const char *s) { Scintilla::WordList::Set(s); };
  void Clear() { Scintilla::WordList::Clear(); };
};

class SCI_IMPORT_EXPORT Catalogue : public Scintilla::Catalogue
{
public:
  static const Scintilla::LexerModule *Find(const char *languageName)
  {
    return Scintilla::Catalogue::Find(languageName);
  };
};

class SCI_IMPORT_EXPORT Accessor : public Scintilla::Accessor
{
public:
  Accessor(Scintilla::IDocument *pAccess_, Scintilla::PropSetSimple *pprops_)
    : Scintilla::Accessor(pAccess_, pprops_) {};
};

} // namespace ScintillaWrapper

#endif // _SCINTILLAIMPORTEXPORT_H_