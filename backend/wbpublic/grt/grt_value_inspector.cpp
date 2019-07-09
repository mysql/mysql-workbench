/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MSC_VER
#include <algorithm>
#endif

#include "common.h"
#include "grt_value_inspector.h"
#include <grtpp_util.h>
#include <grtpp_undo_manager.h>
#include "base/string_utilities.h"

#include <sstream>
#include <boost/tuple/tuple.hpp>

using namespace grt;
using namespace bec;
using namespace base;

//--------------------------------------------------------------------------------------------------

void ValueInspectorBE::monitor_object_changes(const grt::ObjectRef &obj) {
  _changed_conn = obj->signal_changed()->connect(
    std::bind(&ValueInspectorBE::changed_slot, this, std::placeholders::_1, std::placeholders::_2));
}

//--------------------------------------------------------------------------------------------------

void ValueInspectorBE::changed_slot(const std::string &name, const grt::ValueRef &value) {
  refresh();
  do_ui_refresh();
}

//--------------------------------------------------------------------------------------------------

inline bool is_compatible(Type cont_type, Type type) {
  if (cont_type == type || cont_type == AnyType)
    return true;
  return false;
}

//--------------------------------------------------------------------------------------------------

inline bool is_multiple_value(const std::string &value) {
  if (!value.empty() && ('<' == value[0])) {
    static std::string suff = " uniques>";
    std::string::size_type off = value.find(suff);
    if ((off != std::string::npos) && (off + suff.size() == value.size()))
      return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

class ObjectWrapper {
private:
  ObjectRef _object;
  bool _process_editas_flag;

  struct Field {
    std::string name;
    Type type;
    std::string desc;
    std::string is_readonly;
    std::string edit_method;
    std::string group;
    ObjectRef source;

    bool operator<(const Field &f) const {
      return name < f.name;
    };

    Field() : type(UnknownType) {
    }
  };

  std::map<std::string, Field> _fields;

protected:
  ObjectWrapper() : _process_editas_flag(true){};

  // bool append_fields_for(

public:
  ObjectWrapper(const ObjectRef &object, bool process_editas_flag)
    : _object(object), _process_editas_flag(process_editas_flag) {
    MetaClass *gstruct(object.get_metaclass());

    gstruct->foreach_member(std::bind(&ObjectWrapper::setup_member, this, std::placeholders::_1, object));
  }

  //------------------------------------------------------------------------------------------------

  bool setup_member(const MetaClass::Member *mem, const ObjectRef &object) {
    std::string k(mem->name);
    ValueRef v(object.get_member(k));
    std::string desc, readonly, editas, group;

    desc = object.get_metaclass()->get_member_attribute(k, "desc");
    readonly = object.get_metaclass()->get_member_attribute(k, "readonly");
    editas = _process_editas_flag ? object.get_metaclass()->get_member_attribute(k, "editas") : "";
    group = object.get_metaclass()->get_member_attribute(k, "group");

    if (editas == "hide")
      return true;

    bool skip = false;
    if (v.type() == ObjectType) {
      if (base::hasPrefix(editas, "fields:")) {
        ObjectRef mobj(ObjectRef::cast_from(v));
        MetaClass *mstruct(mobj.get_metaclass());
        std::vector<std::string> fields = base::split(editas.substr(7), ",");

        for (std::vector<std::string>::const_iterator iter = fields.begin(); iter != fields.end(); ++iter) {
          std::string dattr = mstruct->get_member_attribute(*iter, "desc");
          std::string roattr = mstruct->get_member_attribute(*iter, "readonly");
          std::string mattr = mstruct->get_member_attribute(*iter, "editas");

          Field field;
          field.name = *iter;
          field.type =
            mstruct->get_member_info(field.name) ? mstruct->get_member_info(field.name)->type.base.type : AnyType;
          field.desc = dattr;
          field.is_readonly = roattr;
          field.edit_method = mattr;
          field.group = group;
          field.source = mobj;

          _fields[field.name] = field;
        }

        skip = true;
      }
    }

    if (!skip) {
      Field field;

      field.name = k;
      field.desc = desc;
      field.is_readonly = readonly;
      field.edit_method = editas;
      field.type = mem->type.base.type;
      field.group = group;
      field.source = object;

      _fields[field.name] = field;
    }
    return true;
  }

  //------------------------------------------------------------------------------------------------

  std::vector<std::string> get_keys() {
    std::vector<std::string> keys;

    for (std::map<std::string, Field>::const_iterator i = _fields.begin(); i != _fields.end(); ++i)
      keys.push_back(i->first);
    return keys;
  }

  //------------------------------------------------------------------------------------------------

  Type get_type(const std::string &field) {
    return _fields[field].type;
  }

  //------------------------------------------------------------------------------------------------

  std::string get_desc(const std::string &field) {
    return _fields[field].desc;
  }

  //------------------------------------------------------------------------------------------------

  std::string is_readonly(const std::string &field) {
    return _fields[field].is_readonly;
  }

  //------------------------------------------------------------------------------------------------

  std::string get_edit_method(const std::string &field) {
    return _fields[field].edit_method;
  }

  //------------------------------------------------------------------------------------------------

  std::string get_group(const std::string &field) {
    return _fields[field].group;
  }

  //------------------------------------------------------------------------------------------------

  size_t count() {
    return _fields.size();
  }

  //------------------------------------------------------------------------------------------------

  virtual ValueRef get(const std::string &field) {
    return _fields[field].source.get_member(field);
  }

  //------------------------------------------------------------------------------------------------

  virtual void set(const std::string &field, const ValueRef &value) {
    grt::AutoUndo undo(!_object->is_global());

    _fields[field].source.set_member(field, value);

    undo.end(strfmt(_("Change '%s'"), field.c_str()));
  }
};

//--------------------------------------------------------------------------------------------------

class ObjectListWrapper : public ObjectWrapper {
public:
  ObjectListWrapper(const ObjectListRef &objectList) {
  }

  std::list<ObjectRef> _objects;
};

//----------------------------------------------------------------------------

class GRTListValueInspectorBE : public ValueInspectorBE {
public:
  GRTListValueInspectorBE(const BaseListRef &value) : _value(value) {
    refresh();
  };

  //------------------------------------------------------------------------------------------------

  virtual size_t count_children(const NodeId &parent) {
    if (parent == NodeId())
      return _value.count();
    return 0;
  }

  //------------------------------------------------------------------------------------------------

  virtual NodeId get_child(const NodeId &parent, size_t index) {
    if ((ssize_t)index < 0 || index >= _value.count())
      return NodeId();

    return index;
  }

  //------------------------------------------------------------------------------------------------

  virtual bool get_field(const NodeId &node, ColumnId column, std::string &value) {
    if (node.depth() < 1 || node[0] >= _value.count())
      return false;

    if (column == Name) {
      std::stringstream out;
      out << "[" << node[0] + 1 << "]";
      value = out.str();

      return true;
    } else
      return ValueInspectorBE::get_field(node, column, value);
  }

  //------------------------------------------------------------------------------------------------

  virtual void refresh(){};

  //------------------------------------------------------------------------------------------------

  bool add_item(NodeId &new_node) {
    new_node = NodeId(_value.count());
    return true;
  }

  //------------------------------------------------------------------------------------------------

  bool delete_item(const NodeId &node) {
    if (node.depth() < 1 || node[0] >= _value.count())
      return false;

    _value.remove(node[0]);

    return true;
  }

private:
  BaseListRef _value;

  //------------------------------------------------------------------------------------------------

  virtual bool get_field_grt(const NodeId &node, ColumnId column, ValueRef &value) {
    if (node.depth() < 1 || node[0] >= _value.count())
      return false;

    switch (column) {
      case Name: {
        std::stringstream out;
        out << "[" << node[0] + 1 << "]";
        value = StringRef(out.str());
      }
        return true;

      case Value:
        value = _value[node[0]];
        return true;
    }
    return false;
  }

  //------------------------------------------------------------------------------------------------

  virtual Type get_field_type(const NodeId &node, ColumnId column) {
    if (node[0] == _value.count())
      return _value.content_type();
    return _value[node[0]].type();
  }

  //------------------------------------------------------------------------------------------------

  virtual Type get_canonical_type(const NodeId &node) {
    return _value.content_type();
  }

  //------------------------------------------------------------------------------------------------

  virtual bool set_value(const NodeId &node, const ValueRef &value) {
    if (node.depth() < 1 || node[0] > _value.count())
      return false;

    if (node[0] == _value.count())
      _value.ginsert(value);
    else
      _value.gset(node[0], value);
    return true;
  }
};

//--------------------------------------------------------------------------------------------------

class GRTDictRefInspectorBE : public ValueInspectorBE {
public:
  GRTDictRefInspectorBE(const DictRef &value) : _value(value) {
    _has_new_item = false;
    refresh();
  }

  //------------------------------------------------------------------------------------------------

  virtual size_t count_children(const NodeId &parent) {
    if (parent == NodeId())
      return _items.size();
    return 0;
  }

  //------------------------------------------------------------------------------------------------

  virtual NodeId get_child(const NodeId &parent, size_t index) {
    if ((ssize_t)index < 0 || index >= _items.size())
      return NodeId();
    return NodeId(index);
  }

  //------------------------------------------------------------------------------------------------

  virtual bool get_field(const NodeId &node, ColumnId column, std::string &value) {
    if (node.depth() < 1 || node[0] >= _items.size())
      return false;

    if (column == Name) {
      value = _items[node[0]];
      return true;
    }
    return ValueInspectorBE::get_field(node, column, value);
  }

  //------------------------------------------------------------------------------------------------

  virtual void refresh() {
    _has_new_item = false;

    _items.clear();
    for (DictRef::const_iterator item = _value.begin(); item != _value.end(); ++item) {
      _items.push_back(item->first);
    }
    std::sort(_items.begin(), _items.end());
  }

  //------------------------------------------------------------------------------------------------

  virtual bool add_item(NodeId &new_node) {
    if (_has_new_item)
      return false;

    _has_new_item = true;
    new_node = NodeId(_items.size());
    _items.push_back("");
    return true;
  }

  //------------------------------------------------------------------------------------------------

  virtual bool delete_item(const NodeId &node) {
    if (node[0] == _items.size() - 1 && _has_new_item) {
      _has_new_item = false;
      _items.pop_back();
      return true;
    }

    if (node[0] >= _items.size())
      return false;
    _value.remove(_items[node[0]]);
    _items.erase(_items.begin() + node[0]);
    return true;
  }

  //------------------------------------------------------------------------------------------------

protected:
  std::vector<std::string> _items;
  DictRef _value;
  bool _has_new_item;

  //------------------------------------------------------------------------------------------------

  virtual bool get_field_grt(const NodeId &node, ColumnId column, ValueRef &value) {
    if (node.depth() < 1 || node[0] >= _items.size())
      return false;

    switch (column) {
      case Name:
        value = StringRef(_items[node[0]]);
        return true;
      case Value:
        value = _value.get(_items[node[0]]);
        return true;
    }
    return false;
  }

  //------------------------------------------------------------------------------------------------

  virtual Type get_field_type(const NodeId &node, ColumnId column) {
    if (_has_new_item && node[0] == _items.size() - 1)
      return _value.content_type();
    return _value.get(_items[node[0]]).type();
  }

  //------------------------------------------------------------------------------------------------

  virtual Type get_canonical_type(const NodeId &node) {
    return _value.content_type();
  }

  //------------------------------------------------------------------------------------------------

  bool set_field(const NodeId &node, ColumnId column, const std::string &value) {
    if (column == Name) {
      if (_items[node[0]] == value)
        return true;

      // duplicate name!
      if (std::find(_items.begin(), _items.end(), value) != _items.end())
        return false;

      if (_has_new_item && node[0] == _items.size() - 1)
        _items[node[0]] = value;
      else {
        // rename the item
        ValueRef dvalue = _value.get(_items[node[0]]);
        _value.remove(_items[node[0]]);
        _value.set(value, dvalue);
        _items[node[0]] = value;
      }
      return true;
    }
    return ValueInspectorBE::set_field(node, column, value);
  }

  //------------------------------------------------------------------------------------------------

  virtual bool set_value(const NodeId &node, const ValueRef &value) {
    try {
      if (_has_new_item && node[0] == _items.size() - 1) {
        _value.set(_items[node[0]], value);
        _has_new_item = false;
        return true;
      }

      _value.set(_items[node[0]], value);
      return true;
    } catch (type_error &) {
      return false;
    };
  }
};

//--------------------------------------------------------------------------------------------------

class GRTObjectRefInspectorBE : public ValueInspectorBE {
public:
  GRTObjectRefInspectorBE(const ObjectRef &value, bool grouped, bool process_editas_flag)
    : _object(value, process_editas_flag), _grouping(grouped) {
    monitor_object_changes(value);
    refresh();
  }

  //------------------------------------------------------------------------------------------------

  virtual size_t count_children(const NodeId &parent) {
    if (_grouping) {
      switch (get_node_depth(parent)) {
        case 0:
          return _groups.size();

        case 1:
          return _keys[_groups[parent[0]]].size();
      }
    } else {
      if (!parent.is_valid())
        return _keys[""].size();
    }
    return 0;
  }

  //------------------------------------------------------------------------------------------------

  virtual NodeId get_child(const NodeId &parent, size_t index) {
    if (_grouping) {
      if (parent.depth() == 1) {
        if ((ssize_t)index >= 0 && index < _keys[_groups[parent[0]]].size()) {
          // NodeId child(parent);

          return NodeId(parent).append(index);
        }
      } else if (parent.depth() == 0 && (ssize_t)index >= 0 && index < _groups.size())
        return NodeId(index);
    } else {
      if ((ssize_t)index >= 0 && index < _keys[""].size())
        return NodeId(index);
    }
    return NodeId();
  }

  //------------------------------------------------------------------------------------------------

  virtual bool get_field(const NodeId &node, ColumnId column, std::string &value) {
    switch (column) {
      case Name: {
        if (_grouping) {
          if ((size_t)node[0] >= _groups.size())
            return false;
          if (get_node_depth(node) == 1)
            value = _groups[node[0]];
          else
            value = _keys[_groups[node[0]]][node[1]];
        } else {
          if ((size_t)node[0] >= _keys[""].size())
            return false;
          value = _keys[""][node[0]];
        }
        return true;
      }
      case Description: {
        if (_grouping) {
          if (get_node_depth(node) == 1)
            value = "";
          else
            value = _object.get_desc(_keys[_groups[node[0]]][node[1]]);
        } else
          value = _object.get_desc(_keys[""][node[0]]);
        return true;
      }
      case IsReadonly: {
        if (_grouping) {
          if (get_node_depth(node) == 1)
            value = "";
          else
            value = _object.is_readonly(_keys[_groups[node[0]]][node[1]]);
        } else
          value = _object.is_readonly(_keys[""][node[0]]);
        return true;
      }
      case EditMethod: {
        if (_grouping) {
          if (get_node_depth(node) == 1)
            value = "";
          else
            value = _object.get_edit_method(_keys[_groups[node[0]]][node[1]]);
        } else
          value = _object.get_edit_method(_keys[""][node[0]]);
        return true;
      }
    }
    return ValueInspectorBE::get_field(node, column, value);
  }

  //------------------------------------------------------------------------------------------------

  virtual void refresh() {
    _groups.clear();
    _keys.clear();

    if (_grouping) {
      std::vector<std::string> keys = _object.get_keys();
      for (std::vector<std::string>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        std::string k = *i;
        std::string group;

        group = _object.get_group(k);

        if (_keys.find(group) == _keys.end()) // no group ("") is also a valid group
        {
          _groups.push_back(group);
          _keys[group] = std::vector<std::string>();
        }

        _keys[group].push_back(k);
      }

      std::map<std::string, std::vector<std::string> >::iterator it;
      it = _keys.begin();
      while (it != _keys.end()) {
        std::sort(it->second.begin(), it->second.end());
        ++it;
      }
      std::sort(_groups.begin(), _groups.end());
    } else {
      std::vector<std::string> keys = _object.get_keys();
      for (std::vector<std::string>::const_iterator i = keys.begin(); i != keys.end(); ++i) {
        std::string k = *i;

        _keys[""].push_back(k);
      }
      std::sort(_keys[""].begin(), _keys[""].end());
    }
  }

  virtual bool add_item(NodeId &new_name) {
    return false;
  }

  virtual bool delete_item(const NodeId &node) {
    return false;
  }

  //------------------------------------------------------------------------------------------------

protected:
  ObjectWrapper _object;

  // list of groups for access by index
  std::vector<std::string> _groups;

  // group -> key. if ungrouped, then group is ""
  std::map<std::string, std::vector<std::string> > _keys;

  bool _grouping;

  //------------------------------------------------------------------------------------------------

  virtual bool get_field_grt(const NodeId &node, ColumnId column, ValueRef &value) {
    if (_grouping) {
      if (get_node_depth(node) < 2)
        return false;

      switch (column) {
        case Name:
          value = StringRef(_keys[_groups[node[0]]][node[1]]);
          return true;
        case Value:
          value = _object.get(_keys[_groups[node[0]]][node[1]]);
          return true;
        case Description:
          value = StringRef(_object.get_desc(_keys[_groups[node[0]]][node[1]]));
          return true;
        case IsReadonly:
          value = StringRef(_object.is_readonly(_keys[_groups[node[0]]][node[1]]));
          return true;
        case EditMethod:
          value = StringRef(_object.get_edit_method(_keys[_groups[node[0]]][node[1]]));
          return true;
      }
    } else {
      if (node.depth() == 0)
        return false;

      switch (column) {
        case Name:
          value = StringRef(_keys[""][node[0]]);
          return true;
        case Value:
          value = _object.get(_keys[""][node[0]]);
          if (!is_simple_type(value.type())) {
            if (value.type() == ObjectType) {
              ObjectRef obj(ObjectRef::cast_from(value));
              value = StringRef("<" + obj.class_name() + ":" + obj.id() + ">");
            } else
              value = StringRef("<" + type_to_str(value.type()) + ">");
          }
          return true;
        case Description:
          value = StringRef(_object.get_desc(_keys[""][node[0]]));
          return true;
        case IsReadonly:
          value = StringRef(_object.is_readonly(_keys[""][node[0]]));
          return true;
        case EditMethod:
          value = StringRef(_object.get_edit_method(_keys[""][node[0]]));
          return true;
      }
    }
    return false;
  }

  //------------------------------------------------------------------------------------------------

  virtual Type get_field_type(const NodeId &node, ColumnId column) {
    if (_grouping) {
      if (get_node_depth(node) < 2)
        return AnyType;

      return _object.get_type(_keys[_groups[node[0]]][node[1]]);
    } else {
      if (node.depth() == 0)
        return AnyType;

      return _object.get_type(_keys[""][node[0]]);
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual Type get_canonical_type(const NodeId &node) {
    return get_field_type(node, Value);
  }

  //------------------------------------------------------------------------------------------------

  bool set_value(const NodeId &node, const ValueRef &value) {
    std::string name;

    if (_grouping && get_node_depth(node) < 2)
      return false;

    if (!get_field(node, Name, name))
      return false;

    _object.set(name, value);

    return true;
  }
};

//--------------------------------------------------------------------------------------------------

class GRTObjectListValueInspectorBE : public ValueInspectorBE {
public:
  GRTObjectListValueInspectorBE(const std::vector<ObjectRef> &objects) : _list(objects) {
    refresh();
  }

  //------------------------------------------------------------------------------------------------

  virtual size_t count_children(const NodeId &parent) {
    if (parent == NodeId())
      return _items.size();
    return 0;
  }

  //------------------------------------------------------------------------------------------------

  virtual NodeId get_child(const NodeId &parent, size_t index) {
    if ((ssize_t)index < 0 || index >= _items.size())
      return NodeId();
    return NodeId(index);
  }

  //------------------------------------------------------------------------------------------------

  virtual bool get_field(const NodeId &node, ColumnId column, std::string &value) {
    if (node[0] >= _items.size())
      return false;

    switch (column) {
      case Name:
        value = _items[node[0]].key;
        return true;
      case Description:
        value = _items[node[0]].desc;
        return true;
      case IsReadonly:
        value = _items[node[0]].is_readonly;
        return true;
      case EditMethod:
        value = _items[node[0]].edit_method;
        return true;
    }
    return ValueInspectorBE::get_field(node, column, value);
  }

  //------------------------------------------------------------------------------------------------

  bool refresh_member(const MetaClass::Member *member,
                      std::map<std::string, boost::tuple<int, std::string, std::string, std::string> > *keys,
                      MetaClass *meta) {
    std::string name(member->name);
    ValueRef value;
    std::string editas;
    boost::tuple<int, std::string, std::string, std::string> item;

    if ((editas = meta->get_member_attribute(name, "editas")) == "hide")
      return true;

    item = (*keys)[name];
    boost::get<0>(item)++;
    boost::get<1>(item) = meta->get_member_attribute(name, "desc");
    boost::get<2>(item) = meta->get_member_attribute(name, "readonly");
    if (boost::get<3>(item).empty())
      boost::get<3>(item) = editas;
    else if (boost::get<3>(item) != editas)
      return true;
    (*keys)[name] = item;

    return true;
  }

  //------------------------------------------------------------------------------------------------

  virtual void refresh() {
    size_t i, c = _list.size();
    std::map<std::string, boost::tuple<int, std::string, std::string, std::string> >
      keys; // key -> (count, desc, readonly, editas)

    for (i = 0; i < c; i++) {
      if (!_list[i].is_valid())
        continue;

      MetaClass *meta = _list[i].get_metaclass();

      meta->foreach_member(
        std::bind(&GRTObjectListValueInspectorBE::refresh_member, this, std::placeholders::_1, &keys, meta));
    }

    _items.clear();
    for (std::map<std::string, boost::tuple<int, std::string, std::string, std::string> >::const_iterator iter =
           keys.begin();
         iter != keys.end(); ++iter) {
      if (boost::get<0>(iter->second) == (ssize_t)_list.size()) {
        Item item;
        item.key = iter->first;
        item.desc = boost::get<1>(iter->second);
        item.is_readonly = boost::get<2>(iter->second);
        item.edit_method = boost::get<3>(iter->second);
        _items.push_back(item);
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual bool add_item(NodeId &new_node) {
    return false;
  }

  //------------------------------------------------------------------------------------------------

  virtual bool delete_item(const NodeId &node) {
    return false;
  }

  //------------------------------------------------------------------------------------------------

protected:
  struct Item {
    std::string key;
    std::string desc;
    std::string is_readonly;
    std::string edit_method;
  };

  struct Itemcmp {
    bool operator()(const Item &a, const Item &b) const {
      return a.key < b.key;
    }
  };

  std::vector<Item> _items;
  std::vector<ObjectRef> _list;

  //------------------------------------------------------------------------------------------------

  virtual bool get_field_grt(const NodeId &node, ColumnId column, ValueRef &value) {
    switch (column) {
      case Name:
        value = StringRef(_items[node[0]].key);
        return true;
      case Value: {
        std::string v;
        size_t item_count = 1;

        for (std::vector<ObjectRef>::iterator iter = _list.begin(); iter != _list.end(); ++iter) {
          value = iter->get_member(_items[node[0]].key);

          if (iter == _list.begin())
            v = value.toString();
          else if (v != value.toString())
            ++item_count;
        }

        if (1 == item_count)
          value = _list[0].get_member(_items[node[0]].key);
        else {
          std::ostringstream oss;
          oss << "<" << item_count << " uniques>";
          value = StringRef(oss.str());
        }
      }
        return true;
      case Description:
        value = StringRef(_items[node[0]].desc);
        break;
      case IsReadonly:
        value = StringRef(_items[node[0]].is_readonly);
        break;
      case EditMethod:
        value = StringRef(_items[node[0]].edit_method);
        break;
    }
    return false;
  }

  //------------------------------------------------------------------------------------------------

  virtual Type get_field_type(const NodeId &node, ColumnId column) {
    MetaClass *meta = _list[0].get_metaclass();

    if (meta) {
      try {
        return meta->get_member_type(_items[node[0]].key).base.type;
      } catch (grt::bad_item &) {
      }
    }
    return UnknownType;
  }

  //------------------------------------------------------------------------------------------------

  virtual Type get_canonical_type(const NodeId &node) {
    MetaClass *meta = _list[0].get_metaclass();

    if (meta) {
      try {
        return meta->get_member_type(_items[node[0]].key).base.type;
      } catch (grt::bad_item &) {
      }
    }
    return UnknownType;
  }

  //------------------------------------------------------------------------------------------------

  bool set_field(const NodeId &node, ColumnId column, const std::string &value) {
    if (column == Name)
      return false;

    if (column == Value && is_multiple_value(value))
      return false;

    return ValueInspectorBE::set_field(node, column, value);
  }

  //------------------------------------------------------------------------------------------------

  virtual bool set_value(const NodeId &node, const ValueRef &value) {
    grt::AutoUndo undo;

    for (std::vector<ObjectRef>::iterator iter = _list.begin(); iter != _list.end(); ++iter) {
      iter->set_member(_items[node[0]].key, value);
    }

    undo.end(strfmt(_("Change '%s'"), _items[node[0]].key.c_str()));

    return true;
  }
};

//--------------------------------------------------------------------------------------------------

ValueInspectorBE::ValueInspectorBE() {
}

//--------------------------------------------------------------------------------------------------

ValueRef ValueInspectorBE::get_grt_value(const NodeId &node, ColumnId column) {
  if (column == Value) {
    ValueRef value;
    if (get_field_grt(node, column, value))
      return value;
  }
  return ValueRef();
}

//--------------------------------------------------------------------------------------------------

bool ValueInspectorBE::set_convert_field(const NodeId &node, ColumnId column, const std::string &value) {
  if (column == Name)
    return set_field(node, column, value);
  else if (column == Value && !is_multiple_value(value))
    return set_value(node, parse_value(get_canonical_type(node), value));
  else
    return false;

  return true;
}

//--------------------------------------------------------------------------------------------------

bool ValueInspectorBE::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  if (column == Value && is_compatible(get_canonical_type(node), StringType))
    return set_value(node, StringRef(value));

  // Name and Type are unchangeable
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ValueInspectorBE::set_field(const NodeId &node, ColumnId column, ssize_t value) {
  if (column == Value && is_compatible(get_canonical_type(node), IntegerType))
    return set_value(node, IntegerRef(value));

  // Name and Type are unchangeable
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ValueInspectorBE::set_field(const NodeId &node, ColumnId column, double value) {
  if (column == Value && is_compatible(get_canonical_type(node), DoubleType))
    return set_value(node, DoubleRef(value));

  // Name and Type are unchangeable
  return false;
}

//--------------------------------------------------------------------------------------------------

IconId ValueInspectorBE::get_field_icon(const NodeId &node, ColumnId column, IconSize size) {
  if (column == Name) {
    switch (get_field_type(node, column)) {
      case ListType:
        return IconManager::get_instance()->get_icon_id("grt_list.png");
      case DictType:
        return IconManager::get_instance()->get_icon_id("grt_dict.png");
      case ObjectType:
        return IconManager::get_instance()->get_icon_id("grt_object.png");
      default:
        return IconManager::get_instance()->get_icon_id("grt_simple_type.png");
    }
  } else
    return IconManager::get_instance()->get_icon_id("");
}

//--------------------------------------------------------------------------------------------------

ValueInspectorBE *ValueInspectorBE::create(const ValueRef &value, bool grouped, bool process_editas_flag) {
  switch (value.type()) {
    case DictType:
      return new GRTDictRefInspectorBE(DictRef::cast_from(value));

    case ListType:
      return new GRTListValueInspectorBE(BaseListRef::cast_from(value));

    case ObjectType:
      return new GRTObjectRefInspectorBE(ObjectRef::cast_from(value), grouped, process_editas_flag);

    default:
      return 0;
  }
}

//--------------------------------------------------------------------------------------------------

ValueInspectorBE *ValueInspectorBE::create(const std::vector<ObjectRef> &objects) {
  return new GRTObjectListValueInspectorBE(objects);
}

//--------------------------------------------------------------------------------------------------
