/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.db.h>

#include "base/string_utilities.h"
#include <grtpp_undo_manager.h>

#include "grt/parse_utils.h"
#include "grt/common.h"
#include "grtdb/db_object_helpers.h"

using namespace base;

//================================================================================
// db_Column

static void notify_visible_member_change(const std::string &member, const grt::ValueRef &ovalue, db_Column* ref)
{
  if (member == "name" || member == "simpleType" || member == "userType")
  {
    if (ovalue != ref->get_member(member) && ref->owner().is_valid())
      (*db_TableRef::cast_from(ref->owner())->signal_refreshDisplay())("column");
  }
}

void db_Column::init()
{
  //No need in disconnet management since signal it part of object
  _changed_signal.connect(boost::bind(notify_visible_member_change, _1, _2, this));
}

db_Column::~db_Column()
{
}


template<class T>
class auto_array_ptr
{
  T *_ptr;

public:
  auto_array_ptr(T* ptr) : _ptr(ptr) {}
  ~auto_array_ptr() { delete[] _ptr; }

  operator T* () { return _ptr; }
};


grt::StringRef db_Column::formattedRawType() const
{
  if (this->userType().is_valid())
  {
    std::string arguments;

    // if no simple or structured datatype is set,
    // simply take the parameters
    if (this->length() != bec::EMPTY_COLUMN_LENGTH)
    {
      arguments= strfmt("(%i)", (int)this->length());
    }
    else if (this->precision() != bec::EMPTY_COLUMN_PRECISION)
    {
      std::string tmp;
      if (this->scale() != bec::EMPTY_COLUMN_SCALE)
        tmp= strfmt("(%i,%i)", (int)this->precision(), (int)this->scale());
  else
        tmp= strfmt("(%i)", (int)this->precision());
      arguments= tmp;
    }
    else if (this->datatypeExplicitParams().is_valid() && *this->datatypeExplicitParams() != "")
    {
      arguments= *this->datatypeExplicitParams();      
    }
    
    return std::string(this->userType()->name()).append(arguments);
  }
  else
    return formattedType();
}


grt::StringRef db_Column::formattedType() const
{
  db_SimpleDatatypeRef simpleType(this->simpleType());
  db_StructuredDatatypeRef structuredType(this->structuredType());
  std::string caption;

  if (simpleType.is_valid())
  {
    ssize_t ptype = simpleType->parameterFormatType();
    caption= simpleType->name();

    if (simpleType->numericPrecision() != bec::EMPTY_TYPE_PRECISION)
    {
      std::string tmp;
      if (this->precision() != bec::EMPTY_COLUMN_PRECISION && this->scale() != bec::EMPTY_COLUMN_SCALE && (ptype == 3 || ptype == 4 || ptype == 5 || ptype == 6))
        tmp= strfmt("(%i,%i)", (int)this->precision(), (int)this->scale());
      else if (this->precision() != bec::EMPTY_COLUMN_PRECISION && (ptype == 1 || ptype == 2 || ptype == 4 || ptype == 6))
        tmp= strfmt("(%i)", (int)this->precision());
      caption.append(tmp);
    }
    else
    {
      if (*simpleType->characterMaximumLength() != bec::EMPTY_TYPE_MAXIMUM_LENGTH 
          || *simpleType->characterOctetLength() != bec::EMPTY_TYPE_OCTET_LENGTH)
      {
        if (this->length() != bec::EMPTY_COLUMN_LENGTH && (ptype == 1 || ptype == 2 || ptype == 4 || ptype == 6))
        {
          caption.append(strfmt("(%i)", (int)this->length()));
        }
      }
      else if (this->datatypeExplicitParams().is_valid() && *this->datatypeExplicitParams() != "")
        caption.append(*this->datatypeExplicitParams());
    }
  }
  else if (structuredType.is_valid())
  {
  }
  else
  {
    std::string arguments;
    
    if (this->userType().is_valid())
      caption= this->userType()->sqlDefinition();

    // if no simple or structured datatype is set,
    // simply take the parameters
    if (this->length() != bec::EMPTY_COLUMN_LENGTH)
    {
      arguments= strfmt("(%i)", (int)this->length());
    }
    else if (this->precision() != bec::EMPTY_COLUMN_PRECISION)
    {
      std::string tmp;
      if (this->scale() != bec::EMPTY_COLUMN_SCALE)
        tmp= strfmt("(%i,%i)", (int)this->precision(), (int)this->scale());
      else
        tmp= strfmt("(%i)", (int)this->precision());
      arguments= tmp;
    }
    else if (this->datatypeExplicitParams().is_valid() && *this->datatypeExplicitParams() != "")
    {
      arguments= *this->datatypeExplicitParams();      
    }
    
    if (!arguments.empty())
    {
      std::string::size_type p;
      if ((p= caption.find('(')) != std::string::npos)
        caption= caption.substr(0, p);
      
      caption.append(arguments);
    }
  }

  return caption;
}


void db_Column::formattedType(const grt::StringRef &value)
{
  if (formattedType() == value.c_str())
    return;
  // add code here
  g_warning("got a request to change %s.formattedType() from '%s' to '%s'",
            owner().is_valid() ? owner()->name().c_str() : "<null>", formattedType().c_str(), value.c_str());
}

/** Sets the datatype defined by a string to the column.
 *
 * Assigns a datatype defined by a string like NUMERIC(7, 2) to the given column.
 *
 * @return 1 on success or 0 on parse error or invalid type/invalid params
 */
grt::IntegerRef db_Column::setParseType(const std::string &type, const grt::ListRef<db_SimpleDatatype> &typeList)
{
  grt::ListRef<db_UserDatatype> user_types;
  grt::ListRef<db_SimpleDatatype> default_type_list;
  GrtVersionRef target_version;
  if (owner().is_valid() && owner()->owner().is_valid() && owner()->owner()->owner().is_valid())
  {
    db_CatalogRef catalog= db_CatalogRef::cast_from(owner()->owner()->owner());
    user_types= catalog->userDatatypes();
    default_type_list = catalog->simpleDatatypes();
    target_version = catalog->version();
  }

  db_UserDatatypeRef userType;
  db_SimpleDatatypeRef simpleType;
  int precision= bec::EMPTY_COLUMN_PRECISION;
  int scale= bec::EMPTY_COLUMN_SCALE;
  int length= bec::EMPTY_COLUMN_LENGTH;
  std::string datatypeExplicitParams;
  grt::AutoUndo undo(get_grt(), !is_global());

  if (!bec::parse_type_definition(type, target_version, typeList, user_types, default_type_list,
      simpleType, userType, precision, scale, length, datatypeExplicitParams))
      return false;
  this->userType(userType);
  this->simpleType(simpleType);
  this->precision(precision);
  this->scale(scale);
  this->length(length);
  this->datatypeExplicitParams(datatypeExplicitParams);

  if (_owner.is_valid())
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("column");

  undo.end(_("Change Column Type"));

  return true;
}





